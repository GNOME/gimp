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
; in v3 it returns a list of the components

; This documents ScriptFu at a version.
; It documents what IS and not the ideal design.

; See also test color.scm, which this duplicates


(script-fu-use-v3)



; Setup
; gimp-logo.png
(define testImage (testing:load-test-image-basic-v3))
; image has one layer
(define testDrawable (vector-ref (gimp-image-get-layers testImage) 0))

(define testImageGray (testing:load-test-image-basic-v3))
(gimp-image-convert-grayscale testImageGray)
(define testDrawableGray (vector-ref (gimp-image-get-layers testImageGray) 0))

(define testImageIndexed (testing:load-test-image-basic-v3))
(gimp-image-convert-indexed
                  testImageIndexed
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  4  ; color count
                  1  ; alpha-dither.  FUTURE: #t
                  1  ; remove-unused. FUTURE: #t
                  "myPalette" ; ignored
                  )
(define testDrawableIndexed (vector-ref (gimp-image-get-layers testImageIndexed) 0))



(test! "get-pixel of RGBA image")

; returned pixel of image of mode RGBA has alpha component
; Test is fragile to chosen testImage.
; Formerly: (71 71 71)
(assert `(equal? (gimp-drawable-get-pixel ,testDrawable 1 1)
                 '(0 0 0 0)))

(test! "set-pixel of RGBA image from a 3 component list.")
; ScriptFu sets alpha to opaque.
(assert `(gimp-drawable-set-pixel ,testDrawable 1 1 '(2 2 2)))
; effective
(assert `(equal? (gimp-drawable-get-pixel ,testDrawable 1 1)
                 '(2 2 2 255)))



; RGB TODO



(test! "get-pixel of GRAYA image")

; returned pixel of image of mode GRAYA has two component
; black transparent
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableGray 1 1)
                 '(0 0)))

; Can set a pixel in GRAYA image from a 3 component list.
; Extra list elements are ignored
(assert `(gimp-drawable-set-pixel ,testDrawableGray 1 1 '(2 2 2)))
; effective
; FIXME why isn't it (2 2) ????
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableGray 1 1)
                 '(2 255)))



; GRAY TODO


(test! "get-pixel of INDEXED A image ")

; pixel of image of mode INDEXED A has four components
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableIndexed 1 1)
                '(19 18 17 0)))
; FIXME the results seem strange, should be (0 0 0 0)?
(display (gimp-drawable-get-pixel testDrawableIndexed 1 1))
; Sometimes???  '(71 71 71 0)))


(script-fu-use-v2)
