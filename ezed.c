#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

void replace(char *str, int start, int amount, char *replacev) {
    int len = strlen(str);
    int replaceLen = strlen(replacev);

    if (start < 0 || start >= len || amount <= 0) {
        return;
    }

    int end = start + amount;
    int resultLen = len - amount + replaceLen;

    if (resultLen >= len) {
        memmove(str + start + replaceLen, str + end, len - end + 1);
        memcpy(str + start, replacev, replaceLen);
    } else {
        char result[resultLen + 1];
        strncpy(result, str, start);
        strncpy(result + start, replacev, replaceLen);
        strncpy(result + start + replaceLen, str + end, len - end);

        result[resultLen] = '\0';
        strcpy(str, result);
    }
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

typedef struct POS {
    int line;
    int offset;
    size_t amount;
} POS;

typedef struct string {
    char* str;
    size_t size;
} string;

typedef struct tokenized {
    string* tokens;
    size_t count;
} tokenized;

struct LoopData;

typedef struct macro {
    string* performs;
    size_t performc;
    size_t args;
    string name;
} macro;

typedef struct LoopData {
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
} LoopData;

tokenized tokenize(char* str, int do_braces, int do_strings) {
    tokenized tokens;
    tokens.count = 0;
    tokens.tokens = NULL;
    size_t new_token_size = 0;
    size_t last_token = 0;
    size_t in_brace = 0;
    int in_string = 0;
    for(char* c = str; *c != '\0'; ++c) {
        if((isspace(*c) || (do_braces && *c == '(')) && (!do_braces || in_brace == 0) && (!do_strings || !in_string)) {
            if(new_token_size != 0) {
                ++tokens.count;
                string* tks = tokens.tokens;
                tokens.tokens = malloc(sizeof(string) * tokens.count);
                for(int i = 0; i < tokens.count-1; ++i)
                    tokens.tokens[i] = tks[i];
                string s;
                s.size = new_token_size;
                s.str = (char*)malloc(new_token_size+1);
                for(int i = 0; i < new_token_size; ++i) 
                    s.str[i] = str[last_token + i];
                s.str[new_token_size] = '\0';
                tokens.tokens[tokens.count-1] = s;
                free(tks);
                last_token += new_token_size;
                new_token_size = 0;
            }
            if(do_braces && *c == '(') ++in_brace, ++new_token_size;
            else ++last_token;
        }
        else {
            if(do_braces && *c == '(') ++in_brace;
            if(do_braces && *c == ')') --in_brace;
            if(do_strings && *c == '"') in_string = !in_string;
            else new_token_size += 1;
        }
    }
    if(new_token_size != 0) {
        ++tokens.count;
        string* tks = tokens.tokens;
        tokens.tokens = malloc(sizeof(string) * tokens.count);
        for(int i = 0; i < tokens.count-1; ++i)
            tokens.tokens[i] = tks[i];
        string s;
        s.size = new_token_size;
        s.str = (char*)malloc(new_token_size+1);
        for(int i = 0; i < new_token_size; ++i) 
            s.str[i] = str[last_token + i];
        s.str[new_token_size] = '\0';
        tokens.tokens[tokens.count-1] = s;
        free(tks);
    }
    return tokens;
}

int pop_front(tokenized* tokens) {
    if((tokens)->count == 0) return 0;
    tokenized rtokens;
    rtokens.count = (tokens)->count - 1;
    rtokens.tokens = malloc(sizeof(string) * rtokens.count);
    for(int i = 1; i < rtokens.count+1; ++i) {
        rtokens.tokens[i-1].str = (tokens)->tokens[i].str;
        rtokens.tokens[i-1].size = (tokens)->tokens[i].size;
    }
    free(tokens->tokens);
    tokens->tokens = rtokens.tokens;
    tokens->count = rtokens.count;
    return 1;
}

string str_new(const char* s) {
    size_t len = strlen(s) + 1;
    string st;
    st.size = len;
    st.str = (char*)malloc(len);
    for(int i = 0; i < len; ++i)
        st.str[i] = s[i];
    return st;
}

int str_insert(string* str, size_t idx, char c) {
    if(idx > str->size) return 0;
    char* new_str = malloc(str->size+1);
    for(int i = 0; i < str->size; ++i) 
        new_str[i] = str->str[i];
    new_str[idx] = c;
    for(int i = idx; i < str->size; ++i) 
        new_str[i+1] = str->str[i];

    ++str->size;
    free(str->str);
    str->str = new_str;
    return 1;
}

int str_delete(string* str, int idx) {
    if(idx >= str->size) return 0;
    char* new_str = malloc(str->size-1);
    for(int i = 0; i < str->size; ++i) {
        if(i > idx) new_str[i-1] = str->str[i];
        else new_str[i] = str->str[i];
    }

    --str->size;
    new_str[str->size] = '\0';
    free(str->str);
    str->str = new_str;
    return 1;
}

void str_kill(string* str) {
    if(str->size == 0) return;
    free(str->str);
    str->size = 0;
    str->str = (char*)0;
}

int parse_params(string str) {
    if(str.size < 1) return -1;
    size_t buf_size = str.size - 2 + 1;
    if(buf_size == 1) return -1;
    char* buf = malloc(buf_size);
    buf[buf_size - 1] = '\0';
    
    for(int i = 0; i < buf_size; ++i) {
        buf[i] = str.str[i+1];
    }
    int res = atoi(buf);
    free(buf);
    return res;
}

void free_tokenized(tokenized* tokens) {
    for(size_t i = 0; i < tokens->count; ++i) {
        str_kill(&tokens->tokens[i]);
        tokens->tokens[i].str = NULL;
    }
    free(tokens->tokens);
    tokens->tokens = NULL;
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
    char *t = malloc(data->inpl);

    for(int i = 2; i < data->inpl; ++i) {
        t[i - 2] = data->inp[i];
    }

    for(int i = 0; i < data->occ_c; ++i) {
        POS *pos = data->occ[i];
        if (pos == NULL) {
            continue;
        }

        replace(data->txt[pos->line], pos->offset, (int)pos->amount, t);

        data->changed = true;
    }

    free(t);

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

void do_list(LoopData* data) { // l
    if (data->inpl <= 2) {
        for (int i = 0; i < data->txt_lines; ++i) {
            printf("%i: %s\n", i, data->txt[i]);
        }
        printf("EOT\n");
        return;
    }
    RangeArgument arg = parse_args_range(data->inpl, data->inp + 2);
    if (arg.is_range) {
        int from = arg.from;
        int to = arg.to;
        if (to == -1 && data->txt_lines > 0) {
            to = data->txt_lines - 1;
        }

        for (int i = from; i <= to; ++i) {
            if (i < data->txt_lines) {
                printf("%d: %s\n", i, data->txt[i]);
            }
            if(i == data->txt_lines-1) {
                printf("EOT\n");
                break;
            }
        }
    }
    else {
        int line = arg.line;
        if (line < data->txt_lines) {
            printf("%d: %s\n", line, data->txt[line]);
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

// TODO: rewrite
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
    //TODO
    printf("Not implemented yet!\n");
    //for (int i = 0; i < data->occ_c; ++i) {
    //    POS *p = data->occ[i];
    //}
}

void do_remove_find_buffer(LoopData* data) { // w
    for (int i = 0; i < data->occ_c; ++i) {
        POS *p = data->occ[i];
        char *w = strstr(data->txt[p->line] + p->offset, data->inp + 2);
        if (w == NULL) {
            continue;
        }
        // remove from find buffer
        data->occ[i] = NULL;
    }
}

void do_macro_def(LoopData* data) { // o
    free_tokenized(&data->tokens);
    data->tokens = tokenize(data->inp,1,1);
    pop_front(&data->tokens);
    macro mac;
    if(data->tokens.count < 2) return;
    string name = str_new(data->tokens.tokens[0].str);
    string args = data->tokens.tokens[1];
    pop_front(&data->tokens);
    pop_front(&data->tokens);
    int params = parse_params(args);

    mac.args = params;
    mac.name = name;
    mac.performs = malloc(sizeof(string) * data->tokens.count);
    for(int i = 0; i < data->tokens.count; ++i) {
        mac.performs[i] = str_new(data->tokens.tokens[i].str);
    }
    mac.performc = data->tokens.count;

    data->macros = realloc(data->macros, sizeof(macro) * (data->macroc + 1));

    data->macros[data->macroc] = mac;
    data->macroc++;
}

void execute_macro(macro* mac, tokenized arguments, LoopData* data);
macro* find_macro(char* name, LoopData* data);
void do_exec_macro(LoopData* data) {
    pop_front(&data->tokens);
    macro* m = find_macro(data->tokens.tokens[0].str,data);
    if(!m) return;
    pop_front(&data->tokens);
    execute_macro(m,data->tokens,data);
}

void resolve_input(LoopData* data);

void execute_macro(macro* mac, tokenized arguments, LoopData* data) {
    if(mac->args != arguments.count) return; // error
    string exec = str_new("");
    for(int i = 0; i < mac->performc; ++i) {
        if(!strcmp(mac->performs[i].str,";")) {
            if(exec.size == 1) continue;
            char* last_inp = data->inp; 
            size_t last_inpl = data->inpl;
            tokenized last_tokens = data->tokens;
            data->inp = exec.str;
            data->inpl = exec.size;
            data->tokens = tokenize(exec.str,0,1);
            resolve_input(data);
            data->inp = last_inp;
            data->inpl = last_inpl;
            free_tokenized(&data->tokens);
            data->tokens = last_tokens;
            str_kill(&exec);
            exec = str_new("");
        }
        else if(mac->performs[i].str[0] == '#') {
            char* arg = mac->performs[i].str + 1;
            int num_arg = atoi(arg) - 1;
            if(arguments.count <= num_arg) return; // error !
            for(int j = 0; j < arguments.tokens[num_arg].size; ++j) {
                str_insert(&exec,exec.size-1,arguments.tokens[num_arg].str[j]);
            }
            if(i+1 < mac->performc)
                str_insert(&exec,exec.size-1,' ');
        }
        else {
            for(int j = 0; j < mac->performs[i].size-1; ++j) {
                str_insert(&exec,exec.size-1,mac->performs[i].str[j]);
            }
            if(i+1 < mac->performc)
                str_insert(&exec,exec.size-1,' ');
        }
    }
    if(exec.size == 1) {
        str_kill(&exec);
        return;
    }
    char* last_inp = data->inp; 
    size_t last_inpl = data->inpl;
    data->inp = exec.str;
    data->inpl = exec.size;
    resolve_input(data);
    data->inp = last_inp;
    data->inpl = last_inpl;
    str_kill(&exec);
}

void free_macro(macro* mac) {
    for(int i = 0; i < mac->performc; ++i) 
        str_kill(&mac->performs[i]);
    
    free(mac->performs);
}

macro* find_macro(char* name, LoopData* data) {
    for(int i = 0; i < data->macroc; ++i) {
        if(!strcmp(name, data->macros[i].name.str)) {
            return &data->macros[i];
        }
    }
    return NULL;
}

void resolve_input(LoopData* data) {
    for (int i = data->inpl - 1; i >= 0; --i) {
        if (data->inp[i] == ' ' || data->inp[i] == '\n') {
            data->inp[i] = 0;
            data->inpl--;
        }
        else
            break;
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
        default:
            printf("Command not found!\n");
            break;
    }
}

void show_help_message(void) {
    printf("Commands:\n");
    printf("q                      quit\n");
    printf("h                      display this\n");
    printf("s                      save to the file\n");
    printf("v                      display infos about the file and others\n");
    printf("e [l] [txt]            changes the text of the line [l] to [txt]\n");
    printf("w [l] [txt]            changes the text of the line [l] to original indent + [txt]\n");
    printf("i [l]                  insert empty line after line [l]\n");
    printf("i [l] [txt]            insert line with text [t] after line [l]\n");
    printf("d [l: range]           delete all line(s) in the range [l]\n");
    printf("a                      append empty line\n");
    printf("a [txt]                append line with text [txt]\n");
    printf("l                      list all lines\n");
    printf("l [l: range]           lists all lines in the range [l]\n");
    printf("m [l]                  get indent (spaces) of line [l]\n");
    printf("c [l] [a]              copy line [l] and insert it after line [a]\n");
    printf("p [l] [a]              move line [l] after line [a]\n");
    printf("f [txt]                finds occurrences of [txt]\n");
    printf("x                      list all contents in the find buffer\n");
    printf("y                      clear the find buffer\n");
    printf("* [l: range]           removes every element in the find buffer where the line IS NOT in the range [l]\n");
    printf("& [l: range]           removes every element in the find buffer where the line IS in the range [l]\n");
    printf("$ [txt]                finds occurrences of [txt] in the find buffer and overwrites the find buffer with it\n");
    printf("%% [txt]                removes every element in the find buffer which contains [txt]\n");
    printf("r [txt]                replaces everything in the find buffer with [txt]\n");
    printf("o [name][args][body..] defines a macro\n");
    printf("t [macro] [args..]     executes a macro\n");
    putchar('\n');

    printf("Ranges:\n");
    printf("a-b                    range from a to b\n");
    printf("a-                     range from a to end\n");
    printf("-b                     range from 0 to b\n");
    printf("a                      range of a\n");
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

    // find buffer
    data.occ_c = 0;
    data.occ = malloc(data.txt_size);
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
    
    data.running = true;
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

        data.tokens = tokenize(data.inp,0,1);

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
