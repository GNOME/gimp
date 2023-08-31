; test memory limits in TS

; TS is known to be non-robust in face of memory exhaustion.
; See Manual.txt which says "TinyScheme is known to misbehave when memory is exhausted."

; numeric constants from tinyscheme-private.h

; There is no document (only the source code itself)
; explaining the limits.
; The limits here are from experiments.

; These only test the limits.
; Methods on the objects (string, vector, etc.) are tested elsewhere.

;                   Symbol limits

; There is no defined limit on count of symbols.
; The objlist is a hash table, entries allocated from cells.
; The lists in the hash table are practically unlimited.


;                   String limits

; Strings are malloced.
; Limit on string size derives from OS malloc limits.
; No practical limit in ScriptFu.

; Seems to work
; (make-string 260000 #\A)


;                   Vector limits.

; A vector is contiguous cells.
; TS allocates in segments.


; A vector can be no larger than two segments?

; succeeds
(assert '(make-vector 25000))
; REPL shows as #(() () ... ()) i.e. a vector of NIL, not initialized

; might not crash?
(define testVector (make-vector 25001))

; ????
(assert `(vector-fill! ,testVector 1))

; seems to hang
; (assert '(make-vector 50001))

; seems to crash
; (assert '(make-vector 200000))

