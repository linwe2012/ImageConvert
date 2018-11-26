#include <iostream>
#include <sstream>
#include <fstream>

#define K_READ_NEXT(xx) xx = ss.get()
#define K_INPUT std::stringstream &ss
#define K_INPUT_PASSON ss

#define TERMINAL_INDICATOR "ready> "


//===----------------------------------------------------------------------===//
// Built-in Functions
//===----------------------------------------------------------------------===//
void pipeFile(const char *file, K_INPUT);
void pipeStdin(K_INPUT);
int suggestOperation(const char *str, K_INPUT);
int islineend(const int Ch);
int isquotation(const int Ch);