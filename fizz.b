
fizz(n) {
    extrn puts, putn, putc;
    if(n) {
        fizz(n-1);
        if(!(n%3)) puts("Fizz");
        if(!(n%5)) puts("Buzz");
        else if(n%3) putn(n);
        putc(' ');
    }
}

main() {
    fizz(15);
    putc('*n');
}
