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

bool readInput(std::string pin){
    int gpio = inputMap[pin];
    return digitalRead(gpio);
}

void writeOutput(std::string pin, bool value){
    int gpio = outputMap[pin];
    digitalWrite(gpio,value);
}