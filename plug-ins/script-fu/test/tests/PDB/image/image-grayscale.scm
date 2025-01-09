; test Image of mode grayscale methods of PDB

; !!! Note inconsistent use in GIMP of GRAY versus GRAYSCALE


(script-fu-use-v3)


;              setup

(define testImage (testing:load-test-image-basic-v3))



(test! "gimp-image-convert-grayscale")
(assert `(gimp-image-convert-grayscale ,testImage))

; conversion was effective:
; basetype of grayscale is GRAY
(assert `(= (gimp-image-get-base-type ,testImage)
            GRAY))

; conversion was effective:
; grayscale image has-a colormap
(assert `(gimp-image-get-palette ,testImage))

(test! "grayscale images have precision PRECISION-U8-NON-LINEAR")
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert `(= (gimp-image-get-precision ,testImage)
            PRECISION-U8-NON-LINEAR ))

(test! "drawable of grayscale image is also grayscale")
(assert `(gimp-drawable-is-gray
           (gimp-image-get-layer-by-name ,testImage "Background")))

; convert precision of grayscale image succeeds
(assert `(gimp-image-convert-precision
            ,testImage
            PRECISION-U8-PERCEPTUAL))

(script-fu-use-v2)


