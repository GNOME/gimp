; test PDB methods that change selection by shape


; setup
(define testImage (testing:load-test-image-basic))
(define testLayer (vector-ref (car (gimp-image-get-layers testImage ))
                                  0))


; rectangle tested in selection.scm



;   initial conditions

; new image has no selection
(assert-PDB-true `(gimp-selection-is-empty ,testImage))
; but selection bounds equal bounds of image
; returns (0 0 0 128 128)
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 128 128)))




; ellipse

; selecting a ellipse does not throw error
(assert `(gimp-image-select-ellipse
            ,testImage
            CHANNEL-OP-ADD
            0 0 ; upper left coords of bounding box
            2 4 ; width height
            ))
; and it is not empty
(assert-PDB-false `(gimp-selection-is-empty ,testImage))
; and it is effective
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 2 4)))




; polygon
; TODO





; round rectangle

; reset test conditions to none selection
(gimp-selection-none testImage)
(assert-PDB-true `(gimp-selection-is-empty ,testImage))

(test! "zero width round rect")
; selecting a zero width round rect does not throw an error
(assert `(gimp-image-select-round-rectangle
            ,testImage
            CHANNEL-OP-ADD
            1 1 ; upper left coords
            0 0 ; width height
            2 3 ; radius x, y
            ))
; but does not create a selection
(assert-PDB-true `(gimp-selection-is-empty ,testImage))

(test! "round rect excess radius")
; selecting a round rect with radius larger than bounding box does not throw an error
(assert `(gimp-image-select-round-rectangle
            ,testImage
            CHANNEL-OP-ADD
            1 1 ; upper left coords
            2 2 ; width height
            3 3 ; radius x, y
            ))
; and does create a selection !
(assert-PDB-false `(gimp-selection-is-empty ,testImage))
