; tests of TextLayer class

; !!! Some methods tested here are named strangely:
; text-fontname returns a new TextLayer




;                  setup

; Require image has no layer
(define testImage (car (gimp-image-new 21 22 RGB)))

; setup (not an assert )
(define
    testTextLayer
       (car (gimp-text-layer-new
              testImage
              "textOfTestTextLayer" ; text
              "fontName" ; fontname
              30 ; fontsize
              UNIT-PIXEL)))


; !!!! fontName is not valid
; The text displays anyway, using some font family, without error.
; The docs don't seem to say which font family is used.
; TODO better documentation
; The text layer still says it is using the given font family.
; TODO yield actual font family used.

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
; font
(assert `(string=? (car (gimp-text-layer-get-font ,testTextLayer))
                        "fontName"))
; font-size
(assert `(= (car (gimp-text-layer-get-font-size ,testTextLayer))
            30))

; is no method to get fontSize unit


;              misc ops

; vectors from text succeeds
(assert `(gimp-vectors-new-from-text-layer
              ,testImage
              ,testTextLayer))
; not capturing returned ID of vectors




;                  misc method

; gimp-text-get-extents-fontname
; Yields extent of rendered text, independent of image or layer.
; Extent is (width, height, ascent, descent) in unstated units, pixels?
; Does not affect image.
(assert `(= (car (gimp-text-get-extents-fontname
              "zed" ; text
              32    ; fontsize
              POINTS  ; size units.  !!! See UNIT-PIXEL
              "fontName" )) ; fontname
            57))
; usual result is (57 38 30 -8)



;           alternate method for creating text layer


; gimp-text-fontname creates text layer AND inserts it into image
; setup, not assert
(define
  testTextLayer2
   (car (gimp-text-fontname
              testImage
              -1     ; drawable.  -1 means NULL means create new text layer
              0 0   ; coords
              "bar" ; text
              1     ; border size
              1     ; antialias true
              31    ; fontsize
              PIXELS  ; size units.  !!! See UNIT-PIXEL
              "fontName" )))


; error to insert layer created by gimp-text-fontname
; TODO make the error message matching by prefix only
(assert-error `(gimp-image-insert-layer
                  ,testImage
                  ,testTextLayer2
                  0  ; parent
                  0  )  ; position within parent
              "Procedure execution of gimp-image-insert-layer failed on invalid input arguments: ")
              ;  "Item 'bar' (17) has already been added to an image"



; for debugging: display
(assert `(= (car (gimp-display-new ,testImage))
            1))