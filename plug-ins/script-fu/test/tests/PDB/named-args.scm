; Test ScriptFu's binding to PDB procedures using named args
; aka keyword arguments

; !!! Named args are mostly valid for calls to non-ScriptFu plugins.
; Not for calls to "INTERNAL" procedures.
; Not for calls to /scripts i.e. plugins named script-fu-
; from extension-script-fu or SF Console.


(script-fu-use-v3)

; setup

(define testImage (testing:load-test-image-basic-v3))
(define testLayer (vector-ref (gimp-image-get-layers testImage) 0))
(define testLayers (vector testLayer))




; This fails: operation not permitted?
;(plug-in-web-browser )
  ; #:url "https://gitlab.gnome.org/GNOME/gimp/issues")


; plug-in-tile returns (image drawable) when new-image arg is #t
; else returns (-1 -1) i.e. null image and drawable.


(test! "Passing no args, not named, using defaults")
; except for non-defaultable args, a prefix.
; since new-image defaults to #t, returns a non null image (not -1)
(assert `(> (car (plug-in-tile
                RUN-NONINTERACTIVE
                ,testImage
                ,testLayers))
            0))


(test! "Using mix of positional and named args")
; TODO not implemented
; We should do this, not that hard
;(plug-in-tile RUN-NONINTERACTIVE
;              testImage
;              testLayers
;              #:new-width  10 ; width)

(test! "All args, named args")
; This is all the declared args of the plugin.
(plug-in-tile #:run-mode   RUN-NONINTERACTIVE
              #:image      testImage
              #:drawables  testLayers
              #:new-width  10 ; width
              #:new-height 20 ; height
              #:new-image  #f)


(test! "Prefix of args, named args")
; Pass only a prefix of the declared args of the plugin.
; The trailing args default.
(plug-in-tile #:run-mode   RUN-NONINTERACTIVE
              #:image      testImage
              #:drawables  testLayers
              )


(test! "Some named args omitted, in order.")
; The prefix is usually non-defaultable.
; But subsequent defaultable args can be omitted.
; And a named arg may follow an omitted arg.
; since new-image passed #f, returns a null image  -1
(assert `(= (car (plug-in-tile
                    #:run-mode   RUN-NONINTERACTIVE
                    #:image      ,testImage
                    #:drawables  ,testLayers
                    ; width and height omitted, they have defaults
                    #:new-image  #f))
            -1))

(test! "Named args out of order (backwards).")
(assert `(= (car (plug-in-tile
                    #:new-image  #f
                    #:drawables  ,testLayers
                    #:image      ,testImage
                    #:run-mode   RUN-NONINTERACTIVE
                    ; width and height omitted, they have defaults
                    ))
            -1))


; Errors


; In SF Console: (....  #:foo) breaks TS with
; Error: Error reading argument slot name
; FIXME An arg name should be terminated by DELIMITERS, including parens etc.
; Now, it is only terminated by whitespace.
; This is an error case anyway, so not critical that #:foo) breaks TS.
; The fix is to call readstr_upto and fix read_arg_name.

(test! "Error to pass an unknown arg name")
(assert-error `(plug-in-tile #:foo )
              "Invalid argument name:")


(test! "Error to pass an arg name i.e. key without value.")
(assert-error `(plug-in-tile #:run-mode )
              "Lonely argument with no value:")


(test! "Error call internal procedure w named args")
; FIXME is there a technical reason why not?
(assert-error `(gimp-drawable-get-bpp #:drawable ,testLayer)
              "in script, expected type: numeric for argument 1 to gimp-drawable-get-bpp")


(test! "Error call /script plugin w named args from extension-script-fu")
; or from SFConsole.
; Because it calls the Scheme run-func without a call via PDB,
; named args are not allowed.
; The error is obscure, it usually is in a PDB procedure deeper in the call tree.
; e.g. this reports an error calling gimp-image-undo-group-start
;; For some reason, this inf loops in the testing framework.
;; It's a problem in the testing framework:
;; the framework does not catch TS errors, only PDB error.
;;(assert-error `(script-fu-guides-remove
;;                  #:run-mode   RUN-NONINTERACTIVE
;;                  #:image      ,testImage
;;                  #:drawables  ,testLayers)
;;              "Error: in script, expected type: numeric for argument 1 to gimp-image-undo-group-start ")

; We can't call this either, the script error ends testing
;(script-fu-guides-remove
;                 #:run-mode   RUN-NONINTERACTIVE
;                 #:image      testImage
;                 #:drawables  testLayers)



(script-fu-use-v2)