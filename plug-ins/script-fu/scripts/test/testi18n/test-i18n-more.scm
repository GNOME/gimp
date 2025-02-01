#!/usr/bin/env gimp-script-fu-interpreter-3.0
;!# Close comment started on first line. Needed by gettext.

; An independently interpreted Scheme plugin
; to test the registration function script-fu-register-18n.

; This is marked with translateable strings,
; but we don't expect translators to translate.
; We mock up incomplete translation data.

; Has translatable dialog.
; Dialog appears in native language
; when the script-fu-register-18n call is proper
; and mockup translation data installed corresponding to said call.
(define (plug-in-test-i18n-3 orientation)
  ; does nothing
)


; Not a filter, always enabled.
(script-fu-register-procedure "plug-in-test-i18n-3"
  _"Test SF i18n Three..."  ; menu item
  ""  ; tooltip
  "LKK"
  "2025"
  ; One arg, just to test the translation of its label
  SF-OPTION     _"Orientation"        '(_"Horizontal"
                                        _"Vertical")
)

(script-fu-menu-register "plug-in-test-i18n-3"
                         "<Image>/Filters/Development/Test")

; !!! Note the domain name is not the same as for the
; other two PDB procedures in the suite
(script-fu-register-i18n "plug-in-test-i18n-3" ; plugin name
                         "scriptfu-test-more") ; domain name




