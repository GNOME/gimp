; Test methods of Resource class

; Miscellaneous tests of error conditions on Resource as abstract class
; Concrete classes are Brush, Font, etc.

; See brush.scm, palette.scm etc. for test of subclasses of Resource

; See resource-ops.scm for tests of:
; gimp-resource-delete  -duplicate  -rename

; See context/context-resource.scm
; for tests of generic methods
; e.g. gimp-resource-get-name   -id-is-valid  -is-editable




; invalid numeric ID
; ScriptFu detects these errors, before GIMP can check for out of range.

; A negative ID is invalid.
; If ScriptFu did not detect, GIMP would yield: "Invalid value for argument 0"
(assert-error `(gimp-resource-get-name -1)
              "runtime: invalid resource ID")

; 12345678 is in range but invalid (presume not exist resource with such a large ID)
(assert-error `(gimp-resource-get-name 12345678)
              "runtime: invalid resource ID")





; an ID of wrong subclass of resource throws an error.
; !!! Also check stderr for CRITICAL
; 1 is not a valid font ID but it IS the ID of say a brush
(assert-error `(gimp-context-set-font 1)
              "runtime: resource ID of improper subclass.")

; an ID that is not the ID of any resource instance is an error
(assert-error `(gimp-context-set-font 9999999)
              "runtime: invalid resource ID")
; It should not say "Invalid value for argument 0"
; since that is a fallback error check.

; zero is never a Resource ID
(assert-error `(gimp-context-set-font 0)
              "runtime: invalid resource ID")

; -1 is never a Resource ID
(assert-error `(gimp-context-set-font -1)
              "runtime: invalid resource ID")

; A v2 string name since v3 does not represent a resource.
; Is an error to pass a string for a Resource ID.
(assert-error `(gimp-context-set-font "Sans Serif")
              "in script, expected type: integer for argument 1 to gimp-context-set-font")