(setf closure
      (let ((y 1))
	(lambda (x) 
	  (setf y (+ y 1))
	  (+ x y))))
(closure 1)
(closure 1)
(closure 1)



