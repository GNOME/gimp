;   test vector methods of TS



;                     make-vector

; make-vector succeeds
(assert '(make-vector 25))
; Note vector is anonymous and will be garbage collected

; make-vector of size 0 succeeds
(assert '(make-vector 0))

(define testVector (make-vector 25))

; make-vector yields a vector
(assert `(vector? ,testVector))

; make-vector yields a vector of given length
(assert `(= (vector-length ,testVector)
            25))

; make-vector initializes each element to empty list
(assert `(equal?
            (vector-ref ,testVector 0)
            '()))


;                   other vector construction methods

(assert '(equal?
            (vector 'a 'b 'c)
            #(a b c)))

(assert '(equal?
            (list->vector '(dididit dah))
            #(dididit dah)))


;                   fill

; fill succeeds
(assert `(vector-fill! ,testVector 99))

; fill effective
(assert `(=
            (vector-ref ,testVector 0)
            99))


;                  referencing out of bounds

; past end fails
(assert-error `(vector-ref ,testVector 25)
              "vector-ref: out of bounds:")
; error msg omits repr of atom

; negative index fails
(assert-error `(vector-ref ,testVector -1)
              "vector-ref: argument 2 must be: non-negative integer")




;                  undefined vector ops in TS

; make-initialized-vector
(assert-error '(equal?
                  (make-initialized-vector 5 (lambda (x) (* x x)))
                  #(0 1 4 9 16))
              "eval: unbound variable:")
; error msg omits prefix "Error: " and suffix "make-initialized-vector"

; vector-copy
; equals the original
(assert-error
         `(equal?
            (vector-copy  ,testVector)
            ,testVector)
         "eval: unbound variable:")





