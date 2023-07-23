
; Test ScriptFu's binding to all Gimp C arg types of the PDB


; The PDB procedures called are arbitrary, chosen for the type of their args.

; The test is only that no error is thrown, not that the call is effective.

; int
; 1 is image ID and 1,1 is an int coord
(assert '(gimp-image-add-sample-point 1 1 1))

; float
(assert '(= (car (gimp-item-id-is-valid -1)) 0))