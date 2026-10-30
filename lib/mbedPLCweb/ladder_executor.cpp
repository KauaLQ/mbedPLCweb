#include "include/ladder_executor.h"
#include "include/io_manager.h"

static bool evaluateCompare(const CompareNode &cmp){
    long left = readTagValue(cmp.left);
    long right = readTagValue(cmp.right);

    switch(cmp.op){
        case CMP_GT: return left > right;
        case CMP_LT: return left < right;
        case CMP_GE: return left >= right;
        case CMP_LE: return left <= right;
        case CMP_EQ: return left == right;
        case CMP_NE: return left != right;
    }
    return false;
}

// O estado anterior fica no próprio nó, então duas linhas com R_TRIG
// no mesmo pino não interferem entre si.
static bool detectEdge(Node &node, bool rising){
    bool current = readTag(node.tag);
    bool edge = false;

    // No primeiro scan só memoriza o estado: evita "borda" falsa ao ligar/carregar
    if(node.primed){
        edge = rising ? (!node.prev && current) : (node.prev && !current);
    }
    node.prev = current;
    node.primed = true;
    return edge;
}

bool evaluateNode(Node &node){
    switch(node.type){
        case CONTACT: {
            bool value = readTag(node.tag);
            return node.invert ? !value : value;
        }
        case R_TRIG:  return detectEdge(node, true);
        case F_TRIG:  return detectEdge(node, false);
        case COMPARE: return evaluateCompare(node.cmp);
        case OP: {
            bool result = (node.op == AND);
            // Avalia SEMPRE todos os filhos (sem short-circuit),
            // senão um R_TRIG/F_TRIG que não foi avaliado perde a borda.
            for(auto &child : node.nodes){
                bool value = evaluateNode(child);
                result = (node.op == AND) ? (result && value) : (result || value);
            }
            return result;
        }
    }
    return false;
}
