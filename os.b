/* virtualb command line */

get "sys.b";

dir 1;

args[40];
argc;

fstreq(a, b) {
    extrn char;
    auto i;
    for(i = 0; i < 13; i++) {
        if(char(a,i) != char(b,i)) return 0;
        if(!char(a,i)) break;
    }
    return 1;
}

strchr(s, c) {
    extrn char;
    auto h;
    while((h=char(s,0)) != c) {
        if(!h) return 0;
        s++;
    }
    return s;
}

findPath(dir, name) {
    extrn char, lchar, findEntry0;
    auto i, buf[64], nom;
    if(char(name,0) == '/') { name++; *dir = 1; }
    for(i = 0; char(name,i); i++) lchar(buf,i, char(name,i));
    lchar(buf,i,0);
    while(nom = strchr(buf, '/')) {
        lchar(nom,0,0);
        *dir = findEntry0(*dir, buf);
        if(!(*dir&0x8000)) return 0;
        *dir =& 0x7fff;
        if(!*dir) return 0;
        name =+ nom+1-buf;
        buf = nom+1;
    }
    return name;
}

findEntry0(dir, name) {
    auto i, buf[4], b;
    b = 0;
    do {
        sys(7, dir<<9);
        sys(4, &dir, 2);
        for(i = 0; i < 510/15; i++) {
            sys(4, buf, 13);
            sys(4, &b, 2);
            if(b) if(fstreq(buf, name)) return b;
        }
    } while(dir);
    return 0;
}

findEntry(dir, name) {
    name = findPath(&dir, name);
    if(!dir) return 0;
    return findEntry0(dir, name);
}

findFile(dir, name) {
    dir = findEntry(dir, name);
    if(!(dir&0x8000)) return dir;
    return 0;
}

findDir(dir, name) {
    dir = findEntry(dir, name);
    if(dir&0x8000) return dir&0x7fff;
    return 0;
}

blockFree(dir, b) {
    auto c, i, p, buf[4];
    c = 0;
    if(dir == b) return 0;
    do {
        sys(7, dir<<9);
        sys(4, &dir, 2);
        if(dir == b) return 0;
        for(i = 0; i < 510/15; i++) {
            sys(4, buf, 13);
            sys(4, &c, 2);
            if(c) {
                p = sys(6);
                if(c&0x8000) {
                    if(!fstreq(buf, ".."))
                        if(!blockFree(c&0x7fff, b)) return 0;
                } else {
                    do {
                        if(c == b) return 0;
                        sys(7, c<<9);
                        sys(4, &c, 2);
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

eraseBlock(b) {
    auto i, buf[128];
    for(i = 0; i < 128; i++) buf[i] = 0;
    sys(7, b<<9);
    sys(5, buf, 512);
}

namecpy(s1, s2) {
    extrn lchar, char;
    auto i;
    for(i = 0; i < 13; i++) {
        lchar(s1, i, char(s2, i));
        if(!char(s2, i)) break;
    }
}

addFile(dir, name, isdir) {
    auto i, b, buf[4];
    b = 0;
    name = findPath(&dir, name);
    if(!dir) return 0;
    if(isdir) {
        if(findDir(dir, name)) return 0;
    } else if(b = findFile(dir, name)) {
        sys(7, b<<9);
        i = 0;
        sys(5, &i, 2);
        return b;
    }
    for(i = 0; i < 4; i++) buf[i] = 0;
    for(;;) {
        sys(7, dir<<9);
        sys(4, &dir, 2);
        for(i = 0; i < 510/15; i++) {
            sys(7, sys(6)+13);
            sys(4, &b, 2);
            if(!b) {
                i = sys(6)-15;
                b = findFree();
                namecpy(buf, name);
                sys(7, i);
                sys(5, buf, 13);
                if(isdir) b =| 0x8000;
                sys(5, &b, 2);
                b =& 0x7fff;
                return b;
            }
        }
        if(!dir) {
            b = sys(6)-512;
            dir = findFree();
            sys(7, b);
            sys(5, &dir, 2);
            b = 0;
            eraseBlock(dir);
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
    for(;;) {
        argc = 0;
        p = buf;
        putstr(">");
        while(!readLine(buf, 240));
        do args[argc++] = p; while(p = next(p));
        if(b = findFile(dir, args[0])) {
            loadFile(b, 0x20000);
            0x20000();
        } else if(b = findFile(1, args[0])) {
            loadFile(b, 0x20000);
            0x20000();
        } else { putstr(args[0]); putstr("?*n"); }
    }
}

main() {
    cli();
    sys(0);
}

