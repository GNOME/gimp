; test Image precision methods of PDB

; Using numeric equality operator '=' on numeric ID's

(script-fu-use-v3)


;             setup

(define testImage (gimp-image-new 21 22 RGB))




(test! "new image has PRECISION-U8-NON-LINEAR")

(assert `(= (gimp-image-get-precision ,testImage)
            PRECISION-U8-NON-LINEAR ))



(test! "Convert precision")

; method convert-precision yields true, with side effect on image
(assert `(gimp-image-convert-precision
                ,testImage
                PRECISION-U8-LINEAR))


; Effective: converted image is the precision
(assert `(= (gimp-image-get-precision ,testImage)
            PRECISION-U8-LINEAR))

; converting to the same precision yields error message
(assert-error `(gimp-image-convert-precision
                  ,testImage
                  PRECISION-U8-LINEAR)
              "Procedure execution of gimp-image-convert-precision failed on invalid input arguments: ")
              ; "Image '[Untitled]' (2) must not be of precision 'u8-linear'"



;       Indexed images precision tested elsewhere



(test! "New with precision")

(define testImageWithPrecision (gimp-image-new-with-precision 21 22 RGB PRECISION-U8-PERCEPTUAL))

; Effective: image has given precision
(assert `(= (gimp-image-get-precision ,testImageWithPrecision)
            PRECISION-U8-PERCEPTUAL))




(test! "conversions to all the precisions")

(define (testConvertPrecision precision)
  (assert `(gimp-image-convert-precision
                ,testImage
                ,precision)))

; Require testImage precision is not first precision in list:
; it will fail if image already that precision.

(define allPrecisions
  `(
    ; skip because testImage already is PRECISION-U8-LINEAR
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
    PRECISION-DOUBLE-PERCEPTUAL))

; GAMMA obsolete since 3.0
; PRECISION-U8-GAMMA
; PRECISION-U16-GAMMA
; PRECISION-U32-GAMMA
; PRECISION-HALF-GAMMA
; PRECISION-FLOAT-GAMMA
; PRECISION-DOUBLE-GAMMA

; sequence through all precisions, converting testImage to them
(for-each
  testConvertPrecision
  allPrecisions)

(script-fu-use-v2)