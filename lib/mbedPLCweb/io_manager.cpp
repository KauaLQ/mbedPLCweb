#include <Arduino.h>
#include <map>
#include <string>

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