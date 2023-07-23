
; Miscellaneous tests of the PDB
; These are not associated with an object class


; 0 is an invalid item id
; FUTURE gimp returns #f instead of 0
; FUTURE gimp doesn't wrap in extra list
(assert '(= (car (gimp-item-id-is-vectors 0)) 0))

; -1 is an invalid item id
; FUTURE: '(not (gimp-item-id-is-valid -1))
(assert '(= (car (gimp-item-id-is-valid -1)) 0))