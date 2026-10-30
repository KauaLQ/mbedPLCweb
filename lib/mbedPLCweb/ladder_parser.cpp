#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include <utility>
#include "include/ladder_parser.h"
#include "include/io_manager.h"

static const int MAX_DEPTH = 8;     // aninhamento máximo de OP

static bool tagError(const char *name){
    Serial.printf("Tag invalida ou nao permitida: %s\n", name);
    return false;
}

static bool parseBoolTag(const char *name, TagRef &ref){
    if(!resolveTag(name, ref) || !isBoolTag(ref)) return tagError(name);
    return true;
}

// Aceita número direto no JSON (2500) ou string ("2500", "T0.ACC", "A0")
static bool parseValueTag(JsonVariantConst v, TagRef &ref){
    if(v.is<long>()){
        ref = TagRef();
        ref.kind = TAG_CONST;
        ref.value = v.as<long>();
        return true;
    }
    const char *name = v | "";
    if(!resolveTag(name, ref) || !isValueTag(ref)) return tagError(name);
    return true;
}

static bool parseCompareOp(const char *op, CmpType &out){
    if(strcmp(op, ">") == 0)       out = CMP_GT;
    else if(strcmp(op, "<") == 0)  out = CMP_LT;
    else if(strcmp(op, ">=") == 0) out = CMP_GE;
    else if(strcmp(op, "<=") == 0) out = CMP_LE;
    else if(strcmp(op, "==") == 0) out = CMP_EQ;
    else if(strcmp(op, "!=") == 0) out = CMP_NE;
    else return false;
    return true;
}

static bool parseNode(JsonObjectConst obj, Node &node, int depth){
    if(depth > MAX_DEPTH){
        Serial.println("Aninhamento de OP excede o limite");
        return false;
    }
    const char *type = obj["type"] | "";

    if(strcmp(type, "CONTACT") == 0){
        node.type = CONTACT;
        const char *mode = obj["mode"] | "NO";
        if(strcmp(mode, "NO") == 0)      node.invert = false;
        else if(strcmp(mode, "NC") == 0) node.invert = true;
        else{
            Serial.printf("Modo invalido: %s\n", mode);
            return false;
        }
        return parseBoolTag(obj["pin"] | "", node.tag);
    }

    if(strcmp(type, "R_TRIG") == 0){
        node.type = R_TRIG;
        return parseBoolTag(obj["pin"] | "", node.tag);
    }

    if(strcmp(type, "F_TRIG") == 0){
        node.type = F_TRIG;
        return parseBoolTag(obj["pin"] | "", node.tag);
    }

    if(strcmp(type, "COMPARE") == 0){
        node.type = COMPARE;
        const char *op = obj["op"] | "";
        if(!parseCompareOp(op, node.cmp.op)){
            Serial.printf("Operador de comparacao invalido: %s\n", op);
            return false;
        }
        return parseValueTag(obj["left"], node.cmp.left) &&
               parseValueTag(obj["right"], node.cmp.right);
    }

    if(strcmp(type, "OP") == 0){
        node.type = OP;
        const char *op = obj["op"] | "";
        if(strcmp(op, "AND") == 0)     node.op = AND;
        else if(strcmp(op, "OR") == 0) node.op = OR;
        else{
            Serial.printf("Operador logico invalido: %s\n", op);
            return false;
        }

        JsonArrayConst children = obj["nodes"];
        if(children.isNull() || children.size() == 0){
            Serial.println("OP sem filhos");
            return false;
        }
        for(JsonObjectConst child : children){
            Node childNode;
            if(!parseNode(child, childNode, depth + 1)) return false;
            node.nodes.push_back(std::move(childNode));
        }
        return true;
    }

    Serial.printf("Tipo de no desconhecido: %s\n", type);
    return false;
}

static bool isTimerTag(const TagRef &ref){
    return ref.kind == TAG_TIMER && ref.field == FLD_NONE;
}

static bool isCounterTag(const TagRef &ref){
    return ref.kind == TAG_COUNTER && ref.field == FLD_NONE;
}

static bool parseOutput(JsonObjectConst obj, Output &out){
    const char *type = obj["type"] | "COIL";
    const char *pin = obj["pin"] | "";
    if(!resolveTag(pin, out.target)) return tagError(pin);

    bool targetOk = false;
    bool needsPreset = false;

    if(strcmp(type, "COIL") == 0)       { out.type = OUT_COIL;        targetOk = isWritableTag(out.target); }
    else if(strcmp(type, "SET") == 0)   { out.type = OUT_SET;         targetOk = isWritableTag(out.target); }
    else if(strcmp(type, "RESET") == 0) { out.type = OUT_RESET;       targetOk = isWritableTag(out.target); }
    else if(strcmp(type, "TON") == 0)   { out.type = OUT_TON;         targetOk = isTimerTag(out.target);   needsPreset = true; }
    else if(strcmp(type, "TOF") == 0)   { out.type = OUT_TOF;         targetOk = isTimerTag(out.target);   needsPreset = true; }
    else if(strcmp(type, "CTU") == 0)   { out.type = OUT_CTU;         targetOk = isCounterTag(out.target); needsPreset = true; }
    else if(strcmp(type, "RES_TIMER") == 0)   { out.type = OUT_RES_TIMER;   targetOk = isTimerTag(out.target); }
    else if(strcmp(type, "RES_COUNTER") == 0) { out.type = OUT_RES_COUNTER; targetOk = isCounterTag(out.target); }
    else{
        Serial.printf("Tipo de saida desconhecido: %s\n", type);
        return false;
    }

    if(!targetOk) return tagError(pin);

    if(needsPreset){
        JsonVariantConst preset = obj["preset"];
        if(!preset.is<long>() || preset.as<long>() < 0){
            Serial.printf("Preset ausente ou invalido em %s\n", pin);
            return false;
        }
        out.preset = preset.as<long>();
    }
    return true;
}

static bool parseLine(JsonObjectConst obj, Line &line){
    if(!parseNode(obj["logic"], line.logic, 0)) return false;

    // outputs é opcional
    JsonArrayConst outputs = obj["outputs"];
    for(JsonObjectConst outObj : outputs){
        Output out;
        if(!parseOutput(outObj, out)) return false;
        line.outputs.push_back(out);
    }
    return true;
}

bool parseProgram(const String &json, Program &out){
    out.clear();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if(error){
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonArrayConst lines = doc["lines"];
    if(lines.isNull()){
        Serial.println("Campo 'lines' ausente");
        return false;
    }

    for(JsonObjectConst lineObj : lines){
        Line line;
        if(!parseLine(lineObj, line)){
            Serial.printf("Erro na linha %u\n", (unsigned)(out.size() + 1));
            out.clear();
            return false;
        }
        out.push_back(std::move(line));
    }

    Serial.println("Programa validado!");
    return true;
}
