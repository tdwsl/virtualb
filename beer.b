
beer(n) {
    extrn putstr, putnumb;
    if(!n) putstr("no bottles");
    else if(n == 1) putstr("1 bottle");
    else { putnumb(n); putstr(" bottles"); }
    putstr(" of beer");
}

main() {
    extrn putchar;
    auto i;
    for(i = 99; i > 0; i--) {
        beer(i); putstr(" on the wall,*n");
        beer(i); putchar(',*n');
        putstr("take one down, pass it around,*n");
        beer(i-1); putstr(" on the wall*n*n");
    }
}

get "put.b";

