(import "classPP.lisp")
(let a pp)
(set (@ a k) (+ 2 (@ a k))) 
((@ a printk)) ; should print 9.420000

(print (@ pp PI)) ; should print 3.140000 

((@ pp printp) 2) ; should print 5.140000 

(system "pause")