; Test methods of Resource class

; Testing may depend on fresh install.
; Depends on the default context.

; !!! ScriptFu currently traffics in string names of resources
; FUTURE traffic in numeric ID
; FIXME numerous script-fu/scripts that deal with brush using name strings

; a brush from context is a string
(assert '(string=?
            (car (gimp-context-get-brush))
            "2. Hardness 050"))

; gimp-brush-get-by-name returns same string, when brush of that name exists
(assert '(string=?
            (car (gimp-brush-get-by-name "2. Hardness 050"))
            "2. Hardness 050"))

; gimp-brush-get-by-name returns error, when brush of that name not exists
(assert-error '(gimp-brush-get-by-name "foo")
              "Procedure execution of gimp-brush-get-by-name failed on invalid input arguments: Brush 'foo' not found")



; TODO the rest of these require ScriptFu to traffic in numeric ID

;(assert '(= (gimp-resource-id-is-valid
;                  (car (gimp-context-get-brush))
;             1))

;gimp-resource-
;delete
;duplicate
;get-name
;id-is-brush
;id-is-font
;id-is_gradient
;id-is-palette
;id-is-pattern
;id-is-valid
;is-editable
;rename