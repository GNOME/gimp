; test stack methods of Context

; push and pop

; We arbitrarily use context:antialias to distinguish context instances.
; Antialias is a setting for the selection tool.
; Antialias is usually true.
; !!! This test depends on it being true initially.

; The two context instances are:
; - original, pushed
; - new one, after a push




; test the sequence push, pop i.e. the normal sequence

; Test initial condition is context:antialias true
(assert-PDB-true `(gimp-context-get-antialias))

; push succeeds
(assert `(gimp-context-push))

; Set antialias false in new context
; FUTURE pass #f
(assert `(gimp-context-set-antialias 0))
(assert-PDB-false `(gimp-context-get-antialias))

; pop succeeds
(assert `(gimp-context-pop))

; pop effective: original context i.e. antialias true
(assert-PDB-true `(gimp-context-get-antialias))



; test abnormal sequence: pop without a prior push.
; Yields an error
(assert-error `(gimp-context-pop)
              "Procedure execution of gimp-context-pop failed")


