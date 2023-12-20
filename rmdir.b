
main() {
    extrn argc, args, dir, sys;
    extrn findEntry, putstr;
    auto i, b;
    if(argc > 1) {
        for(i = 1; i < argc; i++) {
            b = findEntry(dir, args[i]);
            if(!(b&0x8000)) {
                putstr(args[i]); putstr(" is a file*n");
            } else if(b) {
                /* delete file */
                sys(7, sys(6)-15);
                b = 0;
                sys(5, &b, 4);
                sys(5, &b, 4);
                sys(5, &b, 4);
                sys(5, &b, 3);
            } else {
                putstr("file "); putstr(args[i]); putstr(" does not exist*n");
            }
        }
    } else {
        putstr("specify folders to delete*n");
    }
}
