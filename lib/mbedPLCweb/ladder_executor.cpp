#include "include/ladder_types.h"
#include "include/io_manager.h"

bool evaluateCompare(CompareNode &cmp){
    long left = readTagValue(cmp.left);
    long right;

    if(isdigit(cmp.right[0]) || cmp.right[0] == '-'){
        right = atol(cmp.right.c_str());
    }
    else{
        right = readTagValue(cmp.right);
    }
    if(cmp.op == ">") return left > right;
    if(cmp.op == "<") return left < right;
    if(cmp.op == ">=") return left >= right;
    if(cmp.op == "<=") return left <= right;
    if(cmp.op == "==") return left == right;
    if(cmp.op == "!=") return left != right;

    return false;
}

bool evaluateNode(Node &node){
    if(node.type == CONTACT){
        bool value = readTag(node.pin);
        if(node.mode == "NC"){
            value = !value;
        }
        return value;
    }

    if(node.type == R_TRIG){
        return risingEdge(node.pin);
    }

    if(node.type == F_TRIG){
        return fallingEdge(node.pin);
    }

    if(node.type == COMPARE){
        return evaluateCompare(node.cmp);
    }

    if(node.type == OP){
        if(node.op == AND){
            bool result = true;
            for(auto &child : node.nodes){
                result = result && evaluateNode(child);
            }
            return result;
        }

        if(node.op == OR){
            bool result = false;
            for(auto &child : node.nodes){
                result = result || evaluateNode(child);
            }
            return result;
        }
    }
    return false;
}