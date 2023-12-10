
char
    /* pop r2 : pop r0 : pop r1 : add... */
    0x0d818082,
    /* ...r0,r1 : ldb r0,r0 : mov... */
    0x04000701,
    /* ...pc,r2 */
    0x000000d2;

lchar
    /* pop r3 : pop r0 : pop r1 : pop r2 */
    0x82818083,
    /* add r0,r1 : stb r2,r0 */
    0x200c010d,
    /* mov pc,r3 */
    0x0000d304;

