;
; This is the init file for Tom's Lisp, a small lisp created for
; demonstration purposes.
;
; Copyright 2003, Thomas W Bennet
; 
; Tom's Lisp is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by the
; Free Software Foundation; either version 2 of the License, or (at your
; option) any later version.
; 
; Tom's Lisp is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with the submission system distribution (see the file
; COPYING); if not, write to the Free Software Foundation, Inc., 59
; Temple Place, Suite 330, Boston, MA 02111-1307 USA

; Making life a little easier.
(set 'list (lambda lis lis))

; Setq.
(set 'setq 
    (macro (n v) 
	(list 'set (list 'quote n) v)
    )
)

; Function define.
(setq define
    (macro (prot bod)
	(cond
	    ((id? prot) (list 'setq prot bod))
	    (#t
		(list 'setq (car prot)
	    		(list 'lambda (cdr prot) bod)))
	)
    )
)

; General conveniences.
(define (quit) (exit 0))
(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))
(define (caaar x) (car (car (car x))))
(define (caadr x) (car (car (cdr x))))
(define (cadar x) (car (cdr (car x))))
(define (caddr x) (car (cdr (cdr x))))
(define (cdaar x) (cdr (car (car x))))
(define (cdadr x) (cdr (car (cdr x))))
(define (cddar x) (cdr (cdr (car x))))
(define (cdddr x) (cdr (cdr (cdr x))))
(define (println x) (begin (print x) (sprint "\n") x))
(setq #f nil)

; The usual if construct, (if test true) or (if test true false)
(setq if (macro form 
    (list 'cond (list (car form) (cadr form))
    		(cond ((cddr form) (list #t (caddr form))) ())  )
))

; The or operation is builtin.  It returns the first non-nil thing in 
; the list.  Here's some others, though they don't return one of the
; items in the list.

; The not.
(define (not x) (if x () #t))

; The not and.  This is a macro that turns (or x y z) into 
; (and (not x) (not y) (not z))
(setq nand (macro andl
    (cons 'or (map (lambda (dis) (list 'not dis)) andl))
))

; And invert it for regular and.
(setq and (macro andl (list 'not (cons 'nand andl))))

; Can't forget this.
(setq nor (macro orlis (list 'not (cons 'or orlis))))

; Is this the kind of thing we can call?
(define (functional? x) 
    (or (builtin? x) (lambda? x) (macro? x))
)

; The standard let operation, implemented with sets in a scope.  The input
;   (let ((var1 val2) ... (varn valn)) form) is 
; converted to
;   (scope (setq var1 val2) ... (setq varn valn)) form) 
; by a macro, which result is then evaluated by the macro semantics.
;
; This definition uses the (scope) construct to define the helper function
; listcon which is not defined in the global scope.  The (scope) construct
; evaluates to its last form, which is the let macro itself.  When it is
; run, it runs in the scope created and has access to listcon, even though
; it will is undefined anywhere else.
(setq let 
    (scope
        ; This does most of the work.  It converts the list of var val
	; pairs from lis to a series of setq's, then tacks (form) onto
	; the end.  That is (listcon '((a1 v1) (a2 v2)) fred) becomes
	; ((setq a1 v1) (setq a2 v2) fred).  The main macro just sticks
	; a 'scope at the front, and it's ready to go.
        (define (listcon lis form)
	    (if (null? lis) (list form)
	        (cons 
		    (cons 'setq (car lis))
		    (listcon (cdr lis) form)
	        )
	    )
        )

	; Use listcon to rearrange stuff, then complete the scope
	; expression.
        (macro form (cons 'scope (listcon (car form) (cadr form))))
    )
)

; Basic list operations.
(define (length lis)
    (cond
	((null? lis) 0)
	(#t (+ 1 (length (cdr lis))))
    )
)

; The first n items in a list (or the whole list).
(define (first n lis)
    (if (or (null? lis) (<= n 0))
	()
	(cons (car lis) (first (- n 1) (cdr lis)))
    )
)

; Remove the first n items (or remove the whole list).
(define (first-but n lis)
    (if (or (null? lis) (<= n 0)) 
	lis 
	(first-but (- n 1) (cdr lis))
    )
)

; Last n
(define (last n lis)
    (let ((len (length lis))) (first-but (- len n) lis))
)

; First except
(define (last-but n lis)
    (let ((len (length lis))) (first (- len n) lis))
)

; The nth item, counting from zero.
(define (nth n lis)
    (if (< n 0) ()
	(let ((rest (first-but n lis))) (if (null? rest) nil (car rest)))
    )
)

; I again use the (scope) construct to make names for helper functions
; while building append which will not be entered into the larger scope.
; Here I didn't want to make the main definition the last one, so I 
; set it to a name, and make the name the last form in the scope list
; so it will be the value of the scope.
(setq append (scope
    ; Two-list append.
    (define (app2 x y) 
        (cond ((null? x) y)
	      (#t (cons (car x) (app2 (cdr x) y)))
        )
    )

    (setq append (lambda x (appendr x)))
    (define (appendr lists)
        (cond
	    ((null? lists) ())
	    ((null? (car lists)) (appendr (cdr lists)))
	    (#t (app2 (car lists) (appendr (cdr lists))))
        )
    )

    append
))

; Map a unary function and return a list of the results.
(define (map f lis)
    (if (null? lis) lis
	(cons (f (car lis)) (map f (cdr lis)))
    )
)

; Reduce a list with a binary function and a starting value.
(define (reduce f lis go)
    (if (null? lis) go
	(reduce f (cdr lis) (f go (car lis)))
    )
)

; Apply the function to the args.  
(define (apply f args) (eval (cons f args)))

; Select the members of the list approved by the function.
(define (select f lis)
    (cond ((null? lis) lis)
	  ((f (car lis)) (cons (car lis) (select f (cdr lis))))
	  (#t (select f (cdr lis)))
    )
)

; String concat (pretty simple)
(setq concat (lambda x (collect x)))

; Length-based substring function.
(define (substr str start len)
    (collect (first len (last-but start (shatter str))))
)

; Scheme-like substring function
(define (substring str start end)
    (substr str start (- end start))
)

; Scheme names.
(setq string-append concat)
(setq string-length strlen)

; Allocate a new error code with the given identifier: expects an id atom.
(setq errcode (macro (id)
    (list 'begin 
	(list 'setq id 'NEXT_ERR)
	(list 'setq 'NEXT_ERR (+ NEXT_ERR 1))
	NEXT_ERR
    )
))

; A general purpose user error
(errcode ERR_MISC)
(define (boom msg) (error ERR_MISC msg))
(errcode ERR_ABORT)
(define (abort) (error ERR_ABORT "Aborted"))

; Run the code and tell if it succeeds or fails in terms of throwing.
; The content of any error is lost.
(setq succeeds (macro code (list 'car (cons 'catch code))))
(setq fails (macro code (list 'not (cons 'succeeds code))))

; A try block.  (try expr (code1 expr1) ... (coden exprn) [ (#t exprd) ] )
; If expr throws, the expri or exprd is run.  Otherwise, the result of
; expr is returned.  The code1 is usually an atomic integer error code
; number, in which case it is compared to the caught code.  If it is and
; arbitrary expression, it is run with ERROR and MESSAGE set to the code
; and text of the error, and catches if true.
; Complicated try structure, like:
;   (try expr (code1 expr1) ... (coden exprn) [ (#t expr) ] )
;
; Which is translated by the macro to:
;
;   (scope (setq (ERROR (catch expr)))
;	   (if (car ERROR)
;		(cdr ERROR)
;		(begin
;		    (setq MESSAGE (cddr ERROR))
;		    (setq ERROR (cdar ERROR))
;		    (cond
;			...
;			(transi expri)
;			...
;			(#t expr)
;			...
;			(#t (error ERROR MESSAGE))
;		    )
;		)
;          )
;    )
;
; Where the transi is just the codei if it is a pair, or (equal? ERROR)
; if it is an atom other than #t.
;
(setq try (scope
    ; This takes care of a single case.
    (define (do-case case)
	(cond
	    ; The null.  This is for the default.
	    ((equal? #t (car case)) case)

	    ; Single atom.  Compare to ERROR.
	    ((not (pair? (car case)))
		(list (list '= 'ERROR (car case)) (cadr case)))

	    ; Finally, an arbitrary test with ERROR.
	    (#t case)
	)
    )

    ; Create the scope form to evaluate.  Uses the formconv for most of
    ; the work.
    (macro frm (list 'scope
	; Run the expr, catch the result, and set result to that value.
	(list 'setq 'ERROR (list 'catch (car frm)))

	; See if it succeeded ... 
	(list 'if '(car ERROR) 
	    ; Yes.  Return the result.
	    '(cdr ERROR)

	    ; No.  Figure out what to do.  Local helper generates this.
	    (list 'begin 
		'(setq MESSAGE (cddr ERROR))
		'(setq ERROR (cadr ERROR))
		(cons 'cond (append (map do-case (cdr frm))
			'((#t (error ERROR MESSAGE))))
		)
	    )
	)
    ))
))
