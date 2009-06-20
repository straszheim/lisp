(defvar passes 0)
(defvar failures 0)

(defun fail () 
  (setf failures (+ 1 failures)) 
  'fail)

(defun pass () 
  (setf passes (+ 1 passes)) 
  'pass)

(defun test (what) 
  (if what (pass) (fail)))

(test (equal 1 1))
(test (equal (+ 1 2) (+ 2 1)))

(defvar *x* 17)
(setf x 17)
(test (equal x *x*))

(defun f (x) (+ x x))
(defun g (y) (+ y y))
(test (equal (f 17) (g (+ 1 16))))

(defun h (x)
  (setf k 17)
  (+ x k))

(setf k 99)

(test (equal (h 18) (+ 17 18)))

(defun i (x)
  (+ x k))

(print (i 2))

(test (equal (- 1 1) 0))
(test (equal (- 2 1) 1))
(test (equal (- 100 1) 99))


(test (equal (i 2) (+ 99 2)))

(defun factorial (x)
  (print x)
  (if (equal x 1)
      1
    (* x (factorial (- x 1)))))

'(equal (factorial 1) 1)
'(equal (factorial 2) 2)
'(equal (factorial 3) 6)
'(equal (factorial 4) 24)
'(equal (factorial 5) 120)



(print "passes:")
(print passes)
(print "failures:")
(print failures)