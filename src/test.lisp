#!/home/troy/Projects/boospiele/calc/build/lisp

;;
;; silly test suite
;;

;(setf x 'y)
;(print `(',x))


(defvar passes 0)     ; this is where we will keep the passes
(defvar failures 0)   ; lo and behold the failures go here

(defun report-test (result form)
  (if result
      (progn
	(print "pass... ")
	(setf passes (+ passes 1)))
    (progn
      (print "******************** fail ***********************")
      (setf failures (+ failures 1))))
  (print form))

(defmacro check (form)
  `(report-test ,form ',form))

(check (equal 1 1))

(check (equal t t))
(check (equal nil ()))

(check (equal 1 1))
(check (equal (+ 1 2) (+ 2 1)))

(defvar *x* 17)
(setf x 17)
(check (equal x *x*))

(defun f (x) (+ x x))
(defun g (y) (+ y y))
(check (equal (f 17) (g (+ 1 16))))

(defun h (x)
  (setf k 17)
  (+ x k))

(setf k 99)

(check (equal (h 18) (+ 17 18)))

(defun i (x)
  (+ x k))

(print (i 2))   ; hi

(check (equal (- 1 1) 0))
(check (equal (- 2 1) 1))
(check (equal (- 100 1) 99))


;
;  unary divides is reciprocal
;
(check (equal (/ 1) 1))
(check (equal (/ 2) 0.5))

;
;  unary minus is negate
;
(check (equal (- 1) -1))
(check (equal (- 1 1 1) -1))
(check (equal (- 2 1) 1))

;
;  some recursion
;
(defun factorial (x)
  (if (equal x 1)
      1
    (* x (factorial (- x 1)))))

(check (equal (factorial 1) 1))
(check (equal (factorial 2) 2))
(check (equal (factorial 3) 6))
(check (equal (factorial 4) 24))
(check (equal (factorial 5) 120))


;
; quotes, commas, comma-at, backquotes syntax
;
(check (equal 1 `,1))
(check (equal '(1 2) `(1 ,(+ 1 1))))
(check (equal '(1 (2 3) 4) `(1 ,(list (+ 1 1) (+ 1 1 1)) 4)))
(check (equal '(1 2 3 4) `(1 ,@(list (+ 1 1) (+ 1 1 1)) 4)))

(check (equal ``1 1))

(check (equal 1 1))
(check (equal '1 1))
(check (equal ''1 ''1))
(check (equal '''1 '''1))

(check (equal 1 1))
(check (equal `1 1))
(check (equal ``1 ``1))
(check (equal ```1 ```1))

(check (equal (eval '(+ 1 1)) 2))
(check (equal (eval '(+ 1 1)) `2))

;
; macros 101
;
(defvar *b* 7)
(defun    foofun (x) `(list ,x))
(check (equal (foofun *b*) '(list 7)))

(defmacro foomac (x) `(list ,x))
(foomac *b*)

(check (equal (foomac *b*) '(7)))

;
; simple lambdas
;
(check (equal ((lambda (x) (+ x 1)) 1) 2))
(check (equal ((lambda (x) (* x x)) 7) 49))

;
; lambda passed to function
;

(defun twice (fn x)
  (funcall fn (funcall fn x)))

(check (equal (twice (lambda (n) (+ n 1)) 1) 3))
(check (equal (twice (lambda (n) (* n n)) 2) 16))

;
; let
;
(setf x 'outer)
(check (equal x 'outer))
(let ((x 'inner)) 
  (check (equal x 'inner)))
(check (equal x 'outer))

;
; closure!
;

(setf closure
      (let ((y 0))
	(lambda (x) 
	  (setf y (+ y 1))
	  (+ x y))))

(check (equal (funcall closure 1) 2))
(check (equal (funcall closure 1) 3))
(check (equal (funcall closure 1) 4))
(setf y 13)
(check (equal (funcall closure 1) 5))

;
; messy result display
;
(print "passes:")
(print passes)
(print "failures:")
(print failures)

