; tests of TextLayer class

; !!! Some methods tested here are named strangely:
; text-fontname returns a new TextLayer




; No setup
; Reuses image 8 from prior testing
; Require it has no layer



;             new

; new yields ID 15
(assert '(= (car (gimp-text-layer-new
              8     ; image
              "textOfTestTextLayer" ; text
              "fontName" ; fontname
              30 ; fontsize
              UNIT-PIXEL))
            15))

; !!!! fontName is not valid
; The text displays anyway, using some font family, without error.
; The docs don't seem to say which font family is used.
; TODO better documentation
; The text layer still says it is using the given font family.
; TODO yield actual font family used.

; !!! UNIT-PIXEL GimpUnitsType is distinct from PIXELS GimpSizeType


; TODO test UNIT-POINT


; is-a TextLayer
(assert '(= (car (gimp-item-id-is-text-layer 15))
            1))

; text layer is not in image yet
(assert '(= (car (gimp-image-get-layers 8))
            0))

; adding layer to image succeeds
(assert '(gimp-image-insert-layer
            8  ; image
            15 ; layer
            0  ; parent
            0  ))  ; position within parent




;             attributes

; antialias default true
; FIXME doc says false
(assert '(= (car (gimp-text-layer-get-antialias 15))
            1))

; base-direction default TEXT-DIRECTION-LTR
(assert '(= (car (gimp-text-layer-get-base-direction 15))
            TEXT-DIRECTION-LTR))

; language default "C"
(assert '(string=? (car (gimp-text-layer-get-language 15))
                    "C"))

; TODO other attributes

; TODO setters effective

;            attributes as given

; text
(assert '(string=? (car (gimp-text-layer-get-text 15))
                        "textOfTestTextLayer"))
; font
(assert '(string=? (car (gimp-text-layer-get-font 15))
                        "fontName"))
; font-size
(assert '(= (car (gimp-text-layer-get-font-size 15))
            30))

; is no method to get fontSize unit


;              misc ops

; vectors from text yields ID 16
(assert '(= (car (gimp-vectors-new-from-text-layer
                    8    ; image
                    15)) ; text layer
            16))



;                  misc method

; gimp-text-get-extents-fontname
; Yields extent of rendered text, independent of image or layer.
; Extent is (width, height, ascent, descent) in unstated units, pixels?
; Does not affect image.
(assert '(= (car (gimp-text-get-extents-fontname
              "zed" ; text
              32    ; fontsize
              POINTS  ; size units.  !!! See UNIT-PIXEL
              "fontName" )) ; fontname
            57))
; usual result is (57 38 30 -8)



;           alternate method for creating text layer


; gimp-text-fontname creates text layer and inserts it into image
(assert '(= (car (gimp-text-fontname
              8     ; image
              -1     ; drawable.  -1 means NULL means create new text layer
              0 0   ; coords
              "bar" ; text
              1     ; border size
              1     ; antialias true
              31    ; fontsize
              PIXELS  ; size units.  !!! See UNIT-PIXEL
              "fontName" )) ; fontname
            17))
;

; error to insert layer created by gimp-text-fontname
(assert-error '(gimp-image-insert-layer
                  8  ; image
                  17 ; layer
                  0  ; parent
                  0  )  ; position within parent
              (string-append
                "Procedure execution of gimp-image-insert-layer failed on invalid input arguments: "
                "Item 'bar' (17) has already been added to an image"))



; for debugging: display
(assert '(= (car (gimp-display-new 8))
            1))