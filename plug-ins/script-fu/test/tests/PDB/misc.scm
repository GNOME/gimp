
; Miscellaneous tests of the PDB
; These are not associated with an object class


; 0 is an invalid item id
(assert-PDB-false '(gimp-item-id-is-vectors 0))

; -1 is an invalid item id
(assert-PDB-false '(gimp-item-id-is-valid -1))