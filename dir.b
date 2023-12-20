
cpyd(buf, s, isdir) {
    extrn sys, putchar, char, lchar;
    auto i, c;
    for(i = 0; (i < 13) & !!char(s, i); i++)
        lchar(buf, i, char(s, i));
    if(isdir) lchar(buf, i++, '/');
    else lchar(buf, i++, ' ');
    for(; i < 16; i++) lchar(buf, i, ' ');
    lchar(buf, i, 0);
}

putd(s) {
    auto i, c;
    for(i = 0; (i < 16) & !!(c = char(s, i)); i++)
        putchar(c);
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
        sys(7, dir<<9);
        sys(4, &dir, 2);
        for(i = 0; i < 510/15; i++) {
            sys(4, buf, 13);
            sys(4, &b, 2);
            if(b&0x8000) cpyd(dirs+(ndirs++<<4), buf, 1);
            else if(b) cpyd(dirs+(ndirs++<<4), buf, 0);
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
    if(ndirs%5) putchar('*n');
}

main() {
    extrn dir, args, argc, putstr, putchar, findDir;
    auto i, b;
    if(argc > 1) {
        for(i = 1; i < argc; i++) {
            b = findDir(dir, args[i]);
            if(b) {
                printDir(b);
            } else {
                putstr("failed to open directory ");
                putstr(args[i]);
                putchar('*n');
            }
        }
    } else printDir(dir);
}
