#pragma once
#include <vector>
#include <string>

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

struct CompareNode{
    std::string left;
    std::string op;
    std::string right;
};

struct Node {
    NodeType type;
    // CONTACT
    std::string pin;
    std::string mode;
    // OP
    OpType op;
    std::vector<Node> nodes;
    // COMPARE
    CompareNode cmp;
};

struct Output {
    std::string pin;
    std::string type; // COIL SET RESET
};

struct TimerInstruction{
    std::string type;
    std::string name;
    unsigned long preset;
};

struct Timer {
    bool EN = false;
    bool TT = false;
    bool DN = false;

    unsigned long preset = 0;
    unsigned long accum = 0;

    unsigned long startTime = 0;
};

struct CounterInstruction{
    std::string type; // CTU
    std::string name;
    int preset;
};

struct Counter {
    bool DN = false;   // done (ACC >= PRE)
    int ACC = 0;   // acumulador
    int PRE = 0;   // preset
    bool prevCU = false;
};

struct Line {
    Node logic;
    std::vector<Output> outputs;
    bool hasTimer = false;
    TimerInstruction timer;
    bool hasCounter = false;
    CounterInstruction counter;
};

extern std::vector<Line> program;