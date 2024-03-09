; test Image methods of PDB
; where methods deal with layers owned by image.


; setup
; Load test image that already has drawable
(define testImage (testing:load-test-image "gimp-logo.png"))



;                 get-layers
; procedure returns (1 #(<layerID>)) ....in the REPL


; the testImage has one layer
(assert `(= (car (gimp-image-get-layers ,testImage ))
            1))

; get-layers returns second result a vector of ID's
; !!! use cadr to get second result
(assert `(vector? (cadr (gimp-image-get-layers ,testImage ))))

; the vector also has one element
(assert `(= (vector-length (cadr (gimp-image-get-layers ,testImage )))
            1))

; the vector can be indexed at first element
; and is a numeric ID
(assert `(number?
            (vector-ref (cadr (gimp-image-get-layers ,testImage ))
                        0)))

; store the layer ID
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                              0))

; FIXME seems to fail??? because name is actually "Background"

; the same layer can be got by name
; FIXME app shows layer name is "gimp-logo.png" same as image name
(assert `(= (car (gimp-image-get-layer-by-name ,testImage "Background"))
            ,testLayer))

; the single layer's position is zero
; gimp-image-get-layer-position is deprecated
(assert `(= (car (gimp-image-get-item-position ,testImage ,testLayer))
            0))


; TODO gimp-image-get-layer-by-tattoo

; the single layer is selected  in freshly opened image
(assert `(= (car (gimp-image-get-selected-layers ,testImage ))
            1))

; TODO test selected layer is same layer


