/* load the real boot program and execute it */

get "sys.b";

streq(a, b) {
    auto i;
    for(i = 0; i < 13; i++) {
        if((*a&0xff) != (*b++&0xff)) return 0;
        if(!(*a++&0xff)) return 1;
    }
    return 1;
}

findFile(filename) {
    auto buf[4], b;
    sys(7, 512+2);
    for(;;) {
        sys(4, buf, 13);
        if(streq(buf, filename)) break;
        sys(4, buf, 2);
    }
    b = 0;
    sys(4, &b, 2);
    return b;
}

main() {
    auto b, dst;
    b = findFile("boot");
    dst = 0x1000;
    do {
        sys(7, b<<9);
        sys(4, &b, 2);
        sys(4, dst, 510);
        dst =+ 510;
    } while(b);
    0x1000();
}
