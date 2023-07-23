; tests of methods re masks on layers

; masks are a separate class in Gimp GimpLayerMask
; but the methods are named strangely,
; e.g. there is no gimp-layer-mask-get-layer


;        setup
; Image owns Layers

; method new from fresh GIMP state returns ID
(assert '(= (car (gimp-image-new 21 22 RGB))
            9 ))

; new method yields layer ID 12
(assert '(= (car (gimp-layer-new
                    9
                    21
                    22
                    RGB-IMAGE
                    "LayerNew"
                    50.0
                    LAYER-MODE-NORMAL))
            12))
; assert layer is not inserted in image


; gimp-layer-create-mask yields ID 13
; Creating for the layer, but not on the layer yet!!!
(assert '(= (car (gimp-layer-create-mask
                  12
                  ADD-MASK-WHITE))
            13))

; mask is not on layer until added.
; Getting the mask for the layer yields -1.
(assert '(= (car (gimp-layer-mask 12))
            -1))

; add layerMask created on a layer to that layer succeeds
(assert '(gimp-layer-add-mask
            12 ; layer
            13)) ; layer mask

; add layerMask to layer was effective:
; Getting the mask for the layer yields layerMask ID
(assert '(= (car (gimp-layer-mask 12))
            13))

; and vice versa
(assert '(= (car (gimp-layer-from-mask 13))
            12))



;           creating and adding second mask

; creating a second mask from layer succeeds
(assert '(= (car (gimp-layer-create-mask
                  12
                  ADD-MASK-WHITE))
            14))

; adding a second layerMask fails
(assert-error '(gimp-layer-add-mask
                 12 ; layer
                 14) ; layer mask
              (string-append
                "Procedure execution of gimp-layer-add-mask failed: "
                "Unable to add a layer mask since the layer already has one."))



;            mask removal

; remove-mask fails if the layer is not on image
(assert-error '(gimp-layer-remove-mask
                12  ; layer
                MASK-APPLY)   ; removal mode
              (string-append
                "Procedure execution of gimp-layer-remove-mask failed on invalid input arguments: "
                "Item 'LayerNew' (12) cannot be used because it has not been added to an image"))

; adding layer to image succeeds
(assert '(gimp-image-insert-layer
            9  ; image
            12 ; layer
            0  ; parent
            0  ))  ; position within parent

; remove-mask succeeds
; when layer is in image
(assert '(gimp-layer-remove-mask
            12  ; layer
            MASK-APPLY))  ; removal mode

; and is effective
; layer no longer has a mask
(assert '(= (car (gimp-layer-mask 12))
            -1))

; and now we can add the second mask
(assert '(gimp-layer-add-mask
                 12 ; layer
                 14)) ; second layer mask


; fails when mask different size from layer?

; fails create layerMask when ADD-CHANNEL-MASK and no active channel

; create layerMask ADD-ALPHA-MASK works even when no alpha channel

; TODO many variations of create
