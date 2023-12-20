/* file/console input/output functions */

wr.unit 1;
rd.unit 0;

fileb 0,0,0,0,0,0,0,0,0,0,0;
fileo 0,0,0,0,0,0,0,0,0,0,0;

fputc(c) {
    extrn sys, char, findFree;
    auto b;
    if(fileo[wr.unit] >= 510) {
        b = findFree();
        sys(7, fileb[wr.unit]<<9);
        sys(5, &b, 2);
        fileb[wr.unit] = b;
        fileo[wr.unit] = 2;
    }
    sys(7, (fileb[wr.unit]<<9)|fileo[wr.unit]);
    sys(5, &c, 1);
    fileo[wr.unit] = sys(6)-(fileb[wr.unit]<<9);
}

putchar(c) {
    auto b;
    if(wr.unit == 1) {
        sys(1, (c>>24)&0xff);
        sys(1, (c>>16)&0xff);
        sys(1, (c>>8)&0xff);
        sys(1, c&0xff);
    } else if((wr.unit <= 10) & (wr.unit >= 0)) {
        if(fileb[wr.unit]) {
            if(b = (c>>24)&0xff) fputc(b);
            if(b = (c>>16)&0xff) fputc(b);
            if(b = (c>>8)&0xff) fputc(b);
            fputc(c);
        }
    }
}

getchar() {
    auto c;
    if(rd.unit == 0) {
        return sys(2);
    } else if((rd.unit <= 10) & (rd.unit >= 0)) {
        if(fileb[rd.unit]) {
            if(fileo[rd.unit] >= 510) {
                sys(7, fileb[rd.unit]<<9);
                sys(4, &fileb[rd.unit], 2);
                fileo[rd.unit] = 2;
            }
            if(fileb[rd.unit] == 0) return -1;
            sys(7, (fileb[rd.unit]<<9)+fileo[rd.unit]);
            c = 0;
            sys(4, &c, 1);
            fileo[rd.unit] = sys(6)-(fileb[rd.unit]<<9);
            return c;
        }
        return -1;
    }
}

openr(f, name) {
    extrn dir, findFile;
    if((f<0)|(f>10)) return 0;
    fileb[f] = findFile(dir, name);
    if(!fileb[f]) return 0;
    fileo[f] = 2;
    rd.unit = f;
    return 1;
}

openw(f, name) {
    extrn addFile;
    if((f<0)|(f>10)) return 0;
    fileb[f] = addFile(dir, name);
    if(!fileb[f]) return 0;
    fileo[f] = 2;
    wr.unit = f;
    return 1;
}

putstr(s) {
    auto i;
    for(i = 0; char(s, i); i++)
        putchar(char(s, i));
}

getstr(s) {
    extrn lchar;
    auto i, c;
    for(i = 0; ((c = getchar()) > 0) & (c != '*n'); i++)
        lchar(s,i,c);
    lchar(s,i,0);
    return i;
}

putu(n) {
    extrn lchar;
    auto b, nbuf[8];
    b = 0;
    do {
        lchar(nbuf, b++, n%10+'0');
        n =/ 10;
    } while(n);
    do putchar(char(nbuf, --b)); while(b);
}

putnumb(n) {
    if(n < 0) { putchar('-'); n = -n; }
    putu(n);
}

get "char.b", "sys.b";

