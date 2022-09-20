
(print "Loop Method:")
(let a 1)
(let b 1)
(let c 1)
(let p 0)
(while (< c 20) 
    (print a)
    (print b)
    (set c (+ c 1))
    (set a (+ a b))
    (set b (+ a b))   
)  

(def fibo {n}
    (return (if (> n 2) 
            (+ (fibo (- n 1)) (fibo (- n 2))) 
            1)))

(print "Recursion Method:")

(for x (range 1 30) 
    (print (fibo x))
)

(system "pause")