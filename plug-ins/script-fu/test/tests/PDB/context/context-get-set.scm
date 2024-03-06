; test getters and setters of GimpContext
; (sic its not an object or class)

; see context-stack.scm for context push pop
; see paint/paint-methods for context-list-paint-methods

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



;                        color

; On clean install, foreground is black
(assert `(equal? (car (gimp-context-get-foreground))
            '(0 0 0)))

; On clean install, background is white
(assert `(equal? (car (gimp-context-get-background))
            '(255 255 255)))

; swap
(assert `(gimp-context-swap-colors))

; swap effective, foreground now white
(assert `(equal? (car (gimp-context-get-foreground))
            '(255 255 255)))

; can set foreground color by name
(assert `(gimp-context-set-foreground "red"))
; effective
(assert `(equal? (car (gimp-context-get-foreground))
            '(255 0 0)))

; can set foreground by tuple
(assert `(gimp-context-set-foreground '(0 255 0)))
; effective
(assert `(equal? (car (gimp-context-get-foreground))
            '(0 255 0)))

; invalid tuple: one too many components
; alpha is not a component
(assert-error `(gimp-context-set-foreground '(0 255 0 1))
              "in script, expected type: color list of numeric components for argument 1 to gimp-context-set-foreground")

; Test this last so it cleans up
; set to default
(assert `(gimp-context-set-default-colors))

; effective, foreground is back to black
(assert `(equal? (car (gimp-context-get-foreground))
            '(0 0 0)))



;                   gradient blend color space

; default, after a clean install
(assert `(equal? (car (gimp-context-get-gradient-blend-color-space))
             GRADIENT-BLEND-RGB-PERCEPTUAL))

; set
(assert `(gimp-context-set-gradient-blend-color-space GRADIENT-BLEND-RGB-LINEAR))
; set effective
(assert `(equal? (car (gimp-context-get-gradient-blend-color-space))
             GRADIENT-BLEND-RGB-LINEAR))

; clean up after test, restore to default
(assert `(gimp-context-set-gradient-blend-color-space GRADIENT-BLEND-RGB-PERCEPTUAL))

