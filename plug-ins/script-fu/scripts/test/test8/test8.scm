#!/usr/bin/env gimp-script-fu-interpreter-3.0

; Test mismatch between name of defined run function and name for PDB procedure
; Not a high priority: a rare syntax error in a plugin text.
; If authors follow a template, they won't make this mistake.

; The names must match exactly.
; Here, "mismatch" the name of the defined run function
; does not match "script-fu-test8" the name of the PDB proc.

; Expect a warning in the text console as the plugin text is queried:
; script-fu: WARNING: Run function not defined, or does not match PDB procedure name: script-fu-test8.
; Expect the PDB procedure to not exist

; If we don't detect this syntax error:
; A PDB procedure is created.
; When invoked from Test>Test SF interpreter 8"
; the interpreter enters an infinite loop.
; There is no harm to the GIMP app, but the interpreter process can only be killed.
; During the run phase, the "(define foo)"
; should re-define an existing definition in the interpreter state.
; Instead, since the name is mismatched,
; the foo function remains defined to be a call to the PDB procedure named foo.
; So script-fu-script-proc instead calls the PDB again, an infinite loop.

(define (mismatch)
  (gimp-message "mismatch")
)

(script-fu-register "script-fu-test8"
  "Test SF interpreter 8"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test8" "<Image>/Test")
