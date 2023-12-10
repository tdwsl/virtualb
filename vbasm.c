#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSZ 128
#define MEMORYSZ 65536
#define MAXLABELS 2500
#define NAMEBUFSZ 65536

const char *rrIns[] = {
    "mov", "ldw", "ldh", "ldb", "luh", "lub", "stw", "sth",
    "stb", "add", "sub", "adw",
    "and", "lor", "xor", "shr", "shl", "mul", "div", "mod",
    "equ", "neq", "lte", "gtn", "ltn", "gte", "phm", 0,
};

const char *rIns[] = {
    "inc", "dec", "inv", "not", "neg", "psh", "pop", "lwi",
    "lhi", "lbi", "ldr", "str", "beq", "bne", 0,
};

FILE *fp;
char nextC = 0;
char next[BUFSZ];
char ahead[BUFSZ];
const char *filename;
int lineNo = 1;
unsigned char memory[MEMORYSZ];
int nmemory = 0;
char *labels[MAXLABELS];
int labelv[MAXLABELS];
int nlabels = 0;
char nameBuf[NAMEBUFSZ];
char *nameBufP = nameBuf;
int org = 0;

int strindex(const char **strs, char *s) {
    int i;
    for(i = 0; strs[i]; i++) if(!strcmp(strs[i], s)) return i;
    return -1;
}

int strnindex(char **strs, int n, char *s) {
    int i;
    for(i = 0; i < n; i++) if(!strcmp(strs[i], s)) return i;
    return -1;
}

char *addName(char *s) {
    int i;
    strcpy(nameBufP, s);
    s = nameBufP;
    nameBufP += strlen(nameBufP)+1;
    return s;
}

void parseBuf0(char *buf) {
    int i;
    i = 0;
    buf[0] = nextC;
    while(!feof(fp)) {
        if(nextC) nextC = 0;
        else buf[i] = fgetc(fp);
        if(buf[i] == 10) lineNo++;
        if(buf[i] <= 32) { if(i) break; }
        else if(strchr(":;,+-*/%&|^()\"'", buf[i])) {
            if(i) nextC = buf[i]; else i++;
            break;
        } else i++;
    }
    buf[i] = 0;
}

void parseBuf(char *buf) {
    int ol;
    parseBuf0(buf);
    if(!strcmp(buf, ";")) {
        ol = lineNo;
        while(ol == lineNo) parseBuf0(buf);
    }
}

void parseNext() {
    if(*ahead) { strcpy(next, ahead); *ahead = 0; }
    else parseBuf(next);
}

void lookAhead() {
    if(!*ahead) parseBuf(ahead);
}

void perr() {
    printf("%s:%d: ", filename, lineNo);
}

void expect(const char *s) {
    parseNext();
    if(strcmp(next, s)) { perr(); printf("expected %s\n", s); exit(0); }
}

char parseChar() {
    char c;
    c = fgetc(fp);
    if(c == '\n') { perr(); printf("expected character\n"); exit(0); }
    if((c = fgetc(fp)) == '\\')
        switch(fgetc(fp)) {
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 'r': c = '\r'; break;
        case '0': c = '\0'; break;
        case '\'': c = '\''; break;
        case '\\': c = '\\'; break;
        default: perr(); printf("unknown escape character\n"); exit(0);
        }
    expect("'");
    return c;
}

void parseString(char *buf) {
    for(;;) {
        *buf = fgetc(fp);
        switch(*buf) {
        case '\n':
            perr(); printf("unterminated string\n"); exit(0);
        case '\\':
            switch(*buf = fgetc(fp)) {
            case '"': case '\\': break;
            case 'n': *buf = '\n'; break;
            case 't': *buf = '\t'; break;
            case 'r': *buf = '\r'; break;
            case '0': *buf = '\0'; break;
            case '\n': lineNo++; continue;
            default: perr(); printf("unknown escape character\n"); exit(0);
            }
            break;
        case '"': *buf = 0; return;
        }
        buf++;
    }
}

int hex(char *s, int *n) {
    do {
        *n <<= 4;
        if(*s >= '0' && *s <= '9') *n |= *s-'0';
        else if(*s >= 'a' && *s <= 'f') *n |= *s-'a'+10;
        else if(*s >= 'A' && *s <= 'F') *n |= *s-'A'+10;
        else return 0;
    } while(*(++s));
    return 1;
}

int oct(char *s, int *n) {
    do {
        *n <<= 3;
        if(*s >= '0' && *s <= '7') *n |= *s-'0';
        else return 0;
    } while(*(++s));
    return 1;
}

int bin(char *s, int *n) {
    do {
        *n <<= 1;
        if(*s == '1') *n |= 1;
        else if(*s != '0') return 0;
    } while(*(++s));
    return 1;
}

int number(char *s, int *n) {
    *n = 0;
    if(!strncmp(s, "0x", 2)) return hex(s+2, n);
    if(!strncmp(s, "0b", 2)) return bin(s+2, n);
    if(*s == '0') return oct(s, n);
    do {
        *n *= 10;
        if(*s >= '0' && *s <= '9') *n += *s-'0';
        else return 0;
    } while(*(++s));
    return 1;
}

int evalExpr(char skip);

int evalAtom(char skip) {
    int n;
    parseNext();
    if(!strcmp(next, "'")) return parseChar();
    if(!strcmp(next, "-")) return -evalAtom(skip);
    if(number(next, &n) || skip) return n;
    if((n = strnindex(labels, nlabels, next)) != -1) return labelv[n];
    perr(); printf("expected value\n"); exit(0);
}

int evalDiv(char skip) {
    int n;
    n = evalAtom(skip);
    lookAhead();
    if(!strcmp(ahead, "*")) { parseNext(); return n*evalDiv(skip); }
    if(!strcmp(ahead, "/")) { parseNext(); return n/evalDiv(skip); }
    if(!strcmp(ahead, "%")) { parseNext(); return n%evalDiv(skip); }
    return n;
}

int evalAdd(char skip) {
    int n;
    n = evalDiv(skip);
    lookAhead();
    if(!strcmp(ahead, "+")) { parseNext(); return n+evalAdd(skip); }
    if(!strcmp(ahead, "-")) { parseNext(); return n-evalAdd(skip); }
    return n;
}

int evalExpr(char skip) {
    int n;
    n = evalAdd(skip);
    lookAhead();
    if(!strcmp(ahead, "&")) { parseNext(); return n&evalExpr(skip); }
    if(!strcmp(ahead, "|")) { parseNext(); return n|evalExpr(skip); }
    if(!strcmp(ahead, "^")) { parseNext(); return n^evalExpr(skip); }
    return n;
}

int nextReg() {
    int t, n;
    parseNext();
    if(*next != 'r') { perr(); printf("expected register\n"); exit(0); }
    if(!number(next+1, &n)) {
        perr(); printf("expected register\n"); exit(0);
    }
    if(n < 0 || n > 15) { perr(); printf("invalid register\n"); exit(0); }
    return n;
}

void pass1() {
    int i;
    for(;;) {
        parseNext();
        if(!*next) break;
        if(strindex(rrIns, next) != -1) {
            org += 2;
            nextReg(); expect(","); nextReg();
        } else if((i = strindex(rIns, next)) != -1) {
            nextReg();
            org++;
            if(i >= 7) {
                expect(",");
                evalExpr(1);
                if(i == 7) org += 4;
                else if(i == 8) org += 2;
                else if(i >= 9 && i <= 11) org++;
                else org += 2;
            }
        } else if(!strcmp(next, "sys") || !strcmp(next, "nop")) {
            org++;
        } else if(!strcmp(next, "bra")) {
            org += 3;
            evalExpr(1);
        } else if(!strcmp(next, "jsr")) {
            org += 5;
            evalExpr(1);
        } else if(!strcmp(next, "ret")) {
            org += 2;
            evalExpr(1);
        } else if(!strcmp(next, "org")) {
            org = evalExpr(0);
        } else if(!strcmp(next, "=")) {
            if(!nlabels) { perr(); printf("no label before =\n"); exit(0); }
            labelv[nlabels-1] = evalExpr(0);
        } else if(!strcmp(next, "db")) {
            for(;;) {
                lookAhead();
                if(!strcmp(ahead, "\"")) {
                    parseNext(); parseString(next);
                    org += strlen(next);
                } else { evalExpr(1); org++; }
                lookAhead();
                if(strcmp(ahead, ",")) break;
                parseNext();
            }
        } else if(!strcmp(next, "dw")) {
            for(;;) {
                evalExpr(1);
                org += 4;
                lookAhead();
                if(strcmp(ahead, ",")) break;
                parseNext();
            }
        } else {
            if(strnindex(labels, nlabels, next) != -1) {
                perr(); printf("duplicate label\n"); exit(0);
            }
            labelv[nlabels] = org;
            labels[nlabels++] = addName(next);
            expect(":");
        }
    }
}

void sh(int a, int h) {
    memory[a] = h;
    memory[a+1] = h>>8;
}

void pass2() {
    int i;
    for(;;) {
        parseNext();
        if(!*next) break;
        if((i = strindex(rrIns, next)) != -1) {
            memory[nmemory++] = 0x04+i;
            memory[nmemory] = nextReg()<<4;
            expect(",");
            memory[nmemory++] |= nextReg();
            org += 2;
        } else if((i = strindex(rIns, next)) != -1) {
            memory[nmemory++] = 0x20+(i<<4)|nextReg();
            org++;
            if(i >= 7) {
                expect(",");
                if(i == 7) {
                    *(int*)&memory[nmemory] = evalExpr(0);
                    nmemory += 4;
                    org += 4;
                } else if(i == 8) {
                    sh(nmemory, evalExpr(0));
                    nmemory += 2;
                    org += 2;
                } else if(i >= 9 && i <= 11) {
                    memory[nmemory++] = evalExpr(0);
                    org++;
                } else {
                    org += 2;
                    sh(nmemory, evalExpr(0)-org);
                    nmemory += 2;
                }
            }
        } else if(!strcmp(next, "sys")) {
            memory[nmemory++] = 0x00;
            org++;
        } else if(!strcmp(next, "nop")) {
            memory[nmemory++] = 0x1f;
            org++;
        } else if(!strcmp(next, "bra")) {
            org += 3;
            memory[nmemory++] = 0x01;
            sh(nmemory, evalExpr(0)-org);
            nmemory += 2;
        } else if(!strcmp(next, "jsr")) {
            org += 5;
            memory[nmemory++] = 0x02;
            *(int*)&memory[nmemory] = evalExpr(0);
            nmemory += 4;
        } else if(!strcmp(next, "ret")) {
            org += 2;
            memory[nmemory++] = 0x03;
            memory[nmemory++] = evalExpr(0);
        } else if(!strcmp(next, "org")) {
            org = evalExpr(0);
        } else if(!strcmp(next, "=")) {
            evalExpr(0);
        } else if(!strcmp(next, "db")) {
            for(;;) {
                lookAhead();
                if(!strcmp(ahead, "\"")) {
                    parseNext(); parseString(&memory[nmemory]);
                    org += strlen(&memory[nmemory]);
                    nmemory += strlen(&memory[nmemory]);
                } else { memory[nmemory++] = evalExpr(0); org++; }
                lookAhead();
                if(strcmp(ahead, ",")) break;
                parseNext();
            }
        } else if(!strcmp(next, "dw")) {
            for(;;) {
                *(int*)&memory[nmemory] = evalExpr(0);
                nmemory += 4;
                org += 4;
                lookAhead();
                if(strcmp(ahead, ",")) break;
                parseNext();
            }
        } else {
            expect(":");
        }
    }
}

int main(int argc, char **args) {
    if(argc != 3) {
        printf("usage: %s <file> <out>\n", args[0]);
        return 0;
    }
    if(!(fp = fopen(args[1], "r"))) {
        printf("failed to open %s\n", args[1]);
        return 0;
    }
    filename = args[1];
    pass1();
    fseek(fp, 0, SEEK_SET);
    lineNo = 1;
    org = 0;
    pass2();
    fclose(fp);
    if(!(fp = fopen(args[2], "wb"))) {
        printf("failed to open %s\n", args[2]);
        return 0;
    }
    fwrite(memory, 1, nmemory, fp);
    fclose(fp);
    printf("assembled %d bytes\n", nmemory);
    return 0;
}

