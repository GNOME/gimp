; test Image precision methods of PDB

; Using numeric equality operator '=' on numeric ID's


;             setup

(define testImage (car (gimp-image-new 21 22 RGB)))


;              Basic precision tests

; method get_precision on new image yields PRECISION-U8-NON-LINEAR  150
(assert `(=
           (car (gimp-image-get-precision ,testImage))
            PRECISION-U8-NON-LINEAR ))



;            Convert precision

; method convert-precision yields true, with side effect on image
(assert `(car (gimp-image-convert-precision
                ,testImage
                PRECISION-U8-LINEAR)))


; converted image is the precision
(assert `(=
           (car (gimp-image-get-precision ,testImage))
           PRECISION-U8-LINEAR ))

; converting to the same precision yields error message
(assert-error `(gimp-image-convert-precision
                  ,testImage
                  PRECISION-U8-LINEAR)
              "Procedure execution of gimp-image-convert-precision failed on invalid input arguments: ")
              ; "Image '[Untitled]' (2) must not be of precision 'u8-linear'"



;       Indexed images precision tested elsewhere



;       New with precision

;         setup
(define testImageWithPrecision (car (gimp-image-new-with-precision 21 22 RGB PRECISION-DOUBLE-GAMMA)))


; image has given precision
(assert `(=
           (car (gimp-image-get-precision ,testImageWithPrecision))
           PRECISION-DOUBLE-GAMMA ))



; test conversions to all the precisions

(define (testConvertPrecision precision)
  (assert `(car (gimp-image-convert-precision
                ,testImage
                ,precision))))

; First convert away from first precision in list:
; it will fail if image already that precision
(gimp-image-convert-precision testImage PRECISION-U8-NON-LINEAR)

(define allPrecisions
  `(PRECISION-U8-LINEAR
    PRECISION-U8-NON-LINEAR
    PRECISION-U8-PERCEPTUAL
    PRECISION-U16-LINEAR
    PRECISION-U16-NON-LINEAR
    PRECISION-U16-PERCEPTUAL
    PRECISION-U32-LINEAR
    PRECISION-U32-NON-LINEAR
    PRECISION-U32-PERCEPTUAL
    PRECISION-HALF-LINEAR
    PRECISION-HALF-NON-LINEAR
    PRECISION-HALF-PERCEPTUAL
    PRECISION-FLOAT-LINEAR
    PRECISION-FLOAT-NON-LINEAR
    PRECISION-FLOAT-PERCEPTUAL
    PRECISION-DOUBLE-LINEAR
    PRECISION-DOUBLE-NON-LINEAR
    PRECISION-DOUBLE-PERCEPTUAL
    PRECISION-U8-GAMMA
    PRECISION-U16-GAMMA
    PRECISION-U32-GAMMA
    PRECISION-HALF-GAMMA
    PRECISION-FLOAT-GAMMA
    PRECISION-DOUBLE-GAMMA))

; sequence through all precisions, converting testImage to them
(for-each
  testConvertPrecision
  allPrecisions)