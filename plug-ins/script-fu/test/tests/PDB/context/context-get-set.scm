; test getters and setters of GimpContext
; (sic its not an object or class)



;                        set-line-dash-pattern

; tests binding of FloatArray

; Default is no pattern
; Even if user has stroked already and chosen a stroke>line>pattern
(assert `(= (car (gimp-context-get-line-dash-pattern))
            0))

; setter succeeds
(assert `(gimp-context-set-line-dash-pattern 2 #(5.0 11.0)))

; setter effective
(assert `(= (car (gimp-context-get-line-dash-pattern))
            2))
(assert `(equal? (cadr (gimp-context-get-line-dash-pattern))
                  #(5.0 11.0)))


;                        get-line-dash-offset

;tests binding of float i.e. gdouble

; defaults to 0.0 until set
; FIXME why doesn't it persist in settings?
(assert `(= (car (gimp-context-get-line-dash-offset))
            0.0))

; setter succeeds
(assert `(gimp-context-set-line-dash-offset 3.3 ))
; setter effective
(assert `(= (car (gimp-context-get-line-dash-offset))
            3.3))




