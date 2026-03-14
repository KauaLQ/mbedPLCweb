#ifndef IO_MANAGER_H
#define IO_MANAGER_H

bool readTag(std::string pin);
void startOutput();
void writeOutput(std::string pin, bool value);
void writeMemory(std::string pin, bool value);
void executeTON(std::string name, bool rungCondition, unsigned long preset);
void executeTOF(std::string name, bool rungCondition, unsigned long preset);

#endif