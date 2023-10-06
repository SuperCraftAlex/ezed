//
// Created by alex on 10/6/23.
//

#ifndef EZED_MACROS_H
#define EZED_MACROS_H

typedef struct string {
    char* str;
    size_t size;
} string;

typedef struct tokenized {
    string* tokens;
    size_t count;
} tokenized;

typedef struct macro {
    string* performs;
    size_t performc;
    size_t args;
    string name;
} macro;

#include "ezed.h"

void do_exec_macro(LoopData* data);

void do_macro_def(LoopData* data);

tokenized tokenize(char* str, int do_braces, int do_strings);

void free_tokenized(tokenized* tokens);

void free_macro(macro* mac);

#endif //EZED_MACROS_H
