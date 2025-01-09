; test colors

; Color has no methods in the PDB API.
; Test methods on other objects that use colors.

; A color is represented in SF by a list of numeric

; A color to PDB may also be represented by it's name

; This does not test declaration of color args to a script.
; TODO test declaring a default by name "transparent"

; !!! Setting a color to a perceptually identical color should not affect these tests.
; Many tests of perceptually-identical were removed from the GIMP code.

; Returned values not wrapped in lists.
(script-fu-use-v3)


; setup

; Ensure context background color is white
(gimp-context-set-background "white")

; Create separate test images, avoid side effects trying to use one testImage
; load gimp-logo.png using v3 binding

; an image mode rgba i.e. with alpha
; require existing image gimp-logo.png is rgb with alpha
(define testImage (testing:load-test-image-basic-v3))
(define testDrawable (vector-ref (gimp-image-get-layers testImage) 0))

; an image mode rgb flat
; Create separate image, avoid side effects on testImage
(define testImageFlat (testing:load-test-image-basic-v3))
; flatten returns a drawable
(define testDrawableFlat (gimp-image-flatten testImageFlat))

; an image mode grayscale w alpha
(define testImageGrayA (testing:load-test-image-basic-v3))
(gimp-image-convert-grayscale testImageGrayA)
(define testDrawableGrayA (vector-ref (gimp-image-get-layers testImageGrayA) 0))

; an image mode grayscale w/o alpha
(define testImageGray (testing:load-test-image-basic-v3))
(gimp-image-convert-grayscale testImageGray)
; flatten returns a drawable
(define testDrawableGray (gimp-image-flatten testImageGray))

; an image mode indexed
(define testImageIndexed (testing:load-test-image-basic-v3))
; Note these are the defaults, so results are reproducible in the app
(gimp-image-convert-indexed
                  testImageIndexed
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  255  ; color count
                  #f  ; alpha-dither
                  #t ; remove-unused
                  "myPalette" ; ignored
                  )
(define testDrawableIndexed (vector-ref (gimp-image-get-layers testImageIndexed) 0))



(test! "colors from context")

; context color is a list of four
; test requires setup: context background color is white
; white 255 255 255 and opaque 255
(assert `(equal? (gimp-context-get-background)
                 '(255 255 255 255)))



(test! "colors from images of various modes")

(test! "colors from images modes rgba and rgb")
; a color from an image with alpha is a list of four
; upper left pixel in wilber is transparent
; For some reason, black 0 0 0 and transparent 0
(assert `(equal? (gimp-drawable-get-pixel ,testDrawable 1 1)
                 '(0 0 0 0)))

; a color from an image without alpha is a list of three
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableFlat 1 1)
                 '(255 255 255)))

(test! "colors from images modes gray and graya")

; a color from a grayscale w/o alpha image is a list of one
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableGray 1 1)
                  '(255)))

; a color from a grayscale with alpha image is a list of one
; For gimp-logo.png the background is 0 i.e. black
; For wilber.png the background is 71 some intermediate gray
; FIXME, why do two images rgba converted to gray and flattened
; have different values for a total transparent pixel?
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableGrayA 1 1)
                  '(0 0)))

(test! "colors from indexed image")

; a color from an indexed image is a list of four
; The color is not represented in SF as an index, i.e. a single numeric.
; Note the rgb is black 0,0,0 , same as before conversion to indexed.
; Note the image still has an alpha and pixel 1,1 is transparent
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableIndexed 1 1)
                 '(0 0 0 0)))
; FIXME: strange behavior:
; sometimes '(139 127 114 0)))
; sometimes '(0 0 0 0)))

(display (gimp-drawable-get-pixel testDrawableIndexed 1 1))





(test! "colors to context")

; a list of three is a valid color for setting context
; SF will fabricate alpha 1.0 but Gimp will ignore
(assert `(gimp-context-set-background '(255 255 255)))
; but the effect is white opaque
(assert `(equal? (gimp-context-get-background)
                 '(255 255 255 255)))

; a list of four is a valid color for setting context
; GIMP app will ignore the alpha.
(assert `(gimp-context-set-background '(255 255 255 255)))

; transparent
; GIMP app will ignore the alpha and set to black opaque
(assert `(gimp-context-set-background '(0 0 0 0)))
; but the effect is black opaque
(assert `(equal? (gimp-context-get-background)
                 '(0 0 0 255)))

; a color name is a valid color
(assert `(gimp-context-set-background "white"))

; transparent is a valid color name
(assert `(gimp-context-set-background "transparent"))
; but the effect is black opaque
(assert `(equal? (gimp-context-get-background)
                 '(0 0 0 255)))


(test! "color names and notations")

; a color name is a valid color
(assert `(gimp-context-set-background "white"))

; hex notation is a valid color
(assert `(gimp-context-set-background "#FFFFFF"))
(assert `(equal? (gimp-context-get-background)
                 '(255 255 255 255)))

; converter call notation is a valid color
; other converter funcs are "rgb" "rgba" "hsla"
; But this still sets to RGB
(assert `(gimp-context-set-background "hsl(0 0 0)"))
(assert `(equal? (gimp-context-get-background)
                 '(0 0 0 255)))



(test! "colors to image pixel")

; a list of three is valid for an operation on an image with alpha
; SF sets alpha to 1.0
(assert `(gimp-drawable-set-pixel ,testDrawable 1 1 '(2 2 2)))

; a list of four is valid for an operation on an image with alpha
(assert `(gimp-drawable-set-pixel ,testDrawable 1 1 '(3 3 3 3)))

; a color name is valid for an operation on an image with alpha
(assert `(gimp-drawable-set-pixel ,testDrawable 1 1 "white"))

; a list of one is valid for an operation on a gray image w/o alpha
(assert `(gimp-drawable-set-pixel ,testDrawableGray 1 1 '(255)))

; a list of two is valid for an operation on a gray image w alpha
(assert `(gimp-drawable-set-pixel ,testDrawableGrayA 1 1 '(255 50)))




(test! "Gray and other color lists to context")

; any list of length [1,inf] sets context

; length 1,2  sets context from grayscale, i.e. to a shade of black
; Conversion to rgb in GIMP???

(assert `(gimp-context-set-background '(0)))
(assert `(equal? (gimp-context-get-background)
                 '(0 255)))

(assert `(gimp-context-set-background '(0 0)))
(assert `(equal? (gimp-context-get-background)
                 '(0 255)))


(assert `(gimp-context-set-background '(0 0 0)))
; specifies no alpha component, but GIMP makes opaque
(assert `(equal? (gimp-context-get-background)
                 '(0 0 0 255)))

; sets alpha 0, but context ignores alpha
(assert `(gimp-context-set-background '(0 0 0 0)))
(assert `(equal? (gimp-context-get-background)
                 '(0 0 0 255)))




(test! "Extra components in colors")
; length 5, only 4 components used
(assert `(gimp-context-set-background '(0 0 0 0 5)))


(test! "RGB to grayscale image")
; a CSS name is valid for an operation on a gray image
(assert `(gimp-drawable-set-pixel ,testDrawableGray 1 1 "white"))
; conversion to grayscale done in GIMP
(assert `(equal? (gimp-drawable-get-pixel ,testDrawableGray 1 1)
                  '(255)))


(test! "Float numerics in colors")
; a float is accepted but converts to an integer

(assert `(gimp-context-set-background '(0.5)))
; 0.5 rounds down to 0
; the effect is black opaque, in grayscale
(assert `(equal? (gimp-context-get-background)
                 '(0 255)))


(test! "Integers >255 in colors")
; an integer >8 bits is accepted but clamped to 255
(assert `(gimp-context-set-background '(-1 256 -1 0)))
; clamped to [0, 255]
; and opaque (because it is context)
(assert `(equal? (gimp-context-get-background)
                 '(0 255 0 255)))


(test! "invalid colors")

; an empty list is not a valid color
(assert-error `(gimp-context-set-background '())
                "in script, expected type: color list of numeric components ")
; a list of nonumeric is not a valid color
(assert-error `(gimp-context-set-background '("foo" 0))
                "in script, expected type: color list of numeric components ")


(script-fu-use-v2)
