#include "include/ladder_types.h"
#include "include/ladder_executor.h"
#include "include/io_manager.h"

std::vector<Line> program;

void executePLC(){
    for(auto &line : program){
        bool result = evaluateNode(line.logic);

        // Analisa timers da linha
        if(line.hasTimer){
            if(line.timer.type == "TON"){
                executeTON(
                    line.timer.name,
                    result,
                    line.timer.preset
                );
            }
            else if(line.timer.type == "TOF"){
                executeTOF(
                    line.timer.name,
                    result,
                    line.timer.preset
                );
            }
        }

        for(auto &out : line.outputs){
            if(out.type == "COIL"){
                if(out.pin[0] == 'Q'){
                    writeOutput(out.pin,result);
                }
                else if(out.pin[0] == 'M'){
                    writeMemory(out.pin,result);
                }
            }
            if(out.type == "SET"){
                if(result){
                    if(out.pin[0] == 'Q'){
                        writeOutput(out.pin,true);
                    }
                    else if(out.pin[0] == 'M'){
                        writeMemory(out.pin,true);
                    }
                }
            }
            if(out.type == "RESET"){
                if(result){
                    if(out.pin[0] == 'Q'){
                        writeOutput(out.pin,false);
                    }
                    else if(out.pin[0] == 'M'){
                        writeMemory(out.pin,false);
                    }
                }
            }
        }
    }
}