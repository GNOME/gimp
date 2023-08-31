; test Image of mode grayscale methods of PDB

; !!! Note inconsistent use in GIMP of GRAY versus GRAYSCALE



;              Basic grayscale tests


;              setup

(define testImage (car (gimp-image-new 21 22 RGB)))




; method gimp-image-convert-grayscale does not error
(assert `(gimp-image-convert-grayscale ,testImage))

; conversion was effective:
; basetype of grayscale is GRAY
(assert `(=
            (car (gimp-image-get-base-type ,testImage))
            GRAY))

; conversion was effective:
; grayscale image has-a colormap
; colormap is-a vector of length zero, when image has no drawable.
; FIXME doc says num-bytes is returned, obsolete since GBytes
(assert `(=
            (vector-length
              (car (gimp-image-get-colormap ,testImage)))
            0))

; grayscale images have precision PRECISION-U8-NON-LINEAR
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert `(=
           (car (gimp-image-get-precision ,testImage))
           PRECISION-U8-NON-LINEAR ))

; TODO
; drawable of grayscale image is also grayscale
;(assert `(car (gimp-drawable-is-grayscale
;                 ()
;               ,testImage)

; convert precision of grayscale image succeeds
(assert `(gimp-image-convert-precision
            ,testImage
            PRECISION-DOUBLE-GAMMA))




