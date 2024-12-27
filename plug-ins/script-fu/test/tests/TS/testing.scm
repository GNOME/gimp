; test the testing framework

;           assert stmt


; a result that is #t passes
(assert #t)

; other truthy results pass
(assert 1)

; 0 is truthy and passes
(assert 0)

; If you really want to assert that exactly #t is the result,
; you should eval a topmost predicate that yields only #t or #f
; For example, where eq? is equality of pointers:
(assert '(not (eq? 0 #t)))

; a symbol defined outside an assert is visible
; when you backquote and unquote it.
(define aTrue #t)
(assert `,aTrue)

; Here
;  backquote passes the following expression as data without evaluating it
;  singlequote makes a list literal instead of a function call
;  unquote i.e. comma evaluates the following symbol before backquote passes expression as data
(assert `(car '(,aTrue)))



;           assert-error statement

; assert-error tests for error messages
; assert-error omits the "Error: " prefix printed by the REPL

; case: Error1 called with pointer to errant atom
; symbol aFalse is not bound
(assert-error 'aFalse
              "eval: unbound variable:")

; assert-error currently omits the suffix <repr of errant code>
; printed by the usual error mechanism.
; (Since I think error hook mechanism is broken.)

; case: Error0 called with null pointer
; numeric literal 1 is not a function
(assert-error '(1)
              "illegal function")


