
main() {
    extrn putstr(), putnumb(), putchar();
    auto hi, hey;
    putstr(hey = "Hello, world!*n");
    hi = "Hi!*n";
    putstr(hi);
    putnumb(1027); putchar('*n');
    putnumb(-799); putchar('*n');
    putnumb(699); putchar('*n');
}
