
char
    /* pop r2 : pop r1 : pop r0 : add... */
    0x0d808182,
    /* ...r0,r1 : ldb r0,r0 : mov... */
    0x04000701,
    /* ...pc,r2 : nop nop nop */
    0x1f1f1fd2;

lchar
    /* pop r3 : pop r2 : pop r1 : pop r0 */
    0x80818283,
    /* add r0,r1 : stb r2,r0 */
    0x200c010d,
    /* mov pc,r3 : nop nop */
    0x1f1fd304;

