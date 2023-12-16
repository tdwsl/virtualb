
main() {
    extrn argc, args, dir, dp, sys;
    extrn findFile, putstr;
    auto i, b;
    if(*argc > 1) {
        for(i = 1; i < *argc; i++) {
            b = findFile(dir[*dp], args[i]);
            if(b) {
                /* delete file */
                sys(7, sys(6)-16);
                b = 0;
                sys(5, &b, 4);
                sys(5, &b, 4);
                sys(5, &b, 4);
                sys(5, &b, 4);
            } else {
                putstr("file "); putstr(args[i]); putstr(" does not exist*n");
            }
        }
    } else {
        putstr("specify files to delete*n");
    }
}
