This is an x86-64 bomb for self-study students. 

## command
- objdump
> objdump -d bomb > bomb.dump
> 
> readelf -x .rodata bomb 可以查看object file中的data segment，包含所有的static variables及其地址

- gdb
> info registers 查看所有registers的值
> 
> p /x $rax  查看%rax寄存器的值
> 
> gdb -tui bomb 
> 
> (gdb) layout split  可以同时查看源码和assembly code
> 
> (gdb) b 13 在第13行设置断点
> 
> (gdb) b *0x400f51 在地址为0x400f51设置断点
>
> (gdb) run 运行程序到下一个断点
> 
> (gdb) n 运行到下一个断点
> 
> (gdb) s 单步运行 / si 前跳 i steps
> 
> (gdb) l bomb.c:50 查看第50行开始的源码
>
> (gdb) x/u 取地址u中的值
- vim
> /text 查找text | n跳到下一个结果
> 
> h l j k 左右上下，配合数字移动对应行数 20j 上移20行

## Phase_1
1. 目的基本就是熟悉objdump以及gdb的各项指令
2. objdump -d bomb > bomb.dump 得到object file的汇编码
3. gdb 模式下layout split 对照bomb.c源码找到phase_1函数的地址为0x400ee0; 并且read_line函数从控制台读取到user输入的字符串存储到了%rax中，调用phase_1(input)之前，mov %rax %rdi, 将输入字符串作为phase_1的参数
   
![](phase_1_address.png)

4. bomb.dump 中找到phase_1函数对应的assembly code, mov $0x402400 %esi, 将起始地址为0x402400的char*[]起始地址作为第二个参数，然后callq <strings_not_equal>,比较第一个参数(用户输入值存储在%rdi中)和第二个参数(程序中static data 0x402400)是否相等]
   
![](phase_1_assembly.png)

5. readelf -x .rodata bomb找到0x402400字符串, 00000000 代表字符串结束符'\0',查找ascii表2e对应'.';所以phase_1答案为:<strong>Border relations with Canada have never been better.</strong>

![](phase_1_answer.png)


## Phase_2
1. phase_2调用read_six_number -> 调用scanf在phase_2开辟的栈空间地址上写入从stdin读取来的值
2. 400f0a处可得(%rsp) 必须为0x1
3. 400f30 ~ 400f35处限定了地址空间的范围，当%rbx 地址值 != %rbp, 每次作为计算 %rbx+=0x4
4. 400f17 ~ 400f1c 将计算方法确定，(%rbx)值为 2 * (%rbx)
5. mov -0x4(%rbx), %eax 地址计算完再dereferecne M[%rbx - 0x4] -> %eax 而不是 M[%rbx] - 0x4 -> %eax
6. scanf返回值为成功读取值的个数
```
0000000000400efc <phase_2>:
  400efc:       55                      push   %rbp #保存main函数的stack frame
  400efd:       53                      push   %rbx 
  400efe:       48 83 ec 28             sub    $0x28,%rsp # 开辟40bytes的栈空间
  400f02:       48 89 e6                mov    %rsp,%rsi  # %rsp地址作为参数 传递给read_six_numbers
  400f05:       e8 52 05 00 00          callq  40145c <read_six_numbers> #read_six中调用scanf将数值写入栈空间地址%rsp ~ %rsp + 0x18
  400f0a:       83 3c 24 01             cmpl   $0x1,(%rsp)   #(%rsp) 值为0x1
  400f0e:       74 20                   je     400f30 <phase_2+0x34> #跳转到 400f30
  400f10:       e8 25 05 00 00          callq  40143a <explode_bomb>
  400f15:       eb 19                   jmp    400f30 <phase_2+0x34>
  400f17:       8b 43 fc                mov    -0x4(%rbx),%eax  # M[%rbx - 0x4] 中的值赋给%eax
  400f1a:       01 c0                   add    %eax,%eax  # %eax *= 2
  400f1c:       39 03                   cmp    %eax,(%rbx) # %eax 值与 (%rbx)值做比较
  400f1e:       74 05                   je     400f25 <phase_2+0x29>
  400f20:       e8 15 05 00 00          callq  40143a <explode_bomb>
  400f25:       48 83 c3 04             add    $0x4,%rbx # %rbx中地址值+4
  400f29:       48 39 eb                cmp    %rbp,%rbx
  400f2c:       75 e9                   jne    400f17 <phase_2+0x1b>
  400f2e:       eb 0c                   jmp    400f3c <phase_2+0x40>
  400f30:       48 8d 5c 24 04          lea    0x4(%rsp),%rbx # %rbx 取得栈顶+0x4的地址
  400f35:       48 8d 6c 24 18          lea    0x18(%rsp),%rbp # %rbp 取得栈顶+0x18的地址 也就是对应6位数字的最后一位
  400f3a:       eb db                   jmp    400f17 <phase_2+0x1b>
  400f3c:       48 83 c4 28             add    $0x28,%rsp
  400f40:       5b                      pop    %rbx
  400f41:       5d                      pop    %rbp
  400f42:       c3                      retq
```
```
#以phase_2传递过来的%rsp为基准，使用%rcx, %rax, %r8, %r8, read_six_number栈空间+0x8, +0x00的位置存储scanf读取到的数值，
#寄存器及栈空间存储的都是phase_2栈空间上的地址，scanf直接将keyboard输入的值写入
read_six_numbers: 
  40145c:       48 83 ec 18     subq    $24, %rsp
  401460:       48 89 f2        movq    %rsi, %rdx
  401463:       48 8d 4e 04     leaq    4(%rsi), %rcx
  401467:       48 8d 46 14     leaq    20(%rsi), %rax
  40146b:       48 89 44 24 08  movq    %rax, 8(%rsp)
  401470:       48 8d 46 10     leaq    16(%rsi), %rax
  401474:       48 89 04 24     movq    %rax, (%rsp)
  401478:       4c 8d 4e 0c     leaq    12(%rsi), %r9
  40147c:       4c 8d 46 08     leaq    8(%rsi), %r8
  401480:       be c3 25 40 00  movl    $4203971, %esi
  401485:       b8 00 00 00 00  movl    $0, %eax
  40148a:       e8 61 f7 ff ff  callq   -2207 <__isoc99_sscanf@plt>
  40148f:       83 f8 05        cmpl    $5, %eax
  401492:       7f 05   jg      5 <read_six_numbers+0x3d>
  401494:       e8 a1 ff ff ff  callq   -95 <explode_bomb>
  401499:       48 83 c4 18     addq    $24, %rsp
  40149d:       c3      retq
```

![](stack-space.png)

## Phase_3
1. 考察对于switch语句汇编形式的理解
2. 400f51: 	mov   $0x4025cf,%esi  使用x/s 0x4025cf 查看得到“%d %d”, 证明phase_3需要输入两个数字 ｜ 400f60: cmp $0x1,%eax 验证了这一想法，scanf返回值要大于1
3. %rdx %rcx 被赋予
4. 400f6a: cmpl   $0x7,0x8(%rsp) 可知switch范围为 0 <= x <= 7 也是我们输入的“%d %d”的第一个数
5. 400f75 jmpq   *0x402470(,%rax,8)  前一步将输入的第一个数字存储到了rax中， x/8a 0x402470可以看到自0x402470起后每+0x8位置的地址，也就是jump table中的地址空间: switch(0): 0x400f7c swithc(1)0x400fb9 ...
   
![](jump-table.png)

6. 在查看switch语句跳转部分会将我们输入的第二值与switch分支中的值进行比较，所以phase_3 “%d %d” -- switch条件(0~7), switch 语句对应的值

```
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi # "%d %d"
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp) #switch(x)  0 <= x <= 7
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq   
```