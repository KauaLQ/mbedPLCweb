#include <Arduino.h>
#include "plc_runtime.h"
#include "ladder_parser.h"
#include "io_manager.h"
#include "testjson.h"

void setup(){
    pinMode(16, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(18, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);

    Serial.begin(115200);
    parseProgram(testJSON10);
}

void loop(){
    // scan cycle
    // 1 ler entradas
    // (feito dentro de readTag)
    // 2 executar linhas
    executePLC();
    // 3 atualizar saídas
    // (feito dentro de writeOutput)
    delay(10);
}