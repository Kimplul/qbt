# qbt

Optimizing middle/back end for triscv. Takes pretty heavy inspiration from QBE:
https://c9x.me/compile/

Instruction documentation is TODO, please see `src/parser.y` for current list of
instructions. They may still change in the near future.

# ABI

Currently I'm assuming the ABI to be according to the following table, but this
is not quite set in stone yet.

```
| Register | Name |
|----------|------|
| x0       | x0
| x1       | sp
| x2       | fp
| x3       | gp
| x4       | a0
| x5       | a1
| x6       | a2
| x7       | a3
| x8       | a4
| x9       | a5
| x10      | a6
| x11      | t0
| x12      | t1
| x13      | t2
| x14      | t3
| x15      | t4
| x16      | t5
| x17      | t6
| x18      | s0
| x19      | s1
| x20      | s2
| x21      | s3
| x22      | s4
| x23      | s5
| x24      | s6
| x25      | tp
| x26      | ra
| x27      | a7
| x28      | a8
| x29      | a9
| x30      | a10
| x31      | a11
| x32      | a12
| x33      | a13
| x34      | a14
| x35      | a15
| x36      | a16
| x37      | a17
| x38      | a19
| x39      | a20
| x40      | a20
| x41      | a21
| x42      | a22
| x43      | a23
| x44      | tmp0
| x45      | t7
| x46      | t8
| x47      | t9
| x48      | t10
| x49      | t11
| x50      | t12
| x51      | t13
| x52      | t14
| x53      | t15
| x54      | t16
| x55      | t17
| x56      | t18
| x57      | t19
| x58      | t20
| x59      | t21
| x60      | t22
| x61      | t23
| x62      | tmp1
| x63      | s7
| x64      | s8
| x65      | s9
| x66      | s10
| x67      | s11
| x68      | s12
| x69      | s13
| x70      | s14
| x71      | s15
| x72      | s16
| x73      | s17
| x74      | s18
| x75      | s19
| x76      | s20
| x77      | s21
| x78      | s22
| x79      | s23
| x80      | tmp2
```
