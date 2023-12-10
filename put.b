/* some output functions */

putchar(c) {
    extrn sys, char;
    sys(1, (c>>24)&0xff);
    sys(1, (c>>16)&0xff);
    sys(1, (c>>8)&0xff);
    sys(1, c&0xff);
}

putstr(s) {
    auto i;
    for(i = 0; char(s, i); i++)
        putchar(char(s, i));
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

