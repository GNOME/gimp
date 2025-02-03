; tests of methods re masks on layers

; masks are a separate class in Gimp GimpLayerMask
; but the methods are named strangely,
; e.g. there is no gimp-layer-mask-get-layer

; gimp-layer-mask is deprecated


(script-fu-use-v3)

; setup
(define testImage (gimp-image-new 21 22 RGB))
(define testLayer (testing:layer-new testImage))
; assert layer is not inserted in image



(test! "layer-create-mask")

(define
  testLayerMask (gimp-layer-create-mask
                    testLayer
                    ADD-MASK-WHITE))
; assert layerMask not on the layer yet!!!

; mask is not on layer until added.
; Getting the mask for the layer yields -1.
(assert `(= (gimp-layer-get-mask ,testLayer)
            -1))

; ID is-a layerMask
(assert `(gimp-item-id-is-layer-mask ,testLayerMask))



(test! "layer-add-mask")

; add layerMask created on a layer to that layer succeeds
(assert `(gimp-layer-add-mask
            ,testLayer
            ,testLayerMask))

; add layerMask to layer was effective:
; Getting the mask for the layer yields layerMask ID
(assert `(= (gimp-layer-get-mask ,testLayer)
            ,testLayerMask))

; and vice versa
(assert `(= (gimp-layer-from-mask ,testLayerMask)
            ,testLayer))



(test! "creating and adding second mask")

; creating a second mask from layer succeeds
(define testLayerMask2
    (gimp-layer-create-mask
                  testLayer
                  ADD-MASK-WHITE))


; adding a second layerMask fails
(assert-error `(gimp-layer-add-mask
                 ,testLayer
                 ,testLayerMask2)
              (string-append
                "Procedure execution of gimp-layer-add-mask failed: "
                "Unable to add a layer mask since the layer already has one."))



(test! "remove-mask")

; remove-mask fails if the layer is not on image
(assert-error `(gimp-layer-remove-mask
                ,testLayer
                MASK-APPLY)   ; removal mode
              "Procedure execution of gimp-layer-remove-mask failed on invalid input arguments: ")
              ;  "Item 'LayerNew' (12) cannot be used because it has not been added to an image"))

; adding layer to image succeeds
(assert `(gimp-image-insert-layer
            ,testImage
            ,testLayer
            0  ; parent
            0  ))  ; position within parent

; remove-mask succeeds when layer is in image
(assert `(gimp-layer-remove-mask
            ,testLayer
            MASK-APPLY))  ; removal mode

; and is effective
; layer no longer has a mask
(assert `(= (gimp-layer-get-mask ,testLayer)
            -1))

; and now we can add the second mask
(assert `(gimp-layer-add-mask
                 ,testLayer
                 ,testLayerMask2))


(test! "variations of layer-create-mask")

; fails when mask different size from layer?

; fails create layerMask when ADD-CHANNEL-MASK and no active channel
(assert-error `(gimp-layer-create-mask
                  ,testLayer
                  ADD-MASK-CHANNEL)
                "Procedure execution of gimp-layer-create-mask failed")

; create layerMask ADD-ALPHA-MASK works even when no alpha channel
(assert `(gimp-layer-create-mask
                  ,testLayer
                  ADD-MASK-ALPHA))



(script-fu-use-v2)