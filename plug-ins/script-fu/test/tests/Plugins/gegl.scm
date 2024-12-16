
; Test the PDB procedures that wrap GEGL plugins

; This can also be stress test for GIMP itself: opens hundreds of images

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

; When defaulting args, you can't see in the displayed image that tests are effective:
; default args may produce all black, or all transparent, etc.
; IOW, the defaults are not always sane.

; FIXME: color args should default also?
; Possibly a flaw in GeglParamColor.

; If you add tests for effectiveness, leave the defaulted tests.
; Since they have a testing purpose: test PDB declared defaults.

; Most are fundamental filters, and unlikely to change,
; unlike some of the other filters not migrated to GEGL.

; tests in alphabetic order



(script-fu-use-v3)


; setup

; global variables
; Each test is on a new testImage, occasionally we need testLayer.
; testImageCreator is a function called by every test case.
; We set testImageCreator before various rounds of testing.
(define testImage '())
(define testLayer '())
(define testImageCreator '())



; Functions to create new test image and set the global variables.
; Sets global variables. The prior image still exists in GIMP.

; Create an average RGBA image
(define (createRGBATestImage)
  ; "gimp-logo.png"
  (set! testImage (testing:load-test-image-basic-v3))
  (set! testLayer (vector-ref (gimp-image-get-layers testImage) 0))

  ; Optional stressing: change selection
  ; select everything (but I think a filter does that anyway, when no selection exists)
  ;(gimp-selection-all testImage)
  ;(gimp-selection-feather testImage  10)
  )

; Create a smallish image, with alpha.
; You can also test with 1 1
; !!! Note the size is in two places
(define (createSmallTestImage)
  (set! testImage (gimp-image-new 4 4 RGB)) ; base type RBG
  (set! testLayer
    (gimp-layer-new
            testImage
            4
            4
            RGB-IMAGE
            "LayerNew"
            50.0
            LAYER-MODE-MULTIPLY)) ; NORMAL
  ; must insert layer in image
  (gimp-image-insert-layer
            testImage
            testLayer
            0  ; parent
            0  )  ; position within parent
  (gimp-layer-add-alpha testLayer)
)

; Create a small GRAY image, with alpha
(define (createSmallGrayTestImage)
  (set! testImage (gimp-image-new 4 4 GRAY)) ; base type
  (set! testLayer
    (gimp-layer-new
            testImage
            4
            4
            GRAY-IMAGE  ; GRAY
            "LayerNew"
            50.0
            LAYER-MODE-NORMAL))
  ; must insert layer in image
  (gimp-image-insert-layer
            testImage
            testLayer
            0  ; parent
            0  )  ; position within parent
  (gimp-layer-add-alpha testLayer)
)

; Create an INDEXED image, with alpha
(define (createIndexedImage)
  (set! testImage (testing:load-test-image-basic-v3))
  (set! testLayer (vector-ref (gimp-image-get-layers testImage) 0))
  (gimp-image-convert-indexed
                  testImage
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  255  ; color count
                  #f  ; alpha-dither
                  #t ; remove-unused
                  "myPalette" ; ignored
                  )
)





; Define test harness functions

; Function to call gegl wrapper plugin with "usual" arguments
; Omitting most arguments, which are defaulted.
(define (testGeglWrapper name-suffix)
  (test! name-suffix)

  ; new image for each test
  (testImageCreator)

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
  (testImageCreator)
  (assert `(
     ,(string->symbol (string-append "plug-in-" name-suffix))
     RUN-NONINTERACTIVE ,testImage ,testLayer
     ,other-arg))
  (gimp-display-new testImage)
  (gimp-displays-flush)
  )

; !!! autocrop layer is not a GEGL wrapper

; list of names of all gegl wrappers that take no args, or args that properly default
; in alphabetical order.
(define gegl-wrapper-names
  (list
    "antialias"
    "apply-canvas"
    "applylens"
    "autocrop"
    "autostretch-hsv"
    "c-astretch"
    "cartoon"
    "colors-channel-mixer"
    "cubism"
    "deinterlace"
    "diffraction"
    "dog"
    "edge"
    "emboss"
    "engrave"
    "exchange"
    "flarefx"
    "fractal-trace"
    "gauss"
    "glasstile"
    "hsv-noise"
    "illusion"
    "laplace"
    "lens-distortion"
    "make-seamless"
    "maze"
    "mblur"
    "mblur-inward"
    "median-blur"
    "mosaic"
    "neon"
    "newsprint"
    "normalize"
    "oilify"
    "oilify-enhanced"
    "photocopy"
    "pixelize"
    "plasma"
    "polar-coords"
    "randomize-hurl"
    "randomize-pick"
    "randomize-slur"
    "red-eye-removal"
    "rgb-noise"
    "ripple"
    "rotate"
    "noisify"
    "sel-gauss"
    "semiflatten" ; requires alpha and returns error otherwise
    "shift"
    "sobel"
    "softglow"
    "solid-noise"
    "spread"
    "threshold-alpha"  ; requires alpha
    "unsharp-mask"
    "video"
    "vinvert"
    "vpropagate"
    "dilate"
    "erode"
    "waves"
    "whirl-pinch"
    "wind"))


(define (testGeglWrappersTakingNoArg)
  (map testGeglWrapper gegl-wrapper-names))

; tests Gegl wrappers with one other arg
(define (testGeglWrappersTakingOneArg)
  ; These require a non-defaultable arg: bump map
  ; TODO using the testLayer as bump map produces little visible effect?
  (testGeglWrapper2 "bump-map" testLayer)
  (testGeglWrapper2 "bump-map-tiled" testLayer)

  ; Requires an explicit color arg, until we fix defaulting colors in the PDB machinery
  (testGeglWrapper2 "colortoalpha" "red")
  )

(define (testGeglWrappers)
  (testGeglWrappersTakingNoArg)
  (testGeglWrappersTakingOneArg)
  (testSpecialGeglWrappers))



; Tests of gegl wrappers by image modes
; Note you can't just redefine testImageCreator,
; it is not in scope, you must set a global.
(define (testGeglWrappersRGBA)
  (test! "Test GEGL wrappers on RGBA")
  (set! testImageCreator createRGBATestImage)
  (testGeglWrappers))

(define (testGeglWrappersSmallRGBA)
  (test! "Test GEGL wrappers on small image")
  (set! testImageCreator createSmallTestImage)
  (testGeglWrappers))

(define (testGeglWrappersSmallGRAY)
  (test! "Test GEGL wrappers on small gray image")
  (set! testImageCreator createSmallGrayTestImage)
  (testGeglWrappers))

(define (testGeglWrappersINDEXED)
  (test! "Test GEGL wrappers on INDEXED image")
  (set! testImageCreator createIndexedImage)
  (testGeglWrappers))





; test GEGL wrappers having args that don't default
(define (testSpecialGeglWrappers)
  (test! "Special test GEGL wrappers")

  ; Requires non-defaultable maps.
  ; We can never add test auto-defaulting the args.
  ; These are NOT the declared defaults.
  (test! "displace")
  (testImageCreator)
  (let* ((filter (car (gimp-drawable-filter-new ,testLayer "gegl:displace" ""))))
    (gimp-drawable-filter-configure filter LAYER-MODE-REPLACE 1.0
                                    "amount-x" 0.0 "amount-x" 0.0 "abyss-policy" "loop"
                                    "sampler-type" "cubic" "displace-mode" "cartesian")
    (gimp-drawable-filter-set-aux-input filter "aux" ,testLayer)
    (gimp-drawable-filter-set-aux-input filter "aux2" ,testLayer)
    (gimp-drawable-merge-filter ,testLayer filter)
  )
  (gimp-display-new testImage)
  (gimp-displays-flush)
)


; Testing begins here.

; You can comment them out.
; Testing them all creates about 300 images

; tests on RGBA
(testGeglWrappersRGBA)

; tests of small images is an edge test
; and don't seem to reveal any bugs, even for a 1x1 image
(testGeglWrappersSmallRGBA)

; tests of other image modes tests conversions i.e babl
(testGeglWrappersSmallGRAY)

; FIXME some throw CRITICAL
(testGeglWrappersINDEXED)

; Not testing image without alpha.
; It only shows that semiflatten and thresholdalpha return errors,
; because they require alpha.

; Not testing other image precisions and color profiles.
