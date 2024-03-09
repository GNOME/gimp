; test PDB methods that change selection by another object
; such as a color or a channel

; Function to reset the selection
; (define testResetSelection )



; setup
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))
; a layer mask from alpha
(define testLayerMask (car (gimp-layer-create-mask
                              testLayer
                              ADD-MASK-ALPHA)))
(gimp-layer-add-mask testLayer testLayerMask)



; new image has no initial selection
(assert-PDB-true `(gimp-selection-is-empty ,testImage))



; test selection by given color

(assert `(gimp-image-select-color ,testImage CHANNEL-OP-ADD ,testLayer "black"))
; effective: test image has some black pixels, now selection is not empty
(assert-PDB-false `(gimp-selection-is-empty ,testImage))



; test selection by picking coords

; !!! This is not the same as the menu item Select>By Color
; That menu item selects all pixels of a picked color.
; The PDB procedure selects a contiguous area (not disconnected pixels)
; and is more affected by settings in the context particularly sample-transparent.
; This test fails if you pick a coord that is transparent,
; since sample-transparent defaults to false?

; Reset to no selection
(assert `(gimp-selection-none ,testImage))
(assert-PDB-true `(gimp-selection-is-empty ,testImage))
(assert `(= (car (gimp-selection-value ,testImage 125 125))
            0))

; gimp-image-select-contiguous-color does not throw
(assert `(gimp-image-select-contiguous-color ,testImage CHANNEL-OP-ADD ,testLayer 125 125))
; effective, now selection is not empty
(assert-PDB-false `(gimp-selection-is-empty ,testImage))
; effective, the selection value at the picked coords is "totally selected"
(assert `(= (car (gimp-selection-value ,testImage 125 125))
            255))



; selection from item

; selection from the layer itself: selects same as layer's alpha
(assert `(gimp-selection-none ,testImage))
(assert `(gimp-image-select-item ,testImage CHANNEL-OP-ADD ,testLayer))
; effective: selection is not empty
(assert-PDB-false `(gimp-selection-is-empty ,testImage))

; selection from layer mask

(gimp-selection-none testImage)
; layer mask to selection succeeds
(assert `(gimp-image-select-item ,testImage CHANNEL-OP-ADD ,testLayerMask))
; effective: selection is not empty
(assert-PDB-false `(gimp-selection-is-empty ,testImage))

; TODO selection from
; channel, vectors
; TODO selection from layer group? fails?


; for debugging individual test file:
; (gimp-display-new testImage)
