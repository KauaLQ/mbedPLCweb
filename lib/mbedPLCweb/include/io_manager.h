#ifndef IO_MANAGER_H
#define IO_MANAGER_H

bool readTag(std::string pin);
bool risingEdge(std::string pin);
bool fallingEdge(std::string pin);
void startOutput();
void writeOutput(std::string pin, bool value);
void writeMemory(std::string pin, bool value);
void executeTON(std::string name, bool rungCondition, unsigned long preset);
void executeTOF(std::string name, bool rungCondition, unsigned long preset);
void executeCTU(std::string name, bool rungCondition, int preset);
void resetCounter(std::string name);

#endif