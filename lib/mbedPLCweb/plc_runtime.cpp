#include <Arduino.h>
#include <atomic>
#include "include/plc_runtime.h"
#include "include/ladder_executor.h"
#include "include/io_manager.h"

// Programa em execução: só o plcTask mexe nele
static Program *activeProgram = nullptr;
// Programa novo (já validado) esperando o próximo scan
static std::atomic<Program*> pendingProgram{nullptr};

void submitProgram(Program *program){
    // se havia um programa pendente que ninguém consumiu, descarta
    delete pendingProgram.exchange(program);
}

static void executeLine(Line &line){
    bool result = evaluateNode(line.logic);

    for(auto &out : line.outputs){
        switch(out.type){
            case OUT_COIL:
                writeTag(out.target, result);
                break;
            case OUT_SET:
                if(result) writeTag(out.target, true);
                break;
            case OUT_RESET:
                if(result) writeTag(out.target, false);
                break;
            case OUT_TON:
                executeTON(out.target.index, result, out.preset);
                break;
            case OUT_TOF:
                executeTOF(out.target.index, result, out.preset);
                break;
            case OUT_CTU:
                executeCTU(out.target.index, result, (int)out.preset);
                break;
            case OUT_RES_TIMER:
                if(result) resetTimer(out.target.index);
                break;
            case OUT_RES_COUNTER:
                if(result) resetCounter(out.target.index);
                break;
        }
    }
}

void executePLC(){
    // Troca de programa sempre entre scans: nunca no meio de um
    Program *next = pendingProgram.exchange(nullptr);
    if(next){
        delete activeProgram;
        activeProgram = next;
        resetPLCState();
    }

    /*
    scan cycle:
    1 snapshot das entradas
    2 executar linhas (só mexe nas imagens/memórias)
    3 escrever as saídas físicas
    */
    readInputs();
    if(activeProgram){
        for(auto &line : *activeProgram){
            executeLine(line);
        }
    }
    writeOutputs();
}
