#!/usr/bin/env gimp-script-fu-interpreter-3.0

; test-run-error-PDB.scm

; A script that throws a run-time error calling PDB with wrong signature
; The script has a dialog so it can run INTERACTIVE
;
; Setup: copy this file to /scripts
; Example: to ~/.gimp-2.99/scripts/test-run-error-PDB.scm

; Expect "Filters>Dev>Script-Fu>Test>Runtime PDB error" in the menus

; !!! Do not export G_DEBUG=fatal-warnings

; Test interactive:
; Choose "Runtime PDB error".  Expect the plugin's dialog.
; Choose OK.
; Expect:
;   an error dialog in GIMP that must be OK'd
;   OR a CRITICAL and WARNING message in Gimp Error Console when it is open.)

; Test non-interactive:
; Enter "(script-fu-test-run-error-PDB 1)" in SF Console
; Expect SF Console to print the error message.

; In both test case, the error message is like:
; GIMP CRITICAL gimp_procedure_real_execute: assertion 'gimp_value_array_length (args) >= procedure->num_args' failed
; GIMP WARNING gimp_procedure_execute: no return values, shouldn't happen

; The root error is "not enough args to a PDB procedure"
; ScriptFu will warn but proceed to call the PDB procedure.
; Gimp will throw CRITICAL but proceed
; On return, Gimp will throw WARNING that the procedure did not return values.
; ???? Why does it crash and give a backtrace???

(define (script-fu-test-run-error-PDB code)
  (gimp-message)  ; <= run-time error signature mismatch
)

(script-fu-register "script-fu-test-run-error-PDB"
  "Runtime PDB error"
  "Expect error in Gimp, or PDB execution error when called by another"
  "lkk"
  "lkk"
  "2023"
  ""  ; requires no image
  ; The argument here just to ensure a dialog
  SF-ADJUSTMENT "Not used" '(1 -2 2 1 2 0 0)
)

(script-fu-menu-register "script-fu-test-run-error-PDB"
                         "<Image>/Filters/Development/Script-Fu/Test")

