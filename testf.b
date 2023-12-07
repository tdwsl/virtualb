/* try printf */

main() {
    extrn printf;
    printf("Hello, %s!*n", "world");
    printf("%d %x %o*n", 67, 67, 67);
}

get "printf.b";
