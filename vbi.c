/* virtualb interpreter */

#include <stdio.h>
#include <string.h>

#define ORG 0x20000
#define MEMORYSZ 0x200000

enum {
    SYS=0x00, BRA, JSR, RET,
    MOV, LDW, LDH, LDB, LUH, LUB, STW, STH, STB, ADD, SUB, ADW,
    AND, LOR, XOR, SHR, SHL, MUL, DIV, MOD, EQU, NEQ, LTE,
    GTN, LTN, GTE, PHM,
    INC=0x20, DEC=0x30, INV=0x40, NOT=0x50, NEG=0x60, PSH=0x70, POP=0x80,
    LWI=0x90, LHI=0xa0, LBI=0xb0, LDR=0xc0, STR=0xd0, BEQ=0xe0, BNE=0xf0,
};

unsigned char memory[MEMORYSZ];

int regs[16];

extern unsigned char disk[];
extern char *diskname;
void loadDisk();

int lh(int a) {
    return *(short*)&memory[a];
}

void sh(int a, int h) {
    *(short*)&memory[a] = h;
}

void sys();

void run() {
    int n, a, b;
    for(;;) {
        //printf("$%x ", regs[13]);
        /*printf("0x%.8x\n   ", regs[13]);
        for(a = 0; a < 16; a++) printf("0x%x ", regs[a]); printf("\n");
        for(a = regs[15]-16; a < regs[15]+16; a += 4)
            printf("%.8x ", *(int*)&memory[a]);
        printf("\n");*/

        n = memory[regs[13]++];
        a = n&0xf;
        switch(n&0xf0) {
        case 0x00:
        case 0x10:
            if(!(n&0xfc)) {
                switch(n) {
                case SYS: sys(); continue;
                case BRA: regs[13] += lh(regs[13])+2; continue;
                case JSR: regs[15] -= 4;
                    *(int*)&memory[regs[15]] = regs[13]+4;
                    regs[13] = *(int*)&memory[regs[13]]; continue;
                case RET: n = memory[regs[13]];
                    regs[13] = *(int*)&memory[regs[15]];
                    regs[15] += 4 + n*4; continue;
                }
            }
            a = memory[regs[13]++];
            b = a&0xf;
            a >>= 4;
            switch(n) {
            case MOV: regs[a] = regs[b]; break;
            case LDW: regs[a] = *(int*)&memory[regs[b]]; break;
            case LDH: regs[a] = lh(regs[b]); break;
            case LDB: regs[a] = (char)memory[regs[b]]; break;
            case LUH: regs[a] = lh(regs[b])&0xffff; break;
            case LUB: regs[a] = memory[regs[b]]; break;
            case STW: *(int*)&memory[regs[b]] = regs[a]; break;
            case STH: sh(regs[b], regs[a]); break;
            case STB: memory[regs[b]] = regs[a]; break;
            case ADD: regs[a] += regs[b]; break;
            case SUB: regs[a] -= regs[b]; break;
            case ADW: regs[a] += regs[b]*4; break;
            case AND: regs[a] &= regs[b]; break;
            case LOR: regs[a] |= regs[b]; break;
            case XOR: regs[a] ^= regs[b]; break;
            case SHR: regs[a] >>= regs[b]; break;
            case SHL: regs[a] <<= regs[b]; break;
            case MUL: regs[a] *= regs[b]; break;
            case DIV: regs[a] /= regs[b]; break;
            case MOD: regs[a] %= regs[b]; break;
            case EQU: regs[a] = regs[a] == regs[b]; break;
            case NEQ: regs[a] = regs[a] != regs[b]; break;
            case LTE: regs[a] = regs[a] <= regs[b]; break;
            case GTN: regs[a] = regs[a] > regs[b]; break;
            case LTN: regs[a] = regs[a] < regs[b]; break;
            case GTE: regs[a] = regs[a] >= regs[b]; break;
            case PHM: regs[15] -= 4; *(int*)&memory[regs[15]] = regs[a];
                regs[a] = regs[b]; break;
            }
            break;
        case INC: regs[a]++; break;
        case DEC: regs[a]--; break;
        case INV: regs[a] = ~regs[a]; break;
        case NOT: regs[a] = !regs[a]; break;
        case NEG: regs[a] = -regs[a]; break;
        case PSH: regs[15] -= 4; *(int*)&memory[regs[15]] = regs[a]; break;
        case POP: regs[a] = *(int*)&memory[regs[15]]; regs[15] += 4; break;
        case LWI: regs[13] += 4; regs[a] = *(int*)&memory[regs[13]-4]; break;
        case LHI: regs[13] += 2; regs[a] = lh(regs[13]-2); break;
        case LBI: regs[13]++; regs[a] = (char)memory[regs[13]-1]; break;
        case LDR: b = (char)memory[regs[13]++];
            regs[a] = *(int*)&memory[regs[14]+b*4]; break;
        case STR: b = (char)memory[regs[13]++];
            *(int*)&memory[regs[14]+b*4] = regs[a]; break;
        case BEQ: regs[13]+=2; if(!regs[a]) regs[13] += lh(regs[13]-2); break;
        case BNE: regs[13]+=2; if(regs[a]) regs[13] += lh(regs[13]-2); break;
        }
    }
}

int main(int argc, char **args) {
    if(argc < 1 || argc > 2) {
        printf("usage: %s <file>\n", args[0]);
        return 0;
    }
    if(argc == 2) diskname = args[1];
    loadDisk();
    /* jsr ORG */
    memory[ORG-9] = 0x02;
    *(int*)&memory[ORG-8] = ORG;
    /* lbi r0,0 */
    memory[ORG-4] = 0xb0;
    memory[ORG-3] = 0x00;
    /* psh r0 */
    memory[ORG-2] = 0x70;
    /* sys */
    memory[ORG-1] = 0x00;
    regs[15] = ORG;
    regs[14] = ORG;
    regs[13] = ORG;
    memcpy(memory+ORG, disk, 1024);
    run();
    return 0;
}
