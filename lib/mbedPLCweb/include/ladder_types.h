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

struct Line {
    Node logic;
    std::vector<Output> outputs;
};

extern std::vector<Line> program;