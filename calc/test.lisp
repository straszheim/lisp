#!/home/troy/Projects/boospiele/calc/build/lisp

;; #!/usr/bin/clisp
;;
;; silly test suite
;;

(defvar passes 0)     ; this is where we will keep the passes
(defvar failures 0)   ; lo and behold the failures go here

(defun fail (what) 
  (setf failures (+ 1 failures)) 
  (print "FAIL")
  (print what)
  'fail)

(defun pass () 
  (setf passes (+ 1 passes)) 
  'pass)

(defun test (what) 
  (if (eval what) (pass) (fail what)))

(test '(equal t t))
(test '(equal nil ()))

(test '(equal 1 1))
(test '(equal (+ 1 2) (+ 2 1)))

(defvar *x* 17)
(setf x 17)
(test '(equal x *x*))

(defun f (x) (+ x x))
(defun g (y) (+ y y))
(test '(equal (f 17) (g (+ 1 16))))

(defun h (x)
  (setf k 17)
  (+ x k))

(setf k 99)

(test '(equal (h 18) (+ 17 18)))

(defun i (x)
  (+ x k))

(print (i 2))   ; hi

(test '(equal (- 1 1) 0))
(test '(equal (- 2 1) 1))
(test '(equal (- 100 1) 99))


(test '(equal (i 2) (+ 99 2)))

;
;  unary divides is reciprocal
;
(test '(equal (/ 1) 1))
(test '(equal (/ 2) 0.5))

;
;  unary minus is negate
;
(test '(equal (- 1) -1))
(test '(equal (- 1 1 1) -1))
(test '(equal (- 2 1) 1))

(defun factorial (x)
  (print x)
  (if (equal x 1)
      1
    (* x (factorial (- x 1)))))

(test '(equal (factorial 1) 1))
(test '(equal (factorial 2) 2))
(test '(equal (factorial 3) 6))
(test '(equal (factorial 4) 24))
(test '(equal (factorial 5) 120))



(print "passes:")
(print passes)
(print "failures:")
(print failures)
