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

#define SETTINGS_COUNT 1

struct LoopData {
    char *c_file;
    int txt_lines;
    size_t txt_size;
    char **txt;
    int occ_c;
    POS **occ;
    bool running;

    char *inp;
    size_t inpl;

    tokenized tokens;
    macro *macros;
    size_t macroc;

    bool changed;

    char *settings[SETTINGS_COUNT];
};

struct POS {
    int line;
    int offset;
    size_t amount;
};

void resolve_input(LoopData* data);

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#endif //EZED_EZED_H
