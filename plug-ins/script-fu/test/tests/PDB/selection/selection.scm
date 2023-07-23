; Test methods of selection class of the PDB



; setup
; Reusing image 10
(define testImage 10)


; get-selection yields an ID.
; Image always yields a selection object.
; It is a singleton.
(assert `(= (car (gimp-image-get-selection ,testImage))
            18))

(define testSelection 18)

; The returned ID is-a Selection
(assert `(= (car (gimp-item-id-is-selection ,testSelection))
            1))

; !!! Note there is little use for a Selection instance.
; There are no methods on the class per se i.e. taking the instance ID.
; Except for methods on the superclass Item of subclass Selection.
;
; Instead the methods seem to be on an image.
; Its not clear whether changing the selection in an image
; also changes the singleton Selection instance,
; and there is no way of knowing, since the Selection instance
; has no methods.

; selection on new image is empty
; !!! Requre no prior test on this image selected
; !!! Arg is the image, not the selection object instance.
(assert `(= (car (gimp-selection-is-empty ,testImage))
            1))

; selection bounds yields (1 0 0 21 22)
; First element of tuple is 0 (false)
; indicates user or program has not made selection
(assert `(= (car (gimp-selection-bounds ,testImage))
            0))
; selection bounds equal bounds of image
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 21 22)))





;            select all and none

; select all succeeds
(assert `(gimp-selection-all ,testImage))
; !!! A selection operation does not create a new selection object
; i.e. ID is the same.
; get-selection yields same singleton on image
(assert `(= (car (gimp-image-get-selection ,testImage))
            ,testSelection))
; after select all, selection bound indicates selection created
(assert `(= (car (gimp-selection-bounds ,testImage))
            1))
; and now is-empty is false
(assert `(= (car (gimp-selection-is-empty ,testImage))
            0))


; clear and none are the synonyms

; clear does not invalidate a prior selection object
; i.e. get-selection returns same ID

; clear makes selection bounds equal entire image
; TODO

; select none succeeds
(assert `(gimp-selection-none ,testImage))
; effective: is-empty is true
(assert `(= (car (gimp-selection-is-empty ,testImage))
            1))
; same singleton on image exists
(assert `(= (car (gimp-image-get-selection ,testImage))
            ,testSelection))


;                misc selection operations

; gimp-selection-value


;              change selection to totally new selection
; Not a function of existing selection, by color or shape.

;gimp-image-select-color
;  ,testImage
 ; CHANNEL-OP-ADD
;  drawable
;  "red")

; gimp-image-select-contiguous-color
; ellipse
; polygon
; rectangle
; round-rectangle




; gimp-selection-float is tested elsewhere
; It is not an op on the selection, but an op on the image that uses the selection.
; See gimp-image-floating-selection