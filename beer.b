
beer(n) {
    extrn puts, putn;
    if(!n) puts("no bottles");
    else if(n == 1) puts("1 bottle");
    else { putn(n); puts(" bottles"); }
    puts(" of beer");
}

main() {
    extrn putchar;
    auto i;
    for(i = 99; i > 0; i--) {
        beer(i); puts(" on the wall,*n");
        beer(i); putchar(',*n');
        puts("take one down, pass it around,*n");
        beer(i-1); puts(" on the wall*n*n");
    }
}
