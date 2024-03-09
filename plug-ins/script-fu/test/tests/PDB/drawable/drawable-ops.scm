; test op methods of drawable

; operations change pixels of the drawable without reference to other objects,
; or with passed non-drawable args such as curves



; setup

(define testImage (testing:load-test-image "gimp-logo.png"))
; Wilber has one layer
; cadr is vector, first element is a drawable
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage)) 0))



; tests in alphabetic order

(assert `(gimp-drawable-brightness-contrast ,testDrawable 0.1 -0.1))

(assert `(gimp-drawable-color-balance ,testDrawable TRANSFER-MIDTONES 1 0.1 0.1 0.1))

(assert `(gimp-drawable-colorize-hsl ,testDrawable 360 50 -50))

; TODO requires vector of size 256
; (assert `(gimp-drawable-curves-explicit ,testDrawable HISTOGRAM-RED 2 #(1 2)))

;(assert `(gimp-drawable-curves-spline ,testDrawable DESATURATE-LUMA))

(assert `(gimp-drawable-desaturate ,testDrawable DESATURATE-LUMA))

(assert `(gimp-drawable-equalize ,testDrawable 1)) ; boolean mask-only

;(assert `(gimp-drawable-extract-component ,testDrawable DESATURATE-LUMA))

; FIXME crashes
;(assert `(gimp-drawable-fill ,testDrawable FILL-CIELAB-MIDDLE-GRAY))

(assert `(gimp-drawable-foreground-extract ,testDrawable FOREGROUND-EXTRACT-MATTING ,testDrawable))

(assert `(gimp-drawable-hue-saturation ,testDrawable HUE-RANGE-MAGENTA 0 1 2 3))

(assert `(gimp-drawable-invert ,testDrawable 1)) ; boolean invert in linear space

(assert `(gimp-drawable-levels
            ,testDrawable
            HISTOGRAM-LUMINANCE
            0.5 0.5 1 ; boolean clamp input
            8 0.5 0.5 1 ; boolean clamp output
            ))

(assert `(gimp-drawable-levels-stretch ,testDrawable))

(assert `(gimp-drawable-posterize ,testDrawable 2))

(assert `(gimp-drawable-shadows-highlights
             ,testDrawable
             -50 50
             -10
             1300
             50
             0 100))

(assert `(gimp-drawable-threshold
            ,testDrawable
            HISTOGRAM-ALPHA
            0.1 1))

(assert `(gimp-drawable-desaturate ,testDrawable DESATURATE-LUMA))
(assert `(gimp-drawable-desaturate ,testDrawable DESATURATE-LUMA))

(gimp-display-new testImage)

