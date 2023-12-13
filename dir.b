
get "char.b";

cpyd(buf, s, isdir) {
    extrn sys, putc;
    auto i, c;
    for(i = 0; (i < 14) & !!(c = (*s++&0xff)); i++)
        *buf++ = c;
    if(isdir) *buf++ = '/';
    else *buf++ = ' ';
    do *buf++ = ' '; while(i++<14);
    *buf = 0;
}

putd(s) {
    auto i, c;
    for(i = 0; (i < 16) & !!(c = (*s++&0xff)); i++)
        putc(c);
}

printName(dbuf, dir) {
    extrn puts;
    auto p, buf[4];
    p = sys(6);
    sys(7, dir<<9);
    sys(4, buf, 14);
    cpyd(dbuf, buf, 1);
    sys(7, p);
}

nameBefore(s1, s2) {
    auto i;
    for(i = 0; i < 16; i++) {
        if(char(s1,i) > char(s2,i)) return 0;
        if(char(s1,i) < char(s2,i)) return 1;
    }
    return 1;
}

namecpy(s1, s2) {
    auto i;
    for(i = 0; i < 4; i++) s1[i] = s2[i];
}

printDir(dir) {
    auto i, buf[4], b, dirs[400], ndirs;
    b = 0;
    ndirs = 0;
    do {
        sys(7, (dir<<9)+14);
        sys(4, &dir, 2);
        for(i = 0; i < 512/16-1; i++) {
            sys(4, buf, 14);
            sys(4, &b, 2);
            if(*buf) cpyd(dirs+(ndirs++<<4), buf, 0);
            else if(b) printName(dirs+(ndirs++<<4), b);
            else continue;
        }
    } while(dir);
    do {
        *buf = 0;
        for(i = 1; i < ndirs; i++)
            if(nameBefore(dirs+(i<<4), dirs+(i-1<<4))) {
                namecpy(buf, dirs+(i<<4));
                namecpy(dirs+(i<<4), dirs+(i-1<<4));
                namecpy(dirs+(i-1<<4), buf);
            }
    } while(*buf);
    for(i = 0; i < ndirs; i++) putd(dirs+(i<<4));
    if(ndirs%5) putc('*n');
}

main() {
    extrn dir, dp;
    printDir(dir[*dp]);
}
