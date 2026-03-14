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
    JsonArray outputs = obj["outputs"];

    for(JsonObject out : outputs){
        line.outputs.push_back(out["pin"].as<String>().c_str());
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