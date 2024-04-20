; test Layer methods of PDB


(script-fu-use-v3)


;        setup

(define testImage (gimp-image-new 21 22 RGB))

(define testLayer (gimp-layer-new
                    testImage
                    21
                    22
                    RGB-IMAGE
                    "LayerNew"
                    50.0
                    LAYER-MODE-NORMAL))




(test! "new layer is not in the image until inserted")
; returns (length, list), check length is 0
(assert `(= (car (gimp-image-get-layers ,testImage))
            0))



(test! "attributes of new layer")

;        defaulted attributes

; apply-mask default false
(assert `(not (gimp-layer-get-apply-mask ,testLayer)))

; blend-space default LAYER-COLOR-SPACE-AUTO
(assert `(= (gimp-layer-get-blend-space ,testLayer)
            LAYER-COLOR-SPACE-AUTO))

; composite-mode default LAYER-COMPOSITE-AUTO
(assert `(= (gimp-layer-get-composite-mode ,testLayer)
            LAYER-COMPOSITE-AUTO))

; composite-space default LAYER-COLOR-SPACE-AUTO
(assert `(= (gimp-layer-get-composite-space ,testLayer)
            LAYER-COLOR-SPACE-AUTO))

; edit-mask default false
(assert `(not (gimp-layer-get-edit-mask ,testLayer)))

; lock-alpha default false
; deprecated? gimp-layer-get-preserve-trans
(assert `(not (gimp-layer-get-lock-alpha ,testLayer)))

; mask not exist, ID -1
; gimp-layer-mask is deprecated
(assert `(= (gimp-layer-get-mask ,testLayer)
            -1))

; mode default LAYER-MODE-NORMAL
(assert `(= (gimp-layer-get-mode ,testLayer)
            LAYER-MODE-NORMAL))

; show-mask default false
(assert `(not (gimp-layer-get-show-mask ,testLayer)))

; visible default true
; FIXME doc says default false
; gimp-layer-get-visible is deprecated.
(assert `(gimp-item-get-visible ,testLayer))

; is-floating-sel default false
(assert `(not (gimp-layer-is-floating-sel ,testLayer)))

; !!! No get-offsets




(test! "new layer attributes are as given when created")

; name is as given
; gimp-layer-get-name is deprecated
(assert `(string=? (gimp-item-get-name ,testLayer)
                  "LayerNew"))

; opacity is as given
(assert `(= (gimp-layer-get-opacity ,testLayer)
            50.0))


;          generated attributes

; tattoo
; tattoo is generated unique within image?
; gimp-layer-get-tattoo is deprecated
(assert `(= (gimp-item-get-tattoo ,testLayer)
            2))



(script-fu-use-v2)





