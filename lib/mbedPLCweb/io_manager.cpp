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

std::map<std::string,bool> outputState;

bool readTag(std::string pin){
    if(inputMap.count(pin)){
        int gpio = inputMap[pin];
        return digitalRead(gpio);
    }

    if(outputState.count(pin)){
        return outputState[pin];
    }
    return false;
}

// Define as entradas para false inicialmente
void startOutput(){
    for(auto &out : outputMap){
        outputState[out.first] = false;
    }
};

void writeOutput(std::string pin, bool value){
    int gpio = outputMap[pin];
    outputState[pin] = value;
    digitalWrite(gpio,value);
}