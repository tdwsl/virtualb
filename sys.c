#include <stdio.h>
#include <stdlib.h>

extern int regs[];

void sys() {
    switch(regs[0]) {
    case 0: exit(0);
    case 1: fputc(regs[1], stdout); break;
    default:
        printf("unknown syscall %d at %.8x\n", regs[0], regs[13]); exit(0);
    }
}

