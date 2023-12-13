
putd(s) {
    extrn sys, putc;
    auto i, c;
    for(i = 0; (i < 14) & !!(c = (*s++&0xff)); i++)
        putc(c);
    do putc(' '); while(i++<14);
}

printName(dir) {
    extrn puts;
    auto p, buf[4];
    p = sys(6);
    sys(7, dir<<9);
    sys(4, buf, 14);
    putd(buf);
    putc('/');
    sys(7, p);
}

printDir(dir) {
    auto i, buf[4], b, x, d;
    x = 0;
    b = 0;
    do {
        sys(7, (dir<<9)+14);
        sys(4, &dir, 2);
        for(i = 0; i < 512/16-1; i++) {
            sys(4, buf, 14);
            sys(4, &b, 2);
            if(*buf) { putd(buf); putc(' '); }
            else if(b) printName(b);
            else continue;
            x = (x+1)&3;
            if(!x) putc('*n');
        }
    } while(dir);
    if(x) putc('*n');
}

main() {
    extrn dir, dp;
    printDir(dir[*dp]);
}
