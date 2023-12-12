/* generate a disk image from files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char buf[512];
char *rootp = buf+16;
FILE *out;

FILE *open(char *filename, const char *mode) {
    FILE *fp;
    if(!(fp = fopen(filename, mode))) {
        printf("failed to open %s\n", filename);
        exit(0);
    }
    return fp;
}

void saveBoot(char *filename) {
    FILE *fp;
    fp = open(filename, "rb");
    fread(buf, 1, 512, fp);
    fclose(fp);
    fwrite(buf, 1, 512, out);
}

int filesz(char *filename) {
    int i;
    FILE *fp;
    fp = open(filename, "rb");
    fseek(fp, 0, SEEK_END);
    i = ftell(fp);
    fclose(fp);
    return (i/510)+!!(i%510);
}

void addDir(char *filename, int block) {
    strncpy(rootp, filename, 14);
    rootp += 14;
    *(int*)rootp = block;
    rootp += 2;
}

void saveFile(char *filename, int block, int size) {
    FILE *fp;
    fp = open(filename, "rb");
    do {
        if(size>1) *(int*)&buf = ++block;
        else memset(buf, 0, 512);
        fread(buf+2, 1, 510, fp);
        fwrite(buf, 1, 512, out);
    } while(--size);
    fclose(fp);
}

int main(int argc, char **args) {
    int i;
    int blocks[50], sizes[50], next;
    if(argc <= 3) {
        printf("usage: %s <out.dsk> <boot,file1,file2,...>\n", args[0]);
        return 0;
    }
    next = 2;
    out = open(args[1], "wb");
    saveBoot(args[2]);
    for(i = 3; i < argc; i++) {
        blocks[i-3] = next;
        next += (sizes[i-3] = filesz(args[i]));
    }
    memset(buf, 0, 512);
    for(i = 3; i < argc; i++)
        addDir(args[i], blocks[i-3]);
    fwrite(buf, 1, 512, out);
    for(i = 3; i < argc; i++)
        saveFile(args[i], blocks[i-3], sizes[i-3]);
    fclose(out);
    printf("wrote %d blocks\n", next);
    return 0;
}
