(class pp 
    (defVar k 0)
    (defCVar PI 3.14)
    (defMethod printk {} (print (* (@ pp PI) (@ this k))))
    (defCMethod printp {n} (print (+ n (@ pp PI))))
)