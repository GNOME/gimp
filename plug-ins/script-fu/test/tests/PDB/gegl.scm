
; Test the PDB procedures that wrap GEGL plugins

; The procedures defined in /pdb/groups/plug_in_compat.pdb
; Not all GEGL filters are wrapped.
; Some GEGL filters appear in the GUI (apart from Gegl Ops menu.)
; via app/actions/filters-actions.c and image-menu.ui (xml)
; Here we are calling the wrapped GEGL filters non-interactively.

; Each test is a simple call.
; No test for effectiveness, only that a call succeeds.

; Some tests omit args: ScriptFu calls anyway,
; and some machinery (where?) substitutes a default value for actual arg.
; Note some arguments (e.g. drawables) are not defaultable.

; This tests that the formal args declared for the PDB wrapper
; are at least as restrictive in range as declared by GEGL.
; Otherwise, throws CRITICAL: ...out of range...

; When defaulting args, it is not intuitive to see in the image whether tests are effective:
; default args may produce all black, or all transparent, etc.
; IOW, the defaults are not always sane.

; FIXME: color args should default also.
; Possibly a flaw in GeglParamColor,
; not implementing the reset method of the Config interface?

; If you add tests for effectiveness, leave the defaulted tests.
; Since they have a testing purpose: test PDB declared defaults.

; Most are fundamental filters, and unlikely to change,
; unlike some of the other filters not migrated to GEGL.

; tests in alphabetic order


; global setup: each test creates a new image, but occasionally we need testLayer
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))


; This script is not v3, load-test-image is not compatible, yet
; (script-fu-use-v3)


; Define test harness functions

; Function to call gegl wrapper plugin with "usual" arguments
; Omitting most arguments.
(define (testGeglWrapper name-suffix)
  (test! name-suffix)
  ; new image for each test
  (define testImage (testing:load-test-image "gimp-logo.png"))
  (define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                              0))
  (assert `(
      ; form the full name of the plugin, as a symbol
     ,(string->symbol (string-append "plug-in-" name-suffix))
     RUN-NONINTERACTIVE ,testImage ,testLayer))
  ; not essential, but nice, to display result images
  (gimp-display-new testImage)
  (gimp-displays-flush)
  )

; Function to call gegl plugin with "usual" arguments plus another arg
; Omitting trailing arguments.
(define (testGeglWrapper2 name-suffix other-arg)
  (test! name-suffix)
  (define testImage (testing:load-test-image "gimp-logo.png"))
  (define testLayer (vector-ref (cadr (gimp-image-get-layers testImage ))
                              0))
  (assert `(
     ,(string->symbol (string-append "plug-in-" name-suffix))
     RUN-NONINTERACTIVE ,testImage ,testLayer
     ,other-arg))
  (gimp-display-new testImage)
  (gimp-displays-flush)
  )




(test! "GEGL wrappers")

; This is a typical, more elaborate test using sensical, not default, values.
; Note that freq 0 produces little effect
(assert
  `(plug-in-alienmap2
      RUN-NONINTERACTIVE ,testImage ,testLayer
      1 0 ; red freq, angle
      1 0 ; green
      1 0 ; blue
      0 ; TODO what is the enum symbol RGB-MODEL ; color model
      1 1 1 ;  RGB application modes
      ; when script-fu-use-v3 #t #t #t
      ))

; This tests the same as above using defaulted args.
(testGeglWrapper "alienmap2")
(testGeglWrapper "antialias")
(testGeglWrapper "apply-canvas")
(testGeglWrapper "applylens")
(testGeglWrapper "autocrop")
; !!! autocrop layer is not a GEGL wrapper
(testGeglWrapper "autostretch-hsv")

; These require a non-defaultable arg: bump map
; TODO using the testLayer as bump map produces little visible effect?
(testGeglWrapper2 "bump-map" testLayer)
(testGeglWrapper2 "bump-map-tiled" testLayer)

(testGeglWrapper "c-astretch")
(testGeglWrapper "cartoon")
(testGeglWrapper "colors-channel-mixer")

; Requires an explicit color arg, until we fix defaulting colors in the PDB machinery
(testGeglWrapper2 "colortoalpha" "red")

; Requires non-defaultable convolution matrix
(test! "convmatrix")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-convmatrix
     RUN-NONINTERACTIVE ,testImage ,testLayer
     25 ; count elements, must be 25
     #(1 2 3 4 5   1 2 3 4 5  1 2 3 4 5  1 2 3 4 5  1 2 3 4 5) ; conv matrix
     0 0 0
     5
     #(1 0 1 0 1) ; channel mask
     0 ; border mode
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

(testGeglWrapper "cubism")
(testGeglWrapper "deinterlace")
(testGeglWrapper "diffraction")

; Requires non-defaultable maps.
; We can never add test auto-defaulting the args.
; These are NOT the declared defaults.
(test! "displace")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-displace
     RUN-NONINTERACTIVE ,testImage ,testLayer
     0 0 ; x, y  default is -500
     0 0 ; do displace x, y booleans
     ,testLayer ,testLayer ; x, y maps
     1 ; edge behaviour
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

; Requires non-defaultable maps
(test! "displace-polar")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-displace-polar
     RUN-NONINTERACTIVE ,testImage ,testLayer
     0 0 ; multiplier radial, tangent  default is -500
     0 0 ; do displace x, y booleans
     ,testLayer ,testLayer ; x, y maps
     1 ; edge behaviour
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

(testGeglWrapper "dog")
(testGeglWrapper "edge")
(testGeglWrapper "emboss")
(testGeglWrapper "engrave")
(testGeglWrapper "exchange")
(testGeglWrapper "flarefx")
(testGeglWrapper "fractal-trace")
(testGeglWrapper "gauss")
(testGeglWrapper "gauss-iir")
(testGeglWrapper "gauss-iir2")
(testGeglWrapper "gauss-rle")
(testGeglWrapper "gauss-rle2")
(testGeglWrapper "glasstile")
(testGeglWrapper "hsv-noise")
(testGeglWrapper "illusion")
(testGeglWrapper "laplace")
(testGeglWrapper "lens-distortion")
(testGeglWrapper "make-seamless")
(testGeglWrapper "maze")
(testGeglWrapper "mblur")
(testGeglWrapper "mblur-inward")
(testGeglWrapper "median-blur")
(testGeglWrapper "mosaic")
(testGeglWrapper "neon")
(testGeglWrapper "newsprint")
(testGeglWrapper "normalize")

; Requires non-defaultable color
; (testGeglWrapper "nova")
(test! "nova")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-nova
     RUN-NONINTERACTIVE ,testImage ,testLayer
     0 0 "red" ; other args defaulted
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

(testGeglWrapper "oilify")
(testGeglWrapper "oilify-enhanced")

; Requires non-defaultable color
(test! "papertile")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-papertile
     RUN-NONINTERACTIVE ,testImage ,testLayer
     1 ; tile size (width, height as one arg)
     1.0 ; move rate
     0 ; fractional type enum
     0 0 ; wrap around, centering boolean
     5  ; background type enum
     "red" ; color when background type==5
     ; other args defaulted
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

(testGeglWrapper "photocopy")
(testGeglWrapper "pixelize")
(testGeglWrapper "pixelize2")
(testGeglWrapper "plasma")
(testGeglWrapper "polar-coords")
(testGeglWrapper "randomize-hurl")
(testGeglWrapper "randomize-pick")
(testGeglWrapper "randomize-slur")
(testGeglWrapper "red-eye-removal")
(testGeglWrapper "rgb-noise")
(testGeglWrapper "ripple")
(testGeglWrapper "rotate")
(testGeglWrapper "noisify")
(testGeglWrapper "sel-gauss")
(testGeglWrapper "semiflatten")
(testGeglWrapper "shift")

; Requires non-defaultable color
;(testGeglWrapper "sinus")
(test! "sinus")
(define testImage (testing:load-test-image "gimp-logo.png"))
(define testLayer (vector-ref (cadr (gimp-image-get-layers testImage )) 0))
(assert `(plug-in-sinus
     RUN-NONINTERACTIVE ,testImage ,testLayer
     0.1 0.1 ; x, y scale
     0 0 0 0 0 "red" "green"
     ; other args defaulted
     ))
(gimp-display-new testImage)
(gimp-displays-flush)

(testGeglWrapper "sobel")
(testGeglWrapper "softglow")
(testGeglWrapper "solid-noise")
(testGeglWrapper "spread")
(testGeglWrapper "threshold-alpha")
(testGeglWrapper "unsharp-mask")
(testGeglWrapper "video")
(testGeglWrapper "vinvert")
(testGeglWrapper "vpropagate")
(testGeglWrapper "dilate")
(testGeglWrapper "erode")
(testGeglWrapper "waves")
(testGeglWrapper "whirl-pinch")
(testGeglWrapper "wind")

