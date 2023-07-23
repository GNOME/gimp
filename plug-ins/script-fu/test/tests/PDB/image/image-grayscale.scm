; test Image of mode grayscale methods of PDB


; depends on fresh GIMP state
; !!!! tests hardcode image ID 5

; !!! Note inconsistent use in GIMP of GRAY versus GRAYSCALE



;              Basic grayscale tests


; method new from fresh GIMP state returns ID 5
(assert '(=
           (car (gimp-image-new 21 22 RGB))
           5))

; method gimp-image-convert-grayscale does not error
(assert '(gimp-image-convert-grayscale 5))

; conversion was effective:
; basetype of grayscale is GRAY
(assert '(=
            (car (gimp-image-get-base-type 5))
            GRAY))

; conversion was effective:
; grayscale image has-a colormap
; colormap is-a vector of length zero, when image has no drawable.
; FIXME doc says num-bytes is returned, obsolete since GBytes
(assert '(=
            (vector-length
              (car (gimp-image-get-colormap 5)))
            0))

; grayscale images have precision PRECISION-U8-NON-LINEAR
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert '(=
           (car (gimp-image-get-precision 5))
           PRECISION-U8-NON-LINEAR ))

; TODO
; drawable of grayscale image is also grayscale
;(assert '(car (gimp-drawable-is-grayscale
;                 ()
;               5)

; convert precision of grayscale image succeeds
(assert '(gimp-image-convert-precision
            5
            PRECISION-DOUBLE-GAMMA))




