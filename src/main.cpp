#include <Arduino.h>
#include "plc_runtime.h"
#include "ladder_parser.h"

String testJSON = R"rawliteral(
{
 "lines":[
  {
   "logic":{
     "type":"OP",
     "op":"OR",
     "nodes":[
       {
         "type":"OP",
         "op":"OR",
         "nodes":[
           {"type":"CONTACT","pin":"I0","mode":"NO"},
           {"type":"CONTACT","pin":"I1","mode":"NO"}
         ]
       },
       {"type":"CONTACT","pin":"I2","mode":"NO"}
     ]
   },
   "outputs":[
     {"pin":"Q0"},
     {"pin":"Q1"}
   ]
  }
 ]
}
)rawliteral";

String testJSON2 = R"rawliteral(
{
 "lines":[
  {
    "logic":{
      "type":"OP",
      "op":"AND",
      "nodes":[
        {
          "type":"OP",
          "op":"OR",
          "nodes":[
            {
              "type":"OP",
              "op":"AND",
              "nodes":[
                {"type":"CONTACT","pin":"I0","mode":"NO"},
                {"type":"CONTACT","pin":"I1","mode":"NO"}
              ]
            },
            {"type":"CONTACT","pin":"I2","mode":"NO"}
          ]
        },
        {"type":"CONTACT","pin":"I3","mode":"NO"}
      ]
    },
    "outputs":[
      {"pin":"Q0"},
      {"pin":"Q1"}
    ]
  }
 ]
}
)rawliteral";

void setup(){
    pinMode(16, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(18, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);

    Serial.begin(115200);
    parseProgram(testJSON);
}

void loop(){
    // scan cycle
    // 1 ler entradas
    // (feito dentro de readInput)
    // 2 executar linhas
    executePLC();
    // 3 atualizar saídas
    // (feito dentro de writeOutput)
    delay(10);
}