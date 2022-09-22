# ActionLisp

I designed a toy program language: ActionLisp. And I also make an interpreter for it based on C++.

It supports read a whole file or REPL mode.

Why I call it ActionLisp? Because the first program language I learn is ActionScript 2.0 on Flash. ActionScript 2.0 is a simple and interesting language. You can put your codes on any frame or objects. Any objects can have child objects. So my language ActionLisp is the same. You can put any object under others. ActionLisp is also simple and easy to use.

However, this language is a toy. It's lack of functions, and it runs slow. But an interpreter for Lisp-like language is a good point to start for me.

## Demos

### Function

```lisp
(def fibo {n}  
    (return (if (> n 2) 
            (+ (fibo (- n 1)) (fibo (- n 2))) 
            1)))

(for x (range 1 20) 
    (print (fibo x)) ;Output fibonacci(n)
)
```

### Class

```lisp
(class pp 
    (defVar k 0)
    (defCVar PI 3.14)
    (defMethod printk {} (print (@ this k)))
    (defCMethod printp {} (print (@ pp PI)))
)
(let a pp)
(++ (@ a k)) ;a.k+=1
((@ a printk))  ;print 1
(print (@ pp PI)) ;print 3.14000
((@ pp printp)) ;print 3.14000
```

### Array

```lisp
(let a {1 2 3 4 5 6 7 8 9 10})
(print a) ; print { 1 2 3 4 5 6 7 8 9 10 }

(for x (range 0 (length a))
    (++ (ref a x)))

(print a) ; print { 2 3 4 5 6 7 8 9 10 11 }
```

the other demos are in folder "examples". You can drag the \*.lisp files to ActionLisp.exe on Windows to run them. You can also run ActionLisp.exe independently.

## C++ standard

C++ 14



## Differenence from Common Lisp：

Don't use eq/equal, just use =. Such as (print (= a b))

Simple print. 

{} stands of arrays and arguments for functions. Such as (let a {1 2 3})

break/continue. The same as language C.

Use @ to access child variables and functions. For example, (@ a b) is a.b or a.b().



## Basic Classes

| Name    |
| ------- |
| bool    |
| long    |
| double  |
| string  |
| object  |
| pointer |



## Commands


| Name                                                  | Already come true | Functions                                                 | Examples                   |
| --------------------------------------------------------- | ------ | ------------------------------------------------------------ | ----------------------------- |
| + - * /                                                   | T      |  | (/ 1 2) |
| > < >= <=                                              | T     |  | (> 1 0) |
| = equal                                             | T      | == | (= a 1) |
| and, or, not                                              | T      |  | (and true false) |
| true, false                                             | T      |                                                       |  |
| ; comments | T | comments | ;hello! |
| #\|      comments \|# | T | multi-line comments |  |
| (let SYMBOL VALUE)                                        | T      | var SYMBOL = VALUE; | (let a {1 2 3})   |
| (set SYMBOL VALUE)                                        | T      | SYMBOL = VALUE; will not define new variable. | (set a true) |
| (global SYMBOL VALUE)                                     | T      | global var SYMBOL = VALUE;                 | (global b 0) |
| (for x LIST (FUNCTION))                                   | T      |                                                       | (for x {0,3,5,6,7} (print x)) |
| (range 0 5 1)                                             | T      | return a list {0,1,2,3,4} | (for x (range 0 10) <br/>    (if (= x 5) continue)<br/>    (print x)<br/>) |
| (break)或break                                            | T      | break |                               |
| (continue)或continue                                      | T | continue                                                     |                               |
| (doc FUNCTION)                                            |        | return the document of function                     |                               |
| (while SYMBOL/VALUE (FUNCTION))                           | T      | while                                                    |                               |
| (if (SYMBOL/VALUE) (FUNCTION) (FUNCTION))                 | T      | if                                                       |                               |
| (map x (FUNCTION x) LIST)                                 |        |                          |                               |
| (system string)                                | T      | the same to system("") in C.                     |                               |
| (def SYMBOL {SYMBOL...} FUNCTION)                         | T      | define function                                       | (def fibo {n}<br/>    (return (if (> n 2) <br/>            (+ (fibo (- n 1)) (fibo (- n 2))) <br/>            1))) |
| (return VALUE) | T |  |                                                              |
| (randInt VALUEA VALUEB)                                   | T      | random int between VALUEA and VALUEB, including VALUEA and VALUEB |                               |
| (rand)                                                    | T      | random double between 0~1                |                               |
| (eval STRING)                                             |        |                                      |                               |
| ($ STRING)                                                | T      | turn string to symbol                        |                               |
| print                                                     | T      |                                                          | (print "Hello" " World!") |
| (tag SYMBOL)                                              |        | label                                                    |                               |
| (goto SYMBOL)                                             |        | goto label                                              |                               |
| (loop for x from VALUE to VALUE by VALUE do (EXPRESSION)) |        | loop macro                                                 |                               |
| (progn EXPRESSION...)                                     | T  | set the last expression as return value                      |                               |
| (mod VALUE VALUE)                                         | T      | mod. support double.                           |                               |
| (switch a (case {b,d,e} EXPRESSION) (case c EXPRESSION))  | T      | switch. Don't need break.                         |                               |
| (ref vector n)                                            | T      | =vector[n]. If n < 0, find it from the end. | (ref {1 3 5} 1) |
| (length vector) | T | get length of vector. |  |
| (sub list p1 p2)                                          | T      | return sublist list[p1 to p2(p2 not included)] |                               |
| (sort list)                                               | T      | sort.                            | (sort b) |
| (rsort list)                                              | T      | reverse sort                        |                               |
| (shuffle list)                                            | T      | shuffle a list.                            |                               |
| (++ a)                                                    | T      |                                                          | (++ a) |
| (-- a)                                                    | T      |                                                          |                               |
| (push list atom)                                          | T      |                                                  |                               |
| (pop list n)                                              | T    | delete list[n]                                               |                               |
| (append list1 list2)                                      | T      |                                           |                               |
| (reverse list)                                            | T      |                                            |                               |
| (reverseCopy list)                                        | T      |                                   |                               |
| (del list atom...)                                        | T      |  |                               |
| (find list atom)                                          | T      |                                          |                               |
| (rfind list atom)                                         | T      |                                    |                               |
| (split str key)                                           | T      |                                | (split "1 2 3 4" " ") |
| (join list key)                                           | T      |                    | (join {1 2 3 4} " ") |
| (let f (open path "rw"))                                  | T      | Open file. support "r", "rw","rb","wb","a","ab","rwb" |                               |
| (read f)                                                  | T      | read whole f and return a string. |                               |
| (seekRead f n)                                            | T      |                                                 |                               |
| (seekWrite f n)                                           | T      |                                                 |                               |
| (tellRead f)                                              | T      |                                                |                               |
| (tellWrite f)                                             | T      |                                                |                               |
| (close f)                                                 | T      | close fstream f.                                     |                               |
| (readLine f)                                              | T      |             |                               |
| (write f str)                                             | T      |                                                        |                               |
| (writeLine f str)                                         | T      |                                                  |                               |
| (double n)                                                | T      |                                       |                               |
| (long n)                                                  | T      |                                                    |                               |
| (floor n)                                                 | T      |                                                              |                               |
| (ceil n)                                                  | T      |  |                               |
| (round n)                                                 | T      |  |                               |
| (pow x n)                                                 | T      | =(x^n)                                                       |                               |
| (exp x)                                                   | T      | =(e^x)                                                       |                               |
| (log x n)                                                 | T      | =log_{n}x , default n = e.                     |                               |
| (sqrt x)                                                  | T      |  |                               |
| sin, cos, tan                                             | T      |  |                               |
| asin, acos, atan                                          | T      |  |                               |
| sinh, cosh, tanh                                          | T      |  |                               |
| asinh, acosh, atanh                                       | T      |  |                               |
| pi                                                        | T      | π |                               |
| (gets STRING_NAME)                                        | T      | same as  cin >> STRING_NAME;                          |                               |
| (error STRING)                                            | T     | throw error                                                  |                               |
| (class CLASS_NAME EXPRESSIONS...)                         | T      | define class                                            | (class pp <br/>    (defVar k 0)<br/>    (defCVar PI 3.14)<br/>    (defMethod printk {} (print (@ this k)))<br/>    (defCMethod printp {} (print (@ pp PI)))<br/>)<br/>(let a pp)<br/>(++ (@ a k))<br/>((@ a printk))<br/>(print (@ pp PI))<br/>((@ pp printp)) |
| (defVar VARIABLE_NAME VALUE)                              | T      | define member variable                |                                                              |
| (defCVar VARIABLE_NAME VALUE)                             | T      | define static constant. |                                                              |
| (defMethod METHOD_NAME {VARIABLE LIST} EXPRESSION)        | T      | define member function   |                               |
| (defCMethod METHOD_NAME {VARIABLE LIST} EXPRESSION)       | T      | define static function    |                               |
| (import FILENAME)                                         | T | import file | (import "pp.lisp") |
| (@ A B)                                                   | T      | same as A.B in other language.                               |                               |
| (makeChild A B)                                           | T      | define A.B |                               |
| (@ this A)                                                | T      | this.A |                               |

## License

The project is released under MIT License.
