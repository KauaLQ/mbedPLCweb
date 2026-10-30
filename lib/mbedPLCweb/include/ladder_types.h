#pragma once
#include <stdint.h>
#include <vector>

// ---------------- TAGS ----------------
enum TagKind {
    TAG_NONE,
    TAG_CONST,      // número literal (usado no COMPARE)
    TAG_INPUT,      // I0..I3
    TAG_OUTPUT,     // Q0..Q1
    TAG_MEMORY,     // M0..M3
    TAG_TIMER,      // T0..T3
    TAG_COUNTER,    // C0..C3
    TAG_ANALOG      // A0..A1
};

enum TagField {
    FLD_NONE,
    FLD_EN,
    FLD_TT,
    FLD_DN,
    FLD_ACC,
    FLD_PRE
};

// Tag já resolvida no parse: "T0.DN" vira {TAG_TIMER, FLD_DN, 0}.
// Em tempo de scan não existe mais busca por string.
struct TagRef {
    TagKind kind = TAG_NONE;
    TagField field = FLD_NONE;
    uint8_t index = 0;
    long value = 0;     // só usado por TAG_CONST
};

// ---------------- LÓGICA (CONTATOS) ----------------
enum NodeType {
    CONTACT,
    R_TRIG,
    F_TRIG,
    COMPARE,
    OP
};

enum OpType {
    AND,
    OR
};

enum CmpType {
    CMP_GT,
    CMP_LT,
    CMP_GE,
    CMP_LE,
    CMP_EQ,
    CMP_NE
};

struct CompareNode {
    TagRef left;
    CmpType op = CMP_EQ;
    TagRef right;
};

struct Node {
    NodeType type = CONTACT;
    // CONTACT, R_TRIG, F_TRIG
    TagRef tag;
    bool invert = false;    // true = contato NC
    // R_TRIG, F_TRIG: o estado anterior pertence ao nó, não ao pino
    bool prev = false;
    bool primed = false;    // false até o primeiro scan (evita borda falsa ao ligar)
    // OP
    OpType op = AND;
    std::vector<Node> nodes;
    // COMPARE
    CompareNode cmp;
};

// ---------------- SAÍDAS ----------------
enum OutputType {
    OUT_COIL,
    OUT_SET,
    OUT_RESET,          // só Q e M
    OUT_TON,
    OUT_TOF,
    OUT_CTU,
    OUT_RES_TIMER,
    OUT_RES_COUNTER
};

struct Output {
    OutputType type = OUT_COIL;
    TagRef target;
    unsigned long preset = 0;   // TON, TOF, CTU
};

struct Timer {
    bool EN = false;
    bool TT = false;
    bool DN = false;

    unsigned long preset = 0;
    unsigned long accum = 0;

    unsigned long startTime = 0;
};

struct Counter {
    bool DN = false;   // done (ACC >= PRE)
    int ACC = 0;       // acumulador
    int PRE = 0;       // preset
    bool prevCU = false;
};

struct Line {
    Node logic;
    std::vector<Output> outputs;
};

using Program = std::vector<Line>;
