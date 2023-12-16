
messages "Be sweet, parakeet.",
    "Don't get cut by a blade of grass!",
    "Fine. Go.",
    "Don't smile because it's over, cry because it happened.",
    "You'll come crawling back.",
    "Don't get attacked by a bear, it's nighttime.",
    "Peace out, girl scout!",
    "As you wish, jellyfish.",
    "See you when I see you.";
nmessages 9;

main() {
    extrn sys, putchar, putstr;
    putstr(messages[sys(8, nmessages)]);
    putchar('*n');
    sys(0);
}

