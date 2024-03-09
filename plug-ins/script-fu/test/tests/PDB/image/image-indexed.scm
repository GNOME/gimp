; test Image of mode indexed methods of PDB

; Now independent of image ID


;              Basic indexed tests


; an empty image for testing
(define newTestImage (car (gimp-image-new 21 22 RGB)))

; Load test image that already has drawable
(define testImage (testing:load-test-image "gimp-logo.png"))





; testImage is mode RGB
(assert `(=
            (car (gimp-image-get-base-type ,testImage))
            RGB))


; method gimp-image-convert-indexed yields truthy (now yields (#t) )
(assert `(gimp-image-convert-indexed
                  ,testImage
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  2  ; color count
                  1  ; alpha-dither.  FUTURE: #t
                  1  ; remove-unused. FUTURE: #t
                  "myPalette" ; ignored
                  ))

; method gimp-image-convert-indexed works even on empty image
(assert `(gimp-image-convert-indexed
                  ,newTestImage
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  25  ; color count
                  1  ; alpha-dither.  FUTURE: #t
                  1  ; remove-unused. FUTURE: #t
                  "myPalette" ; ignored
                  ))

; conversion was effective:
; basetype of indexed image is INDEXED
(assert `(=
            (car (gimp-image-get-base-type ,testImage))
            INDEXED))

; conversion was effective:
; basetype of indexed image is INDEXED
(assert `(=
            (car (gimp-image-get-base-type ,newTestImage))
            INDEXED))


; testImage has a layer named same as file "gimp-logo.png"
; TODO Why does "Background" work but app shows "gimp-logo.png"

; drawable of indexed image is also indexed
(assert `(= (car (gimp-drawable-is-indexed
                    ; unwrap the drawable ID
                    (car (gimp-image-get-layer-by-name ,testImage "Background"))))
            1)) ; FUTURE #t



;                     colormaps of indexed images

; conversion was effective:
; indexed image has-a colormap

; colormap is-a vector of length zero, when image has no drawable.
; get-colormap returns (#( <bytes of color>))
; FIXME doc says num-bytes is returned, obsolete since GBytes
(assert `(=
            (vector-length
              (car (gimp-image-get-colormap ,newTestImage)))
            0))

; colormap is-a vector of length 3*<color count given during conversion>,
; when image has a drawable.
; 3*2=6
; FIXME doc says num-bytes is returned, obsolete since GBytes
(assert `(=
            (vector-length
              (car (gimp-image-get-colormap ,testImage)))
            (* 3 2)))

; set-colormap succeeds
; This tests marshalling of GBytes to PDB
(assert `(gimp-image-set-colormap ,testImage #(1 1 1 9 9 9)))

; TODO set-colormap effective
; colormap vector is same as given
(assert `(equal?
              (car (gimp-image-get-colormap ,testImage))
              #(1 1 1 9 9 9)))





;                  precision of indexed images

; indexed images have precision PRECISION-U8-NON-LINEAR
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert `(=
           (car (gimp-image-get-precision ,testImage))
           PRECISION-U8-NON-LINEAR ))




; !!! This depends on ID 4 for image

; convert precision of indexed images yields error
(assert-error `(car (gimp-image-convert-precision
                  ,newTestImage
                  PRECISION-DOUBLE-GAMMA))
               "Procedure execution of gimp-image-convert-precision failed on invalid input arguments: ")
               ; "Image '[Untitled]' (4) must not be of type 'indexed'"



