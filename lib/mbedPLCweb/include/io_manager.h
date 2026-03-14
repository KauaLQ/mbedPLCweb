#ifndef IO_MANAGER_H
#define IO_MANAGER_H

bool readTag(std::string pin);
void startOutput();
void writeOutput(std::string pin, bool value);
void writeMemory(std::string pin, bool value);

#endif