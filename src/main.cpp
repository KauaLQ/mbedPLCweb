#include <Arduino.h>
#include "plc_runtime.h"
#include "ladder_parser.h"
#include "io_manager.h"
#include "testjson.h"

bool receiving = false;
String serialBuffer = "";

void blinkLED(int pin){
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
}

void readSerial(){
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
                resetPLCState();
                blinkLED(2);
            }
            else{
                Serial.println("Programa inválido - ignorado");
            }
            serialBuffer = "";
        }
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
    parseProgram(testJSON10);
}

void loop(){
    readSerial(); // recebe programa
    // scan cycle
    // 1 ler entradas
    // (feito dentro de readTag)
    // 2 executar linhas
    executePLC();
    // 3 atualizar saídas
    // (feito dentro de writeOutput)
    delay(10);
}