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
            Timer &t = timerMap[timerName];
            if(field == "DN") return t.DN;
            if(field == "TT") return t.TT;
            if(field == "EN") return t.EN;
        }
    }

    if(pin.find('.') != std::string::npos){
        std::string counterName = pin.substr(0, pin.find('.'));
        std::string field = pin.substr(pin.find('.')+1);
        if(counterMap.count(counterName)){
            Counter &c = counterMap[counterName];
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

/*
* TODO: Dar um jeito de remover essa função de inicialização dos
*       Estados das saídas para deixar o main.cpp mais limpo
*/
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

/*
* NOTE: Em relação ao TON e TOF, eu não sei exatamente como eles funcionam.
*       Em alguns simuladores Eles são acionados com pulso e fica energinazados sem precisar de selo.
*       Eles são desativados com uma saída de RESET nomeada com seu label.
*       Já em alguns outros, é necessário fazer o selo com as próprias saídas do timer, por exemplo.
*       Atualmente, esta implementação é baseada no segundo caso.
*       Para o TON permanecer ligado, eu faço um selo com TX.EN, e coloco uma entrada em série para desligar.
*       e para o TOF permanecer ligado, eu faço um selo com TX.DN, e coloco uma entrada em série para desligar.
* TODO: Talvez seja interessante padronizar para ambos desligarem com a saída reset nomeada com seus labels
*       Ao invés de usar selos manuais.
*/
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

/*
* TODO: Talvez seja interessante não resetar o enable do timer,
*       pois resetar EN reseta a rung de selo, e não só a saída
*/
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