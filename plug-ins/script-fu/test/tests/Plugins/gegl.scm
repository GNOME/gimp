
; Test GEGL plugins

; This can also be stress test for GIMP itself: opens hundreds of images

; WORK IN PROGRESS
; This is brute force, it doesn't test effects, mainly tests for crashes.
; The API is very new and subject to change.

; HISTORY
; For plugin authors porting older plugins.
;
; In GIMP 2 (say about 2.8) many filters were moved to GEGL
; and in GIMP, wrapper plugins were defined in the PDB.
; The wrappers were defined in /pdb/groups/plug_in_compat.pdb
; A wrapper procedure was named like plug-in-antialias,
; where "anti-alias" is a name of a wrapped filter in GEGL
; Not all GEGL filters were wrapped.
;
; Some GEGL filters appear in the GUI (apart from Gegl Ops menu.)
; via app/actions/filters-actions.c and image-menu.ui (xml)
;
; Since GIMP 3, new procedures were added to the PDB for a plugin
; to "use" a GEGL filter.
; Filters are "added" to a layer.  They are initially NDE (non-destructive)
; After adding a filter, they can be "merged" meaning convert to destructive
; (the effect becomes more permanent, the parameters of the filter
; can no longer be tweaked, i.e. the filter is no longer parameterized.)
;
; The new API in the PDB is:
; gimp-drawable-filter-new etc.
; ScriptFu also has new functions (not in the PDB):
; gimp-drawable-filter-configure, gimp-drawable-filter-set-aux-input,
; gimp-drawable-merge-filter, gimp-drawable-append-filter, etc.
;
; The wrappers also declared ranges for formal args, matching GEGL declared ranges.
; Since wrappers no longer exist, GEGL declared ranges are the only ones effective.
;
; The new API lets an author call ANY Gegl plugin.
; Formerly, some Gegl filters had no wrapper, so you could not call them.


; ABOUT THE TESTS
;
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
            "LayerNew"
            4
            4
            RGB-IMAGE
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
            "LayerNew"
            4
            4
            GRAY-IMAGE  ; GRAY
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


; Functions to create filters
; Filters are now objects with methods.

; Returns a filter on the global testLayer
; The filter is configured minimally.
(define (createGeglFilter name-suffix)
  (let* ((filter-name (string-append "gegl:" name-suffix))
         (filter (gimp-drawable-filter-new
            testLayer
            filter-name
            ""    ; user given "effect name"
            )))
    ; configure the filter.
    ; ??? Do these default?
    (gimp-drawable-filter-configure filter
      LAYER-MODE-REPLACE   ; blend mode
      1.0)                 ; opacity

    ; Return the filter
    filter))


; CRUFT to call wrappers, now obsolete.
;(assert `(
;    ; form the full name of the plugin, as a symbol
;  ,(string->symbol (string-append "plug-in-" name-suffix))
;   RUN-NONINTERACTIVE ,testImage ,testLayer))


; Define test harness functions

; Function to "add" and "merge" GEGL filter with "usual" arguments
; Omitting most arguments, which are defaulted.
(define (testGeglFilter name-suffix)
  (test! name-suffix)

  ; new image for each test
  (testImageCreator)

  (let* ((filter (createGeglFilter name-suffix))

    ; key/value pairs for other args
    ;"amount-x" 0.0 "amount-x" 0.0 "abyss-policy" "loop"
    ;"sampler-type" "cubic" "displace-mode" "cartesian")

    ; The filter is floating, i.e. not on the layer until we append it
    ; FIXME: why isn't it named attach like other PDB functions?
    (gimp-drawable-append-filter testLayer filter)

    ; Make the filter visible in the GUI
    ; TODO and effective?
    (gimp_drawable_filter_set_visible filter #t)

    ; Make the filter destructive i.e. effects permanent
    ;(gimp-drawable-merge-filter testLayer filter)
  ) ; end let*

  ; not essential, but nice, to display result images
  (gimp-display-new testImage)
  (gimp-displays-flush)
  ))


; Function to call gegl plugin with "usual" arguments plus another arg
; Omitting trailing arguments.
(define (testGeglFilterAuxInput name-suffix aux-input)
  (test! name-suffix)
  (testImageCreator)

  (let* ((filter (createGeglFilter name-suffix))
    ;
    (gimp-drawable-filter-set-aux-input filter "aux" aux-input)
    (gimp-drawable-merge-filter testLayer filter)))

  (gimp-display-new testImage)
  (gimp-displays-flush)
  )

; NOTES
; These notes are about historical renaming and can be wrong....
; Certain filters named like plug-in- are compatibility plugins but not GEGL wrappers:
;    autocrop
;    autocrop-layer
; Certain GEGL wrappers were on the same GEGL plugin:
;    plug-in-bump-map-tiled a second wrapper of gegl:bump-map
; Certain GEGL wrappers had names not the same as the wrapped GEGL plugin:
;    apply-canvas    texturize-canvas
;    applylens       apply-lens
;    autostretch-hsv stretch-contrast-hsv
;    c-astretch      stretch-contrast
;    colortoalpha    color-to-alpha
;    colors-channel-mixer channel-mixer
;    diffraction     diffraction-patterns
;    dog             difference-of-gaussians
;    exchange        color-exchange
;    flarefx         lens-flare
;    gauss           gaussian-blur
;    glasstile       tile-glass
;    hsv-noise       noise-hsv
;    laplace         edge-laplace
;    make-seamless   seamless-clone or seamless-clone-compose
;    neon            edge-neon
;    noisify         ???
;    polar-coords    polar-coordinates
;    randomize-hurl  noise-hurl
;    randomize-pick  noise-pick
;    randomize-slur  noise-slur
;    rgb-noise       noise-rgb
;    sel-gauss       gaussian-blur-selective
;    sobel           edge-sobel
;    softglow        soft-light ???
;    solid-noise     simplex-noise ???
;    spread          noise-spread
;    video           video-degradation
;    vinvert         value-invert
;    vpropagate      value-propagate
;
; Certain filters are GIMP operations, no longer wrapped and callable from SF?
;     semi-flatten
;     threshold-alpha      ; requires alpha
;
; Certain compatibility procedures might have been specialized calls of gegl filters
; (with special, non-default parameters)
;    dilate          value-propagate, specialized?
;    erode           value-propagate, specialized?
;    mblur-inward    mblur specialized?
;    normalize       stretch-contrast specialized?
;    oilify-enhanced oilify specialized?



; list of names of all gegl filters
; That take no args, or args that properly default.
; In alphabetical order.
; This list is hand-built from former wrapper names, and may be incomplete.
; FIXME it should be extracted from GEGL but there is no API?

; a short list for use while developing tests
(define gegl-filter-names2
  (list
    "antialias" ))

(define gegl-filter-names
  (list
    "antialias"
    "alpha-clip"   ; see threshold-alpha, similar ?
    "apply-lens"
    ; "bump-map" tested separately, requires aux input
    "cartoon"
    "channel-mixer"
    "color-to-alpha"   ; color defaultable?
    "color-exchange"
    "cubism"
    "deinterlace"
    "diffraction-patterns"
    ; "displace" tested separately, requires two aux inputs
    "difference-of-gaussians"  ; an edge detect
    "edge"
    "edge-laplace"
    "edge-neon"
    "edge-sobel"
    "emboss"
    "engrave"
    "fractal-trace"
    "gaussian-blur"
    "gaussian-blur-selective"
    "illusion"
    "image-gradient"   ; an edge detect
    "invert-linear"
    "lens-distortion"
    "lens-flare"
    "maze"
    "mblur"
    "median-blur"
    "mosaic"
    "newsprint"
    "noise-hsv"
    "noise-hurl"
    "noise-pick"
    "noise-spread"
    "noise-slur"
    "noise-rgb"
    ; TODO more noise- not wrapped e.g. iCH
    "oilify"
    "photocopy"
    "pixelize"
    "plasma"
    "polar-coordinates"
    "red-eye-removal"
    "ripple"
    ; FIXME crashes SF "rotate"
    "seamless-clone"
    "seamless-clone-compose"
    "shift"
    "soft-light"
    "spherize"
    "simplex-noise"
    "stretch-contrast"
    "stretch-contrast-hsv"
    "texturize-canvas"
    "tile-glass"
    "unsharp-mask"
    "value-invert"
    "value-propagate"
    "video-degradation"
    ; distorts, see also engrave, ripple, shift, spherize
    "waves"
    "whirl-pinch"
    "wind"))


(define (testGeglFiltersTakingNoArg)
  ; change this to names2 for a shorter test
  (map testGeglFilter gegl-filter-names))

; tests Gegl wrappers taking an aux input
; Most filters have input a single layer
; An aux input is an argument that is a second input, another layer
(define (testGeglFiltersTakingAuxInput)
  ; These require a non-defaultable aux input arg: bump map
  ; TODO using the testLayer as bump map produces little visible effect?
  (testGeglFilterAuxInput "bump-map" testLayer)
  ; "bump-map-tiled" was a wrapper but calls "bump-map"

  ; Requires an explicit color arg, until we fix defaulting colors in the PDB machinery
  ; TODO (testGeglFilter2 "colortoalpha" "red")
  )

(define (testGeglFilters)
  (testGeglFiltersTakingNoArg)
  (testGeglFiltersTakingAuxInput)
  (testSpecialGeglFilters))



; Tests of gegl wrappers by image modes
; Note you can't just redefine testImageCreator,
; it is not in scope, you must set a global.
(define (testGeglFiltersRGBA)
  (test! "Test GEGL wrappers on RGBA")
  (set! testImageCreator createRGBATestImage)
  (testGeglFilters))

(define (testGeglFiltersSmallRGBA)
  (test! "Test GEGL wrappers on small image")
  (set! testImageCreator createSmallTestImage)
  (testGeglFilters))

(define (testGeglFiltersSmallGRAY)
  (test! "Test GEGL wrappers on small gray image")
  (set! testImageCreator createSmallGrayTestImage)
  (testGeglFilters))

(define (testGeglFiltersINDEXED)
  (test! "Test GEGL wrappers on INDEXED image")
  (set! testImageCreator createIndexedImage)
  (testGeglFilters))





; test GEGL wrappers having args that don't default
(define (testSpecialGeglFilters)
  (test! "Special test GEGL wrappers")

  ; "displace" requires two non-defaultable aux inputs i.e. maps.
  ; We can never add test auto-defaulting the args.
  ; These are NOT the declared defaults.
  (test! "displace")
  (testImageCreator)
  (let* ((filter (createGeglFilter "displace"))
    ; using testLayer as both aux inputs
    (gimp-drawable-filter-set-aux-input filter "aux" testLayer)
    (gimp-drawable-filter-set-aux-input filter "aux2" testLayer)
    (gimp-drawable-merge-filter testLayer filter)))

  (gimp-display-new testImage)
  (gimp-displays-flush)
)


; Testing begins here.

; You can comment them out.
; Testing them all creates about 300 images

; tests on RGBA
(testGeglFiltersRGBA)

; tests of small images is an edge test
; and don't seem to reveal any bugs, even for a 1x1 image
(testGeglFiltersSmallRGBA)

; tests of other image modes tests conversions i.e babl
(testGeglFiltersSmallGRAY)

; FIXME some throw CRITICAL
(testGeglFiltersINDEXED)

; Not testing image without alpha.
; It only shows that semiflatten and thresholdalpha return errors,
; because they require alpha.

; Not testing other image precisions and color profiles.


(test! "Non-existing filter")
; test creating filter for an invalid name
; TODO
; Procedure execution of gimp-drawable-filter-new failed on invalid input arguments:
; drawable_filter_new_invoker: the filter "gegl:bump-map-tiled" is not installed.

