#include <Arduino.h>

/*
* Programa default (usado quando não há programa salvo na flash).
* Entradas são ativas em nível baixo: contato NO = botão PRESSIONADO.
*
*   I0 = start | I1 = stop (contato NC: fecha enquanto solto) | I2 = incrementa contador
*   L1: M0 = (I0 OU M0) E NÃO I1            (selo)
*   L2: M0 liga o timer T0 (5 s)
*   L3: T0.DN  -> Q0
*   L4: borda de subida de I2 conta em C0 (preset 3)
*   L5: T0.ACC >= 2500 -> Q1
*   L6: C0.DN reseta T0 e C0
*/
String testJSON10 = R"rawliteral(
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
           {"type":"CONTACT","pin":"I0","mode":"NO"},
           {"type":"CONTACT","pin":"M0","mode":"NO"}
         ]
       },
       {"type":"CONTACT","pin":"I1","mode":"NC"}
     ]
   },
   "outputs":[
     {"type":"COIL","pin":"M0"}
   ]
  },
  {
   "logic":{"type":"CONTACT","pin":"M0","mode":"NO"},
   "outputs":[
     {"type":"TON","pin":"T0","preset":5000}
   ]
  },
  {
   "logic":{"type":"CONTACT","pin":"T0.DN","mode":"NO"},
   "outputs":[
     {"type":"COIL","pin":"Q0"}
   ]
  },
  {
   "logic":{"type":"R_TRIG","pin":"I2"},
   "outputs":[
     {"type":"CTU","pin":"C0","preset":3}
   ]
  },
  {
   "logic":{"type":"COMPARE","left":"T0.ACC","op":">=","right":"2500"},
   "outputs":[
     {"type":"COIL","pin":"Q1"}
   ]
  },
  {
   "logic":{"type":"CONTACT","pin":"C0.DN","mode":"NO"},
   "outputs":[
     {"type":"RES_TIMER","pin":"T0"},
     {"type":"RES_COUNTER","pin":"C0"}
   ]
  }
 ]
}
)rawliteral";
