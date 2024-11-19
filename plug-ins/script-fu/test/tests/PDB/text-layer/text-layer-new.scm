; tests of TextLayer class

; !!! Some methods tested here are named strangely:
; text-font returns a new TextLayer




;                  setup

; Require image has no layer
(define testImage (car (gimp-image-new 21 22 RGB)))

(define testFont (car (gimp-context-get-font)))

; setup (not an assert )
(define
    testTextLayer
       (car (gimp-text-layer-new
              testImage
              "textOfTestTextLayer" ; text
              testFont ; font
              30 ; fontsize
              UNIT-PIXEL)))


; TOTO test if font is not valid or NULL


; !!! UNIT-PIXEL GimpUnitsType is distinct from PIXELS GimpSizeType


; TODO test UNIT-POINT


; is-a TextLayer
(assert `(= (car (gimp-item-id-is-text-layer ,testTextLayer))
            1))

; text layer is not in image yet
(assert `(= (car (gimp-image-get-layers ,testImage))
            0))

; adding layer to image succeeds
(assert `(gimp-image-insert-layer
            ,testImage
            ,testTextLayer ; layer
            0  ; parent
            0  ))  ; position within parent




;             attributes

; antialias default true
; FIXME doc says false
(assert `(= (car (gimp-text-layer-get-antialias ,testTextLayer))
            1))

; base-direction default TEXT-DIRECTION-LTR
(assert `(= (car (gimp-text-layer-get-base-direction ,testTextLayer))
            TEXT-DIRECTION-LTR))

; language default "C"
(assert `(string=? (car (gimp-text-layer-get-language ,testTextLayer))
                    "C"))

; TODO other attributes

; TODO setters effective

;            attributes as given

; text
(assert `(string=? (car (gimp-text-layer-get-text ,testTextLayer))
                        "textOfTestTextLayer"))
; font, numeric ID's equal
(assert `(= (car (gimp-text-layer-get-font ,testTextLayer))
            ,testFont))
; font-size
(assert `(= (car (gimp-text-layer-get-font-size ,testTextLayer))
            30))

; is no method to get fontSize unit


;              misc ops

; path from text succeeds
(assert `(gimp-path-new-from-text-layer
              ,testImage
              ,testTextLayer))
; not capturing returned ID of path




;                  misc method

; gimp-text-get-extents-font
; Yields extent of rendered text, independent of image or layer.
; Extent is (width, height, ascent, descent) in unstated units, pixels?
; Does not affect image.
(assert `(= (car (gimp-text-get-extents-font
              "zed" ; text
              32    ; fontsize
              ,testFont ))
            53))
; usual result is (57 38 30 -8)
; recent result is (53 45 35 10) ??? something changed in cairo?



;           alternate method for creating text layer


; gimp-text-font creates text layer AND inserts it into image
; setup, not assert
(define
  testTextLayer2
   (car (gimp-text-font
              testImage
              -1     ; drawable.  -1 means NULL means create new text layer
              0 0   ; coords
              "bar" ; text
              1     ; border size
              1     ; antialias true
              31    ; fontsize
              testFont )))


; error to insert layer created by gimp-text-font
(assert-error `(gimp-image-insert-layer
                  ,testImage
                  ,testTextLayer2
                  0  ; parent
                  0  )  ; position within parent
              "Procedure execution of gimp-image-insert-layer failed on invalid input arguments: ")
              ;  "Item 'bar' (17) has already been added to an image"



; for debugging: display
;(gimp-display-new ,testImage)
