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

struct Line {
    Node logic;
    std::vector<std::string> outputs;
};

extern std::vector<Line> program;