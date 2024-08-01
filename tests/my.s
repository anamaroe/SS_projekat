# file: my.s

.extern handler, mathAdd, mathSub, mathMul, mathDiv

.global my_start

.global value1, value2, value3, value4, value5, value6, value7


.section my_code
my_start:
    ld $0xFFFFFEFE, %sp
    ld $0xF000000e, %sp
    ld $0xFFFFFEFE, %sp
    ld $0xFFFFFEFE, %sp
    ld $handler, %r1
    csrwr %r1, %handler

    int # software interrupt
    call 0xF000000e
    call mathDiv
    halt

.section my_data
value1:
.word 0
value2:
.word 0
value3:
.word 0
value4:
.word 0
value5:
.word 0
value6:
.word 0
value7:
.word 0

.end
