#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include "ladder_types.h"

const uint8_t NUM_INPUTS   = 4;
const uint8_t NUM_OUTPUTS  = 2;
const uint8_t NUM_MEMORIES = 4;
const uint8_t NUM_TIMERS   = 4;
const uint8_t NUM_COUNTERS = 4;
const uint8_t NUM_ANALOGS  = 2;

void initIO();

// ---- Tags (usadas pelo parser) ----
bool resolveTag(const char *name, TagRef &ref);
bool isBoolTag(const TagRef &ref);      // pode ser usada em contatos
bool isValueTag(const TagRef &ref);     // pode ser usada no COMPARE
bool isWritableTag(const TagRef &ref);  // pode receber COIL/SET/RESET (Q ou M)

// ---- Ciclo de scan ----
void readInputs();      // 1. snapshot das entradas
void writeOutputs();    // 3. escreve as saídas físicas uma única vez

// ---- Acesso às tags durante o scan ----
bool readTag(const TagRef &ref);
long readTagValue(const TagRef &ref);
void writeTag(const TagRef &ref, bool value);

// ---- Timers e contadores (idx = número da tag) ----
void executeTON(uint8_t idx, bool rungCondition, unsigned long preset);
void executeTOF(uint8_t idx, bool rungCondition, unsigned long preset);
void resetTimer(uint8_t idx);
void executeCTU(uint8_t idx, bool rungCondition, int preset);
void resetCounter(uint8_t idx);

void resetPLCState();
void printStateIfChanged();   // debug: imprime I/Q/M/T/C quando algo muda

#endif
