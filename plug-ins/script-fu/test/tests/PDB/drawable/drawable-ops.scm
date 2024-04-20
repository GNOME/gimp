; test op methods of drawable

; operations change pixels of the drawable without reference to other objects,
; or with passed non-drawable args such as curves

; So that #t binds to boolean arg to PDB
(script-fu-use-v3)

; setup

(define testImage (testing:load-test-image-basic-v3))
; Wilber has one layer

; cadr is vector, first element is a drawable
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage)) 0))


(test! "drawable operations")

; tests in alphabetic order

(assert `(gimp-drawable-brightness-contrast ,testDrawable 0.1 -0.1))

(assert `(gimp-drawable-color-balance ,testDrawable TRANSFER-MIDTONES 1 0.1 0.1 0.1))

(assert `(gimp-drawable-colorize-hsl ,testDrawable 360 50 -50))

; requires vector of size 256 of floats
(assert `(gimp-drawable-curves-explicit ,testDrawable HISTOGRAM-RED
      256 (make-vector 256 1.0)))

; two pairs of float control points of a spline, four floats in total
(assert `(gimp-drawable-curves-spline ,testDrawable HISTOGRAM-RED 4 #(0 0 25.0 25.0) ))

(assert `(gimp-drawable-desaturate ,testDrawable DESATURATE-LUMA))

(assert `(gimp-drawable-equalize ,testDrawable 1)) ; boolean mask-only

(assert `(gimp-drawable-extract-component ,testDrawable SELECT-CRITERION-HSV-SATURATION #t #t))

(assert `(gimp-drawable-fill ,testDrawable FILL-CIELAB-MIDDLE-GRAY))

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

(testing:show testImage)

(script-fu-use-v2)