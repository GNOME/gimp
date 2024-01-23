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
; returns (length, list), check length is 0
(assert `(= (car (gimp-image-get-layers ,testImage))
            0))



;        attributes of new layer

;        defaulted attributes

; apply-mask default false
(assert-PDB-false `(gimp-layer-get-apply-mask ,testLayer))

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
(assert-PDB-false `(gimp-layer-get-edit-mask ,testLayer))

; lock-alpha default false
; deprecated? gimp-layer-get-preserve-trans
(assert-PDB-false `(gimp-layer-get-lock-alpha ,testLayer))

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
(assert-PDB-false `(gimp-layer-get-show-mask ,testLayer))

; visible default true
; FIXME doc says default false
(assert-PDB-true `(gimp-layer-get-visible ,testLayer))

; is-floating-sel default false
(assert-PDB-false `(gimp-layer-is-floating-sel ,testLayer))

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







