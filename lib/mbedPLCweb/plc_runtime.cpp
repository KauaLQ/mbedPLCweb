#include "include/ladder_types.h"
#include "include/ladder_executor.h"
#include "include/io_manager.h"

std::vector<Line> program;

void executePLC(){
    for(auto &line : program){
        bool result = evaluateNode(line.logic);
        for(auto &out : line.outputs){
            writeOutput(out,result);
        }
    }
}