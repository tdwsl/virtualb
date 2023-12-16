
main() {
    extrn args, argc, putstr, putchar, findFree;
    extrn findFile, addFile, sys, dir, dp;
    auto b1, b2, b, block[128];
    if(*argc != 3) {
        putstr("usage: copy <src> <dst>*n");
        return;
    }
    b1 = findFile(dir[*dp], args[1]);
    if(!b1) {
        putstr("failed to open ");
        putstr(args[1]);
        putchar('*n');
        return;
    }
    b2 = findFile(dir[*dp], args[2]);
    if(b2) {
        b2 = 0;
        sys(7, sys(6)-16);
        sys(5, &b2, 4);
        sys(5, &b2, 4);
        sys(5, &b2, 4);
        sys(5, &b2, 4);
    }
    b2 = addFile(dir[*dp], args[2]);
    do {
        sys(7, b1<<9);
        sys(4, &b1, 2);
        sys(4, block, 510);
        sys(7, b2<<9);
        if(b1) b2 = findFree();
        else b2 = 0;
        sys(5, &b2, 2);
        sys(5, block, 510);
    } while(b1);
}
