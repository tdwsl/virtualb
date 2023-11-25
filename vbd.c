/* virtualb disassembler */

#include <stdio.h>
#include <stdlib.h>

#define ORG 0x10000;

const char *istrs1[] = {
    "sys", "bra", "jsr", "ret", "mov", "ldw", "ldh", "ldb",
    "luh", "lub", "stw", "sth", "stb", "add", "sub", "adw",
    "and", "lor", "xor", "shr", "shl", "mul", "div", "mod",
    "equ", "neq", "lte", "gtn", "ltn", "gte", "phm", "nop",
};

const char *istrs2[] = {
    "---", "---", "inc", "dec", "inv", "not", "neg", "psh",
    "pop", "lwi", "lhi", "lbi", "ldr", "str", "beq", "bne",
};

int lw(FILE *fp) {
    int i;
    fread(&i, 4, 1, fp);
    return i;
}

int lh(FILE *fp) {
    int i;
    i = 0;
    fread(&i, 2, 1, fp);
    if(i & 0x8000) i |= 0xffff0000;
    return i;
}

void disasm(const char *filename) {
    FILE *fp;
    unsigned char i;
    int pc;
    if(!(fp = fopen(filename, "r"))) {
        printf("failed to open %s\n", filename);
        exit(0);
    }
    pc = ORG;
    while(!feof(fp)) {
        printf("0x%.8x ", pc);
        pc++;
        switch((i = fgetc(fp))&0xf0) {
        case 0x00:
        case 0x10:
            printf("%s ", istrs1[i]);
            if(i == 0x1f);
            else if(i&0xfc) {
                i = fgetc(fp);
                pc++;
                printf("r%d,r%d", (i>>4)&0xf, i&0xf);
            } else {
                switch(i) {
                case 0: break;
                case 1: printf("0x%.8x", pc+2+lh(fp)); pc += 2; break;
                case 2: printf("0x%.8x", lw(fp)); pc += 4; break;
                case 3: printf("%d", (char)fgetc(fp)); pc++; break;
                }
            }
            break;
        default:
            printf("%s r%d", istrs2[i>>4], i&0xf);
            switch(i&0xf0) {
            case 0x90: printf(",0x%.8x", lw(fp)); pc += 4; break;
            case 0xa0: printf(",0x%.4x", lh(fp)); pc += 2; break;
            case 0xb0: printf(",0x%.2x", fgetc(fp)); pc++; break;
            case 0xc0: case 0xd0: printf(",%d", (char)fgetc(fp)); pc++; break;
            case 0xe0: case 0xf0: printf(",0x%.8x", pc+2+lh(fp)); pc+=2; break;
            default: break;
            }
            break;
        }
        printf("\n");
    }
    fclose(fp);
}

int main(int argc, char **args) {
    int i;
    if(argc <= 1) {
        printf("usage: %s <file1 file2 ...>\n", args[0]);
        return 0;
    }
    for(i = 1; i < argc; i++)
        disasm(args[i]);
    return 0;
}
