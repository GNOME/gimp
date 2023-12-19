; Test methods of Edit module of the PDB



; setup
; Load test image that already has drawable
(define testImage (testing:load-test-image "wilber.png"))

; get all the root layers
; testImage has exactly one root layer.
(define testLayers (cadr (gimp-image-get-layers testImage)))
;testLayers is-a list

(define testLayer (vector-ref testLayers 0))

; !!! Testing requires initial condition: clipboard is empty.
; But there is no programmatic way to empty the clipboard,
; or even to determine if the clipboard is empty.
; So these tests might not pass when you run this test file
; in the wrong order.

; paste

; ordinary paste is to a drawable

; paste always returns a layer, even when clipboard empty
; FIXME: this is undocumented in the PDB,
; where it seems to imply that a layer is always returned.
(assert-error `(gimp-edit-paste
                   ,testLayer
                   TRUE) ; paste-into
              "Procedure execution of gimp-edit-paste failed" )

; paste-as-new-image returns NULL image when clipboard empty
; paste-as-new is deprecated
(assert `(= (car (gimp-edit-paste-as-new-image))
            -1))  ; the NULL ID


; copy

; copy returns true when no selection and copies entire drawables
(assert-PDB-true `(gimp-edit-copy 1 ,testLayers))


; copy returns false when is a selection
; but it doesn't intersect any chosen layers
; When one layer is offset from another and bounds of layer
; do not intersect bounds of selection.
; TODO

; copy-visible takes only an image
(assert-PDB-true `(gimp-edit-copy-visible ,testImage))



; paste when clipboard is not empty returns a vector of length one
(assert `(= (car (gimp-edit-paste
                   ,testLayer
                   TRUE)) ; paste-into
            1))
; TODO test paste-into FALSE

; for debugging individual test file:
;(gimp-display-new testImage)
