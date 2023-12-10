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

void sys() {
    switch(regs[0]) {
    case 0: exit(0);
    case 1: fputc(regs[1], stdout); break;
    case 2: regs[1] = fgetc(stdin); break;
    case 4:
        regs[0] = 0;
        while(curs < DISKSZ && regs[2]) {
            memory[regs[1]++] = disk[curs++];
            regs[2]--; regs[0]++;
        }
        regs[2] = regs[0];
        regs[0] = 4;
        break;
    case 5:
        regs[0] = 0;
        while(curs < DISKSZ && regs[2]) {
            disk[curs++] = memory[regs[1]++];
            regs[2]--; regs[0]++;
        }
        regs[2] = regs[0];
        regs[0] = 5;
        saveDisk(diskname);
        break;
    case 6: regs[1] = curs; break;
    case 7: curs = regs[1]; break;
    default:
        printf("unknown syscall %d at %.8x\n", regs[0], regs[13]); exit(0);
    }
}

