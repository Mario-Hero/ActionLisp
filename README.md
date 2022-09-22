# ActionLisp

设计了一个玩具语言ActionLisp，以及它的基于C++的解释器。语法为LISP类型。

I designed a toy program language: ActionLisp. And I also make an interpreter for it based on C++.

为什么取名叫ActionLisp，是因为我入门编程的第一门语言是Flash的ActionScript 2.0，ActionScript 2.0很简单、使用起来很愉快，代码可以写在帧或任何对象上，可以没有类，还可以任意地加子对象。所以ActionLisp也是可以任意地加子对象的，还抛弃了Common Lisp的一些麻烦的语法。

Why I call it ActionLisp? Because the first program language I learn is ActionScript 2.0 on Flash. ActionScript 2.0 is a simple and interesting language. You can put your codes on any frame or objects. Any objects can have child objects. So my language ActionLisp is the same. You can put any object under others. ActionLisp is also simple and easy to use.

支持解释整个文件，也支持输入一行执行一行。

It supports read a whole file or REPL mode.

但是这个语言终归只是一个玩具，功能少、效率低。之所以用类Lisp的语法也是因为这种语法的解释器或编译器制作比较方便，适合拿来练练手。

However, this language is a toy. It's lack of functions, and it runs slow. But an interpreter for Lisp-like language is a good point to start for me.

[README English Version](https://github.com/Mario-Hero/ActionLisp/blob/main/README_EN.md)

## 程序例子

### 函数

```lisp
(def fibo {n}  
    (return (if (> n 2) 
            (+ (fibo (- n 1)) (fibo (- n 2))) 
            1)))

(for x (range 1 20) 
    (print (fibo x)) ;递归输出斐波那契数列前20项，运行缓慢。如果是循环就好很多。
)
```

### 类

```lisp
(class pp 
    (defVar k 0)
    (defCVar PI 3.14)
    (defMethod printk {} (print (@ this k)))
    (defCMethod printp {} (print (@ pp PI)))
)
(let a pp)
(++ (@ a k)) ;a.k+=1
((@ a printk))  ;输出1
(print (@ pp PI)) ;输出3.14000
((@ pp printp)) ;输出3.14000
```

### 数组

```lisp
(let a {1 2 3 4 5 6 7 8 9 10})
(print a) ; 输出 { 1 2 3 4 5 6 7 8 9 10 }

(for x (range 0 (length a))
    (++ (ref a x)))

(print a) ; 输出 { 2 3 4 5 6 7 8 9 10 11 }
```

## 仓库内容

examples文件夹：放置了代码例子，在Windows系统中可以把\*.lisp文件拖入到ActionLisp.exe上执行。也可以直接执行ActionLisp.exe，输入一行执行一行。

ActionLisp.cpp：主程序C++源码。

Interpreter.h：解释器C++源码。



## 源码编译标准

C++ 14标准



## 不同于Common Lisp的点：

弃用eq等，只使用=来判断相等。比如 (= a b)

简单print。比如(print "a is" a)

{}表示数组以及函数输入参数，比如(let a {1 2 3})

新增break和continue。用法与C语言的相同。

用@访问子对象和函数，比如(@ a b)相当于其他语言的a.b或a.b()



## 基础类型表格 

| 类型名  | 类型                   |
| ------- | ---------------------- |
| bool    | 布尔值                 |
| long    | 长整型                 |
| double  | 双精度浮点             |
| string  | 字符串                 |
| object  | 对象。                 |
| pointer | 指针。一般为文件指针。 |



## 命令表格


| 名称                                                      | 是否已实现(True) | 功能                                                         | 例子                          |
| --------------------------------------------------------- | ------ | ------------------------------------------------------------ | ----------------------------- |
| + - * /                                                   | T      | 加减乘除 | (/ 1 2) |
| > < >= <=                                              | T     | 大于、小于、大于等于、小于等于 | (> 1 0) |
| =或equal                                                  | T      | 判断相等 | (= a 1) |
| and, or, not                                              | T      | 与、或、非 | (and true false) |
| true, false                                             | T      | 真，假                                                      |  |
| ; 注释内容 | T | 单行注释 | ;hello! |
| #\|      注释内容         \|# | T | 多行注释 |  |
| (let SYMBOL VALUE)                                        | T      | 新建变量并赋值。如果已有变量，且当前在函数体中，则新建作用域为当前函数体的变量。 | (let a {1 2 3})   |
| (set SYMBOL VALUE)                                        | T      | 赋值。不新建变量。                                           | (set a true) |
| (global SYMBOL VALUE)                                     | T      | 新建全局变量并赋值。                                           | (global b 0) |
| (for x LIST (FUNCTION))                                   | T      | 循环                                                         | (for x {0,3,5,6,7} (print x)) |
| (range 0 5 1)                                             | T      | 返回一个list，其值为{0,1,2,3,4}，不包含5，步长为1            | (for x (range 0 10) <br/>    (if (= x 5) continue)<br/>    (print x)<br/>) |
| (break)或break                                            | T      | 跳出循环。相当于C语言break |                               |
| (continue)或continue                                      | T      | 继续循环。相当于C语言continue                                      |                               |
| (doc FUNCTION)                                            |        | 返回函数的document                                           |                               |
| (while SYMBOL/VALUE (FUNCTION))                           | T      | while循环                                                    |                               |
| (if (SYMBOL/VALUE) (FUNCTION) (FUNCTION))                 | T      | if函数                                                       |                               |
| (map x (FUNCTION x) LIST)                                 |        | 把函数应用到列表每个元素，且返回列表                         |                               |
| (system string)                                | T      | 相当于c语言system                                            |                               |
| (def SYMBOL {SYMBOL...} FUNCTION)                         | T      | 声明函数                                                     | (def fibo {n}<br/>    (return (if (> n 2) <br/>            (+ (fibo (- n 1)) (fibo (- n 2))) <br/>            1))) |
| (return VALUE) | T | 函数返回 |                                                              |
| (randInt VALUEA VALUEB)                                   | T      | 返回随机整数。能取到VALUEA和VALUEB                              |                               |
| (rand)                                                    | T      | 返回随机0~1的双精度浮点数                                         |                               |
| (eval STRING)                                             |        | 以字符串执行fastLisp代码                                     |                               |
| ($ STRING)                                                | T      | 字符串转符号                                                 |                               |
| (funCall STRING ARGUMENTS)                                |        | 以函数名称调用函数                                           |                               |
| (on (@ Event LOOP) e FUNCTION)                            |        | 相当于flash的on，写在对象上。也可以写类里。                  |                               |
| print                                                     | T      | 输出                                                         | (print "Hello" " World!") |
| (tag SYMBOL)                                              |        | 标签                                                         |                               |
| (goto SYMBOL)                                             |        | goto标签                                                     |                               |
| (loop for x from VALUE to VALUE by VALUE do (EXPRESSION)) |        | loop宏                                                       |                               |
| (progn EXPRESSION...)                                     | T      | 不断执行，把最后一个expression作为返回值                     |                               |
| (mod VALUE VALUE)                                         | T      | 求余，支持小数                                               |                               |
| (switch a (case {b,d,e} EXPRESSION) (case c EXPRESSION))  | T      | switch.会自动break                                           |                               |
| (ref vector n)                                            | T      | 列表访问。若n小于0，则为反向寻找。-1表示最后一个。           | (ref {1 3 5} 1) |
| (length vector) | T | 返回vector的长度。 |  |
| (sub list p1 p2)                                          | T      | 返回子列表。包括p1和p2位置。p2默认为列表末尾。 |                               |
| (sort list)                                               | T      | 从小到大排序（破坏性（会改变原值））                                | (sort b) |
| (rsort list)                                              | T      | 从大到小排序（破坏性）                                       |                               |
| (shuffle list)                                            | T      | 随机排列（破坏性）                                           |                               |
| (++ a)                                                    | T      | 自加                                                         | (++ a) |
| (-- a)                                                    | T      | 自减                                                         |                               |
| (push list atom)                                          | T      | 列表添加元素                                                 |                               |
| (pop list n)                                              | T      | 删除第n个元素，默认删除最后一个                               |                               |
| (append list1 list2)                                      | T      | 合并列表（非破坏性）                                          |                               |
| (reverse list)                                            | T      | 翻转列表（破坏性）                                           |                               |
| (reverseCopy list)                                        | T      | 翻转列表（非破坏性（不改变原值））                                  |                               |
| (del list atom...)                                        | T      | 删除列表中的全部多个元素atom(破坏性)。如果不输入atom，则删除列表。 |                               |
| (find list atom)                                          | T      | 查找第一个出现该元素的位置                                         |                               |
| (rfind list atom)                                         | T      | 反向查找第一个出现该元素的位置                                   |                               |
| (split str key)                                           | T      | 用某个字符串分割字符得到vector                               | (split "1 2 3 4" " ") |
| (join list key)                                           | T      | 把vector用字符合并。不输入key时，key为空。                   | (join {1 2 3 4} " ") |
| (let f (open path "rw"))                                  | T      | 打开文件。支持"r", "rw","rb","wb","a","ab","rwb"          |                               |
| (read f)                                                  | T      | 把f整个读取，返回一个字符串                                  |                               |
| (seekRead f n)                                            | T      | 读指针定位到n                                                |                               |
| (seekWrite f n)                                           | T      | 写指针定位到n                                                |                               |
| (tellRead f)                                              | T      | 返回写指针位置                                               |                               |
| (tellWrite f)                                             | T      | 返回读指针位置                                               |                               |
| (close f)                                                 | T      | 关闭文件                                                     |                               |
| (readLine f)                                              | T      | 把f读取一行，返回一个字符串。如果到文件末尾返回空            |                               |
| (write f str)                                             | T      | 写文件                                                       |                               |
| (writeLine f str)                                         | T      | 文件写入一行                                                 |                               |
| (double n)                                                | T      | 转换为双精度浮点                                      |                               |
| (long n)                                                  | T      | 转换为长整型                                                   |                               |
| (floor n)                                                 | T      | 向下取整 |                               |
| (ceil n)                                                  | T      | 向上取整 |                               |
| (round n)                                                 | T      | 四舍五入 |                               |
| (pow x n)                                                 | T      | =(x^n)                                                       |                               |
| (exp x)                                                   | T      | =(e^x)                                                       |                               |
| (log x n)                                                 | T      | =log_{n}x , 若缺省n，则n为e                                  |                               |
| (sqrt x)                                                  | T      | 开平方 |                               |
| sin, cos, tan                                             | T      | 三角函数 |                               |
| asin, acos, atan                                          | T      | 反三角函数 |                               |
| sinh, cosh, tanh                                          | T      | 双曲三角函数 |                               |
| asinh, acosh, atanh                                       | T      | 反双曲三角函数 |                               |
| pi                                                        | T      | π |                               |
| (gets STRING_NAME)                                        | T      | 相当于 cin >> STRING_NAME;                                  |                               |
| (error STRING)                                            | T      | 抛出异常                                                     |                               |
| (class CLASS_NAME EXPRESSIONS...)                         | T      | 声明类                                                       | (class pp <br/>    (defVar k 0)<br/>    (defCVar PI 3.14)<br/>    (defMethod printk {} (print (@ this k)))<br/>    (defCMethod printp {} (print (@ pp PI)))<br/>)<br/>(let a pp)<br/>(++ (@ a k))<br/>((@ a printk))<br/>(print (@ pp PI))<br/>((@ pp printp)) |
| (defVar VARIABLE_NAME VALUE)                              | T      | 声明类变量。使用@调用。                                       |                                                              |
| (defCVar VARIABLE_NAME VALUE)                             | T      | 声明类的静态常量。使用@调用。                                 |                                                              |
| (defMethod METHOD_NAME {VARIABLE LIST} EXPRESSION)        | T      | 声明类的成员函数。使用@调用。                                |                               |
| (defCMethod METHOD_NAME {VARIABLE LIST} EXPRESSION)       | T      | 声明类的静态函数。使用@调用。                                |                               |
| (import FILENAME)                                         | T | 导入包。 | (import "pp.lisp") |
| (@ A B)                                                   | T      | 相当于其他语言的A.B。可访问成员变量或函数。如果不存在则报错。也可以访问类的静态常量和函数。格式为((@ 类名 函数) 参数) |                               |
| (makeChild A B)                                           | T      | 声明A.B，即使存在A.B也不报错。可以这样用(let (makeChild a b) 2) |                               |
| (@ this A)                                                | T      | 返回类对象的子对象的引用。 |                               |

## License

The project is released under MIT License.
