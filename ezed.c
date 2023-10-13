#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "ezed.h"

void replace(char **str, int start, size_t amount, char *rep) {
    char *new = malloc(strlen(*str) + strlen(rep) + 1);

    (*str)[start] = '\0';

    new[0] = 0;
    strcat(new, *str);

    strcat(new, rep);

    strcat(new, *str + start + amount);

    free(*str);
    *str = new;
}


size_t inp_alloc_s = sizeof(char) * 50 * 50;
size_t txt_lines_amount = 500;
size_t txt_line_alloc_s = 400;

int get_indent(char *str) {
    int in = 0;
    for (int i = 0; i < strlen(str); ++i) {
        if (str[i] == ' ')
            in++;
        else
            break;
    }
    return in;
}

void show_help_message(void);

#define ALLOC_INC_STEP 200
void auto_increase_alloc(LoopData *data) {
    if (data->txt_lines > txt_lines_amount - 1) {
        // increase size of alloc
        int oldc = txt_lines_amount;
        int newc = txt_lines_amount + ALLOC_INC_STEP;
        data->txt = realloc(data->txt, newc * sizeof(char *));
        for (int i = oldc-1; i < newc; ++i) {
            data->txt[i] = malloc(txt_line_alloc_s);
        }
        txt_lines_amount = newc;
    }
}

typedef struct {
    bool is_range;
    int from;       // if open start, it is 0
    int to;         // if open end, it is -1

    // else
    int line;
} RangeArgument;

RangeArgument parse_args_range(size_t inpl, char *inp) {
    RangeArgument arg = {false, 0, -1, -1};
    if (inpl == 0) {
        return arg;
    }
    if (inp[0] == 0) {
        return arg;
    }
    for (size_t i = 0; i < inpl; ++i) {
        char c = inp[i];
        if (c == '-') {
            inp[i] = 0;
            arg.from = atoi(inp);
            arg.is_range = true;
            if (inpl > i + 1 && inp[i + 1] != 0) {
                arg.to = atoi(inp + i + 1);
            }
            return arg;
        }
    }
    arg.line = atoi(inp);
    return arg;
}

void do_replace(LoopData* data) { // r
    // replace

    for(int i = 0; i < data->occ_c; ++i) {
        POS *pos = data->occ[i];
        if (pos == NULL) {
            continue;
        }

        replace(&data->txt[pos->line], pos->offset, pos->amount, data->inp + 2);

        data->changed = true;
    }

    data->occ_c = 0;
}

void do_save(LoopData* data) { // s
    // save to file
    FILE *file = fopen(data->c_file, "w");
    if (file == NULL)
        printf("Error opening the file %s\n", data->c_file);

    for (int i = 0; i < data->txt_lines; ++i)
        fprintf(file, "%s\n", data->txt[i]);
    fclose(file);

    data->changed = false;
}

void do_file_info(LoopData* data) { // v
    printf("Find buffer length: %i\n", data->occ_c);
    putchar('\n');
    printf("File: %s\n", data->c_file);
    printf("Total lines: %i\n", data->txt_lines);
    printf("Total size: ~%zu bytes\n", data->txt_size);
    putchar('\n');
    printf("DEBUG:\n");
    printf("Allocated lines: %zu\n", txt_lines_amount);
    printf("Macros: %zu\n", data->macroc);
    putchar('\n');
}

void do_edit(LoopData* data, int keep_indent) {
    if(data->inpl <= 4) return;
    char *line_s = malloc(data->inpl);
    char *ltxt = malloc(data->inpl);

    int stage = 0;
    int c = 0;
    for(int i = 2; i < data->inpl; ++i) {
        if((data->inp[i] == ' ' && stage == 0) || data->inp[i] == '\n') {
            stage++;
            c = 0;
            continue;
        }
        if(stage == 0)
            line_s[c] = data->inp[i];
        else if(stage == 1)
            ltxt[c] = data->inp[i];
        c++;
    }
            
    int line = atoi(line_s);
    size_t ltxt_l = strlen(ltxt);

    if(line < data->txt_lines)
        data->txt_size -= strlen(data->txt[line]) + 1;

    if(keep_indent) {
        memcpy(data->txt[line] + get_indent(data->txt[line]), ltxt, ltxt_l + 1);
    }
    else {
        memcpy(data->txt[line], ltxt, ltxt_l + 1);
    }
    data->txt_size += ltxt_l + 1;

    free(line_s);
    free(ltxt);

    data->changed = true;
}

#define do_normal_edit(data) do_edit(data, false)
#define do_indent_edit(data) do_edit(data, true)

void do_insert(LoopData* data) { // i
    // insert line after...
    if(data->inpl <= 2) return;
    char *line_s = malloc(data->inpl);
    char *ltxt = malloc(data->inpl);

    int ltxt_lb = 0;

    int stage = 0;
    int c = 0;
    for (int i = 2; i < data->inpl; ++i) {
        if ((data->inp[i] == ' ' && stage == 0) || data->inp[i] == '\n' || data->inp[i] == 10) {
            stage++;
            c = 0;
            continue;
        }
        if (stage == 0)
            line_s[c] = data->inp[i];
        else if (stage == 1) {
            ltxt_lb ++;
            ltxt[c] = data->inp[i];
        }
        c++;
    }

    int line = atoi(line_s) + 1;
    size_t ltxt_l = strlen(ltxt);

    for (int i = data->txt_lines; i > line; --i) {
        memcpy(data->txt[i], data->txt[i - 1], strlen(data->txt[i - 1]) + 1);
    }

    if (ltxt_l > 1 && ltxt_lb > 0) {
        memcpy(data->txt[line], ltxt, ltxt_l + 1);
        data->txt_size += ltxt_l + 1;
    } else {
        memset(data->txt[line], 0, strlen(data->txt[line]));
        data->txt_size++;
    }
    data->txt_lines++;

    free(line_s);
    free(ltxt);

    auto_increase_alloc(data);

    data->changed = true;
}

static void delete_line(LoopData *data, int line) {
    data->txt_size -= strlen(data->txt[line]) + 1;

    for (int i = line; i < data->txt_lines-1; ++i) {
        memcpy(data->txt[i], data->txt[i + 1], strlen(data->txt[i + 1]) + 1);
    }

    data->txt_lines--;
}

static void delete_lines(LoopData *data, int from, int to) {
    size_t size = 0;
    for (int i = from; i <= to; ++i) {
        size += strlen(data->txt[i]) + 1;
    }
    data->txt_size -= size;

    for (int i = from; i < data->txt_lines - (to - from); ++i) {
        memcpy(data->txt[i], data->txt[i + (to - from) + 1], strlen(data->txt[i + (to - from) + 1]) + 1);
    }

    data->txt_lines -= (to - from) + 1;
}

void do_delete(LoopData* data) { // d
    if (data->inpl <= 2) return;

    RangeArgument arg = parse_args_range(data->inpl, data->inp + 2);
    if (arg.is_range) {
        int from = arg.from;
        int to = arg.to;
        if (to == -1 && data->txt_lines > 0) {
            to = data->txt_lines - 1;
        }

        delete_lines(data, from, to);
    }
    else {
        int line = arg.line;
        if (line < data->txt_lines) {
            delete_line(data, line);
        }
        else {
            printf("Line %i out of bounds!\n", line);
        }
    }

    data->changed = true;
}

void do_append(LoopData* data) { // a
    // append line
    if (data->inpl > 2) {
        memcpy(data->txt[data->txt_lines], data->inp+2, data->inpl-1);
        data->txt[data->txt_lines][data->inpl-3] = 0;
        data->txt[data->txt_lines][data->inpl-2] = 0;
        data->txt_lines ++;
        data->txt_size += strlen(data->inp + 2) + 1;
    }
    else {
        data->txt[data->txt_lines][0] = 0;
        data->txt_lines ++;
        data->txt_size += 1;
    }

    auto_increase_alloc(data);

    data->changed = true;
}

void writestr(char *str) {
    char c = ' ';
    while (c != 0) {
        c = *str;
        putchar(c);
        str++;
    }
}

static void list_range(LoopData *data, size_t from, size_t to) {
    if (from > to) {
        return;
    }
    if (from == to) {
        printf("%zu| %s\n", from, data->txt[from]);
        if (from == data->txt_lines - 1) {
            printf("EOT\n");
        }
        return;
    }
    bool left_align = strcmp(data->settings[0], "left") == 0;

    char buffer[sizeof(int) * 8 + 1];

    sprintf(buffer, "%zu", to);
    size_t len = strlen(buffer);
    for (int i = from; i <= to; ++i) {
        sprintf(buffer, "%i", i);
        size_t l = strlen(buffer);
        if (!left_align) {
            for (int j = 0; j < len - l; ++j) { putchar(' '); }
        }
        writestr(buffer);
        if (left_align) {
            for (int j = 0; j < len - l; ++j) { putchar(' '); }
        }
        putchar('|');
        putchar(' ');
        writestr(data->txt[i]);
        putchar('\n');
    }
    if (to == data->txt_lines - 1) {
        printf("EOT\n");
    }
}

void do_list(LoopData* data) { // l
    if (data->inpl <= 2) {
        if (data->txt_lines == 0) {
            printf("EOT\n");
            return;
        }
        list_range(data, 0, data->txt_lines - 1);
        return;
    }
    RangeArgument arg = parse_args_range(data->inpl, data->inp + 2);
    if (arg.is_range) {
        if (data->txt_lines == 0) {
            printf("Out of range!\n");
            return;
        }

        int from = arg.from;
        int to = arg.to;

        if (to == -1 && data->txt_lines > 0) {
            to = data->txt_lines - 1;
        }

        if (to > data->txt_lines - 1) {
            printf("Out of range!\n");
            return;
        }

        if (from < 0) {
            printf("Out of range!\n");
            return;
        }

        if (from > to) {
            printf("Invalid range!\n");
            return;
        }

        list_range(data, from, to);
    }
    else {
        int line = arg.line;
        if (line < data->txt_lines) {
            printf("%i| %s\n", line, data->txt[line]);
        }
        if (line == data->txt_lines - 1) {
            printf("EOT\n");
        }
    }
}

void do_get_indents(LoopData* data) { // m
    // get indent of line
    if(data->inpl <= 2) return;
    char *t = malloc(data->inpl);

    for (int i = 2; i < data->inpl; ++i)
        t[i - 2] = data->inp[i];

    int l = atoi(t);
    free(t);

    printf("indent of line %i: %i\n", l, get_indent(data->txt[l]));
}

static void copy_fromto(LoopData *data, int from, int whereInsert) {
    for (int i = data->txt_lines; i > whereInsert; --i) {
        memcpy(data->txt[i], data->txt[i - 1], strlen(data->txt[i - 1]) + 1);
    }

    memcpy(data->txt[whereInsert], data->txt[from], strlen(data->txt[from]) + 1);
    data->txt_size += strlen(data->txt[from]) + 1;

    data->txt_lines++;

    auto_increase_alloc(data);
}

void do_copy(LoopData* data) { // c
    if (data->inpl <= 4) return;
    // copies line l and inserts it after line a
    char *a = malloc(data->inpl);
    char *b = malloc(data->inpl);

    int stage = 0;
    int c = 0;
    for (int i = 2; i < data->inpl; ++i) {
        if ((data->inp[i] == ' ' && stage == 0) || data->inp[i] == '\n') {
            stage++;
            c = 0;
            continue;
        }
        if (stage == 0)
            a[c] = data->inp[i];
        else if (stage == 1)
            b[c] = data->inp[i];
        c++;
    }

    int av = atoi(a);     // what line to copy
    int bv = atoi(b) + 1;   // after what to insert to

    free(a);
    free(b);

    copy_fromto(data, av, bv);

    data->changed = true;
}

void do_move(LoopData* data) { // p [from] [where to insert]
    if (data->inpl <= 4) return;
    // moves line l and inserts it after line a
    char *a = malloc(data->inpl);
    char *b = malloc(data->inpl);

    int stage = 0;
    int c = 0;
    for (int i = 2; i < data->inpl; ++i) {
        if ((data->inp[i] == ' ' && stage == 0) || data->inp[i] == '\n') {
            stage++;
            c = 0;
            continue;
        }
        if (stage == 0)
            a[c] = data->inp[i];
        else if (stage == 1)
            b[c] = data->inp[i];
        c++;
    }

    int av = atoi(a);     // what line to copy
    int bv = atoi(b) + 1;   // after what to insert to

    free(a);
    free(b);

    copy_fromto(data, av, bv);
    delete_line(data, av);

    data->changed = true;
}

void do_find(LoopData* data) { // f
    if (data->inpl <= 2) return;

    char *t = data->inp + 2;

    int occ_c_old = data->occ_c;
    for (int l = 0; l < data->txt_lines; ++l) {
        int o = 0;
        int matching_step = 0;
        while (o < strlen(data->txt[l])) {
            char c = data->txt[l][o];
            if (c == t[matching_step]) {
                matching_step++;
            } else {
                matching_step = 0;
            }
            if (matching_step == strlen(t)) {
                data->occ_c ++;
                POS *occ = malloc(sizeof(POS));
                occ->line = l;
                occ->offset = o - matching_step + 1;
                occ->amount = strlen(t);
                data->occ = realloc(data->occ, sizeof(POS *) * data->occ_c);
                data->occ[data->occ_c-1] = occ;
                break;
            }
            o++;
        }
    }

    printf("%i occurrences of \"%s\" found!\n", data->occ_c - occ_c_old, t);
}

void do_list_findbuffer(LoopData* data) { // x
    for (int i = 0; i < data->occ_c; ++i) {
        POS *p = data->occ[i];
        if (p == NULL) {
            continue;
        }
        printf("line: %i at: %i amount: %zu\n", p->line, p->offset, p->amount);
    }
}

void do_clear_findbuffer(LoopData* data) { // y 
    data->occ_c = 0;
}

typedef enum {
    IN_RANGE,
    NOT_IN_RANGE
} RangeOperation;

void remove_range_from_find_buffer(LoopData* data, RangeOperation op) {
    if (data->inpl <= 2) return;

    RangeArgument arg = parse_args_range(data->inpl, data->inp + 2);
    if (arg.is_range) {
        int from = arg.from;
        int to = arg.to;
        if (to == -1 && data->txt_lines > 0) {
            to = data->txt_lines - 1;
        }

        for (int i = 0; i < data->occ_c; ++i) {
            POS *p = data->occ[i];
            if (p->line >= from && p->line <= to) {
                if (op == IN_RANGE) {
                    data->occ[i] = NULL;
                }
            }
            else {
                if (op == NOT_IN_RANGE) {
                    data->occ[i] = NULL;
                }
            }
        }
    }
    else {
        int line = arg.line;
        if (line < data->txt_lines) {
            for (int i = 0; i < data->occ_c; ++i) {
                POS *p = data->occ[i];
                if (p->line == line) {
                    if (op == IN_RANGE) {
                        data->occ[i] = NULL;
                    }
                }
                else {
                    if (op == NOT_IN_RANGE) {
                        data->occ[i] = NULL;
                    }
                }
            }
        }
        else {
            printf("Line %i out of bounds!\n", line);
        }
    }

    int new_occ_c = 0;
    for (int i = 0; i < data->occ_c; ++i) {
        if (data->occ[i] != NULL) {
            data->occ[new_occ_c] = data->occ[i];
            new_occ_c++;
        }
    }
    data->occ_c = new_occ_c;
}

void do_replace_find_buffer(LoopData* data) { // z
    // finds all occurences of (data->inp + 2) in the find buffer and makes that the new find buffer
    for (int i = 0; i < data->occ_c; ++i) {
        POS *p = data->occ[i];
        char *line = data->txt[p->line];
        char *w = strstr(line + p->offset, data->inp + 2);
        if (w == NULL) {
            data->occ[i] = NULL;
            continue;
        }
        int offset = (int)(w - line);
        if (offset > p->amount) {
            data->occ[i] = NULL;
            continue;
        }
        p->offset = offset;
        p->amount = strlen(data->inp + 2);
    }
}

void do_remove_find_buffer(LoopData* data) { // w
    for (int i = 0; i < data->occ_c; ++i) {
        POS *p = data->occ[i];
        char *line = data->txt[p->line];
        char *w = strstr(line + p->offset, data->inp + 2);
        if (w == NULL) {
            continue;
        }
        int offset = (int)(w - line);
        if (offset > p->amount) {
            continue;
        }
        data->occ[i] = NULL;
    }
}

static void validate_and_set(LoopData *data, int setting, char *value) {
    if (setting == 0) {
        if (!(strcmp(value, "right") == 0 || strcmp(value, "left") == 0)) {
            printf("Invalid value!\n");
            return;
        }
    }
    else {
        printf("Invalid setting!\n");
    }

    size_t s = strlen(value) + 1;
    data->settings[setting] = realloc(data->settings[setting], s);
    memcpy(data->settings[setting], value, s);
}

static void do_set(LoopData *data) {
    if (data->inpl <= 2) {
        printf("Usage: ! [setting] [value]\n");
        return;
    }

    char *setting_s = malloc(data->inpl);
    char *value_s = malloc(data->inpl);

    int stage = 0;
    int c = 0;
    for (int i = 2; i < data->inpl; ++i) {
        if ((data->inp[i] == ' ' && stage == 0) || data->inp[i] == '\n') {
            stage++;
            c = 0;
            continue;
        }
        if (stage == 0)
            setting_s[c] = data->inp[i];
        else if (stage == 1)
            value_s[c] = data->inp[i];
        c++;
    }

    int setting = atoi(setting_s);
    validate_and_set(data, setting, value_s);

    free(setting_s);
    free(value_s);
}

static void do_print_settings(LoopData *data) {
    for (size_t i = 0; i < SETTINGS_COUNT; i++) {
        printf("%zu=%s\n", i, data->settings[i]);
    }
}

static void do_print_macros(LoopData *data) {
    for (size_t i = 0; i < data->macroc; i++) {
        writestr(data->macros[i].name.str);
        printf("(%zu)", data->macros[i].args);
        writestr(" = ");
        for (size_t i = 0; i < data->macros[i].performc; i++) {
            writestr(data->macros[i].performs[i].str);
            writestr(" ; ");
        }
        putchar('\n');
    }
}

static void do_shell_command(LoopData *data) {
    char *cmd = data->inp + 2;
    (void) system(cmd);
}

static void do_shell_command_append(LoopData *data) {
    size_t old_next_line = data->txt_lines;

    char *cmd = data->inp + 2;
    FILE *p = popen(cmd, "r");
    if (p == NULL) {
        printf("Error executing command!\n");
        return;
    }

    size_t line = data->txt_lines;
    data->txt_size += 1;
    data->txt[line][0] = 0;
    data->txt_lines ++;
    auto_increase_alloc(data);

    size_t writer = 0;

    char c;
    while ((c = fgetc(p)) != EOF) {
        if (c == '\n') {
            data->txt_size += writer + 1;
            data->txt[line][writer] = 0;
            data->txt_size += 1;
            line = data->txt_lines ++;
            auto_increase_alloc(data);
            data->txt[line][0] = 0;
            writer = 0;
            continue;
        }
        data->txt[line][writer++] = c;
    }

    data->txt[line][writer] = 0;
    data->txt_size += writer + 1;

    data->changed = true;

    pclose(p);

    printf("Added lines from %zu to %zu!\n", old_next_line, line);
}

static void do_trim_trailing(LoopData *data) {
    // reverse iterate over all lines
    for (size_t i = data->txt_lines - 1; i > 0; i--) {
        char *line = data->txt[i];
        if (line[0] != '\0') {
            break;
        }
        data->txt_size --;
        data->txt_lines --;
    }
}

void resolve_input(LoopData *data) {
    for (int i = data->inpl - 1; i >= 0; --i) {
        if (data->inp[i] == ' ' || data->inp[i] == '\n' || data->inp[i] == '\r') {
            data->inp[i] = 0;
            data->inpl--;
        }
        else {
            break;
        }
    }

    if (data->inpl == 0) {
        return;
    }

    if (data->inpl == 1 || !(data->inp[1] == ' ' || data->inp[1] == 0)) {
        printf("Command not found!\n");
        return;
    }

    switch(data->inp[0]) {
        case 'q':
            data->running = false;
            break;
        case 'h':
            show_help_message();
            break;
        case 'r':
            do_replace(data);
            break;
        case 'x':
            do_list_findbuffer(data);
            break;
        case 'y':
           do_clear_findbuffer(data);
            break;
        case 'f':
            do_find(data);
            break;
        case 's':
            do_save(data);
            break;
        case 'c':
            do_copy(data);
            break;
        case 'm':
            do_get_indents(data);
            break;
        case 'e':
            do_normal_edit(data);
            break;
        case 'w':
            do_indent_edit(data);
            break;
        case 'i':
            do_insert(data);
            break;
        case 'd':
            do_delete(data);
            break;
        case 'a':
            do_append(data);
            break;
        case 'l':
            do_list(data);
            break;
        case 'v':
            do_file_info(data);
            break;
        case 'o':
            do_macro_def(data);
            break;
        case 't':
            do_exec_macro(data);
            break;
        case 'p':
            do_move(data);
            break;
        case '*':
            remove_range_from_find_buffer(data, NOT_IN_RANGE);
            break;
        case '&':
            remove_range_from_find_buffer(data, IN_RANGE);
            break;
        case '$':
            do_replace_find_buffer(data);
            break;
        case '%':
            do_remove_find_buffer(data);
            break;
        case '!':
            do_set(data);
            break;
        case '(':
            do_print_settings(data);
            break;
        case ')':
            do_print_macros(data);
            break;
        case '\'':
            do_shell_command(data);
            break;
        case '/':
            do_shell_command_append(data);
            break;
        case 'k':
            do_trim_trailing(data);
            break;
        default:
            printf("Command not found!\n");
            break;
    }
}

void show_help_message(void) {
    printf("Commands:\n");
    printf("q                       quit\n");
    printf("h                       display this\n");
    printf("s                       save to the file\n");
    printf("v                       display infos about the file and others\n");
    printf("e [l] [txt...]          changes the text of the line [l] to [txt]\n");
    printf("w [l] [txt...]          changes the text of the line [l] to original indent + [txt]\n");
    printf("i [l]                   insert empty line after line [l]\n");
    printf("i [l] [txt...]          insert line with text [t] after line [l]\n");
    printf("d [l: range]            delete all line(s) in the range [l]\n");
    printf("a                       append empty line\n");
    printf("a [txt...]              append line with text [txt]\n");
    printf("l                       list all lines\n");
    printf("l [l: range]            lists all lines in the range [l]\n");
    printf("m [l]                   get indent (spaces) of line [l]\n");
    printf("c [l] [a]               copy line [l] and insert it after line [a]\n");
    printf("p [l] [a]               move line [l] after line [a]\n");
    printf("f [txt...]              finds occurrences of [txt]\n");
    printf("x                       list all contents in the find buffer\n");
    printf("y                       clear the find buffer\n");
    printf("* [l: range]            removes every element in the find buffer where the line IS NOT in the range [l]\n");
    printf("& [l: range]            removes every element in the find buffer where the line IS in the range [l]\n");
    printf("$ [txt...]              finds occurrences of [txt] in the find buffer and overwrites the find buffer with it\n");
    printf("%% [txt...]              removes every element in the find buffer which contains [txt]\n");
    printf("r [txt...]              replaces everything in the find buffer with [txt]\n");
    printf("o [name][args][body...] defines a macro\n");
    printf("t [macro] [args...]     executes a macro\n");
    printf("! [setting] [val...]    sets a setting\n");
    printf("(                       prints all settings\n");
    printf(")                       prints all macros\n");
    printf("' [cmd...]              execute shell command\n");
    printf("/ [cmd...]              executes the shell command and appends the output to the current file\n");
    printf("k                       trim trailing lines\n");
    putchar('\n');

    printf("Ranges:\n");
    printf("a-b                     range from a to b\n");
    printf("a-                      range from a to end\n");
    printf("-b                      range from 0 to b\n");
    printf("a                       range of a\n");
    putchar('\n');

    printf("Settings:\n");
    printf("0   [\"right\"|\"left\"]    sets the alignment of line numbers (default: right)\n");
}

static char *strclone(char *str) {
    size_t len = strlen(str) + 1;
    char *clone = malloc(len);
    memcpy(clone, str, len);
    return clone;
}

static void load_defaults(LoopData *data) {
    data->settings[0] = strclone("right");
}

static void trimTrailing(char *str) {
    int index, i;
    index = -1;

    i = 0;
    while(str[i] != '\0') {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            index = i;
        }
        i++;
    }

    str[index + 1] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid arguments! Usage: ezed [file path]\n");
        return 1;
    }
    
    LoopData data;
    data.changed = false;
    data.c_file = argv[1];

    data.txt_lines = 0;
    data.txt_size = 0;

    data.txt = malloc(txt_lines_amount * sizeof(char *));
    for (size_t i = 0; i < txt_lines_amount; ++i)
        data.txt[i] = malloc(txt_line_alloc_s);

    data.macroc = 0;
    data.macros = malloc(sizeof(macro) * 1);

    if(access(data.c_file, F_OK) == 0) {
        FILE* file;
        char *buffer = malloc(txt_line_alloc_s);
        memset(buffer, 0, txt_line_alloc_s);

        file = fopen(data.c_file, "r");

        fseek(file, 0L, SEEK_END);
        data.txt_size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        int line = 0;
        while(fgets(buffer, (int) txt_line_alloc_s, file)) {
            data.txt_lines = line + 1;
            auto_increase_alloc(&data);
            memcpy(data.txt[line], buffer, txt_line_alloc_s);
            data.txt[line][strlen(buffer)-1] = 0;
            line ++;
        }

        fclose(file);
        free(buffer);

        // remove trailing lines
        for (int i = data.txt_lines - 1; i >= 0; --i) {
            if (data.txt[i][0] != 0) {
                break;
            }
            data.txt_lines--;
        }

        printf("Loaded %d lines (%zu bytes) from file %s!\n", data.txt_lines, data.txt_size, data.c_file);
    }

    // find buffer
    data.occ_c = 0;
    data.occ = malloc(data.txt_size * sizeof(POS *));
    
    data.running = true;

    load_defaults(&data);

    // .ezedrc
    char *user_home = getenv("HOME");
    size_t user_home_len = strlen(user_home);
    char *ezedrc_path = malloc(user_home_len + 9);
    strcpy(ezedrc_path, user_home);
    memcpy(ezedrc_path + user_home_len, "/.ezedrc", 8);
    ezedrc_path[user_home_len + 8] = 0;

    FILE *ezedrc;
    if(access(ezedrc_path, F_OK) != 0) {
        printf("Creating %s...\n", ezedrc_path);
        ezedrc = fopen(ezedrc_path, "w");
        if (ezedrc == NULL) {
            printf("Error creating %s!\n", ezedrc_path);
            return 1;
        }
        fprintf(ezedrc, "# Every line in this file will be executed on the start of EzEd (after loading the file)\n");
        fprintf(ezedrc, "! 0 right      # Sets the alignment of line numbers to right align\n");
        fclose(ezedrc);
    }
    ezedrc = fopen(ezedrc_path, "r");
    if (ezedrc == NULL) {
        printf("Error opening %s!\n", ezedrc_path);
        return 1;
    }
    free(ezedrc_path);

    char *line = NULL;
    size_t line_len = 0;
    ssize_t read;
    while ((read = getline(&line, &line_len, ezedrc)) != -1) {
        line[read] = 0;

        for (size_t i = 0; i < read; ++i) {
            if (line[i] == '\n' || line[i] == '\r' || line[i] == 0 || line[i] == '#') {
                line[i] = 0;
                break;
            }
        }

        trimTrailing(line);

        if (line[0] == 0) {
            continue;
        }

        data.inp = line;

        //writestr("> ");
        //writestr(line);
        //putchar('\n');

        data.inpl = strlen(line) + 1;

        data.tokens = tokenize(data.inp, 0, 1);

        if(data.tokens.count == 0) {
            free_tokenized(&data.tokens);
            continue;
        }

        resolve_input(&data);

        free_tokenized(&data.tokens);

        if (!data.running) {
            printf("Quitting from .ezedrc is not allowed!\n");
            data.running = true;
        }
    }
    fclose(ezedrc);
    if (line != NULL) {
        free(line);
    }

    while(data.running) {
        data.inp = malloc(inp_alloc_s + 1);
        printf("> ");
        fgets(data.inp, (int) inp_alloc_s, stdin);
        data.inpl = strlen(data.inp);
        data.inp[data.inpl-1] = '\0';

        if(data.inp[0] == '\n') {
            free(data.inp);
            continue;
        }

        data.tokens = tokenize(data.inp, 0, 1);

        if(data.tokens.count == 0) {
            free_tokenized(&data.tokens);
            free(data.inp);
            continue;
        }

        resolve_input(&data);

        free_tokenized(&data.tokens);
        free(data.inp);

        if (!data.running && data.changed) {
            printf("Quit without saving? (y/n) ");
            char c = getchar();
            getchar();
            if (c == 'y') {
                break;
            }
            data.running = true;
        }
    }

    for (size_t i = 0; i < txt_lines_amount; ++i) {
        free(data.txt[i]);
    }

    if(data.macroc == 0) free(data.macros);
    for(size_t i = 0; i < data.macroc; ++i) {
        free_macro(&data.macros[i]);
        free(&data.macros[i]);
    }

    free(data.occ);
    free(data.txt);

    return 0;
}
