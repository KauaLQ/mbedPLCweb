#include <vector>
#include <ArduinoJson.h>
#include "include/ladder_types.h"

Node parseNode(JsonObject obj){
    Node node;
    String type = obj["type"] | "";

    if(type == "CONTACT"){
        node.type = CONTACT;
        node.pin = obj["pin"].as<String>().c_str();
        node.mode = obj["mode"].as<String>().c_str();
    }
    else if(type == "R_TRIG"){
        node.type = R_TRIG;
        node.pin = obj["pin"].as<String>().c_str();
    }
    else if(type == "F_TRIG"){
        node.type = F_TRIG;
        node.pin = obj["pin"].as<String>().c_str();
    }
    else if(type == "OP"){
        node.type = OP;
        String op = obj["op"];

        if(op == "AND"){
            node.op = AND;
        }
        else{
            node.op = OR;
        }

        JsonArray children = obj["nodes"];

        for(JsonObject child : children){
            node.nodes.push_back(parseNode(child));
        }
    }
    else{
        Serial.println("Tipo desconhecido");
    }
    return node;
}

Line parseLine(JsonObject obj){
    Line line;
    JsonObject logicObj = obj["logic"];
    line.logic = parseNode(logicObj);
    // -------- OUTPUTS --------
    if(obj["outputs"].is<JsonArray>()){
        JsonArray outputs = obj["outputs"];
        for(JsonObject out : outputs){
            Output o;
            o.pin = out["pin"].as<String>().c_str();
            o.type = out["type"] | "COIL";
            line.outputs.push_back(o);
        }
    }
    // -------- TIMER --------
    if(obj["timer"].is<JsonObject>()){
        JsonObject timerObj = obj["timer"];
        line.hasTimer = true;
        line.timer.type = timerObj["type"].as<String>().c_str();
        line.timer.name = timerObj["name"].as<String>().c_str();
        line.timer.preset = timerObj["preset"];
    }
    return line;
}

void parseProgram(String json){
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc,json);

    if(error){
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(error.c_str());
        return;
    }

    JsonArray lines = doc["lines"];
    program.clear();

    for(JsonObject lineObj : lines){
        Line line = parseLine(lineObj);
        program.push_back(line);
    }

    Serial.println("Programa carregado!");
}