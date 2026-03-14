#pragma once
#include <vector>
#include <string>

enum NodeType {
    CONTACT,
    OP
};

enum OpType {
    AND,
    OR
};

struct Node {
    NodeType type;
    // CONTACT
    std::string pin;
    std::string mode;
    // OP
    OpType op;
    std::vector<Node> nodes;
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

struct Line {
    Node logic;
    std::vector<Output> outputs;
    bool hasTimer = false;
    TimerInstruction timer;
};

extern std::vector<Line> program;