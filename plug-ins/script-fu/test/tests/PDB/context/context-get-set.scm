; test getters and setters of GimpContext
; (sic its not an object or class)

; see context-stack.scm for context push pop
; see paint/paint-methods for context-list-paint-methods


(script-fu-use-v3)



(test! "set-line-dash-pattern")

; tests binding of FloatArray

; Default is pattern length zero
; Even if user has stroked already and chosen a stroke>line>pattern
; Returns (<len> #(<pattern>)))
(assert `(= (vector-length (gimp-context-get-line-dash-pattern))
            0))

; setter succeeds
(assert `(gimp-context-set-line-dash-pattern #(5.0 11.0)))

; setter effective, length now two
(assert `(= (vector-length (gimp-context-get-line-dash-pattern))
            2))
; and pattern is as set
(assert `(equal? (gimp-context-get-line-dash-pattern)
                  #(5.0 11.0)))


;                        get-line-dash-offset

;tests binding of float i.e. gdouble

; defaults to 0.0 until set
; FIXME why doesn't it persist in settings?
(assert `(= (gimp-context-get-line-dash-offset)
            0.0))

; setter succeeds
(assert `(gimp-context-set-line-dash-offset 3.3 ))
; setter effective
(assert `(= (gimp-context-get-line-dash-offset)
            3.3))



(test! "get set foreground background color")

; Not testing clean install, default foreground is black, background white

(assert `(gimp-context-set-foreground "black"))
(assert `(gimp-context-set-background "white"))
; foreground is black opaque
(assert `(equal? (gimp-context-get-foreground)
                 '(0 0 0 255)))
; background is white opaque
(assert `(equal? (gimp-context-get-background)
                 '(255 255 255 255)))



(test! "swap foreground with background")
(assert `(gimp-context-swap-colors))

; swap effective, foreground now white
(assert `(equal? (gimp-context-get-foreground)
                 '(255 255 255 255)))



; can set foreground color by name
(assert `(gimp-context-set-foreground "red"))
; effective
(assert `(equal? (gimp-context-get-foreground)
                 '(255 0 0 255)))

; can set foreground by tuple
(assert `(gimp-context-set-foreground '(0 255 0)))
; effective
(assert `(equal? (gimp-context-get-foreground)
                 '(0 255 0 255)))

(test! "can set foreground passing an alpha component, ineffectively")
;(assert-error `(gimp-context-set-foreground '(0 255 0 1))
;              "in script, expected type: color list of numeric components for argument 1 to gimp-context-set-foreground")
(assert `(gimp-context-set-foreground '(0 255 0 1)))
; not effective, alpha is returned but not as passed
(assert `(equal? (gimp-context-get-foreground)
                 '(0 255 0 255)))


(test! "set foreground background to default")
; Test this last so it cleans up
(assert `(gimp-context-set-default-colors))

; effective, foreground is back to black
(assert `(equal? (gimp-context-get-foreground)
                 '(0 0 0 255)))



(test! "get set gradient blend color space")

; default, after a clean install
(assert `(equal? (gimp-context-get-gradient-blend-color-space)
                  GRADIENT-BLEND-RGB-PERCEPTUAL))

; set
(assert `(gimp-context-set-gradient-blend-color-space GRADIENT-BLEND-RGB-LINEAR))
; set effective
(assert `(equal? (gimp-context-get-gradient-blend-color-space)
                 GRADIENT-BLEND-RGB-LINEAR))

; clean up after test, restore to default
(assert `(gimp-context-set-gradient-blend-color-space GRADIENT-BLEND-RGB-PERCEPTUAL))

(script-fu-use-v2)
