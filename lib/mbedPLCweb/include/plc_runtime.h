#ifndef PLC_RUNTIME_H
#define PLC_RUNTIME_H

#include "ladder_types.h"

// Entrega um programa já validado ao PLC (pode ser chamada de qualquer task).
// O PLC passa a usar o novo programa no início do próximo scan.
// A função assume a posse do ponteiro.
void submitProgram(Program *program);

// Um ciclo de scan completo. Chamar somente a partir do plcTask.
void executePLC();

#endif
