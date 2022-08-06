; !!! No shebang here

; Test a second .scm file in the same directory as a queried .scm.
; The second .scm file need not be executable.
; The second .scm file need not have a shebang.
; The gimp-script-fu-interpreter will nevertheless load the second .scm
; while it is querying the first, executable .scm in the dir.
; The plugin manager queries the first executable,
; and gimp-script-fu-interpreter loads (and returns defined names in)
; the second during the query of the first.

; Expect "Test>Test SF interpreter 3" in the menus
; Expect when chosen, message on GIMP message bar.

; plug-ins/test1/test1.scm is executable
; plug-ins/test1/test3.scm is NOT executable

(define (script-fu-test3)
  (gimp-message "Hello script-fu-test3")
)

(script-fu-register "script-fu-test3"
  "Test SF interpreter 3"
  "Just gives a message from Gimp"
  "lkk"
  "lkk"
  "2022"
  ""  ; all image types
)

(script-fu-menu-register "script-fu-test3" "<Image>/Test")
