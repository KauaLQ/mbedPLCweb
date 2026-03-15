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
// Inicializa o estado das saídas automaticamente baseando-se no outputMap
std::map<std::string, bool> outputState = []() {
    std::map<std::string, bool> initialMap;
    for (auto const& pair : outputMap) {
        initialMap[pair.first] = false;
    }
    return initialMap;
}();

// Armazena o estado de memórias internas da lógica ladder
std::map<std::string,bool> memoryState = {
    {"M0",false},
    {"M1",false},
    {"M2",false},
    {"M3",false}
};

std::map<std::string, Timer> timerMap = {
    {"T0",Timer()},
    {"T1",Timer()},
    {"T2",Timer()},
    {"T3",Timer()}
};

std::map<std::string, Counter> counterMap = {
    {"C0", Counter()},
    {"C1", Counter()},
    {"C2", Counter()},
    {"C3", Counter()}
};

std::map<std::string,bool> previousState;

/*
* NOTE: Esse map serve para a implementação futura de entradas analógicas.
*       Eu pretendo utilizar um AD externo para essa tarefa, como exemplo o ADS1115.
*       Então, na prática, o map seria algo como <NOME, ENDEREÇO> e não <NOME, PINO>.
*/
std::map<std::string,int> analogMap = {
    {"A0",34},
    {"A1",35}
};

long readTagValue(std::string tag){
    // campo tipo T0.ACC ou C0.ACC
    if(tag.find('.') != std::string::npos){
        size_t dotPos = tag.find('.');
        std::string name = tag.substr(0, dotPos);
        std::string field = tag.substr(dotPos + 1);

        if(timerMap.count(name)){
            Timer &t = timerMap[name];
            if(field == "ACC") return t.accum;
            if(field == "PRE") return t.preset;
        }
        if(counterMap.count(name)){
            Counter &c = counterMap[name];
            if(field == "ACC") return c.ACC;
            if(field == "PRE") return c.PRE;
        }
        // Implementação futura, provavelmente será alterada
        if(analogMap.count(tag)){
            int gpio = analogMap[tag];
            return analogRead(gpio);
        }
    }
    // fallback
    return 0;
}

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
        size_t dotPos = pin.find('.');
        std::string name = pin.substr(0, dotPos);
        std::string field = pin.substr(dotPos + 1);

        if(timerMap.count(name)){
            Timer &t = timerMap[name];
            if(field == "DN") return t.DN;
            if(field == "TT") return t.TT;
            if(field == "EN") return t.EN;
        }
        
        if(counterMap.count(name)){
            Counter &c = counterMap[name];
            if(field == "DN") return c.DN;
        }
    }
    return false;
}

bool risingEdge(std::string pin){
    bool current = readTag(pin);
    bool prev = previousState[pin];
    previousState[pin] = current;
    return (!prev && current);
}

bool fallingEdge(std::string pin){
    bool current = readTag(pin);
    bool prev = previousState[pin];
    previousState[pin] = current;
    return (prev && !current);
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
    Timer &t = timerMap[name];
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

void executeTOF(std::string name, bool rungCondition, unsigned long preset){
    Timer &t = timerMap[name];
    t.preset = preset;
    unsigned long now = millis();

    if(rungCondition){
        // entrada ativa → saída ativa imediatamente
        t.EN = true;
        t.DN = true;
        t.TT = false;
        // reseta temporização
        t.accum = 0;
    }
    else{
        // entrada desligou → começa temporizar
        if(t.EN){
            if(!t.TT){
                t.startTime = now;
                t.TT = true;
            }
            t.accum = now - t.startTime;

            if(t.accum >= t.preset){
                t.DN = false;
                t.TT = false;
                t.EN = false;
            }
        }
    }
}

void resetTimer(std::string name){
    Timer &t = timerMap[name];
    t.EN = false;
    t.TT = false;
    t.DN = false;
    t.accum = 0;
    t.startTime = 0;
}

void executeCTU(std::string name, bool rungCondition, int preset){
    Counter &c = counterMap[name];
    c.PRE = preset;
    // detectar borda de subida
    if(rungCondition && !c.prevCU){
        c.ACC++;
        if(c.ACC >= c.PRE){
            c.DN = true;
        }
    }
    c.prevCU = rungCondition;
}

void resetCounter(std::string name){
    Counter &c = counterMap[name];
    c.ACC = 0;
    c.DN = false;
}