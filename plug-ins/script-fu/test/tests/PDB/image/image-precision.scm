; test Image precision methods of PDB


; depends on fresh GIMP state
; !!!! tests hardcode image ID 2

; Using numeric equality operator '=' on numeric ID's


;              Basic precision tests


; method new from fresh GIMP state returns ID 2
(assert '(=
           (car (gimp-image-new 21 22 RGB))
           2 ))

; method get_precision on new image yields PRECISION-U8-NON-LINEAR  150
(assert '(=
           (car (gimp-image-get-precision 2))
           PRECISION-U8-NON-LINEAR ))



;            Convert precision

; method convert-precision yields true, with side effect on image
(assert '(car (gimp-image-convert-precision
                2
                PRECISION-U8-LINEAR)))


; converted image is the precision
(assert '(=
           (car (gimp-image-get-precision 2))
           PRECISION-U8-LINEAR ))

; converting to the same precision yields error message
(assert-error '(gimp-image-convert-precision
                  2
                  PRECISION-U8-LINEAR)
"Procedure execution of gimp-image-convert-precision failed on invalid input arguments: Image '[Untitled]' (2) must not be of precision 'u8-linear'")



;       Indexed images precision tested elsewhere



;       New with precision

; method new-with-precision from fresh GIMP state returns ID 3
(assert '(=
           (car (gimp-image-new-with-precision 21 22 RGB PRECISION-DOUBLE-GAMMA))
           3 ))

; image has given precision
(assert '(=
           (car (gimp-image-get-precision 3))
           PRECISION-DOUBLE-GAMMA ))