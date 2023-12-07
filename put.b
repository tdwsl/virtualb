/* some output functions */

putchar
    /* pop r2 : lbi r0,1 : adw ... */
    0x0f01b082,
    /* ... sp,r0 : dec sp : ldb r1,sp */
    0x1f073ff0,
    /* sys : dec sp : ldb r1,sp */
    0x1f073f00,
    /* sys : dec sp : ldb r1,sp */
    0x1f073f00,
    /* sys : dec sp : ldb r1,sp */
    0x1f073f00,
    /* sys : adw sp,r0 : mov ... */
    0x04f00f00,
    /* ... pc,r2 : nop nop nop */
    0x1f1f1fd2;

putstr(s) {
    extrn char;
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

get "char.b";

