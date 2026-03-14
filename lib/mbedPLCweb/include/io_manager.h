#ifndef IO_MANAGER_H
#define IO_MANAGER_H

bool readTag(std::string pin);
void writeOutput(std::string pin, bool value);
void startOutput();

#endif