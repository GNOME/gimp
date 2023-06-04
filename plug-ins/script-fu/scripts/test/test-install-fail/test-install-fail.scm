; An old style script, not an independent plugin

; A script that fails at install time: has syntax error
;
; Setup: copy this file w/ executable permission, to one of the /scripts dirs.
; Example: to ~/snap/393/.config/GIMP/2.10/scripts/test-quit.scm

; Start Gimp, configure to have Error Console open, and quit.
; Install this plugin.
; Restart Gimp from a terminal
; Expect:
;   - an error in the Gimp Error Console
;   - an error in the terminal
; !!! the first is currently failing.  So this script is to test FUTURE.

(define (script-fu-test-install-fail )
  (   ; <= syntax error
)

; Much is moot, since this should fail to install
(script-fu-register "script-fu-test-install-fail"
  "Moot"
  "Moot"
  "lkk"
  "lkk"
  "2023"
  ""  ; requires no image
  ; no args
)

(script-fu-menu-register "script-fu-test-install-fail"
                         "<Image>/Filters/Development/Script-Fu/Test")

