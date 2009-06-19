(defvar passes 0)
(defvar failures 0)
(defun fail () (setf failures (+ 1 failures)) 'fail)
(defun pass () (setf passes (+ 1 passes)) 'pass)
(defun test (what) (if what (pass) (fail)))
(test (equal 1 1))



(print "passes:")
passes
(print "failures:")
failures
