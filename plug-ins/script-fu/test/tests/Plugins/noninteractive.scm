; Test script calling plugin scripts non-interactive

; Some are scripts that must be non-interactive (no menu item.)
; Most scripts can be tested using app menus.
; These scripts can only be tested by another plugin or script.

; This also tests and illustrates aspects
; of calling plugin scripts from other scripts:
; inter-process calls versus intra-process calls.

; This also illustrates run-time changing of binding:
; calling a script intraprocess that binds dialect v2 binding
; cannot be done from the dialect v3 binding state.
; While calling a dialect v2 script interprocess
; can be done from v3 binding state.

; FUTURE: call all plugin scripts in repo



; Restored at end of this script
; Return values from PDB bound using v3 semantics
(script-fu-use-v3)



(define testImage (testing:load-test-image-basic-v3))

; get-layers returns (length #(layers))
; Calling another script requires a vector of layers (ever since multi-layer feature.)
; cdr is always a list i.e. (#(layers)), so we need cadr here.
(define testDrawables (cadr (gimp-image-get-layers testImage)))
(define testDrawable  (vector-ref testDrawables 0))

;(newline)
;(display (gimp-image-get-layers testImage)) (newline)
;(display testDrawables) (newline)



(test! "script-fu-unsharp-mask")

; script-fu-unsharp-mask has been replaced by a GEGL filter.
; If it is removed from the repo, please keep a test of some script here,
; if only as an example of how the testing framework can test plugin scripts.

; The test is only: ensure it doesn't error or crash.

; unsharp-mask uses v2 binding
; If called without this, usually error is "must be pair"
(script-fu-use-v2)

; This is an intra-process call, all inside one interpreter.
; (extension-script-fu or script-fu-console or script-fu-interpreter.)
; The interpreter does not actually make a PDB call,
; only a call to a Scheme function, the run-func of the script.
; Use the Scheme signature of the run-func:
;    no run_mode argument.
;    no "num-drawables" argument of the C signature.
(script-fu-unsharp-mask
  ; !!! Not pass run_mode
  testImage
  ; !!! Not pass num_drawables, just a Scheme vector of int ID of drawables
  testDrawables
  128   ; mask-size, radius in pixels
  50    ; opacity percent
  )
; Expect an image with extra layers: "lighter mask" and "darker mask"


; back to v3 binding so we don't need to unwrap calls to gimp-foo in this scope.
(script-fu-use-v3)




(test! "script-fu-helloworld")

; helloworld IS in the menus but here we call it non-interactively

; helloworld is independently interpreted.
; This is an inter-process call from script-fu-console (when testing there)
; to the independent script-fu-interpreter.
; run-mode is required.

; helloworld is not a filter, but a renderer, and takes no image

(script-fu-helloworld
  RUN-NONINTERACTIVE
  "Hello world" ; text
  ; Default for font in declared ScriptFu formal argument can be a simple string.
  ; Here requires a font object ID, not a string.
  ; FUTURE: enhance binding of font in libscriptfu/scheme-wrapper.c
  (gimp-font-get-by-name "Sans-serif")  ; font
  32            ; size
  "red")        ; A string CSS name suffices to denote a color
; expect the same image as when tested interactively, "Hello world" on transparent.

; The script executes in its own process, with its own dialect v3 binding.
; Expect we are still in dialect v3 binding.




(test! "script-fu-test-sphere-v3")

; test the binding of all possible types to GIMP

; Sphere v3 registers as a filter, but is actually a renderer.
; It does not use passed image and drawables.

; The script is independently interpreted in another process.
; !!! When called from another script, use the C API as definition of args.
; It requires:
;   run_mode
;   an int count of drawables
;   a vector of drawable ID for drawables

(script-fu-test-sphere-v3
  RUN-NONINTERACTIVE           ; run_mode       see above interprocess
  testImage                    ; unused          Image
  1                            ; num-drawables   int size of GimpObjectArray
  testDrawables                ; unused          GimpObjectArray, Scheme vector of int ID
  ; radius is actually dimension of the square rendered image
  300                          ; radius          int
  45                           ; light           float
  #t                           ; shadow          boolean
  "blue"                       ; bg-color
  "green"                      ; sphere-color
  (gimp-brush-get-by-name
    "2. Hardness 100")         ; brush
  "foo"                        ; text             string
  "bar \n zed"                 ; multi-text       string
  (gimp-pattern-get-by-name
    "Maple Leaves")            ; pattern
  (gimp-gradient-get-by-name
    "Deep Sea")                ; gradient
  #f                           ; gradient-reverse  boolean
  (gimp-font-get-by-name
    "Sans-serif")              ; font
  32                           ; font size         int
  (gimp-palette-get-by-name
    "Gold")                    ; unused-palette
  (string-append
    gimp-data-directory
    "/scripts/images/beavis.jpg") ;unused-filename
  ; A script can define symbols for enum constants, but here we haven't.
  ; Such defined symbols are an enum private to the script.
  ; The GUI fakes enum names i.e. choices.
  ; The default choice "Horizontal" of the GUI is not a symbol we can use here.
  0                            ; orientation            script local enum
  INTERPOLATION-LINEAR         ; unused-interpolation   GIMP enum
  "not used"                   ; unused-dirname
  testImage                    ; unused-image
  testDrawable                 ; unused-layer
  ; GimpObject passed as int ID, zero suffices when not used.
  0                            ; unused-channel
  testDrawable                 ; unused-drawable
  )
; Expect a rendering of a sphere, as when tested from GUI,
; but different in color, font, etc.



; !!! Restore dialect binding state so SF Console remains binding v2
; (when testing from the SF Console.)
(script-fu-use-v2)