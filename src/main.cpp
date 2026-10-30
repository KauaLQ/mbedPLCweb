#include <Arduino.h>
#include "LittleFS.h"
#include "plc_runtime.h"
#include "ladder_parser.h"
#include "io_manager.h"
#include "testjson.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PLC_DEBUG   // comente para desativar o log de estados / tempo de scan no Serial

const uint8_t LED_PIN = 2;
const unsigned long SCAN_MS = 10;               // período do scan
const size_t MAX_PROGRAM_SIZE = 8192;           // bytes (JSON)
const unsigned long RX_TIMEOUT_MS = 3000;       // upload parado por mais que isso é descartado
const char *PROGRAM_FILE = "/program.json";
const char *TEMP_FILE = "/program.tmp";

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

// Grava em arquivo temporário e só depois troca pelo definitivo.
// Se a energia cair no meio, o program.json antigo continua intacto.
bool saveProgramToFlash(const String &json){
    File file = LittleFS.open(TEMP_FILE, FILE_WRITE);
    if(!file){
        Serial.println("Erro ao abrir arquivo para escrita");
        return false;
    }
    size_t written = file.print(json);
    file.close();

    if(written != json.length()){
        Serial.println("Falha na gravação!");
        LittleFS.remove(TEMP_FILE);
        return false;
    }
    if(!LittleFS.rename(TEMP_FILE, PROGRAM_FILE)){
        Serial.println("Falha ao renomear arquivo");
        return false;
    }
    Serial.println("Dados gravados com sucesso!");
    return true;
}

// Um arquivo truncado/corrompido não passa no parseProgram e cai no programa default
bool loadProgramFromFlash(Program &program){
    if(!LittleFS.exists(PROGRAM_FILE)){
        Serial.println("Nenhum programa salvo no LittleFS");
        return false;
    }

    File file = LittleFS.open(PROGRAM_FILE, FILE_READ);
    if(!file){
        Serial.println("Erro ao abrir arquivo para leitura");
        return false;
    }

    String json = file.readString();
    file.close();
    Serial.println("Programa carregado do LittleFS.");
    return parseProgram(json, program);
}

void plcTask(void *pvParameters){
    TickType_t lastWake = xTaskGetTickCount();
    unsigned long maxScanUs = 0;
    unsigned long lastReport = millis();

    while(true){
        unsigned long start = micros();
        executePLC();
        unsigned long scanUs = micros() - start;
        if(scanUs > maxScanUs) maxScanUs = scanUs;

#ifdef PLC_DEBUG
        printStateIfChanged();
        if(millis() - lastReport >= 5000){
            Serial.printf("Scan max: %lu us | Stack livre (plcTask): %u bytes\n",
                          maxScanUs, (unsigned)uxTaskGetStackHighWaterMark(NULL));
            maxScanUs = 0;
            lastReport = millis();
        }
#endif
        // período fixo de SCAN_MS (não SCAN_MS + tempo de execução)
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SCAN_MS));
    }
}

void handleUpload(const String &json){
    Serial.printf("Programa recebido (%u bytes)\n", (unsigned)json.length());

    // O parse é feito num Program separado: o PLC continua rodando o programa antigo
    Program *program = new Program();
    if(!parseProgram(json, *program)){
        delete program;
        Serial.println("Programa inválido - ignorado");
        return;
    }

    submitProgram(program);     // o plcTask troca no início do próximo scan
    if(!saveProgramToFlash(json)){
        Serial.println("Aviso: programa ativo, mas NÃO foi salvo na flash");
    }
    blinkLED(LED_PIN);          // Feedback para o usuário de upload bem-sucedido

#ifdef PLC_DEBUG
    Serial.printf("Stack livre (serialTask): %u bytes\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));
#endif
}

void serialTask(void *pvParameters){
    String buffer;
    buffer.reserve(MAX_PROGRAM_SIZE + 16);
    bool receiving = false;
    unsigned long lastByteMs = 0;

    while(true){
        while(Serial.available()){
            char c = Serial.read();
            lastByteMs = millis();
            buffer += c;

            if(!receiving){
                // Fora de uma transferência só guarda os últimos caracteres, procurando <START>
                if(buffer.endsWith("<START>")){
                    receiving = true;
                    buffer = "";
                }
                else if(buffer.length() > 16){
                    buffer.remove(0, buffer.length() - 16);
                }
            }
            else if(buffer.endsWith("<END>")){
                receiving = false;
                buffer.remove(buffer.length() - 5);
                handleUpload(buffer);
                buffer = "";
            }
            else if(buffer.length() > MAX_PROGRAM_SIZE){
                Serial.println("Programa excede o tamanho máximo - descartado");
                receiving = false;
                buffer = "";
            }
        }

        if(receiving && millis() - lastByteMs > RX_TIMEOUT_MS){
            Serial.println("Timeout de recepção - descartado");
            receiving = false;
            buffer = "";
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup(){
    Serial.setRxBufferSize(2048);   // antes do begin
    Serial.begin(115200);
    initIO();                       // pinMode de entradas/saídas (pinos definidos no io_manager)
    pinMode(LED_PIN, OUTPUT);

    if(!LittleFS.begin(true)){
        Serial.println("Erro ao montar LittleFS");
    }

    Program *program = new Program();
    if(!loadProgramFromFlash(*program)){
        Serial.println("Carregando programa default...");
        parseProgram(testJSON10, *program);
    }
    submitProgram(program);

    xTaskCreatePinnedToCore(plcTask, "PLC Task", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(serialTask, "Serial Task", 8192, NULL, 2, NULL, 0);
}

void loop(){/* Nada aqui, tudo roda via FreeRTOS */}
