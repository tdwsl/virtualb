
main() {
    extrn argc, args[], dir, putstr, putchar, sys;
    extrn findEntry, addFile, namecpy, eraseBlock;
    auto b, buf[4], i, z;
    if(argc != 2) {
        putstr("usage: mkdir <name>*n");
        return;
    }
    if(findEntry(dir, args[1])) {
        putstr("file already exists*n");
        return;
    }
    b = addFile(dir, args[1], 1);
    if(!b) {
        putstr("failed to create directory*n");
        return;
    }
    eraseBlock(b);
    sys(7, (b<<9)+2);
    sys(5, "..", 2);
    sys(7, sys(6)+11);
    b = dir|0x8000;
    sys(5, &b, 2);
}

