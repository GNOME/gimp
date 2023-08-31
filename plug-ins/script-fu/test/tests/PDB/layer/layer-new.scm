; test Layer methods of PDB


;        setup


(define testImage (car (gimp-image-new 21 22 RGB)))

(define testLayer
         (car (gimp-layer-new
                    testImage
                    21
                    22
                    RGB-IMAGE
                    "LayerNew"
                    50.0
                    LAYER-MODE-NORMAL)))




; new layer is not in the image until inserted
(assert `(= (car (gimp-image-get-layers ,testImage))
            0))



;        attributes of new layer

;        defaulted attributes

; apply-mask default false
(assert `(=
            (car (gimp-layer-get-apply-mask ,testLayer))
            0))

; blend-space default LAYER-COLOR-SPACE-AUTO
(assert `(=
            (car (gimp-layer-get-blend-space ,testLayer))
            LAYER-COLOR-SPACE-AUTO))

; composite-mode default LAYER-COMPOSITE-AUTO
(assert `(=
            (car (gimp-layer-get-composite-mode ,testLayer))
            LAYER-COMPOSITE-AUTO))

; composite-space default LAYER-COLOR-SPACE-AUTO
(assert `(=
            (car (gimp-layer-get-composite-space ,testLayer))
            LAYER-COLOR-SPACE-AUTO))

; edit-mask default false
(assert `(=
            (car (gimp-layer-get-edit-mask ,testLayer))
            0))

; lock-alpha default false
; deprecated? gimp-layer-get-preserve-trans
(assert `(=
            (car (gimp-layer-get-lock-alpha ,testLayer))
            0))

; mask not exist, ID -1
; deprecated? gimp-layer-mask
(assert `(=
            (car (gimp-layer-get-mask ,testLayer))
            -1))

; mode default LAYER-MODE-NORMAL
(assert `(=
            (car (gimp-layer-get-mode ,testLayer))
            LAYER-MODE-NORMAL))

; show-mask default false
(assert `(=
            (car (gimp-layer-get-show-mask ,testLayer))
            0))

; visible default true
; FIXME doc says default false
(assert `(=
            (car (gimp-layer-get-visible ,testLayer))
            1))

; is-floating-sel default false
(assert `(=
            (car (gimp-layer-is-floating-sel ,testLayer))
            0))

; !!! No get-offsets




;         attributes are as given when created

; name is as given
(assert `(string=? (car (gimp-layer-get-name ,testLayer))
                  "LayerNew"))

; opacity is as given
(assert `(=
            (car (gimp-layer-get-opacity ,testLayer))
            50.0))


;          generated attributes

; tattoo
; tattoo is generated unique within image?
(assert `(=
            (car (gimp-layer-get-tattoo ,testLayer))
            2))







