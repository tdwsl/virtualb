
fizz(n) {
    extrn putstr, putnumb, putchar;
    if(n) {
        fizz(n-1);
        if(!(n%3)) putstr("Fizz");
        if(!(n%5)) putstr("Buzz");
        else if(n%3) putnumb(n);
        putchar(' ');
    }
}

main() {
    fizz(15);
    putchar('*n');
}
