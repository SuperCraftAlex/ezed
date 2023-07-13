#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))


// copied from chat gpt cuzzzzz yeah
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
size_t txt_lines_amount = 2000;
size_t txt_line_alloc_s = sizeof(char) * 100;

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

void update_txt_alloc(char **txt) {
    txt = realloc(txt, txt_lines_amount * sizeof(char *));
    for (int i = 0; i < txt_lines_amount; ++i)
        txt[i] = realloc(txt[i], txt_line_alloc_s);
}

typedef struct {
    int line;
    int offset;
    size_t amount;
} POS;

// TODO: reallocate txt alloc automatically

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid arguments! Usage: ezed [file path]");
        return 64;
    }
    char *c_file = argv[1];

    clear();
    gotoxy(0, 0);

    int txt_lines = 0;
    size_t txt_size = 0;

    char **txt = malloc(txt_lines_amount * sizeof(char *));
    for (int i = 0; i < txt_lines_amount; ++i)
        txt[i] = malloc(txt_line_alloc_s);

    // find buffer
    int occ_c = 0;
    POS **occ = malloc(txt_size);

    if(access(c_file, F_OK) == 0) {
        FILE* file;
        char *buffer = malloc(txt_line_alloc_s);
        memset(buffer, 0, txt_line_alloc_s);

        file = fopen(c_file, "r");

        fseek(file, 0L, SEEK_END);
        txt_size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        int line = 0;
        while(fgets(buffer, (int) txt_line_alloc_s, file)) {
            memcpy(txt[line], buffer, txt_line_alloc_s);
            txt[line][strlen(buffer)-1] = 0;
            line ++;
        }
        txt_lines = line;

        fclose(file);
        free(buffer);

        // remove trailing lines
        for (int i = txt_lines - 1; i >= 0; --i) {
            if (txt[i][0] == 0)
                txt_lines--;
            else
                break;
        }

        printf("Loaded %i lines (%zu bytes) from file %s!\n", txt_lines, txt_size, c_file);
    }

    bool running = true;
    while (running) {
        char *inp = malloc(inp_alloc_s + 1);
        printf("> ");
        fgets(inp, (int) inp_alloc_s, stdin);
        size_t inpl = strlen(inp);

        inp[inpl-1] = 0;

        if (inp[0] == '\n') {
            free(inp);
            continue;
        }

        // TODO: move line
        if (inp[0] == 'q') {
            // quit
            running = false;
        }
        else if (inp[0] == 'h') {
            // help
            printf("Commands:\n");
            printf("q               quit\n");
            printf("h               display this\n");
            printf("s               save to the file\n");
            printf("v               display infos about the file and others\n");
            printf("e [l] [txt]     changes the text of the line [l] to [txt]\n");
            printf("i [l]           insert empty line after line [l]\n");
            printf("i [l] [txt]     insert line with text [t] after line [l]\n");
            printf("d [l]           delete line [l]\n");
            printf("a               append empty line\n");
            printf("a [txt]         append line with text [txt]\n");
            printf("l               list all lines\n");
            printf("l [l]           lists only line [l]\n");
            printf("l [a]-[b]       lists all lines from [a] to [b]\n");
            printf("l -[b]          lists all lines from the start to [b]\n");
            printf("l [a]-          lists all lines from [a] to the end\n");
            printf("m [l]           get indent (spaces) of line [l]\n");
            printf("c [l] [a]       copy line [l] and insert it after line [a]\n");
            printf("p [l] [a]       move line [l] after line [a]\n");
            printf("f [txt]         finds occurrences of [txt]\n");
            printf("x               list all contents in the find buffer\n");
            printf("y               clear the find buffer\n");
            printf("r [txt]         replaces everything in the find buffer with [txt]\n");
            printf("\n");
        }
        else if (inp[0] == 'r') {
            // replace
            char *t = malloc(inpl);

            for (int i = 2; i < inpl; ++i) {
                t[i - 2] = inp[i];
            }

            for (int i = 0; i < occ_c; ++i) {
                POS *pos = occ[i];

                replace(txt[pos->line], pos->offset, (int) pos->amount, t);
            }

            free(t);

            occ_c = 0;
        }
        else if (inp[0] == 'x') {
            for (int i = 0; i < occ_c; ++i) {
                POS *p = occ[i];
                printf("line: %i at: %i amount: %zu\n", p->line, p->offset, p->amount);
            }
            putchar('\n');
        }
        else if (inp[0] == 'y') {
           occ_c = 0;
        }
        else if (inp[0] == 'f') {
            // find
            char *t = malloc(inpl);

            for (int i = 2; i < inpl; ++i) {
                t[i - 2] = inp[i];
            }

            int occ_c_old = occ_c;
            for (int l = 0; l < txt_lines; ++l) {
                int o = 0;
                int matching_step = 0;
                while (o < strlen(txt[l])) {
                    char c = txt[l][o];
                    if (c == t[matching_step]) {
                        matching_step++;
                    } else {
                        matching_step = 0;
                    }
                    if (matching_step == strlen(t)) {
                        occ_c ++;
                        occ[occ_c-1] = malloc(sizeof(POS));
                        occ[occ_c-1]->line = l;
                        occ[occ_c-1]->offset = o-matching_step+1;
                        occ[occ_c-1]->amount = strlen(t);
                        break;
                    }
                    o++;
                }
            }

            printf("%i occurrences of \"%s\" found!\n", occ_c - occ_c_old, t);

            free(t);
        }
        else if (inp[0] == 's') {
            // save to file
            FILE *file = fopen(c_file, "w");
            if (file == NULL)
            {
                printf("Error opening the file %s\n", c_file);
            }

            for (int i = 0; i < txt_lines; ++i) {
                fprintf(file, "%s\n", txt[i]);
            }
            fclose(file);
        }
        else if (inp[0] == 'c' && inpl > 4) {
            // copies line l and inserts it after line a

            char *a = malloc(inpl);
            char *b = malloc(inpl);

            int stage = 0;
            int c = 0;
            for (int i = 2; i < inpl; ++i) {
                if (inp[i] == ' ' && stage == 0 || inp[i] == '\n') {
                    stage++;
                    c = 0;
                    continue;
                }
                if (stage == 0)
                    a[c] = inp[i];
                else if (stage == 1)
                    b[c] = inp[i];
                c++;
            }

            int av = atoi(a);     // what line to copy
            int bv = atoi(b)+1;   // after what to insert to

            free(a);
            free(b);

            for (int i = txt_lines; i > bv; --i) {
                memcpy(txt[i], txt[i - 1], strlen(txt[i - 1]) + 1);
            }

            memcpy(txt[bv], txt[av], strlen(txt[av]) + 1);
            txt_size += strlen(txt[av]) + 1;

            txt_lines++;
        }
        else if (inp[0] == 'm' && inpl > 2) {
            // get indent of line
            char *t = malloc(inpl);

            for (int i = 2; i < inpl; ++i)
                t[i - 2] = inp[i];

            int l = atoi(t);
            free(t);

            printf("indent of line %i: %i\n", l, get_indent(txt[l]));
        }
        else if (inp[0] == 'e' && inpl > 4) {
            // edit line
            char *line_s = malloc(inpl);
            char *ltxt = malloc(inpl);

            int stage = 0;
            int c = 0;
            for (int i = 2; i < inpl; ++i) {
                if (inp[i] == ' ' && stage == 0 || inp[i] == '\n') {
                    stage++;
                    c = 0;
                    continue;
                }
                if (stage == 0)
                    line_s[c] = inp[i];
                else if (stage == 1)
                    ltxt[c] = inp[i];
                c++;
            }

            int line = atoi(line_s);
            size_t ltxt_l = strlen(ltxt);

            if (line < txt_lines)
                txt_size -= strlen(txt[line]) + 1;
            memcpy(txt[line], ltxt, ltxt_l + 1);
            txt_size += ltxt_l + 1;

            free(line_s);
            free(ltxt);
        }
        else if (inp[0] == 'i' && inpl > 2) {
            // insert line after...
            char *line_s = malloc(inpl);
            char *ltxt = malloc(inpl);

            int ltxt_lb = 0;

            int stage = 0;
            int c = 0;
            for (int i = 2; i < inpl; ++i) {
                if (inp[i] == ' ' && stage == 0 || inp[i] == '\n' || inp[i] == 10) {
                    stage++;
                    c = 0;
                    continue;
                }
                if (stage == 0)
                    line_s[c] = inp[i];
                else if (stage == 1) {
                    ltxt_lb ++;
                    ltxt[c] = inp[i];
                }
                c++;
            }

            int line = atoi(line_s) + 1;
            size_t ltxt_l = strlen(ltxt);

            for (int i = txt_lines; i > line; --i) {
                memcpy(txt[i], txt[i - 1], strlen(txt[i - 1]) + 1);
            }

            if (ltxt_l > 1 && ltxt_lb > 0) {
                memcpy(txt[line], ltxt, ltxt_l + 1);
                txt_size += ltxt_l + 1;
            } else {
                memset(txt[line], 0, strlen(txt[line]));
                txt_size++;
            }
            txt_lines++;

            free(line_s);
            free(ltxt);
        }
        else if (inp[0] == 'd' && inpl > 2) {
            // TODO: d [from]-[to]

            // delete line
            char *t = malloc(inpl);

            for (int i = 2; i < inpl; ++i)
                t[i - 2] = inp[i];

            int l = atoi(t);
            free(t);
            txt_size -= strlen(txt[l]) + 1;

            for (int i = l; i < txt_lines-1; ++i) {
                memcpy(txt[i], txt[i + 1], strlen(txt[i + 1]) + 1);
            }

            txt_lines--;
        }
        else if (inp[0] == 'a') {
            // append line
            if (inpl > 2) {
                memcpy(txt[txt_lines], inp+2, inpl-1);
                txt[txt_lines][inpl-3] = 0;
                txt[txt_lines][inpl-2] = 0;
                txt_lines ++;
                txt_size += strlen(inp + 2) + 1;
            }
            else {
                txt[txt_lines] = "";
                txt_lines ++;
                txt_size += 1;
            }
        }
        else if (inp[0] == 'l') {
            // list
            if (inpl > 2) {
                char *t = malloc(inpl);
                for (int i = 2; i < inpl; ++i) {
                    if (inp[i] == ' ')
                        break;
                    t[i-2] = inp[i];
                }
                bool is_range = false;
                for (int i = 0; i < strlen(t); ++i) {
                    if (t[i] == '-') {
                        is_range = true;
                        break;
                    }
                }

                if (is_range) {
                    char *a = malloc(strlen(t));
                    char *b = malloc(strlen(t));

                    int c = 0;
                    bool first = true;
                    for (int i = 0; i < strlen(t); ++i) {
                        if (t[i] == '-') {
                            c = 0;
                            first = false;
                            continue;
                        }
                        else if (first) {
                            a[c] = t[i];
                        }
                        else {
                            b[c] = t[i];
                        }
                        c++;
                    }

                    int from = atoi(a);
                    int to;
                    if (b[0] == ' ' || b[0] == 0 || b[0] == '\n') {
                        if (txt_lines == 0)
                            to = 0;
                        else
                            to = txt_lines-1;
                    } else {
                        to = atoi(b);
                    }


                    if (from > to) {
                        int t_a = to;
                        to = from;
                        from = t_a;
                    }

                    for (int i = from; i <= to; ++i) {
                        if (i < txt_lines)
                            printf("%i: %s\n", i, txt[i]);
                        if(i == txt_lines-1)
                            printf("EOT\n");
                    }

                    free(a);
                    free(b);
                } else {
                    int line = atoi(t);
                    if (line < txt_lines) {
                        printf("%i: %s\n", line, txt[line]);
                    }
                    if(line == txt_lines-1)
                        printf("EOT\n");
                }

                free(t);
            }
            else {
                for (int i = 0; i < txt_lines; ++i) {
                    printf("%i: %s\n", i, txt[i]);
                }
                printf("EOT\n");
            }
            printf("\n");
        }
        else if (inp[0] == 'v') {
            printf("Find buffer length: %i\n", occ_c);
            putchar('\n');
            printf("File: %s\n", c_file);
            printf("Total lines: %i\n", txt_lines);
            printf("Total size: ~%zu bytes\n", txt_size);
            putchar('\n');
        }

        free(inp);
    }

    for (int i = 0; i < txt_lines_amount; ++i) {
        free(txt[i]);
    }

    free(occ);
    free(txt);

    return 0;
}
