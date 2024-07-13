
; Miscellaneous tests of the PDB
; These are not associated with an object class


; Restored at end of this script
(script-fu-use-v3)

(test! "misc item-id-is-foo")

; 0 is not a valid id for any object
(assert `(not (gimp-item-id-is-channel 0)))
(assert `(not (gimp-item-id-is-drawable 0)))
(assert `(not (gimp-item-id-is-group-layer 0)))
(assert `(not (gimp-item-id-is-layer 0)))
(assert `(not (gimp-item-id-is-layer-mask 0)))
(assert `(not (gimp-item-id-is-path 0)))
(assert `(not (gimp-item-id-is-selection 0)))
(assert `(not (gimp-item-id-is-text-layer 0)))

; -1 is not a valid item id
(assert `(not (gimp-item-id-is-valid -1)))

; !!! Restore
(script-fu-use-v2)