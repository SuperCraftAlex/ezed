//
// Created by alex on 10/6/23.
//

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "macros.h"

static int parse_params(string str) {
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

static string str_new(const char* s) {
    size_t len = strlen(s) + 1;
    string st;
    st.size = len;
    st.str = (char*)malloc(len);
    for(int i = 0; i < len; ++i)
        st.str[i] = s[i];
    return st;
}

static int str_insert(string* str, size_t idx, char c) {
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

static void str_kill(string* str) {
    if(str->size == 0) return;
    free(str->str);
    str->size = 0;
    str->str = (char*)0;
}

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

static int pop_front(tokenized* tokens) {
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

void free_tokenized(tokenized* tokens) {
    for(size_t i = 0; i < tokens->count; ++i) {
        str_kill(&tokens->tokens[i]);
        tokens->tokens[i].str = NULL;
    }
    free(tokens->tokens);
    tokens->tokens = NULL;
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

static void execute_macro(macro* mac, tokenized arguments, LoopData* data);

static macro* find_macro(char* name, LoopData* data);

void do_exec_macro(LoopData* data) {
    pop_front(&data->tokens);
    macro* m = find_macro(data->tokens.tokens[0].str,data);
    if(!m) return;
    pop_front(&data->tokens);
    execute_macro(m,data->tokens,data);
}

static void execute_macro(macro* mac, tokenized arguments, LoopData* data) {
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

static macro* find_macro(char* name, LoopData* data) {
    for(int i = 0; i < data->macroc; ++i) {
        if(!strcmp(name, data->macros[i].name.str)) {
            return &data->macros[i];
        }
    }
    return NULL;
}