; test Image of mode indexed methods of PDB

; In v2 "palette" was named "colormap"
; and get-colormap returned a vector.
; num-bytes was returned, obsolete since GBytes.
;
; In v3 Palette is first class object.

; See also tests in palette.scm



(script-fu-use-v3)


; setup

; an empty image for testing
(define newTestImage (gimp-image-new 21 22 RGB))

; Load test image that already has drawable
; "gimp-logo.png"
(define testImage (testing:load-test-image-basic-v3 ))





; testImage is mode RGB
(assert `(= (gimp-image-get-base-type ,testImage)
            RGB))


(test! "convert-indexed")

; method gimp-image-convert-indexed yields truthy (now yields (#t) )
(assert `(gimp-image-convert-indexed
                  ,testImage
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  2  ; color count
                  #t  ; alpha-dither.
                  #t  ; remove-unused.
                  "myPalette" ; ignored
                  ))

; method gimp-image-convert-indexed works even on empty image
(assert `(gimp-image-convert-indexed
                  ,newTestImage
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  25  ; color count
                  #t  ; alpha-dither.
                  #t  ; remove-unused.
                  "myPalette" ; ignored
                  ))

; conversion was effective:
; basetype of indexed image is INDEXED
(assert `(= (gimp-image-get-base-type ,testImage)
            INDEXED))

; conversion was effective:
; basetype of indexed image is INDEXED
(assert `(= (gimp-image-get-base-type ,newTestImage)
            INDEXED))


; testImage has a layer named same as file "gimp-logo.png"
; TODO Why does "Background" work but app shows "gimp-logo.png"

(test! "drawable of indexed image is also indexed")
(assert `(gimp-drawable-is-indexed
            (gimp-image-get-layer-by-name ,testImage "Background")))



(test! "palettes of indexed images")

; conversion was effective:
; indexed image has-a palette

; v3 palette is first class object, an ID in SF
; Here only test it does not fail.
; Use testImage since newTestImage has no palette until has drawable?
(assert `(gimp-image-get-palette ,testImage))

; Now get the palette
(define testPalette (gimp-image-get-palette testImage))

; And test its count of colors, same as given during conversion
;  when image has a drawable.
(assert `(= (gimp-palette-get-color-count ,testPalette)
            2))
(display (gimp-palette-get-color-count testPalette))


(define bearsPalette (gimp-palette-get-by-name "Bears"))
; set-palette succeeds
(assert `(gimp-image-set-palette ,testImage ,bearsPalette))

; effective: palette vector is same as given
; ID is same ID





(test! "precision of indexed images")

; indexed images have precision PRECISION-U8-NON-LINEAR
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert `(=
           (gimp-image-get-precision ,testImage)
           PRECISION-U8-NON-LINEAR ))


; convert precision of indexed images yields error
(assert-error `(gimp-image-convert-precision
                  ,newTestImage
                  PRECISION-U8-PERCEPTUAL)
               "Procedure execution of gimp-image-convert-precision failed on invalid input arguments: ")
               ; "Image '[Untitled]' (4) must not be of type 'indexed'"


(script-fu-use-v2)

