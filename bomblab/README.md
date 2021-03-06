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
```asm
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

#以phase_2传递过来的%rsp为基准，使用%rcx, %rax, %r8, %r8, read_six_number栈空间+0x8, +0x00的位置存储scanf读取到的数值，
#寄存器及栈空间存储的都是phase_2栈空间上的地址，scanf直接将keyboard输入的值写入
```asm
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

```asm
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

## Phase_4
1. 同样先检查40101a: movl	$4203983, %esi, 再次得到“%d %d", 证明phase_4同样是输入两个数字, 401029调用scanf后的%eax值必须为2同样验证了需要两个数字

![](phase_4.png)

2. 40102e: cmpl $0xe,0x8(%rsp), 输入的第一个数需要 <= 0xe
3. 40103a ~ 401044 将0xe传入%edx(第三个参数寄存器), 0x0传入%esi(第二个参数寄存器), user输入的第一个数字传入%edi(第一个参数寄存器)，之后调用func4
4. func4的小trick是400fe2: cmp  %edi,%ecx，其中edi为我们输入的第一个数字，只有edi <= ecx才会跳转400ff2, 而400ff2 将return value %eax置0，之后  400ff7: cmp %edi,%ecx 400ff9: jge 401007 再次比较edi和ecx的值 并且需要 %edi >= ecx才return 0，而return 0在phase_4中40104d中满足不引爆炸弹的必要条件，所以我们输入的数值只要与ecx通过 400fce~400fdf得到的结果相同就可以，而这个值为0xe >> 1 = 0x7
5. 第二个值很简单401051行与0做比较，所以输入的两个值为 0x7 和 0x0

```asm
phase_4:
000000000040100c <phase_4>:
  40100c:       48 83 ec 18             sub    $0x18,%rsp
  401010:       48 8d 4c 24 0c          lea    0xc(%rsp),%rcx
  401015:       48 8d 54 24 08          lea    0x8(%rsp),%rdx
  40101a:       be cf 25 40 00          mov    $0x4025cf,%esi
  40101f:       b8 00 00 00 00          mov    $0x0,%eax
  401024:       e8 c7 fb ff ff          callq  400bf0 <__isoc99_sscanf@plt>
  401029:       83 f8 02                cmp    $0x2,%eax #返回值为2，需要两个digits
  40102c:       75 07                   jne    401035 <phase_4+0x29>
  40102e:       83 7c 24 08 0e          cmpl   $0xe,0x8(%rsp)
  401033:       76 05                   jbe    40103a <phase_4+0x2e>
  401035:       e8 00 04 00 00          callq  40143a <explode_bomb>
  40103a:       ba 0e 00 00 00          mov    $0xe,%edx
  40103f:       be 00 00 00 00          mov    $0x0,%esi
  401044:       8b 7c 24 08             mov    0x8(%rsp),%edi
  401048:       e8 81 ff ff ff          callq  400fce <func4>
  40104d:       85 c0                   test   %eax,%eax
  40104f:       75 07                   jne    401058 <phase_4+0x4c>
  401051:       83 7c 24 0c 00          cmpl   $0x0,0xc(%rsp)
  401056:       74 05                   je     40105d <phase_4+0x51>
  401058:       e8 dd 03 00 00          callq  40143a <explode_bomb>
  40105d:       48 83 c4 18             add    $0x18,%rsp
  401061:       c3                      retq
```
```asm
0000000000400fce <func4>:
  400fce:       48 83 ec 08             sub    $0x8,%rsp
  400fd2:       89 d0                   mov    %edx,%eax
  400fd4:       29 f0                   sub    %esi,%eax
  400fd6:       89 c1                   mov    %eax,%ecx
  400fd8:       c1 e9 1f                shr    $0x1f,%ecx
  400fdb:       01 c8                   add    %ecx,%eax
  400fdd:       d1 f8                   sar    %eax
  400fdf:       8d 0c 30                lea    (%rax,%rsi,1),%ecx
  400fe2:       39 f9                   cmp    %edi,%ecx
  400fe4:       7e 0c                   jle    400ff2 <func4+0x24>
  400fe6:       8d 51 ff                lea    -0x1(%rcx),%edx
  400fe9:       e8 e0 ff ff ff          callq  400fce <func4>
  400fee:       01 c0                   add    %eax,%eax
  400ff0:       eb 15                   jmp    401007 <func4+0x39>
  400ff2:       b8 00 00 00 00          mov    $0x0,%eax
  400ff7:       39 f9                   cmp    %edi,%ecx
  400ff9:       7d 0c                   jge    401007 <func4+0x39>
  400ffb:       8d 71 01                lea    0x1(%rcx),%esi
  400ffe:       e8 cb ff ff ff          callq  400fce <func4>
  401003:       8d 44 00 01             lea    0x1(%rax,%rax,1),%eax
  401007:       48 83 c4 08             add    $0x8,%rsp
  40100b:       c3                      retq
```

## Phase_5
1. 大概扫一遍assembly code,比较重要的几个关键点: 40107a 调用string_length 返回值需要是6，401099 中0x4024b0 string字符串的内容，4010b3 调用strings_not_equal之前传入%esi的结果

![](stringDecode.png)

2. 有了前4个phase的练习，很容易看出，phase_5需要我们输入6位字符串，然后401096 and $0xf, %edx，就是将每一个字符的bits 与 00001111做and，得到的偏移量，到0x4024b0中取出对应的字符,使得得到得结果为"flyers". 
3. 此题答案也不唯一，我的解是“ionefg”

```asm
0000000000401062 <phase_5>:
  401062:       53                      push   %rbx
  401063:       48 83 ec 20             sub    $0x20,%rsp
  401067:       48 89 fb                mov    %rdi,%rbx
  40106a:       64 48 8b 04 25 28 00    mov    %fs:0x28,%rax
  401071:       00 00
  401073:       48 89 44 24 18          mov    %rax,0x18(%rsp)
  401078:       31 c0                   xor    %eax,%eax
  40107a:       e8 9c 02 00 00          callq  40131b <string_length>
  40107f:       83 f8 06                cmp    $0x6,%eax
  401082:       74 4e                   je     4010d2 <phase_5+0x70>
  401084:       e8 b1 03 00 00          callq  40143a <explode_bomb>
  401089:       eb 47                   jmp    4010d2 <phase_5+0x70>
  40108b:       0f b6 0c 03             movzbl (%rbx,%rax,1),%ecx
  40108f:       88 0c 24                mov    %cl,(%rsp)
  401092:       48 8b 14 24             mov    (%rsp),%rdx
  401096:       83 e2 0f                and    $0xf,%edx
  401099:       0f b6 92 b0 24 40 00    movzbl 0x4024b0(%rdx),%edx
  4010a0:       88 54 04 10             mov    %dl,0x10(%rsp,%rax,1)
  4010a4:       48 83 c0 01             add    $0x1,%rax
  4010a8:       48 83 f8 06             cmp    $0x6,%rax
  4010ac:       75 dd                   jne    40108b <phase_5+0x29>
  4010ae:       c6 44 24 16 00          movb   $0x0,0x16(%rsp)
  4010b3:       be 5e 24 40 00          mov    $0x40245e,%esi #正确的字符串 flyers`
  4010b8:       48 8d 7c 24 10          lea    0x10(%rsp),%rdi
  4010bd:       e8 76 02 00 00          callq  401338 <strings_not_equal>
  4010c2:       85 c0                   test   %eax,%eax
  4010c4:       74 13                   je     4010d9 <phase_5+0x77>
  4010c6:       e8 6f 03 00 00          callq  40143a <explode_bomb>
  4010cb:       0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
  4010d0:       eb 07                   jmp    4010d9 <phase_5+0x77>
  4010d2:       b8 00 00 00 00          mov    $0x0,%eax
  4010d7:       eb b2                   jmp    40108b <phase_5+0x29>
  4010d9:       48 8b 44 24 18          mov    0x18(%rsp),%rax
  4010de:       64 48 33 04 25 28 00    xor    %fs:0x28,%rax
  4010e5:       00 00
  4010e7:       74 05                   je     4010ee <phase_5+0x8c>
  4010e9:       e8 42 fa ff ff          callq  400b30 <__stack_chk_fail@plt>
  4010ee:       48 83 c4 20             add    $0x20,%rsp
  4010f2:       5b                      pop    %rbx
  4010f3:       c3                      retq
```
## Phase_6
1. 第6题感觉难度提升了很多，主要是循环的嵌套导致assembly code比较复杂
2. 另外找到$0x6032d0中这个Linked-Node数据结构非常关键。

![](linkedNode.png)

3. phase_6主要拆解为 1）输入六个数字 要求在区间[1,6] 2)输入的6个数字一次被7减，得到新的序列 3)新的序列对应0x6032d0中的6个nodes，会按照新的序列的顺序一次将nodes连接起来 4)连接好的nodes 要求以value值从大到小排序
4. 答案是 4 3 2 1 6 5
5. 将不同function的assembly code拆分开，会更容易理解程序

```asm
phase_6:
  4010f4:	41 56 	pushq	%r14
  4010f6:	41 55 	pushq	%r13
  4010f8:	41 54 	pushq	%r12
  4010fa:	55 	pushq	%rbp
  4010fb:	53 	pushq	%rbx
  4010fc:	48 83 ec 50 	subq	$80, %rsp
  401100:	49 89 e5 	movq	%rsp, %r13
  401103:	48 89 e6 	movq	%rsp, %rsi
  401106:	e8 51 03 00 00 	callq	849 <read_six_numbers>
  40110b:	49 89 e6 	movq	%rsp, %r14
  40110e:	41 bc 00 00 00 00 	movl	$0, %r12d
  401114:	4c 89 ed 	movq	%r13, %rbp
  401117:	41 8b 45 00 	movl	(%r13), %eax
  40111b:	83 e8 01 	subl	$1, %eax
  40111e:	83 f8 05 	cmpl	$5, %eax
  401121:	76 05 	jbe	5 <phase_6+0x34>
  401123:	e8 12 03 00 00 	callq	786 <explode_bomb>
  401128:	41 83 c4 01 	addl	$1, %r12d
  40112c:	41 83 fc 06 	cmpl	$6, %r12d
  401130:	74 21 	je	33 <phase_6+0x5f>
  401132:	44 89 e3 	movl	%r12d, %ebx
  401135:	48 63 c3 	movslq	%ebx, %rax
  401138:	8b 04 84 	movl	(%rsp,%rax,4), %eax
  40113b:	39 45 00 	cmpl	%eax, (%rbp)
  40113e:	75 05 	jne	5 <phase_6+0x51>
  401140:	e8 f5 02 00 00 	callq	757 <explode_bomb>
  401145:	83 c3 01 	addl	$1, %ebx
  401148:	83 fb 05 	cmpl	$5, %ebx
  40114b:	7e e8 	jle	-24 <phase_6+0x41>
  40114d:	49 83 c5 04 	addq	$4, %r13
  401151:	eb c1 	jmp	-63 <phase_6+0x20>
  401153:	48 8d 74 24 18 	leaq	24(%rsp), %rsi
  401158:	4c 89 f0 	movq	%r14, %rax
  40115b:	b9 07 00 00 00 	movl	$7, %ecx
  401160:	89 ca 	movl	%ecx, %edx
  401162:	2b 10 	subl	(%rax), %edx
  401164:	89 10 	movl	%edx, (%rax)
  401166:	48 83 c0 04 	addq	$4, %rax
  40116a:	48 39 f0 	cmpq	%rsi, %rax
  40116d:	75 f1 	jne	-15 <phase_6+0x6c>
  40116f:	be 00 00 00 00 	movl	$0, %esi
  401174:	eb 21 	jmp	33 <phase_6+0xa3>
  401176:	48 8b 52 08 	movq	8(%rdx), %rdx
  40117a:	83 c0 01 	addl	$1, %eax
  40117d:	39 c8 	cmpl	%ecx, %eax
  40117f:	75 f5 	jne	-11 <phase_6+0x82>
  401181:	eb 05 	jmp	5 <phase_6+0x94>
  401183:	ba d0 32 60 00 	movl	$6304464, %edx
  401188:	48 89 54 74 20 	movq	%rdx, 32(%rsp,%rsi,2)
  40118d:	48 83 c6 04 	addq	$4, %rsi
  401191:	48 83 fe 18 	cmpq	$24, %rsi
  401195:	74 14 	je	20 <phase_6+0xb7>
  401197:	8b 0c 34 	movl	(%rsp,%rsi), %ecx
  40119a:	83 f9 01 	cmpl	$1, %ecx
  40119d:	7e e4 	jle	-28 <phase_6+0x8f>
  40119f:	b8 01 00 00 00 	movl	$1, %eax
  4011a4:	ba d0 32 60 00 	movl	$6304464, %edx
  4011a9:	eb cb 	jmp	-53 <phase_6+0x82>
  4011ab:	48 8b 5c 24 20 	movq	32(%rsp), %rbx
  4011b0:	48 8d 44 24 28 	leaq	40(%rsp), %rax
  4011b5:	48 8d 74 24 50 	leaq	80(%rsp), %rsi
  4011ba:	48 89 d9 	movq	%rbx, %rcx
  4011bd:	48 8b 10 	movq	(%rax), %rdx
  4011c0:	48 89 51 08 	movq	%rdx, 8(%rcx)
  4011c4:	48 83 c0 08 	addq	$8, %rax
  4011c8:	48 39 f0 	cmpq	%rsi, %rax
  4011cb:	74 05 	je	5 <phase_6+0xde>
  4011cd:	48 89 d1 	movq	%rdx, %rcx
  4011d0:	eb eb 	jmp	-21 <phase_6+0xc9>
  4011d2:	48 c7 42 08 00 00 00 00 	movq	$0, 8(%rdx)
  4011da:	bd 05 00 00 00 	movl	$5, %ebp
  4011df:	48 8b 43 08 	movq	8(%rbx), %rax
  4011e3:	8b 00 	movl	(%rax), %eax
  4011e5:	39 03 	cmpl	%eax, (%rbx)
  4011e7:	7d 05 	jge	5 <phase_6+0xfa>
  4011e9:	e8 4c 02 00 00 	callq	588 <explode_bomb>
  4011ee:	48 8b 5b 08 	movq	8(%rbx), %rbx
  4011f2:	83 ed 01 	subl	$1, %ebp
  4011f5:	75 e8 	jne	-24 <phase_6+0xeb>
  4011f7:	48 83 c4 50 	addq	$80, %rsp
  4011fb:	5b 	popq	%rbx
  4011fc:	5d 	popq	%rbp
  4011fd:	41 5c 	popq	%r12
  4011ff:	41 5d 	popq	%r13
  401201:	41 5e 	popq	%r14
  401203:	c3 	retq
```

## Summary
- Lab本身很有趣，如果对常用的汇编指令理解无误，耐心去解，把杂糅在一起的assembly code按功能切分好还是可以解出来的。
- gdb常用的打断点和x p指令查看registers 或者内存中的值非常有用，大概明白程序的各个function就输入值，打断点观察寄存器的变化，不要纯看汇编去decode。我最开始就是这么做的，很折磨也很费时间。让程序跑起来才是关键，切分成小块去decode
