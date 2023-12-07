
get "put.b", "char.b";

ret
    /* pop r0 : pop r0 : mov sp,bp */
    0xfe048080,
    /* pop bp : pop r1 : adw sp,r0 */
    0xf00f818e,
    /* mov pc,r1 : nop nop */
    0x1f1fd104;

puth(n) {
    auto b, c, nbuf[2];
    b = 0;
    do {
        c = n&0xf;
        if(c>=10) c=+'a';
        else c=+'0';
        lchar(nbuf, b++, c);
        n =>> 4;
    } while(n);
    do putchar(char(nbuf, --b)); while(b);
}

putH(n) {
    auto b, c, nbuf[2];
    b = 0;
    do {
        c = n&0xf;
        if(c>=10) c=+'A';
        else c=+'0';
        lchar(nbuf, b++, c);
        n =>> 4;
    } while(n);
    do putchar(char(nbuf, --b)); while(b);
}

puto(n) {
    auto b, nbuf[3];
    b = 0;
    do {
        lchar(nbuf, b++, (n&7)+'0');
        n =>> 3;
    } while(n);
    do putchar(char(nbuf, --b)); while(b);
}

printf(s) {
    auto c, a, r;
    a = &s+4;
    r = 1;
    while(c = *(s++)&0xff) {
        if(c=='%') {
            switch(c = *(s++)&0xff) {
            case 'x': puth(*a); break;
            case 'X': putH(*a); break;
            case 'u': putu(*a); break;
            case 'o': puto(*a); break;
            case 's': putstr(*a); break;
            case 'c': putchar(*a); break;
            case 'd': putnumb(*a); break;
            case 0: s--; continue;
            default: putchar(c); continue;
            }
            a =+ 4;
            r++;
        } else putchar(c);
    }
    ret(r);
}

