; test Layer methods of PDB


;        setup
; Image owns Layers

; method new from fresh GIMP state returns ID
(assert '(=
           (car (gimp-image-new 21 22 RGB))
           7 ))






; new method yields layer ID 8
(assert '(= (car (gimp-layer-new
                    7
                    21
                    22
                    RGB-IMAGE
                    "LayerNew"
                    50.0
                    LAYER-MODE-NORMAL))
            8))


; new layer is not in the image until inserted
(assert '(= (car (gimp-image-get-layers 7))
            0))



;        attributes of new layer

;        defaulted attributes

; apply-mask default false
(assert '(=
            (car (gimp-layer-get-apply-mask 8))
            0))

; blend-space default LAYER-COLOR-SPACE-AUTO
(assert '(=
            (car (gimp-layer-get-blend-space 8))
            LAYER-COLOR-SPACE-AUTO))

; composite-mode default LAYER-COMPOSITE-AUTO
(assert '(=
            (car (gimp-layer-get-composite-mode 8))
            LAYER-COMPOSITE-AUTO))

; composite-space default LAYER-COLOR-SPACE-AUTO
(assert '(=
            (car (gimp-layer-get-composite-space 8))
            LAYER-COLOR-SPACE-AUTO))

; edit-mask default false
(assert '(=
            (car (gimp-layer-get-edit-mask 8))
            0))

; lock-alpha default false
; deprecated? gimp-layer-get-preserve-trans
(assert '(=
            (car (gimp-layer-get-lock-alpha 8))
            0))

; mask not exist, ID -1
; deprecated? gimp-layer-mask
(assert '(=
            (car (gimp-layer-get-mask 8))
            -1))

; mode default LAYER-MODE-NORMAL
(assert '(=
            (car (gimp-layer-get-mode 8))
            LAYER-MODE-NORMAL))

; show-mask default false
(assert '(=
            (car (gimp-layer-get-show-mask 8))
            0))

; visible default true
; FIXME doc says default false
(assert '(=
            (car (gimp-layer-get-visible 8))
            1))

; is-floating-sel default false
(assert '(=
            (car (gimp-layer-is-floating-sel 8))
            0))

; !!! No get-offsets




;         attributes are as given when created

; name is as given
assert '(string=? (car (gimp-layer-get-name 8))
                  "LayerNew")

; opacity is as given
(assert '(=
            (car (gimp-layer-get-opacity 8))
            50.0))


;          generated attributes

; tattoo
; tattoo is generated unique within image?
(assert '(=
            (car (gimp-layer-get-tattoo 8))
            2))







