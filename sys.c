#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISKSZ 1024*1024

extern int regs[];
extern unsigned char memory[];

char *diskname = "default.dsk";
unsigned char disk[DISKSZ];
unsigned int curs = 0;

void loadDisk() {
    FILE *fp;
    if(!(fp = fopen(diskname, "rb"))) {
        printf("failed to load disk %s\n", diskname);
        exit(0);
    }
    memset(disk, 0, DISKSZ);
    fread(disk, 1, DISKSZ, fp);
    fclose(fp);
}

void saveDisk() {
    FILE *fp;
    int i;
    if(fp = fopen(diskname, "wb")) {
        for(i = DISKSZ-1; !disk[i]; i--);
        i++;
        fwrite(disk, 1, i, fp);
        fclose(fp);
    } else printf("failed to save disk %s\n", diskname);
}

int pop() {
    int i;
    i = *(int*)&memory[regs[15]];
    regs[15] += 4;
    return i;
}

void dump(const char *filename) {
    int a, b;
    FILE *fp;
    if(!(fp = fopen(filename, "wb"))) {
        printf("failed to dump %s\n", filename);
        exit(0);
    }
    a = pop();
    b = pop();
    fwrite(&memory[a], 1, b, fp);
    fclose(fp);
}

void sys() {
    int a, b, c;
    switch(c = pop()) {
    case 0: exit(0);
    case 1: fputc(pop(), stdout); break;
    case 2: regs[0] = fgetc(stdin); break;
    case 3: dump("dump.bin"); break;
    case 4:
        a = pop();
        b = pop();
        regs[0] = 0;
        while(curs < DISKSZ && b) {
            memory[a++] = disk[curs++];
            b--; regs[0]++;
        }
        break;
    case 5:
        regs[0] = 0;
        a = pop();
        b = pop();
        while(curs < DISKSZ && b) {
            disk[curs++] = memory[a++];
            b--; regs[0]++;
        }
        saveDisk(diskname);
        break;
    case 6: regs[0] = curs; break;
    case 7: curs = pop(); break;
    default:
        printf("unknown syscall %d at %.8x\n", c, regs[13]); exit(0);
    }
}

