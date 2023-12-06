/* virtualb compiler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ORG 0x10000
#define BUFSZ 200
#define MEMORYSZ 32768
#define DATASZ 32768
#define MAXGLOBALS 600
#define MAXBSS 200
#define MAXDATA 200
#define STRINGBUFSZ 32768
#define MAXDSTRINGS 20
#define NAMEBUFSZ 8192
#define TEMPNAMEBUFSZ 512
#define MAXLOCALS 64
#define MAXARGS 64
#define MAXEXREFS 2000
#define MAXTOKENS 300
#define MAXLISTS 70
#define MAXBRKS 200
#define MAXCONS 150
#define MAXRETS 200

enum {
    EXTRN,
    BSS,
    BSSV,
    DATA,
    DATAV,
    FUN,
    STRING,
};

enum {
    ADD=0x0d,
    SUB, INDEX, AND, LOR, XOR, SHR, SHL, MUL, DIV, MOD,
    EQU, NEQ, LTE, GTN, LTN, GTE,

    INC=0x20, DEC=0x30, INV=0x40, NOT=0x50, NEG=0x60,

    OP1, OP2,
    PREINC, PREDEC,
    DEREF, POINT,
    IMM, GLOBAL, LOCAL, ARG, CALL, ARGLIST,
    COND, COL, ASSIGN, ASSIGNEQ,
};

const char *delim = "-=+/%*&|^!~(){}[]'\"?:;,<>";

const char *opStrs[] = {
    "+", "-", "&", "|", "^", ">>", "<<", "*", "/", "%", 0,
};
const int ops[] = {
    0x0d, 0x0e, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
};
const char *term[] = { ";", ")", 0, };

struct global {
    char *name;
    int addr;
    char type;
};

struct list {
    char type;
    struct list *a, *b, *c;
    struct list **args;
    struct list *parent;
    union {
        int value;
        int argc;
    };
};

int lineNo, aheadLine;
FILE *fp;
const char *filename;
char next[BUFSZ];
char ahead[BUFSZ];
char ac = 0;
struct global globals[MAXGLOBALS];
int nglobals = 0;
char data[DATASZ];
char memory[MEMORYSZ];
int nmemory = 0;
int ndata = 0;
int nextBss = 0;
char stringBuf[STRINGBUFSZ];
int nstringBuf = 0;
char nameBuf[NAMEBUFSZ];
char *nameP = nameBuf;
char *locals[MAXLOCALS];
char *args[MAXARGS];
int nlocals, nargs;
char tnameBuf[TEMPNAMEBUFSZ];
char *tnameP;
int exrefs[MAXEXREFS];
int nexrefs = 0;
char pahead[BUFSZ];
char pnext[BUFSZ];
long ppos;
int pline;
char tokens[MAXTOKENS];
int ntokens;
struct list lists[MAXLISTS];
int nlists, rn = 0;
int brks[MAXBRKS];
int bsp = 0;
int cons[MAXCONS];
int csp = 0;
int rets[MAXRETS];
int rsp;

void perr() {
    printf("%s:%d: error: ", filename, lineNo);
}

void charAhead() {
    if(!ac) ac = fgetc(fp);
}

char charAheadAhead() {
    char c;
    charAhead();
    c = fgetc(fp);
    fseek(fp, -1, SEEK_CUR);
    return c;
}

char nextChar() {
    char c;
    if(ac) { c = ac; ac = 0; }
    else c = fgetc(fp);
    if(c == '\n') lineNo++;
    return c;
}

void parseBuf0(char *buf) {
    const char *ops = "+-/%*&|^!~";

    while((*buf = nextChar()) <= 32 && !feof(fp));
    if(!strchr(delim, *buf)) {
        while(*buf > 32 && !strchr(delim, *buf) && !feof(fp))
            *(++buf) = nextChar();
        ac = *(buf--);
    }

    switch(*buf) {
    case '=':
        charAhead();
        if(strchr(ops, ac) || ac == '=')
            *(++buf) = nextChar();
        else if((ac == '>' || ac == '<') && charAheadAhead() == ac) {
            *(++buf) = nextChar();
            *(++buf) = nextChar();
        }
        break;
    case '<': case '>':
        charAhead();
        if(ac == '=')
            *(++buf) = nextChar();
        else if(ac == *buf)
            *(++buf) = nextChar();
        break;
    case '&': case '|': case '^':
        charAhead();
        if(ac == *buf)
            *(++buf) = nextChar();
        break;
    case '!':
        charAhead();
        if(ac == '=')
            *(++buf) = nextChar();
        break;
    case '+':
        charAhead();
        if(ac == '+')
            *(++buf) = nextChar();
        break;
    case '-':
        charAhead();
        if(ac == '-')
            *(++buf) = nextChar();
        break;
    case '/':
        charAhead();
        if(ac == '/' || ac == '*')
            *(++buf) = nextChar();
        break;
    case '*':
        charAhead();
        if(ac == '/')
            *(++buf) = nextChar();
        break;
    }

    buf[1] = 0;
}

void parseBuf(char *buf) {
    int ln;
    for(;;) {
        parseBuf0(buf);
        if(!strcmp(buf, "/*")) {
            ln = lineNo;
            do {
                if(feof(fp)) {
                    lineNo = ln;
                    perr();
                    printf("unterminated comment /*\n");
                    exit(0);
                }
                parseBuf0(buf);
            } while(strcmp(buf, "*/"));
        } else if(!strcmp(buf, "//")) {
            while(nextChar() != '\n' && !feof(fp));
        } else break;
    }
}

void parseNext() {
    if(*ahead) { strcpy(next, ahead); *ahead = 0; lineNo = aheadLine; }
    else parseBuf(next);
}

void lookAhead() {
    int pline;
    if(!(*ahead)) {
        pline = lineNo;
        parseBuf(ahead);
        aheadLine = lineNo;
        lineNo = pline;
    }
}

void savePos() {
    pline = lineNo;
    ppos = ftell(fp);
    strcpy(pahead, ahead);
    strcpy(pnext, next);
}

void restorePos() {
    lineNo = pline;
    fseek(fp, ppos, SEEK_SET);
    strcpy(ahead, pahead);
    strcpy(next, pnext);
}

void swapPos() {
    int ln;
    long p;
    char buf[BUFSZ];
    strcpy(buf, ahead);
    strcpy(ahead, pahead);
    strcpy(pahead, buf);
    strcpy(buf, next);
    strcpy(next, pnext);
    strcpy(pnext, buf);
    ln = pline;
    pline = lineNo;
    lineNo = ln;
    p = ppos;
    ppos = ftell(fp);
    fseek(fp, p, SEEK_SET);
}

void parseString0(char *buf) {
    for(;;) {
        switch(*buf = nextChar()) {
        case '*':
            switch(*buf = nextChar()) {
            case '*': *buf = '*'; break;
            case '"': *buf = '"'; break;
            case 'n': *buf = '\n'; break;
            case 'r': *buf = '\r'; break;
            case 't': *buf = '\t'; break;
            case 'b': *buf = '\b'; break;
            case '0': *buf = '\0'; break;
            default:
                perr(); printf("unknown escape character %c\n", *buf); exit(0);
            }
            break;
        case '"':
            *buf = 0;
            return;
        case '\n':
            perr(); printf("EOL before end of string\n"); exit(0);
        default:
            if(feof(fp)) {
                perr(); printf("EOF before end of string\n"); exit(0);
            }
            break;
        }
        buf++;
    }
}

void parseString(char *buf) {
    do {
        parseString0(buf);
        buf += strlen(buf);
        lookAhead();
    } while(!strcmp(ahead, "\""));
}

int number(char *s, int *n);

void validName(char *buf) {
    int n;
    if(strchr(delim, *buf) || number(buf, &n)) {
        perr();
        printf("expected identifier, got %s\n", buf);
        exit(0);
    }
}

int evalExpr();

void expect(const char *s) {
    parseNext();
    if(strcmp(next, s)) {
        perr(); printf("expected %s, not %s\n", s, next); exit(0);
    }
}

int hex(char *s, int *n) {
    do {
        *n <<= 4;
        if(*s >= '0' && *s <= '9')
            *n |= *s-'0';
        else if(*s >= 'a' && *s <= 'f')
            *n |= *s-'a'+10;
        else if(*s >= 'A' && *s <= 'F')
            *n |= *s-'A'+10;
        else return 0;
    } while(*(++s));
    return 1;
}

int oct(char *s, int *n) {
    do {
        *n <<= 3;
        if(*s >= '0' && *s <= '7')
            *n |= *s-'0';
        else return 0;
    } while(*(++s));
    return 1;
}

int number(char *s, int *n) {
    *n = 0;
    if(*s == '0') {
        if(s[1] == 'x') return hex(s+2, n);
        else if(s[1]) return oct(s+1, n);
    }
    do {
        *n *= 10;
        if(*s >= '0' && *s <= '9')
            *n += *s-'0';
        else return 0;
    } while(*(++s));
    return 1;
}

int parseChar() {
    int h;
    char c;
    h = 0;
    for(;;) {
        switch(c = nextChar()) {
        case '*':
            switch(c = nextChar()) {
            case '*': c = '*'; break;
            case '\'': c = '\''; break;
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'b': c = '\b'; break;
            case '0': c = '\0'; break;
            default: perr(); printf("unknown escape char %c\n", c); exit(0);
            }
            break;
        case '\'':
            return h;
        case '\n': case EOF: case 0:
            perr(); printf("expected char\n"); exit(0);
        }
        h = h<<8|c&0xff;
    }
}

int value(char *s) {
    int n;
    if(number(s, &n)) return n;
    if(!strcmp(s, "'")) return parseChar();
    perr(); printf("expected value\n"); exit(0);
}

int evalAtom() {
    int n;
    parseNext();
    if(!strcmp(next, "++")) return evalAtom()+1;
    if(!strcmp(next, "--")) return evalAtom()-1;
    if(!strcmp(next, "-")) return -evalAtom();
    if(!strcmp(next, "~")) return ~evalAtom();
    if(!strcmp(next, "!")) return !evalAtom();
    if(!strcmp(next, "(")) { n = evalExpr(); expect(")"); return n; }
    return value(next);
}

int evalDiv() {
    int n;
    n = evalAtom();
    lookAhead();
    if(!strcmp(ahead, "*")) { parseNext(); return n*evalDiv(); }
    if(!strcmp(ahead, "/")) { parseNext(); return n/evalDiv(); }
    if(!strcmp(ahead, "%")) { parseNext(); return n%evalDiv(); }
    return n;
}

int evalAdd() {
    int n;
    n = evalDiv();
    lookAhead();
    if(!strcmp(ahead, "+")) { parseNext(); return n+evalAdd(); }
    if(!strcmp(ahead, "-")) { parseNext(); return n-evalAdd(); }
    return n;
}

int evalShift() {
    int n;
    n = evalAdd();
    lookAhead();
    if(!strcmp(ahead, ">>")) { parseNext(); return n>>evalShift(); }
    if(!strcmp(ahead, "<<")) { parseNext(); return n<<evalShift(); }
    return n;
}

int evalComp() {
    int n;
    n = evalShift();
    lookAhead();
    if(!strcmp(ahead, ">")) { parseNext(); return n>evalComp(); }
    if(!strcmp(ahead, "<")) { parseNext(); return n<evalComp(); }
    if(!strcmp(ahead, ">=")) { parseNext(); return n>=evalComp(); }
    if(!strcmp(ahead, "<=")) { parseNext(); return n<=evalComp(); }
    return n;
}

int evalEq() {
    int n;
    n = evalComp();
    lookAhead();
    if(!strcmp(ahead, "==")) { parseNext(); return n==evalEq(); }
    if(!strcmp(ahead, "!=")) { parseNext(); return n!=evalEq(); }
    return n;
}

int evalAnd() {
    int n;
    n = evalEq();
    lookAhead();
    if(!strcmp(ahead, "&")) { parseNext(); return n&evalAnd(); }
    return n;
}

int evalXor() {
    int n;
    n = evalAnd();
    lookAhead();
    if(!strcmp(ahead, "^")) { parseNext(); return n^evalXor(); }
    return n;
}

int evalExpr() {
    int n;
    n = evalXor();
    lookAhead();
    if(!strcmp(ahead, "|")) { parseNext(); return n|evalExpr(); }
    if(!strcmp(ahead, "^")) { parseNext(); return n|evalExpr(); }
    if(!strcmp(ahead, "&")) { parseNext(); return n|evalExpr(); }
    return n;
}

void deferString() {
    globals[nglobals++] = (struct global) { "*", nstringBuf, STRING, };
    parseString(&stringBuf[nstringBuf]);
    nstringBuf += strlen(&stringBuf[nstringBuf])+1;
}

void addName(char *name) {
    strcpy(nameP, name);
    nameP += strlen(name)+1;
}

void addTname(char *name) {
    strcpy(tnameP, name);
    tnameP += strlen(name)+1;
}

int findGlobal(char *name) {
    int i;
    for(i = 0; i < nglobals; i++)
        if(globals[i].name && !strcmp(globals[i].name, name)) return i;
    return -1;
}

int strindex(const char **strs, char *s) {
    int i;
    for(i = 0; strs[i]; i++)
        if(!strcmp(strs[i], s)) return i;
    return -1;
}

int strnindex(char **strs, int len, char *s) {
    int i;
    for(i = 0; i < len; i++)
        if(!strcmp(strs[i], s)) return i;
    return -1;
}

void sh(int a, int h) {
    memory[a] = h;
    memory[a+1] = h>>8;
}

struct list *listExpr();

struct list *listAtom() {
    int i;
    struct list *l;
    l = &lists[nlists++];
    parseNext();
    if((i = findGlobal(next)) != -1) {
        l->type = GLOBAL;
        l->value = i;
    } else if((i = strnindex(locals, nlocals, next)) != -1) {
        l->type = LOCAL;
        l->value = i;
    } else if((i = strnindex(args, nargs, next)) != -1) {
        l->type = ARG;
        l->value = i;
    } else if(!strcmp(next, "(")) {
        nlists--;
        l = listExpr();
        expect(")");
    } else if(!strcmp(next, "\"")) {
        l->type = GLOBAL;
        l->value = nglobals;
        deferString();
    } else if(!strcmp(next, "'")) {
        l->type = IMM;
        l->value = parseChar();
    } else {
        if(!number(next, &i)) {
            perr(); printf("expected value, got %s\n", next); exit(0);
        }
        l->type = IMM;
        l->value = i;
    }
    return l;
}

struct list *listPost() {
    struct list *l, *a;
    a = listAtom();
    l = &lists[nlists];
    l->a = a;
    l->a->parent = l;
    lookAhead();
    if(!strcmp(ahead, "[")) {
        parseNext();
        nlists++;
        l->b = listExpr();
        l->type = INDEX;
        expect("]");
        l->b->parent = l;
        return l;
    } else if(!strcmp(ahead, "++")) {
        parseNext();
        l->type = INC;
    } else if(!strcmp(ahead, "--")) {
        parseNext();
        l->type = DEC;
    } else if(!strcmp(ahead, "(")) {
        parseNext();
        l->type = CALL;
        l->value = 0;
        lookAhead();
        nlists++;
        if(strcmp(ahead, ")")) {
            l->b = &lists[nlists++];
            a = l->b;
            for(;;) {
                a->type = ARGLIST;
                a->value = ++l->value;
                a->a = listExpr();
                a->b = &lists[nlists++];
                a->a->parent = a;
                a->b->parent = a;
                a = a->b;
                lookAhead();
                if(!strcmp(ahead, ")")) break;
                expect(",");
            }
            nlists--;
            a->parent->b = 0;
        } else l->b = 0;
        parseNext();
        return l;
    } else return a;
    nlists++;
    return l;
}

struct list *listUnary() {
    struct list *l;
    l = &lists[nlists++];
    l->type = OP1;
    lookAhead();
    if(!strcmp(ahead, "++"))
        l->type = PREINC;
    else if(!strcmp(ahead, "--"))
        l->type = PREDEC;
    else if(!strcmp(ahead, "-"))
        l->value = NEG;
    else if(!strcmp(ahead, "~"))
        l->value = INV;
    else if(!strcmp(ahead, "!"))
        l->value = NOT;
    else if(!strcmp(ahead, "*"))
        l->type = DEREF;
    else if(!strcmp(ahead, "&"))
        l->type = POINT;
    else { nlists--; return listPost(); }
    parseNext();
    l->a = listUnary();
    l->a->parent = l;
    return l;
}

struct list *listDiv() {
    struct list *l, *a;
    a = listUnary();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "*")) {
        parseNext();
        l->type = OP2;
        l->value = MUL;
    } else if(!strcmp(ahead, "/")) {
        parseNext();
        l->type = OP2;
        l->value = DIV;
    } else if(!strcmp(ahead, "%")) {
        parseNext();
        l->type = OP2;
        l->value = MOD;
    } else return a;
    nlists++;
    l->b = listDiv();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

struct list *listAdd() {
    struct list *l, *a;
    a = listDiv();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "+")) {
        parseNext();
        l->type = OP2;
        l->value = ADD;
    } else if(!strcmp(ahead, "-")) {
        parseNext();
        l->type = OP2;
        l->value = SUB;
    } else return a;
    nlists++;
    l->b = listAdd();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

struct list *listShift() {
    struct list *l, *a;
    a = listAdd();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, ">>")) {
        parseNext();
        l->type = OP2;
        l->value = SHL;
    } else if(!strcmp(ahead, "<<")) {
        parseNext();
        l->type = OP2;
        l->value = SHR;
    } else return a;
    nlists++;
    l->b = listShift();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

struct list *listCmp() {
    struct list *l, *a;
    a = listShift();
    l = &lists[nlists];
    l->a = a;
    l->type = OP2;
    lookAhead();
    if(!strcmp(ahead, ">=")) {
        parseNext();
        l->value = GTE;
    } else if(!strcmp(ahead, "<")) {
        parseNext();
        l->value = LTN;
    } else if(!strcmp(ahead, "<=")) {
        parseNext();
        l->value = LTE;
    } else if(!strcmp(ahead, ">")) {
        parseNext();
        l->value = GTN;
    } else return a;
    nlists++;
    l->b = listCmp();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

struct list *listEq() {
    struct list *l, *a;
    a = listCmp();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "==")) {
        parseNext();
        l->type = OP2;
        l->value = EQU;
    } else if(!strcmp(ahead, "!=")) {
        parseNext();
        l->type = OP2;
        l->value = NEQ;
    } else return a;
    nlists++;
    l->b = listCmp();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

struct list *listAnd() {
    struct list *l, *a;
    a = listEq();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "&")) {
        parseNext();
        l->type = OP2;
        l->value = AND;
        nlists++;
        l->b = listAnd();
        l->a->parent = l;
        l->b->parent = l;
        return l;
    }
    return a;
}

struct list *listXor() {
    struct list *l, *a;
    a = listAnd();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "^")) {
        parseNext();
        l->type = OP2;
        l->value = XOR;
        nlists++;
        l->b = listXor();
        l->a->parent = l;
        l->b->parent = l;
        return l;
    }
    return a;
}

struct list *listOr() {
    struct list *l, *a;
    a = listXor();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "|")) {
        parseNext();
        l->type = OP2;
        l->value = LOR;
        nlists++;
        l->b = listOr();
        l->a->parent = l;
        l->b->parent = l;
        return l;
    }
    return a;
}

struct list *listCond() {
    return listOr();
}

struct list *listExpr() {
    int i;
    struct list *l, *a;
    a = listCond();
    l = &lists[nlists];
    l->a = a;
    lookAhead();
    if(!strcmp(ahead, "=")) {
        parseNext();
        l->type = ASSIGNEQ;
    } else if(*ahead == '=' && (i = strindex(opStrs, ahead+1)) != -1) {
        parseNext();
        l->type = ASSIGN;
        l->value = ops[i];
    } else return a;
    nlists++;
    l->b = listExpr();
    l->a->parent = l;
    l->b->parent = l;
    return l;
}

void evalList(struct list *l) {
    switch(l->type) {
    case OP2:
        evalList(l->a);
        evalList(l->b);
        if(l->a->type == IMM && l->b->type == IMM) {
            switch(l->value) {
            case ADD: l->value = l->a->value+l->b->value; break;
            case SUB: l->value = l->a->value-l->b->value; break;
            case AND: l->value = l->a->value&l->b->value; break;
            case LOR: l->value = l->a->value|l->b->value; break;
            case XOR: l->value = l->a->value^l->b->value; break;
            case SHR: l->value = l->a->value>>l->b->value; break;
            case SHL: l->value = l->a->value<<l->b->value; break;
            case MUL: l->value = l->a->value*l->b->value; break;
            case DIV: l->value = l->a->value/l->b->value; break;
            case MOD: l->value = l->a->value%l->b->value; break;
            case EQU: l->value = l->a->value==l->b->value; break;
            case NEQ: l->value = l->a->value!=l->b->value; break;
            case LTE: l->value = l->a->value<=l->b->value; break;
            case GTN: l->value = l->a->value>l->b->value; break;
            case LTN: l->value = l->a->value<l->b->value; break;
            case GTE: l->value = l->a->value>=l->b->value; break;
            }
            l->type = IMM;
        }
        break;
    case OP1:
        evalList(l->a);
        if(l->a->type == IMM) {
            l->type = IMM;
            switch(l->value) {
            case NEG: l->value=-l->a->value; break;
            case INV: l->value=~l->a->value; break;
            case NOT: l->value=!l->a->value; break;
            }
        }
        break;
    }
}

void compileList(struct list *l);

void storeLval(struct list *l) {
    rn++;
    switch(l->type) {
    case LOCAL:
        /* str rn,local */
        memory[nmemory++] = 0xd0|(rn-1);
        memory[nmemory++] = -l->value-1;
        break;
    case ARG:
        /* str rn,arg */
        memory[nmemory++] = 0xd0|(rn-1);
        memory[nmemory++] = nargs-l->value+1;
        break;
    case GLOBAL:
        /* lwi rn,global */
        memory[nmemory++] = 0x90|rn;
        *(int*)&memory[nmemory] = l->value;
        exrefs[nexrefs++] = nmemory;
        nmemory += 4;
        /* stw r0,rn */
        memory[nmemory++] = 0x0a;
        memory[nmemory++] = (rn-1)<<4|rn;
        break;
    case INDEX:
        switch(l->a->type) {
        case GLOBAL:
            /* lwi rn,global */
            memory[nmemory++] = 0x90|rn;
            *(int*)&memory[nmemory] = l->value;
            exrefs[nexrefs++] = nmemory;
            nmemory += 4;
            rn++;
            compileList(l->b);
            break;
        default:
            compileList(l->a);
            rn++;
            compileList(l->b);
            break;
        }
        rn--;
        /* adw rn,r1 */
        memory[nmemory++] = 0x0f;
        memory[nmemory++] = rn<<4|rn+1;
        /* stw r0,rn */
        memory[nmemory++] = 0x0a;
        memory[nmemory++] = (rn-1)<<4|rn;
        break;
    case DEREF:
        compileList(l->a);
        rn--;
        /* stw r0,rn */
        memory[nmemory++] = 0x0a;
        memory[nmemory++] = rn<<4|rn+1;
        break;
    default:
        perr(); printf("expected lvalue\n"); exit(0);
        break;
    }
    rn--;
}

void compileCall(struct list *l) {
    int i, orn;
    struct list *a;

    for(i = 0; i < rn; i++)
        /* psh rn */
        memory[nmemory++] = 0x70|i;

    orn = rn;
    rn = 0;
    i = 0;

    for(a = l->b; a; a = a->b) {
        compileList(a->a);
        /* psh r0 */
        memory[nmemory++] = 0x70;
    }

    switch(l->a->type) {
    case GLOBAL:
        /* jsr global */
        memory[nmemory++] = 0x02;
        *(int*)&memory[nmemory] = l->a->value;
        exrefs[nexrefs++] = nmemory;
        nmemory += 4;
        break;
    default:
        compileList(l->a);
        /* phm pc,r0 */
        memory[nmemory++] = 0x1e;
        memory[nmemory++] = 0xd0;
        break;
    }
    rn = orn;

    if(rn) {
        /* mov rn,r0 */
        memory[nmemory++] = 0x04;
        memory[nmemory++] = rn<<4;
    }

    for(i = rn-1; i >= 0; i--)
        /* psh rn */
        memory[nmemory++] = 0x70|i;
}

int isAdd(int op) {
    return (op==ADD)|(op==SUB);
}

int samePrecedence(struct list *l1, struct list *l2) {
    if(!l1 || !l2) return 0;
    if(l1->type != OP2 || l2->type != OP2) return 0;
    if(l1->value == MOD) return 0;
    if(l1->value == DIV) return 0;
    if(l1->value == l2->value) return 1;
    if(isAdd(l1->value) & isAdd(l2->value)) return 1;
    return 0;
}

void compileList(struct list *l) {
    evalList(l);
    switch(l->type) {
    case ASSIGNEQ:
        compileList(l->b);
        storeLval(l->a);
        break;
    case ASSIGN:
        compileList(l->a);
        rn++;
        compileList(l->b);
        rn--;
        memory[nmemory++] = l->value;
        memory[nmemory++] = rn<<4|rn+1;
        storeLval(l->a);
        break;
    case INC: case DEC:
        compileList(l->a);
        /* mov r1,rn */
        memory[nmemory++] = 0x04;
        memory[nmemory++] = (rn+1)<<4|rn;
        rn++;
        /* inc/dec r1 */
        memory[nmemory++] = l->type|rn;
        storeLval(l->a);
        rn--;
        break;
    case PREINC:
        compileList(l->a);
        /* inc rn */
        memory[nmemory++] = 0x20|rn;
        storeLval(l->a);
        break;
    case PREDEC:
        compileList(l->a);
        /* dec rn */
        memory[nmemory++] = 0x30|rn;
        storeLval(l->a);
        break;
    case OP2:
        if(samePrecedence(l, l->parent)) {
            compileList(l->a);
            memory[nmemory++] = l->parent->value;
            memory[nmemory++] = (rn-1)<<4|rn;
            compileList(l->b);
            memory[nmemory++] = l->value;
            memory[nmemory++] = (rn-1)<<4|rn;
        } else {
            compileList(l->a);
            rn++;
            compileList(l->b);
            rn--;
            memory[nmemory++] = l->value;
            memory[nmemory++] = rn<<4|rn+1;
        }
        if(samePrecedence(l, l->b)) nmemory -= 2;
        break;
    case OP1:
        compileList(l->a);
        memory[nmemory++] = l->value|rn;
        break;
    case CALL:
        compileCall(l);
        break;
    case LOCAL:
        /* ldr rn,local */
        memory[nmemory++] = 0xc0|rn;
        memory[nmemory++] = -l->value-1;
        break;
    case ARG:
        /* ldr rn,arg */
        memory[nmemory++] = 0xc0|rn;
        memory[nmemory++] = nargs-l->value+1;
        break;
    case GLOBAL:
        /* lwi rn,global */
        memory[nmemory++] = 0x90|rn;
        *(int*)&memory[nmemory] = l->value;
        exrefs[nexrefs++] = nmemory;
        nmemory += 4;
        if(globals[l->value].type==DATAV || globals[l->value].type==BSSV) {
            /* ldw rn,rn */
            memory[nmemory++] = 0x05;
            memory[nmemory++] = rn<<4|rn;
        }
        break;
    case INDEX:
        switch(l->a->type) {
        case GLOBAL:
            /* lwi rn,global */
            memory[nmemory++] = 0x90|rn;
            *(int*)&memory[nmemory] = l->value;
            exrefs[nexrefs++] = nmemory;
            nmemory += 4;
            rn++;
            compileList(l->b);
            break;
        default:
            compileList(l->a);
            rn++;
            compileList(l->b);
            break;
        }
        rn--;
        /* adw rn,r1 */
        memory[nmemory++] = 0x0f;
        memory[nmemory++] = rn<<4|rn+1;
        /* ldw rn,rn */
        memory[nmemory++] = 0x05;
        memory[nmemory++] = rn<<4|rn;
        break;
    case POINT:
        switch(l->a->type) {
        case GLOBAL:
            /* lwi rn,global */
            memory[nmemory++] = 0x90|rn;
            *(int*)&memory[nmemory] = l->a->value;
            exrefs[nexrefs++] = nmemory;
            nmemory += 4;
            break;
        case LOCAL:
            /* mov rn,bp */
            memory[nmemory++] = 0x04;
            memory[nmemory++] = rn<<4|0xe;
            /* lbi r1,local */
            memory[nmemory++] = 0xb0|rn+1;
            memory[nmemory++] = -l->a->value-1;
            /* adw rn,r1 */
            memory[nmemory++] = 0x0f;
            memory[nmemory++] = rn<<4|rn+1;
            break;
        case ARG:
            /* mov rn,bp */
            memory[nmemory++] = 0x04;
            memory[nmemory++] = rn<<4|0xe;
            /* lbi r1,local */
            memory[nmemory++] = 0xb0|rn+1;
            memory[nmemory++] = nargs-l->a->value+1;
            /* adw rn,r1 */
            memory[nmemory++] = 0x0f;
            memory[nmemory++] = rn<<4|rn+1;
            break;
        default:
            perr(); printf("expected lvalue\n"); exit(0);
        }
        break;
    case DEREF:
        compileList(l->a);
        /* ldw rn,rn */
        memory[nmemory++] = 0x05;
        memory[nmemory++] = rn<<4|rn;
        break;
    case IMM:
        if(l->value >= -128 && l->value <= 127) {
            /* lbi rn,value */
            memory[nmemory++] = 0xb0|rn;
            memory[nmemory++] = l->value;
        } else if(l->value >= -32768 && l->value <= 32767) {
            /* lhi rn,value */
            memory[nmemory++] = 0xa0|rn;
            sh(nmemory, l->value);
            nmemory += 2;
        } else {
            /* lwi rn,value */
            memory[nmemory++] = 0x90|rn;
            *(int*)&memory[nmemory] = l->value;
            nmemory += 4;
        }
        break;
    }
}

void resolveBreaks() {
    while(bsp && brks[--bsp]) sh(brks[bsp], nmemory-brks[bsp]-2);
}

void resolveRets() {
    while(rsp--) sh(rets[rsp], nmemory-rets[rsp]-2);
}

void printOp2(int op) {
    switch(op) {
    case ADD: printf("+"); break;
    case SUB: printf("-"); break;
    case AND: printf("&"); break;
    case LOR: printf("|"); break;
    case XOR: printf("^"); break;
    case SHR: printf(">>"); break;
    case SHL: printf("<<"); break;
    case MUL: printf("*"); break;
    case DIV: printf("/"); break;
    case MOD: printf("%%"); break;
    case EQU: printf("=="); break;
    case NEQ: printf("!="); break;
    case LTE: printf("<="); break;
    case GTN: printf(">"); break;
    case LTN: printf("<"); break;
    case GTE: printf(">="); break;
    }
}

void printOp1(int op) {
    switch(op) {
    case NEG: printf("-"); break;
    case NOT: printf("!"); break;
    case INV: printf("~"); break;
    }
}

void printList(struct list *l) {
    int i;
    switch(l->type) {
    case IMM: printf("%d", l->value); return;
    case GLOBAL: printf("g_%s", globals[l->value].name); return;
    case LOCAL: printf("l_%s", locals[l->value]); return;
    case ARG: printf("a_%s", args[l->value]); return;
    case ARGLIST:
        printList(l->a);
        if(l->b) {
            printf(" ");
            printList(l->b);
        }
        return;
    }
    printf("(");
    switch(l->type) {
    case OP1: printOp1(l->value); break;
    case OP2: printOp2(l->value); break;
    case DEREF: printf("deref*"); break;
    case POINT: printf("point&"); break;
    case CALL: printf("call"); break;
    case INDEX: printf("index"); break;
    case ASSIGNEQ: printf("="); break;
    case ASSIGN: printOp2(l->value); printf("="); break;
    case INC: printf("n++"); break;
    case DEC: printf("n--"); break;
    case PREINC: printf("++n"); break;
    case PREDEC: printf("--n"); break;
    }
    switch(l->type) {
    case OP1: case DEREF: case POINT:
    case INC: case DEC: case PREINC: case PREDEC:
        printf(" ");
        printList(l->a);
        break;
    case CALL:
        printf(" ");
        printList(l->a);
        printf(" ");
        if(l->b) printList(l->b);
        break;
    default:
        printf(" ");
        printList(l->a);
        printf(" ");
        printList(l->b);
        break;
    }
    printf(")");
}

void compileExpr() {
    int i;
    struct list *l;
    lookAhead();
    if(strindex(term, ahead) != -1) return;
    nlists = 0;
    rn = 0;
    l = listExpr();
    l->parent = 0;
    printList(l);
    printf("\n");
    /*for(i = 0; i < nlists; i++) {
        printList(&lists[i]);
        printf("\n");
    }*/
    compileList(l);
}

void beforeEof(const char *s) {
    if(feof(fp)) { perr(); printf("expected %s before EOF\n", s); exit(0); }
}

void compileStatement() {
    int o;
    lookAhead();
    if(!strcmp(ahead, "{")) {
        parseNext();
        for(;;) {
            lookAhead();
            if(!strcmp(ahead, "}")) break;
            beforeEof("}");
            compileStatement();
        }
        parseNext();
    } else if(!strcmp(ahead, "return")) {
        parseNext();
        compileExpr();
        expect(";");
        /* bra end */
        memory[nmemory++] = 0x01;
        rets[rsp++] = nmemory;
        nmemory += 2;
    } else if(!strcmp(ahead, "continue")) {
        parseNext();
        expect(";");
        if(!csp) { perr(); printf("unexpected continue\n"); exit(0); }
        /* bra loop */
        memory[nmemory++] = 0x01;
        sh(nmemory, cons[csp-1]);
        nmemory += 2;
    } else if(!strcmp(ahead, "break")) {
        parseNext();
        if(!bsp) { perr(); printf("unexpected break\n"); exit(0); }
        /* bra out */
        memory[nmemory++] = 0x01;
        brks[bsp++] = nmemory;
        nmemory += 2;
    } else if(!strcmp(ahead, "if")) {
        parseNext();
        expect("(");
        compileExpr();
        expect(")");
        /* beq r0,addr */
        memory[nmemory++] = 0xe0;
        o = nmemory;
        nmemory += 2;
        compileStatement();
        lookAhead();
        if(!strcmp(ahead, "else")) {
            parseNext();
            /* bra endif */
            memory[nmemory++] = 0x01;
            sh(o, nmemory+2-o-2);
            o = nmemory;
            nmemory += 2;
            compileStatement();
        }
        sh(o, nmemory-o-2);
    } else if(!strcmp(ahead, "while")) {
        parseNext();
        cons[csp++] = nmemory;
        brks[bsp++] = 0;
        expect("(");
        compileExpr();
        expect(")");
        /* beq r0,addr */
        memory[nmemory++] = 0xe0;
        o = nmemory;
        nmemory += 2;
        compileStatement();
        /* bra addr */
        memory[nmemory++] = 0x01;
        sh(nmemory, cons[--csp]-nmemory-2);
        nmemory += 2;
        sh(o, nmemory-o-2);
        resolveBreaks();
    } else if(!strcmp(ahead, "do")) {
        parseNext();
        cons[csp++] = nmemory;
        brks[bsp++] = 0;
        compileStatement();
        expect("while");
        expect("(");
        compileExpr();
        expect(")");
        expect(";");
        /* bne addr */
        memory[nmemory++] = 0xf0;
        sh(nmemory, cons[--csp]-nmemory-2);
        nmemory += 2;
        resolveBreaks();
    } else if(!strcmp(ahead, "for")) {
        parseNext();
        expect("(");
        compileExpr();
        expect(";");
        cons[csp++] = nmemory;
        brks[bsp++] = 0;
        compileExpr();
        expect(";");
        /* beq r0,addr */
        memory[nmemory++] = 0xe0;
        o = nmemory;
        nmemory += 2;
        savePos();
        listExpr();
        expect(")");
        compileStatement();
        swapPos();
        compileExpr();
        restorePos();
        /* bra addr */
        memory[nmemory++] = 0x01;
        sh(nmemory, cons[--csp]-nmemory-2);
        nmemory += 2;
        sh(o, nmemory-o-2);
        resolveBreaks();
    } else {
        compileExpr();
        expect(";");
    }
}

struct global *addGlobal(char *name, int addr, char type) {
    int i;
    validName(name);
    if((i = findGlobal(name)) != -1) {
        if(type == EXTRN)
            return &globals[i];
        if(globals[i].type != EXTRN) {
            perr(); printf("%s already defined\n", name); exit(0);
        }
        globals[i].type = type;
        globals[i].addr = addr;
        return &globals[i];
    } else {
        globals[nglobals] = (struct global) {
            nameP, addr, type,
        };
        addName(name);
        return &globals[nglobals++];
    }
}

void compileFunction(char *name) {
    int i;
    struct global *current;

    current = addGlobal(name, nmemory, FUN);

    tnameP = tnameBuf;
    nlocals = 0;
    nargs = 0;
    rsp = 0;
    for(;;) {
        parseNext();
        if(!strcmp(next, ")")) break;
        validName(next);
        args[nargs++] = tnameP;
        addTname(next);
        lookAhead();
        if(!strcmp(ahead, ")")) { parseNext(); break; }
        expect(",");
    }
    for(i = 0; i < nargs; i++)
        printf("%s ", args[i]);
    printf("\n");

    expect("{");

    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "auto")) {
            parseNext();
            parseNext();
            if(strcmp(next, ";")) {
                for(;;) {
                    validName(next);
                    locals[nlocals++] = tnameP;
                    addTname(next);
                    lookAhead();
                    if(!strcmp(ahead, ";")) break;
                    expect(",");
                    parseNext();
                }
                parseNext();
            }
        } else if(!strcmp(ahead, "extrn")) {
            parseNext();
            parseNext();
            if(strcmp(next, ";")) {
                for(;;) {
                    addGlobal(next, 0, EXTRN);
                    lookAhead();
                    if(!strcmp(ahead, "(")) {
                        parseNext(); expect(")"); lookAhead();
                    }
                    if(!strcmp(ahead, ";")) break;
                    expect(",");
                    parseNext();
                }
                parseNext();
            }
        } else break;
    }

    /* psh bp */
    memory[nmemory++] = 0x7e;
    /* mov bp,sp */
    memory[nmemory++] = 0x04;
    memory[nmemory++] = 0xef;
    if(nlocals) {
        /* lbi r0,-nlocals */
        memory[nmemory++] = 0xb0;
        memory[nmemory++] = -nlocals;
        /* adw sp,r0 */
        memory[nmemory++] = 0x0f;
        memory[nmemory++] = 0xf0;
    }

    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "}")) break;
        if(feof(fp)) { perr(); printf("expected }\n"); exit(0); }
        compileStatement();
    }
    parseNext();

    resolveRets();
    /* mov sp,bp */
    memory[nmemory++] = 0x04;
    memory[nmemory++] = 0xfe;
    /* pop bp */
    memory[nmemory++] = 0x8e;
    /* ret argc */
    memory[nmemory++] = 0x03;
    memory[nmemory++] = nargs;
}

void compileData(char *name, char type) {
    struct global *g;
    g = addGlobal(name, ndata, type);
    lookAhead();
    for(;;) {
        printf("%s\n", ahead);
        if(!strcmp(ahead, "\"")) {
            *(int*)&data[ndata] = nglobals;
            parseNext();
            deferString();
        } else *(int*)&data[ndata] = evalExpr();
        ndata += 4;
        lookAhead();
        if(!strcmp(ahead, ";")) break;
        expect(",");
        g->type = DATAV;
    }
    parseNext();
}

void compileOuter() {
    char name[BUFSZ];
    int n;

    parseNext();
    if(*next == 0) return;
    printf("%s\n", next);

    strcpy(name, next);
    lookAhead();
    if(!strcmp(ahead, "[")) {
        parseNext();
        lookAhead();
        if(!strcmp(ahead, "]")) {
            parseNext();
            lookAhead();
            if(!strcmp(ahead, ";")) {
                perr(); printf("unexpected ;\n"); exit(0);
            }
            compileData(name, DATAV);
        } else {
            addGlobal(name, nextBss, BSS);
            nextBss += evalExpr();
            expect("]");
            expect(";");
        }
    } else if(!strcmp(ahead, ";")) {
        parseNext();
        addGlobal(name, nextBss, BSSV);
        nextBss += 4;
    } else if(!strcmp(ahead, "(")) {
        parseNext();
        compileFunction(name);
    } else {
        compileData(name, DATA);
    }
}

void compileFile(const char *name) {
    FILE *pfp;
    const char *pname;
    int pline;

    pline = lineNo;
    pname = filename;
    pfp = fp;

    filename = name;
    lineNo = 1;
    fp = fopen(filename, "r");
    if(!fp) { perr(); printf("failed to open file\n"); exit(0); }

    while(!feof(fp) || *ahead) {
        compileOuter();
    }

    fclose(fp);

    fp = pfp;
    filename = pname;
    lineNo = pline;
}

void resolveExrefs() {
    int i, a;
    struct global *g;
    for(i = 0; i < nexrefs; i++) {
        g = &globals[*(int*)&memory[a=exrefs[i]]];
        switch(g->type) {
        case BSS: case BSSV:
            *(int*)&memory[a] = g->addr+ORG+nmemory+ndata+nstringBuf;
            break;
        case STRING:
            *(int*)&memory[a] = g->addr+ORG+nmemory+ndata;
            break;
        case DATA: case DATAV:
            *(int*)&memory[a] = g->addr+ORG+nmemory;
            break;
        case FUN:
            *(int*)&memory[a] = g->addr+ORG;
            break;
        default:
            perr(); printf("unresolved reference to %s\n", g->name); exit(0);
            break;
        }
    }
}

void saveFile(const char *filename) {
    FILE *fp;
    fp = fopen(filename, "wb");
    if(!fp) { printf("failed to open output file %s\n", filename); exit(0); }
    fwrite(memory, 1, nmemory, fp);
    fwrite(data, 1, ndata, fp);
    fwrite(stringBuf, 1, nstringBuf, fp);
    fclose(fp);
}

int main(int argc, char **args) {
    int i;
    assert(sizeof(char) == 1);
    assert(sizeof(int) == 4);
    char *outFile;
    outFile = "a.out";
    if(argc <= 1) {
        printf("usage: %s <file1.b file2.b ...>\n", args[0]);
        return 0;
    }
    *ahead = 0;
    /* jsr ORG+5 */
    memory[nmemory++] = 0x02;
    *(int*)&memory[nmemory] = ORG+5;
    nmemory += 4;
    /* lbi r0,0 */
    memory[nmemory++] = 0xb0;
    memory[nmemory++] = 0x00;
    /* sys */
    memory[nmemory++] = 0x00;
    for(i = 1; i < argc; i++) {
        if(!strcmp(args[i], "-o")) {
            if(++i < argc) outFile = args[i];
        } else compileFile(args[i]);
    }
    resolveExrefs();
    i = findGlobal("main");
    if(i == -1 || globals[i].type == EXTRN) printf("no main\n");
    else *(int*)&memory[1] = globals[i].addr+ORG;
    saveFile(outFile);
    return 0;
}
