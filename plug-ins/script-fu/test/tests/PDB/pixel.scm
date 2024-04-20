; Test pixel methods of the PDB

; There are only two methods, set-pixel and get-pixel

; A Pixel is a set of intensity numeric values, one per channel.
; A Color describes a perception.
; Pixel and Color are not the same.

; A Pixel is implementation that intends a Color.
; The actual perception of a Pixel may not match the intended Color.
; For example, when the Pixel is viewed on paper or a different monitor.
; I.E. when the rest of the implementation fails to make a human see the intent.

; ScriptFu does not support all the methods of Color.
; For PDB methods that take/return Color, ScriptFu converts Color to Pixel.
; With loss of capability.

; in v2 get-pixel returned a count of components and a vector of components
; in v3 it returns a list of the component intensities

; This documents ScriptFu at a version.
; It documents what IS and not the ideal design.





; Setup
(define testImage (testing:load-test-image-basic))
; image has one layer
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage)) 0))

(define testImageGray (testing:load-test-image-basic))
(gimp-image-convert-grayscale testImageGray)
(define testDrawableGray (vector-ref (cadr (gimp-image-get-layers testImageGray)) 0))

(define testImageIndexed (testing:load-test-image-basic))
(gimp-image-convert-indexed
                  testImageIndexed
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  4  ; color count
                  1  ; alpha-dither.  FUTURE: #t
                  1  ; remove-unused. FUTURE: #t
                  "myPalette" ; ignored
                  )
(define testDrawableIndexed (vector-ref (cadr (gimp-image-get-layers testImageIndexed)) 0))



(test! "get-pixel of RGBA image")

; returned pixel of image of mode RGBA is missing alpha component
; Test is fragile to chosen testImage.
; Formerly: (71 71 71)
(assert `(equal? (car (gimp-drawable-get-pixel ,testDrawable 1 1))
                 '(0 0 0)))

(test! "set-pixel of RGBA image from a 3 component list.")
; ScriptFu sets alpha to opaque.
(assert `(gimp-drawable-set-pixel ,testDrawable 1 1 '(2 2 2)))
; effective
(assert `(equal? (car (gimp-drawable-get-pixel ,testDrawable 1 1))
                 '(2 2 2)))



; RGB TODO



(test! "get-pixel of GRAY image")

; returned pixel of image of mode GRAY has extra components
; You might think it only has one component.
(assert `(equal? (car (gimp-drawable-get-pixel ,testDrawableGray 1 1))
                 '(0 0 0)))

; Can set a pixel in GRAY image from a 3 component list.
; You might think it only takes component
(assert `(gimp-drawable-set-pixel ,testDrawableGray 1 1 '(2 2 2)))
; effective
(assert `(equal? (car (gimp-drawable-get-pixel ,testDrawableGray 1 1))
                 '(2 2 2)))



; GRAYA TODO


(test! "get-pixel of INDEXED image")

; pixel of image of mode INDEXED has extra components
; FIXME this crashes in babl_fatal
;(assert `(equal? (car (gimp-drawable-get-pixel ,testDrawableIndexed 1 1))
;                '(71 71 71)))