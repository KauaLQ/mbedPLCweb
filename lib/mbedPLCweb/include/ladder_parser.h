#ifndef LADDER_PARSER_H
#define LADDER_PARSER_H

#include <Arduino.h>
#include "ladder_types.h"

// Valida o JSON e converte para `out`.
// Se qualquer coisa estiver errada retorna false e deixa `out` vazio.
bool parseProgram(const String &json, Program &out);

#endif
