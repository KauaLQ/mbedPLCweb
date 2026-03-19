#include <Arduino.h>
#include "plc_runtime.h"
#include "ladder_parser.h"
#include "io_manager.h"
#include "testjson.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t plcMutex;
bool receiving = false;
String serialBuffer = "";

void blinkLED(int pin){
    digitalWrite(pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void plcTask(void *pvParameters){
    while(true){
        if(xSemaphoreTake(plcMutex, portMAX_DELAY)){
            /*
            scan cycle:
            1 ler entradas (feito dentro de readTag)
            2 executar linhas
            3 atualizar saídas (feito dentro de writeOutput)
            */
            executePLC();
            xSemaphoreGive(plcMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // ciclo de scan
    }
}

void serialTask(void *pvParameters){
    while(true){
        while(Serial.available()){
            char c = Serial.read();
            serialBuffer += c;

            if(serialBuffer.endsWith("<START>")){
                receiving = true;
                serialBuffer = "";
            }
            else if(serialBuffer.endsWith("<END>")){
                receiving = false;
                String json = serialBuffer.substring(0, serialBuffer.length() - 5);
                Serial.println("Programa recebido:");
                Serial.println(json);
                if(parseProgram(json)){
                    if(xSemaphoreTake(plcMutex, portMAX_DELAY)){
                        resetPLCState();
                        xSemaphoreGive(plcMutex);
                    }
                    blinkLED(2); // Feedback para o usuário de upload bem-sucedido
                }
                else{
                    Serial.println("Programa inválido - ignorado");
                }
                serialBuffer = "";
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup(){
    pinMode(16, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(18, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(2, OUTPUT);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);
    Serial.begin(115200);

    plcMutex = xSemaphoreCreateMutex();

    parseProgram(testJSON10); // programa default

    xTaskCreatePinnedToCore(plcTask, "PLC Task", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTask, "Serial Task", 4096, NULL, 2, NULL, 0);
}

void loop(){/* Nada aqui, tudo roda via FreeRTOS */}