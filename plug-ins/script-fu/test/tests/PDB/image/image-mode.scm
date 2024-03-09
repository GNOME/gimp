; test combinations of mode conversion
; COLOR=>GRAY=>INDEXED=>GRAY

; setup
(define testImage (testing:load-test-image "gimp-logo.png"))

; test image is RGB
(assert `(= (car (gimp-image-get-base-type ,testImage))
            RGB))


; convert RGB=>GRAY
(assert `(gimp-image-convert-grayscale ,testImage))
; mode conversion was effective
(assert `(= (car (gimp-image-get-base-type ,testImage))
                 GRAY))
; effective color profile is the built-in one for GRAY
(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
            544))

; convert GRAY=>INDEXED
(assert `(gimp-image-convert-indexed
            ,testImage
            CONVERT-DITHER-NONE
            CONVERT-PALETTE-GENERATE
            25  ; color count
            1  ; alpha-dither.  FUTURE: #t
            1  ; remove-unused. FUTURE: #t
            "myPalette" ; ignored
            ))
; mode conversion was effective
(assert `(= (car (gimp-image-get-base-type ,testImage))
                 INDEXED))
; effective color profile is the built-in one for COLOR
(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
            672))

; convert INDEXED=>GRAY
(assert `(gimp-image-convert-grayscale ,testImage))
; mode conversion was effective
(assert `(= (car (gimp-image-get-base-type ,testImage))
                 GRAY))
; effective color profile is the built-in one for GRAY
(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
            544))

; convert GRAY=>COLOR
(assert `(gimp-image-convert-rgb ,testImage))
; mode conversion was effective
(assert `(= (car (gimp-image-get-base-type ,testImage))
                 RGB))
; effective color profile is the built-in one for RGB
(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
            672))



