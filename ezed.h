//
// Created by alex on 10/6/23.
//

#ifndef EZED_EZED_H
#define EZED_EZED_H

#include <stdbool.h>

struct LoopData;
typedef struct LoopData LoopData;

struct POS;
typedef struct POS POS;

#include "macros.h"

struct LoopData {
    char* c_file;
    int txt_lines;
    size_t txt_size;
    char** txt;
    int occ_c;
    POS** occ;
    bool running;

    char* inp;
    size_t inpl;

    tokenized tokens;
    macro* macros;
    size_t macroc;

    bool changed;
};

struct POS {
    int line;
    int offset;
    size_t amount;
};

void resolve_input(LoopData* data);

#endif //EZED_EZED_H
