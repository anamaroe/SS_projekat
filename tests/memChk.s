# file: memChk.s

.global biba
.extern maumau
.section bib

biba:

    ld $0xFFFFFEFE, %sp
    ld $10, %r7

    st %r7, [%sp + 4]   # src i dst
    ld 0xFFFFFEFE, %r1
    ld [%sp + 4], %r6   # src i dst

    .word maumau


    halt

.end