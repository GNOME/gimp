#!/usr/bin/env gimp-script-fu-interpreter-3.0

; A script that fails at query/registration time: ill-formed args in registration data

; Tests the fix for #6157

; Incomplete: FUTURE add more test cases.

; One script can't test all cases.
; Several cases are tested here, but you must edit the script,
; putting each case to the top: the failing top case stops interpretation.

; Install this script.
; Start Gimp
; Expect:
;   - an error in the stderr console, errors from SF re args
;       e.g. "adjustment default must be list"
;   - plugin should not install to the menus
;   - the interpreter must not crash !!!
;   - the procedure is not in ProcedureBrowser

; Note errors come before Gimp Error Console is ready

; moot run func
(define (script-fu-test-registration-fail ) ())


; Case SF-ENUM not a list of two strings
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: 1 is not a string
  SF-ENUM     "foo"          '("InterpolationType" 1)
)

; This should be moot (since it should fail earlier.)
; But leave this here so when it does succeed,
; you know earlier test failed to fail.
(script-fu-menu-register "script-fu-test-registration-fail"
                         "<Image>/Test")



; More tests below.
; Copy them to above, and retest.
; Can't test them all at once.


; Case SF-ADJUSTMENT default not a list
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed : 0 is not a list
  SF-ADJUSTMENT     "foo"        0
)


; Case: SF-ADJUSTMENT wrong list length
; At query time, SF throws error.
; At query time, a critical from Gimp re arg default not in range
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: a list, but not proper length
  ; Expect SF error
  SF-ADJUSTMENT     "foo"        '(0 )
)



; Case SF-OPTIONS not a list
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: a list, but not proper length
  SF-OPTION     "foo"        "bar"
)


; Case SF-OPTIONS default an empty list
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; semantics : empty list makes no sense
  SF-OPTION     "foo"        '()
)


; Case SF-OPTIONS list contains non-string
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; 1 is not a string
  SF-OPTION     "foo"        '("foo" 1)
)



; Case SF-COLOR not a list or a string
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: 1 is not a list or a string
  SF-COLOR     "foo"        1  4  5
)

; FUTURE more test cases for SF-COLOR


; Case SF-ENUM not a list
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: "bar" is not a list
  SF-ENUM     "foo"          "bar"
)

; Case SF-ENUM not a list of two strings
(script-fu-register "script-fu-test-registration-fail"
  "Moot" "Moot" "lkk" "lkk" "2023" ""  ; requires no image

  ; ill-formed arg spec: 1 is not a string
  SF-ENUM     "foo"          '("InterpolationType" 1)
)

; FUTURE more test cases for SF-ENUM