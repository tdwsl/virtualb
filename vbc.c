/* virtualb compiler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
#define MAXGOT 120
#define MAXSTRINGS 200
#define MAXCONSTANTS 400

enum {
    EXTRN,
    BSS,
    BSSV,
    DATA,
    DATAV,
    FUN,
    STRING,
    ABSOLUTE,
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

struct constant {
    char *name;
    int value;
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

struct pos {
    int lineNo;
    long pos;
    char next[BUFSZ];
    char ahead[BUFSZ];
};

int org = 0x20000;
int lineNo, aheadLine;
FILE *fp;
const char *filename = "";
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
int localsz[MAXLOCALS];
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
int bcns[MAXBRKS];
int bcp = 0;
int cons[MAXCONS];
int csp = 0;
int rets[MAXRETS];
int rsp;
char *got[MAXGOT];
int ngot = 0;
char *currentFunction = 0;
int strings[MAXSTRINGS];
int nstrings = 0;
struct constant constants[MAXCONSTANTS];
int nconstants = 0;

void perr() {
    printf("%s:%d: error", filename, lineNo);
    if(currentFunction) printf(" in %s", currentFunction);
    printf(": ");
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
                    exit(1);
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

void savePos(struct pos *p) {
    p->lineNo = lineNo;
    p->pos = ftell(fp);
    strcpy(p->ahead, ahead);
    strcpy(p->next, next);
}

void restorePos(struct pos *p) {
    lineNo = p->lineNo;
    fseek(fp, p->pos, SEEK_SET);
    strcpy(ahead, p->ahead);
    strcpy(next, p->next);
}

void swapPos(struct pos *p) {
    int ln;
    long ps;
    char buf[BUFSZ];
    strcpy(buf, ahead);
    strcpy(ahead, p->ahead);
    strcpy(p->ahead, buf);
    strcpy(buf, next);
    strcpy(next, p->next);
    strcpy(p->next, buf);
    ln = p->lineNo;
    p->lineNo = lineNo;
    lineNo = ln;
    ps = p->pos;
    p->pos = ftell(fp);
    fseek(fp, ps, SEEK_SET);
}

int parseString0(char *buf) {
    char *p;
    p = buf;
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
            case 'e': *buf = '\0'; break;
            default:
                perr(); printf("unknown escape character %c\n", *buf); exit(1);
            }
            break;
        case '"':
            *buf = 0;
            return buf-p;
        case '\n':
            perr(); printf("EOL before end of string\n"); exit(1);
        default:
            if(feof(fp)) {
                perr(); printf("EOF before end of string\n"); exit(1);
            }
            break;
        }
        buf++;
    }
}

int parseString(char *buf) {
    int len;
    len = 0;
    do {
        len += parseString0(buf);
        buf += strlen(buf);
        lookAhead();
    } while(!strcmp(ahead, "\""));
    return len;
}

int number(char *s, int *n);

void validName(char *buf) {
    int n;
    if(strchr(delim, *buf) || number(buf, &n)) {
        perr();
        printf("expected identifier, got %s\n", buf);
        exit(1);
    }
}

int evalExpr();

void expect(const char *s) {
    parseNext();
    if(strcmp(next, s)) {
        perr(); printf("expected %s, not %s\n", s, next); exit(1);
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
        else return oct(s, n);
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
    char c;
    int h;
    h = 0;
    for(;;) {
        switch(c = nextChar()) {
        case '*':
            switch(c = nextChar()) {
            case '*': c = '*'; break;
            case '\'': c = '\''; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'b': c = '\b'; break;
            case 'e': c = '\0'; break;
            default: perr(); printf("unknown escape char %c\n", c); exit(1);
            }
            break;
        case '\'':
            return h;
        case '\n': case EOF: case 0:
            perr(); printf("expected char\n"); exit(1);
        }
        h = h<<8|c;
    }
    return h;
}

int findConstant(char *s) {
    int i;
    for(i = 0; i < nconstants; i++)
        if(!strcmp(constants[i].name, s)) return i;
    return -1;
}

int value(char *s) {
    int n;
    if(number(s, &n)) return n;
    if(!strcmp(s, "'")) return parseChar();
    if((n = findConstant(s)) != -1) return constants[n].value;
    perr(); printf("expected value, got %s\n", s); exit(1);
}

int evalAtom() {
    int n;
    parseNext();
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
    return n;
}

void deferString() {
    int len;
    globals[nglobals].name = "*";
    globals[nglobals].addr = nstringBuf;
    globals[nglobals++].type = STRING;
    //globals[nglobals++] = (struct global) { "*", nstringBuf, STRING, };
    len = parseString(&stringBuf[nstringBuf]);
    nstringBuf += len+1;
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
    if((i = strnindex(locals, nlocals, next)) != -1) {
        l->type = LOCAL;
        l->value = i;
    } else if((i = strnindex(args, nargs, next)) != -1) {
        l->type = ARG;
        l->value = i;
    } else if((i = findConstant(next)) != -1) {
        l->type = IMM;
        l->value = constants[i].value;
    } else if((i = findGlobal(next)) != -1) {
        l->type = GLOBAL;
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
            perr(); printf("expected value, got %s\n", next); exit(1);
        }
        l->type = IMM;
        l->value = i;
    }
    return l;
}

struct list *listPost(struct list *a) {
    struct list *l;
    if(!a) a = listAtom();
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
        return listPost(l);
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
        return listPost(l);
    } else return a;
    nlists++;
    return listPost(l);
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
    else { nlists--; return listPost(0); }
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
        l->value = SHR;
    } else if(!strcmp(ahead, "<<")) {
        parseNext();
        l->type = OP2;
        l->value = SHL;
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
        memory[nmemory++] = l->value+2;
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
            if(globals[l->a->value].type!=DATAV
                    && globals[l->a->value].type!=BSSV) {
                /* lwi rn,global */
                memory[nmemory++] = 0x90|rn;
                *(int*)&memory[nmemory] = l->a->value;
                exrefs[nexrefs++] = nmemory;
                nmemory += 4;
                break;
            }
        default:
            compileList(l->a);
            break;
        }
        rn++;
        compileList(l->b);
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
        perr(); printf("expected lvalue\n"); exit(1);
        break;
    }
    rn--;
}

void compileArg(struct list *l) {
    if(l) {
        compileArg(l->b);
        compileList(l->a);
        /* psh r0 */
        memory[nmemory++] = 0x70;
    }
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

    compileArg(l->b);

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
        /* pop rn */
        memory[nmemory++] = 0x80|i;
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

void compileLiteral(int rn, int n) {
    if(n >= -128 && n <= 127) {
        /* lbi rn,value */
        memory[nmemory++] = 0xb0|rn;
        memory[nmemory++] = n;
    } else if(n >= -32768 && n <= 32767) {
        /* lhi rn,value */
        memory[nmemory++] = 0xa0|rn;
        sh(nmemory, n);
        nmemory += 2;
    } else {
        /* lwi rn,value */
        memory[nmemory++] = 0x90|rn;
        *(int*)&memory[nmemory] = n;
        nmemory += 4;
    }
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
        memory[nmemory++] = l->value+2;
        break;
    case GLOBAL:
        /* lwi rn,global */
        memory[nmemory++] = 0x90|rn;
        *(int*)&memory[nmemory] = l->value;
        exrefs[nexrefs++] = nmemory;
        nmemory += 4;
        if(globals[l->value].type==DATAV || globals[l->value].type==BSSV
                || globals[l->value].type == EXTRN) {
            /* ldw rn,rn */
            memory[nmemory++] = 0x05;
            memory[nmemory++] = rn<<4|rn;
        }
        break;
    case INDEX:
        switch(l->a->type) {
        case GLOBAL:
            if(globals[l->a->value].type!=DATAV
                    && globals[l->a->value].type!=BSSV) {
                /* lwi rn,global */
                memory[nmemory++] = 0x90|rn;
                *(int*)&memory[nmemory] = l->a->value;
                exrefs[nexrefs++] = nmemory;
                nmemory += 4;
                break;
            }
        default:
            compileList(l->a);
            break;
        }
        rn++;
        compileList(l->b);
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
            /* lbi r1,arg */
            memory[nmemory++] = 0xb0|rn+1;
            memory[nmemory++] = l->a->value+2;
            /* adw rn,r1 */
            memory[nmemory++] = 0x0f;
            memory[nmemory++] = rn<<4|rn+1;
            break;
        case INDEX:
            compileList(l->a);
            nmemory -= 2;
            break;
        default:
            perr(); printf("expected lvalue\n"); exit(1);
        }
        break;
    case DEREF:
        compileList(l->a);
        /* ldw rn,rn */
        memory[nmemory++] = 0x05;
        memory[nmemory++] = rn<<4|rn;
        break;
    case IMM:
        compileLiteral(rn, l->value);
        break;
    }
}

void resolveBreaks() {
    while(bsp && brks[--bsp]) sh(brks[bsp], nmemory-brks[bsp]-2);
}

void resolveContinues() {
    while(bcp && bcns[--bcp]) sh(bcns[bcp], nmemory-bcns[bcp]-2);
    csp--;
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
    l = listExpr();
    l->parent = 0;
    /*printList(l);
    printf("\n");*/
    /*for(i = 0; i < nlists; i++) {
        printList(&lists[i]);
        printf("\n");
    }*/
    rn = 0;
    compileList(l);
}

void beforeEof(const char *s) {
    if(feof(fp)) { perr(); printf("expected %s before EOF\n", s); exit(1); }
}

void skipExpr() {
    int ng, ns;
    ns = nstringBuf;
    ng = nglobals;
    lookAhead();
    if(strindex(term, ahead) != -1) return;
    nlists = 0;
    listExpr();
    nglobals = ng;
    nstringBuf = ns;
}

void skipStatement() {
    lookAhead();
    if(!strcmp(ahead, "for")) {
        parseNext();
        expect("("); skipExpr(); expect(";"); skipExpr();
        expect(";"); skipExpr(); expect(")");
        skipStatement();
    } else if(!strcmp(ahead, "while")) {
        parseNext(); expect("("); skipExpr(); expect(")"); skipStatement();
    } else if(!strcmp(ahead, "if")) {
        parseNext(); expect("("); skipExpr(); expect(")"); skipStatement();
        lookAhead();
        if(!strcmp(ahead, "else")) { parseNext(); skipStatement(); }
    } else if(!strcmp(ahead, "do")) {
        parseNext(); skipStatement();
        expect("while"); expect("("); skipExpr(); expect(")"); expect(";");
    } else if(!strcmp(ahead, "break")) {
        parseNext(); expect(";");
    } else if(!strcmp(ahead, "continue")) {
        parseNext(); expect(";");
    } else if(!strcmp(ahead, "return")) {
        parseNext(); skipExpr(); expect(";");
    } else if(!strcmp(ahead, "goto")) {
        parseNext(); parseNext(); expect(";");
    } else if(!strcmp(ahead, "case")) {
        parseNext(); evalExpr(); expect(":");
    } else if(!strcmp(ahead, "default")) {
        parseNext(); expect(":");
    } else if(!strcmp(ahead, "switch")) {
        parseNext(); expect("("); skipExpr(); expect(")"); skipStatement();
    } else if(!strcmp(ahead, "{")) {
        parseNext(); for(;;) { lookAhead(); if(!strcmp(ahead, "}")) break;
        skipStatement(); } parseNext();
    } else { skipExpr(); expect(";"); }
}

void compileStatement();

void compileSwitch() {
    int b, c, csz, m, d;
    struct pos pos;
    parseNext();
    expect("(");
    compileExpr();
    expect(")");
    expect("{");
    savePos(&pos);

    csz = 0;
    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "}")) break;
        if(!strcmp(ahead, "case")) {
            parseNext();
            b = nmemory;
            compileLiteral(0, evalExpr());
            expect(":");
            csz += 5+nmemory-b;
            nmemory = b;
        } else skipStatement();
    }
    parseNext();

    restorePos(&pos);
    b = nmemory;
    nmemory += csz+3;
    d = 0;
    brks[bsp++] = 0;
    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "}")) break;
        if(!strcmp(ahead, "case")) {
            parseNext();
            m = nmemory;
            nmemory = b;
            compileLiteral(1, evalExpr());
            expect(":");
            b = nmemory;
            nmemory = m;
            /* sub r1,r0 */
            memory[b++] = 0x0e;
            memory[b++] = 0x10;
            /* beq r1,nmemory */
            memory[b++] = 0xe1;
            sh(b, nmemory-b-2);
            b += 2;
        } else if(!strcmp(ahead, "default")) {
            parseNext();
            expect(":");
            d = nmemory;
        } else compileStatement();
    }
    parseNext();
    /* bra nmemory */
    memory[b++] = 0x01;
    sh(b, (d ? d : nmemory)-b-2);
    resolveBreaks();
}

void compileStatement() {
    int o, r;
    struct pos pos;
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
        if(!csp) { perr(); printf("unexpected continue\n"); exit(1); }
        /* bra loop */
        memory[nmemory++] = 0x01;
        if(cons[csp-1])
            sh(nmemory, cons[csp-1]-nmemory-2);
        else bcns[bcp++] = nmemory;
        nmemory += 2;
    } else if(!strcmp(ahead, "break")) {
        parseNext();
        if(!bsp) { perr(); printf("unexpected break\n"); exit(1); }
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
        /* bne r0,addr */
        memory[nmemory++] = 0xf0;
        sh(nmemory, cons[--csp]-nmemory-2);
        nmemory += 2;
        resolveBreaks();
    } else if(!strcmp(ahead, "for")) {
        parseNext();
        expect("(");
        compileExpr();
        expect(";");
        cons[csp++] = 0;
        brks[bsp++] = 0;
        bcns[bcp++] = 0;
        o = nmemory;
        compileExpr();
        expect(";");
        if(r = (o != nmemory)) {
            /* beq r0,addr */
            memory[nmemory++] = 0xe0;
            r = nmemory;
            nmemory += 2;
        }
        savePos(&pos);
        skipExpr();
        expect(")");
        compileStatement();
        swapPos(&pos);
        resolveContinues();
        compileExpr();
        restorePos(&pos);
        /* bra addr */
        memory[nmemory++] = 0x01;
        sh(nmemory, o-nmemory-2);
        if(r) sh(r, nmemory-r);
        nmemory += 2;
        resolveBreaks();
    } else if(!strcmp(ahead, "switch")) {
        compileSwitch();
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
            perr(); printf("%s already defined\n", name); exit(1);
        }
        globals[i].type = type;
        globals[i].addr = addr;
        return &globals[i];
    } else {
        globals[nglobals].name = nameP;
        globals[nglobals].addr = addr;
        globals[nglobals].type = type;
        /*globals[nglobals] = (struct global) {
            nameP, addr, type,
        };*/
        addName(name);
        return &globals[nglobals++];
    }
}

void compileFunction(char *name) {
    int i;
    struct global *current;
    char *prevf;

    current = addGlobal(name, nmemory, FUN);

    prevf = currentFunction;
    currentFunction = name;
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
    /*for(i = 0; i < nargs; i++)
        printf("%s ", args[i]);
    printf("\n");*/

    expect("{");

    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "auto")) {
            parseNext();
            parseNext();
            if(strcmp(next, ";")) {
                for(;;) {
                    validName(next);
                    localsz[nlocals] = 0;
                    locals[nlocals++] = tnameP;
                    addTname(next);
                    lookAhead();
                    if(!strcmp(ahead, "[")) {
                        parseNext();
                        localsz[nlocals-1] = evalExpr();
                        if(localsz[nlocals-1] <= 0) {
                            perr(); printf("invalid size\n"); exit(1);
                        }
                        expect("]");
                        lookAhead();
                    }
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
                    } else if(!strcmp(ahead, "[")) {
                        parseNext(); expect("]"); lookAhead();
                    } else {
                    }
                    if(!strcmp(ahead, ";")) break;
                    expect(",");
                    parseNext();
                }
                parseNext();
            }
        } else break;
    }

    if(nlocals || nargs) {
        /* psh bp */
        memory[nmemory++] = 0x7e;
        /* mov bp,sp */
        memory[nmemory++] = 0x04;
        memory[nmemory++] = 0xef;
    }
    if(nlocals) {
        /* lbi r0,-nlocals */
        memory[nmemory++] = 0xb0;
        memory[nmemory++] = -nlocals;
        /* adw sp,r0 */
        memory[nmemory++] = 0x0f;
        memory[nmemory++] = 0xf0;

        for(i = 0; i < nlocals; i++) {
            if(!localsz[i]) continue;
            compileLiteral(0, -localsz[i]);
            /* adw sp,r0 */
            memory[nmemory++] = 0x0f;
            memory[nmemory++] = 0xf0;
            /* str sp,local */
            memory[nmemory++] = 0xdf;
            memory[nmemory++] = -i-1;
        }
    }

    for(;;) {
        lookAhead();
        if(!strcmp(ahead, "}")) break;
        if(feof(fp)) { perr(); printf("expected }\n"); exit(1); }
        compileStatement();
    }
    parseNext();

    resolveRets();
    if(nlocals || nargs) {
        /* mov sp,bp */
        memory[nmemory++] = 0x04;
        memory[nmemory++] = 0xfe;
        /* pop bp */
        memory[nmemory++] = 0x8e;
    }
    /* ret argc */
    memory[nmemory++] = 0x03;
    memory[nmemory++] = nargs;

    currentFunction = prevf;
}

void compileData(char *name, char type) {
    struct global *g;
    g = addGlobal(name, ndata, type);
    for(;;) {
        /*printf("%s\n", ahead);*/
        lookAhead();
        if(!strcmp(ahead, "\"")) {
            *(int*)&data[ndata] = nglobals;
            parseNext();
            deferString();
            strings[nstrings++] = ndata;
        } else *(int*)&data[ndata] = evalExpr();
        ndata += 4;
        lookAhead();
        if(!strcmp(ahead, ";")) break;
        expect(",");
        g->type = DATA;
    }
    parseNext();
}

void compileFile(const char *filename);

void compileOuter() {
    char name[BUFSZ];
    int n;

    parseNext();
    if(*next == 0) return;
    /*printf("%d:%s\n", lineNo, next);*/

    strcpy(name, next);
    lookAhead();
    if(!strcmp(ahead, "[")) {
        parseNext();
        lookAhead();
        if(!strcmp(ahead, "]")) {
            parseNext();
            lookAhead();
            if(!strcmp(ahead, ";")) {
                perr(); printf("unexpected ;\n"); exit(1);
            }
            compileData(name, DATA);
        } else {
            addGlobal(name, nextBss, BSS);
            nextBss += evalExpr()*4;
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
    } else if(!strcmp(ahead, "=")) {
        parseNext();
        constants[nconstants].name = nameP;
        addName(name);
        constants[nconstants++].value = evalExpr();
        expect(";");
    } else {
        compileData(name, DATAV);
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
    if(!fp) { perr(); printf("failed to open file\n"); exit(1); }

    while(!feof(fp) || *ahead) {
        compileOuter();
    }

    fclose(fp);

    fp = pfp;
    filename = pname;
    lineNo = pline;
}

int resolveAddr(struct global *g) {
    switch(g->type) {
    case BSS: case BSSV:
        return g->addr+org+nmemory+ndata+nstringBuf;
    case STRING:
        return g->addr+org+nmemory+ndata;
    case DATA: case DATAV:
        return g->addr+org+nmemory;
    case FUN:
        return g->addr+org;
    case ABSOLUTE:
        return g->addr;
    default:
        perr(); printf("unresolved reference to %s\n", g->name); exit(1);
        break;
    }
}

void resolveExrefs() {
    int i, a;
    struct global *g;
    for(i = 0; i < nstrings; i++) {
        g = &globals[*(int*)&data[a=strings[i]]];
        *(int*)&data[a] = resolveAddr(g);
    }
    for(i = 0; i < nexrefs; i++) {
        g = &globals[*(int*)&memory[a=exrefs[i]]];
        *(int*)&memory[a] = resolveAddr(g);
    }
}

void saveFile(char *filename) {
    FILE *fp;
    fp = fopen(filename, "wb");
    if(!fp) { printf("failed to open output file %s\n", filename); exit(1); }
    fwrite(memory, 1, nmemory, fp);
    fwrite(data, 1, ndata, fp);
    fwrite(stringBuf, 1, nstringBuf, fp);
    fclose(fp);
}

void listGlobals() {
    int i;
    struct global *g;
    for(i = 0; i < nglobals; i++)
        if(globals[i].type != STRING)
            printf("0x%.8x %s\n", resolveAddr(&globals[i]), globals[i].name);
}

void saveGlobals(char *filename) {
    int i;
    struct global *g;
    FILE *fp;
    if(!(fp = fopen(filename, "w"))) {
        printf("failed to open %s\n", filename);
        exit(1);
    }
    for(i = 0; i < nglobals; i++)
        if(globals[i].type != STRING && strcmp(globals[i].name, "main")) {
            fprintf(fp, "0x%.8x %s\n", resolveAddr(&globals[i]),
                globals[i].name);
        }
    fclose(fp);
}

void loadGlobals(char *filename) {
    char buf[BUFSZ];
    FILE *fp;
    int a;
    struct global *g;
    if(!(fp = fopen(filename, "r"))) {
        printf("failed to open %s\n", filename);
        exit(1);
    }
    for(;;) {
        fscanf(fp, "%s", buf);
        if(feof(fp)) break;
        a = value(buf);
        fscanf(fp, "%s", buf);
        g = &globals[findGlobal(buf)];
        if(g && g->type == EXTRN) {
            g->type = ABSOLUTE;
            g->addr = a;
        }
        fgetc(fp);
    }
}

int main(int argc, char **args) {
    int i;
    assert(sizeof(char) == 1);
    assert(sizeof(int) == 4);
    char *outFile, *globalFile, *globalOutFile;
    outFile = "a.out";
    globalOutFile = 0;
    globalFile = 0;
    if(argc <= 1) {
        printf("usage: %s <file1.b file2.b ...>\n", args[0]);
        printf("  -o <file> - specify output file\n");
        printf("  -r <addr> - specify ORG\n");
        printf("  -g <file> - output global symbols\n");
        printf("  -G <file> - include global symbols\n");
        return 0;
    }
    *ahead = 0;
    /* lwi pc,org+6 */
    memory[nmemory++] = 0x9d;
    *(int*)&memory[nmemory] = org+6;
    nmemory += 4;
    for(i = 1; i < argc; i++) {
        if(!strcmp(args[i], "-o")) {
            if(++i < argc) outFile = args[i];
        } else if(!strcmp(args[i], "-r")) {
            if(++i < argc) org = value(args[i]);
        } else if(!strcmp(args[i], "-g")) {
            if(++i < argc) globalOutFile = args[i];
        } else if(!strcmp(args[i], "-G")) {
            if(++i < argc) globalFile = args[i];
        } else compileFile(args[i]);
    }
    if(globalFile) loadGlobals(globalFile);
    resolveExrefs();
    i = findGlobal("main");
    if(i == -1 || globals[i].type == EXTRN) printf("no main\n");
    else *(int*)&memory[1] = globals[i].addr+org;
    if(globalOutFile) saveGlobals(globalOutFile);
    saveFile(outFile);
    listGlobals();
    printf("compiled %d bytes\n", nmemory+ndata+nstringBuf);
    return 0;
}
