/* virtualb command line */

get "sys.b";

dir[20];
dp 0;

args[40];
argc;

fstreq(a, b) {
    auto i;
    for(i = 0; i < 14; i++) {
        if((*a&0xff) != (*b++&0xff)) return 0;
        if(!(*a++&0xff)) break;
    }
    return 1;
}

findFile(dir, name) {
    auto i, buf[4], b;
    b = 0;
    do {
        sys(7, (dir<<9)+14);
        dir = 0;
        sys(4, &dir, 2);
        for(i = 0; i < 512/16-1; i++) {
            sys(4, buf, 14);
            sys(4, &b, 2);
            if(fstreq(buf, name)) return b;
        }
    } while(dir);
    return 0;
}

loadFile(b, dst) {
    do {
        sys(7, b<<9);
        sys(4, &b, 2);
        sys(4, dst, 510);
        dst =+ 510;
    } while(b);
}

readLine(buf, sz) {
    auto i;
    sz =- 3;
    for(i = 0; i < sz; i++) {
        *buf = sys(2);
        if(*buf == 10) break;
        buf++;
    }
    *buf = 0;
    return i;
}

next(buf) {
    while((*buf&0xff) > 32) buf++;
    *buf = *buf&0xffffff00;
    buf++;
    while(!!(*buf&0xff) & ((*buf&0xff) <= 32)) buf++;
    if(*buf&0xff) return buf;
    return 0;
}

puts(s) {
    auto c;
    while(c = (*s++&0xff))
        sys(1, c);
}

putc(c) {
    sys(1, c);
}

main() {
    auto buf[240], p, i, b;
    puts("virtualb command line*n");
    dir[dp] = 1;
    for(;;) {
        argc = 0;
        p = buf;
        puts(">");
        while(!readLine(buf, 240));
        do args[argc++] = p; while(p = next(p));
        if(b = findFile(dir[dp], args[0])) {
            loadFile(b, 0x20000);
            0x20000();
        } else { puts(args[0]); puts("?*n"); }
    }
}

