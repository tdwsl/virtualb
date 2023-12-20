
main() {
    extrn argc, args, putstr, putchar, dir, char, lchar, findDir;
    auto b;
    if(argc != 2) {
        putstr("usage: cd <directory>*n");
        return;
    }
    if(char(args[1], 0) == '/') { args[1]++; b = 1; }
    else b = dir;
    b = findDir(b, args[1]);
    if(b) {
        dir = b;
    } else {
        putstr("directory "); putstr(args[1]); putstr(" does not exist*n");
    }
}
