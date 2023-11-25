/* some output functions */

putc
    /* pop r2 : pop r1 : lbi r0,1 */
    0x01b08182,
    /* sys : mov pc,r2 : nop */
    0x1fd20400;

putchar(c) {
    putc(char(&c, 3));
    putc(char(&c, 2));
    putc(char(&c, 1));
    putc(char(&c, 0));
}

puts(s) {
    extrn char;
    auto i;
    for(i = 0; char(s, i); i++)
        putc(char(s, i));
}

nbuf[8];

putn(n) {
    auto b;
    if(n < 0) { putc('-'); n = -n; }
    b = 0;
    do {
        lchar(nbuf, b++, n%10+'0');
        n =/ 10;
    } while(n);
    while(b >= 0) putc(char(nbuf, --b));
}

