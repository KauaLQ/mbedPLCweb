#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "include/io_manager.h"

// Entradas com INPUT_PULLUP: botão pressionado = nível LOW.
// true  -> contato NO fecha ao pressionar o botão (comportamento de um PLC real)
// false -> lógica direta (HIGH = ativo)
static const bool INPUT_ACTIVE_LOW = true;

// O índice do array é o número da tag: I0 -> inputPins[0], Q1 -> outputPins[1]...
static const uint8_t inputPins[NUM_INPUTS]   = {16, 17, 18, 19};
static const uint8_t outputPins[NUM_OUTPUTS] = {32, 33};

/*
* NOTE: Esse array serve para a implementação futura de entradas analógicas.
*       Eu pretendo utilizar um AD externo para essa tarefa, como exemplo o ADS1115.
*       Então, na prática, seria algo como <NOME, ENDEREÇO> e não <NOME, PINO>.
*/
static const uint8_t analogPins[NUM_ANALOGS] = {34, 35};

// ---------------- ESTADO DO PLC ----------------
// Imagem das entradas: lida uma vez no início do scan
static bool inputImage[NUM_INPUTS];
static long analogImage[NUM_ANALOGS];
// Imagem das saídas: a lógica escreve aqui, o GPIO só muda no fim do scan
static bool outputImage[NUM_OUTPUTS];
// Memórias internas da lógica ladder
static bool memoryState[NUM_MEMORIES];
static Timer timers[NUM_TIMERS];
static Counter counters[NUM_COUNTERS];

// Última linha impressa por printStateIfChanged()
static char lastState[128] = "";

void initIO(){
    for(uint8_t i = 0; i < NUM_INPUTS; i++){
        pinMode(inputPins[i], INPUT_PULLUP);
    }
    for(uint8_t i = 0; i < NUM_OUTPUTS; i++){
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
    }
}

// ---------------- TAGS ----------------
static bool parseField(const char *s, TagField &field){
    if(strcmp(s, "EN") == 0)  field = FLD_EN;
    else if(strcmp(s, "TT") == 0)  field = FLD_TT;
    else if(strcmp(s, "DN") == 0)  field = FLD_DN;
    else if(strcmp(s, "ACC") == 0) field = FLD_ACC;
    else if(strcmp(s, "PRE") == 0) field = FLD_PRE;
    else return false;
    return true;
}

// Converte "I0", "Q1", "T0.DN", "C2.ACC", "A0" ou "2500" em TagRef.
// Retorna false para qualquer tag que não exista no hardware/memória.
bool resolveTag(const char *name, TagRef &ref){
    ref = TagRef();
    if(name == nullptr || name[0] == '\0') return false;

    // número literal (COMPARE): 2500, -10
    if(isdigit((unsigned char)name[0]) || name[0] == '-'){
        char *end;
        ref.kind = TAG_CONST;
        ref.value = strtol(name, &end, 10);
        return end != name && *end == '\0';
    }

    // letra + número: I0, Q1, M3, T0, C2, A0
    const char *p = name + 1;
    if(!isdigit((unsigned char)*p)) return false;

    int index = 0;
    while(isdigit((unsigned char)*p)){
        index = index * 10 + (*p - '0');
        if(index > 255) return false;
        p++;
    }

    uint8_t limit = 0;
    switch(name[0]){
        case 'I': ref.kind = TAG_INPUT;   limit = NUM_INPUTS;   break;
        case 'Q': ref.kind = TAG_OUTPUT;  limit = NUM_OUTPUTS;  break;
        case 'M': ref.kind = TAG_MEMORY;  limit = NUM_MEMORIES; break;
        case 'T': ref.kind = TAG_TIMER;   limit = NUM_TIMERS;   break;
        case 'C': ref.kind = TAG_COUNTER; limit = NUM_COUNTERS; break;
        case 'A': ref.kind = TAG_ANALOG;  limit = NUM_ANALOGS;  break;
        default: return false;
    }
    if(index >= limit) return false;
    ref.index = index;

    // campo opcional: T0.DN, C1.ACC (só existe em timers e contadores)
    if(*p == '\0') return true;
    if(*p != '.') return false;
    if(ref.kind != TAG_TIMER && ref.kind != TAG_COUNTER) return false;
    if(!parseField(p + 1, ref.field)) return false;
    if(ref.kind == TAG_COUNTER && (ref.field == FLD_EN || ref.field == FLD_TT)) return false;
    return true;
}

bool isBoolTag(const TagRef &ref){
    switch(ref.kind){
        case TAG_INPUT:
        case TAG_OUTPUT:
        case TAG_MEMORY:  return ref.field == FLD_NONE;
        case TAG_TIMER:   return ref.field == FLD_EN || ref.field == FLD_TT || ref.field == FLD_DN;
        case TAG_COUNTER: return ref.field == FLD_DN;
        default:          return false;
    }
}

bool isValueTag(const TagRef &ref){
    switch(ref.kind){
        case TAG_CONST:
        case TAG_ANALOG:  return true;
        case TAG_TIMER:
        case TAG_COUNTER: return ref.field == FLD_ACC || ref.field == FLD_PRE;
        default:          return false;
    }
}

bool isWritableTag(const TagRef &ref){
    return (ref.kind == TAG_OUTPUT || ref.kind == TAG_MEMORY) && ref.field == FLD_NONE;
}

// ---------------- CICLO DE SCAN ----------------
void readInputs(){
    for(uint8_t i = 0; i < NUM_INPUTS; i++){
        bool level = digitalRead(inputPins[i]);
        inputImage[i] = INPUT_ACTIVE_LOW ? !level : level;
    }
    for(uint8_t i = 0; i < NUM_ANALOGS; i++){
        analogImage[i] = analogRead(analogPins[i]);
    }
}

void writeOutputs(){
    for(uint8_t i = 0; i < NUM_OUTPUTS; i++){
        digitalWrite(outputPins[i], outputImage[i]);
    }
}

bool readTag(const TagRef &ref){
    switch(ref.kind){
        case TAG_INPUT:  return inputImage[ref.index];
        case TAG_OUTPUT: return outputImage[ref.index];
        case TAG_MEMORY: return memoryState[ref.index];
        case TAG_TIMER: {
            Timer &t = timers[ref.index];
            if(ref.field == FLD_EN) return t.EN;
            if(ref.field == FLD_TT) return t.TT;
            if(ref.field == FLD_DN) return t.DN;
            return false;
        }
        case TAG_COUNTER:
            return ref.field == FLD_DN && counters[ref.index].DN;
        default:
            return false;
    }
}

long readTagValue(const TagRef &ref){
    switch(ref.kind){
        case TAG_CONST:  return ref.value;
        case TAG_ANALOG: return analogImage[ref.index];
        case TAG_TIMER: {
            Timer &t = timers[ref.index];
            if(ref.field == FLD_ACC) return t.accum;
            if(ref.field == FLD_PRE) return t.preset;
            return 0;
        }
        case TAG_COUNTER: {
            Counter &c = counters[ref.index];
            if(ref.field == FLD_ACC) return c.ACC;
            if(ref.field == FLD_PRE) return c.PRE;
            return 0;
        }
        default:
            return 0;
    }
}

// Só escreve na imagem; o GPIO é atualizado por writeOutputs()
void writeTag(const TagRef &ref, bool value){
    if(ref.kind == TAG_OUTPUT)      outputImage[ref.index] = value;
    else if(ref.kind == TAG_MEMORY) memoryState[ref.index] = value;
}

// ---------------- TIMERS ----------------
void executeTON(uint8_t idx, bool rungCondition, unsigned long preset){
    Timer &t = timers[idx];
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

void executeTOF(uint8_t idx, bool rungCondition, unsigned long preset){
    Timer &t = timers[idx];
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

void resetTimer(uint8_t idx){
    timers[idx] = Timer();
}

// ---------------- CONTADORES ----------------
void executeCTU(uint8_t idx, bool rungCondition, int preset){
    Counter &c = counters[idx];
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

void resetCounter(uint8_t idx){
    Counter &c = counters[idx];
    c.ACC = 0;
    c.DN = false;
}

// ---------------- RESET / DEBUG ----------------
void resetPLCState(){
    for(uint8_t i = 0; i < NUM_OUTPUTS; i++)  outputImage[i] = false;
    for(uint8_t i = 0; i < NUM_MEMORIES; i++) memoryState[i] = false;
    for(uint8_t i = 0; i < NUM_TIMERS; i++)   timers[i] = Timer();
    for(uint8_t i = 0; i < NUM_COUNTERS; i++) counters[i] = Counter();

    lastState[0] = '\0';    // força a impressão do estado inicial
    Serial.println("PLC state resetado!");
}

// Formato: I=0100 Q=10 M=0001 T=110,000,000,000 C=2,0,0,0
//   I/Q/M = um bit por tag (I0 primeiro) | T = EN,TT,DN de cada timer | C = ACC de cada contador
// Só imprime quando algo muda (o ACC do timer fica de fora para não imprimir a cada scan).
void printStateIfChanged(){
    char buf[128];
    size_t n = 0;

    n += snprintf(buf + n, sizeof(buf) - n, "I=");
    for(uint8_t i = 0; i < NUM_INPUTS; i++)   n += snprintf(buf + n, sizeof(buf) - n, "%d", inputImage[i]);
    n += snprintf(buf + n, sizeof(buf) - n, " Q=");
    for(uint8_t i = 0; i < NUM_OUTPUTS; i++)  n += snprintf(buf + n, sizeof(buf) - n, "%d", outputImage[i]);
    n += snprintf(buf + n, sizeof(buf) - n, " M=");
    for(uint8_t i = 0; i < NUM_MEMORIES; i++) n += snprintf(buf + n, sizeof(buf) - n, "%d", memoryState[i]);
    n += snprintf(buf + n, sizeof(buf) - n, " T=");
    for(uint8_t i = 0; i < NUM_TIMERS; i++){
        n += snprintf(buf + n, sizeof(buf) - n, "%s%d%d%d", i ? "," : "", timers[i].EN, timers[i].TT, timers[i].DN);
    }
    n += snprintf(buf + n, sizeof(buf) - n, " C=");
    for(uint8_t i = 0; i < NUM_COUNTERS; i++){
        n += snprintf(buf + n, sizeof(buf) - n, "%s%d", i ? "," : "", counters[i].ACC);
    }

    if(strcmp(buf, lastState) != 0){
        Serial.println(buf);
        strcpy(lastState, buf);
    }
}
