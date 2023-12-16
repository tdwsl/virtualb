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

blockFree(dir, b) {
    auto c, i, buf[4], p;
    c = 0;
    if(dir == b) return 0;
    do {
        sys(7, (dir<<9)+14);
        sys(4, &dir, 2);
        if(dir == b) return 0;
        for(i = 0; i < 512/16-1; i++) {
            sys(4, buf, 14);
            sys(4, &c, 2);
            if(b == c) return 0;
            if(c) {
                p = sys(6);
                if(!*buf) {
                    if(!blockFree(c, b)) return 0;
                } else {
                    do {
                        sys(7, c<<9);
                        sys(4, &c, 2);
                        if(c == b) return 0;
                    } while(c);
                }
                sys(7, p);
            }
        }
    } while(dir);
    return 1;
}

findFree() {
    auto i;
    for(i = 2; !blockFree(1, i); i++);
    return i;
}

namecpy(s1, s2) {
    extrn lchar, char;
    auto i;
    for(i = 0; i < 14; i++) {
        lchar(s1, i, char(s2, i));
        if(!char(s2, i)) break;
    }
}

addFile(dir, name) {
    auto i, b, buf[4];
    b = 0;
    for(i = 0; i < 4; i++) buf[i] = 0;
    for(;;) {
        sys(7, (dir<<9)+14);
        sys(4, &dir, 2);
        for(i = 0; i < 512/16-1; i++) {
            sys(7, sys(6)+14);
            sys(4, &b, 2);
            if(!b) {
                i = sys(6)-16;
                b = findFree();
                namecpy(buf, name);
                sys(7, i);
                sys(5, buf, 14);
                sys(5, &b, 2);
                return b;
            }
        }
        if(!dir) {
            b = sys(6)-512+14;
            dir = findFree();
            sys(7, b);
            sys(5, &dir, 2);
            b = 0;
            sys(7, dir<<9);
            for(i = 0; i < 128; i++)
                sys(5, &b, 4);
        }
    }
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

get "put.b";

cli() {
    extrn putstr;
    auto buf[240], p, i, b;
    putstr("virtualb command line*n");
    dir[dp] = 1;
    for(;;) {
        argc = 0;
        p = buf;
        putstr(">");
        while(!readLine(buf, 240));
        do args[argc++] = p; while(p = next(p));
        if(b = findFile(dir[dp], args[0])) {
            loadFile(b, 0x20000);
            0x20000();
        } else { putstr(args[0]); putstr("?*n"); }
    }
}

main() {
    cli();
    sys(0);
}

