#include <Arduino.h>
#include <map>
#include <string>
#include "include/ladder_types.h"

std::map<std::string,int> inputMap = {
    {"I0",16},
    {"I1",17},
    {"I2",18},
    {"I3",19}
};

std::map<std::string,int> outputMap = {
    {"Q0",32},
    {"Q1",33}
};

// Armazena o estado das saídas
std::map<std::string,bool> outputState;

// Armazena o estado de memórias internas da lógica ladder
std::map<std::string,bool> memoryState = {
    {"M0",false},
    {"M1",false},
    {"M2",false},
    {"M3",false}
};

std::map<std::string, TimerTON> timerMap = {
    {"T0",TimerTON()},
    {"T1",TimerTON()},
    {"T2",TimerTON()},
    {"T3",TimerTON()}
};

bool readTag(std::string pin){
    if(inputMap.count(pin)){
        int gpio = inputMap[pin];
        return digitalRead(gpio);
    }

    if(outputState.count(pin)){
        return outputState[pin];
    }

    if(memoryState.count(pin)){
        return memoryState[pin];
    }

    if(pin.find('.') != std::string::npos){
        std::string timerName = pin.substr(0, pin.find('.'));
        std::string field = pin.substr(pin.find('.')+1);
        if(timerMap.count(timerName)){
            TimerTON &t = timerMap[timerName];
            if(field == "DN") return t.DN;
            if(field == "TT") return t.TT;
            if(field == "EN") return t.EN;
        }
    }
    return false;
}

// Define as entradas para false inicialmente
void startOutput(){
    for(auto &out : outputMap){
        outputState[out.first] = false;
    }
}

void writeOutput(std::string pin, bool value){
    int gpio = outputMap[pin];
    outputState[pin] = value;
    digitalWrite(gpio,value);
}

void writeMemory(std::string pin, bool value){
    memoryState[pin] = value;
}

void executeTON(std::string name, bool rungCondition, unsigned long preset){
    TimerTON &t = timerMap[name];
    t.preset = preset;
    unsigned long now = millis();

    if(rungCondition){
        if(!t.EN){
            t.EN = true;
            t.startTime = now;
        }
        t.accum = now - t.startTime;

        if(t.accum >= t.preset){
            t.DN = true;
            t.TT = false;
        }
        else{
            t.TT = true;
            t.DN = false;
        }
    }else{
        t.EN = false;
        t.TT = false;
        t.DN = false;
        t.accum = 0;
    }
}