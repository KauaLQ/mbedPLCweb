#include "include/ladder_types.h"
#include "include/io_manager.h"

bool evaluateNode(Node &node){
    if(node.type == CONTACT){
        bool value = readInput(node.pin);
        if(node.mode == "NC"){
            value = !value;
        }
        return value;
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