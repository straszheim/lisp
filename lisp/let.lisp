;
;  Should print outer, inner, outer
;
(setf x 'outer)
(print x)
(let ((x 'inner)) 
  (print x))
(print x)
