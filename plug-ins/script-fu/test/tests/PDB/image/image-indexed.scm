; test Image of mode indexed methods of PDB


; depends on fresh GIMP state
; !!!! tests hardcode image ID 4

; Using numeric equality operator '=' on numeric ID's


;              Basic indexed tests


; method new from fresh GIMP state returns ID 4
(assert '(=
           (car (gimp-image-new 21 22 RGB))
           4))

; method gimp-image-convert-indexed yields truthy (now yields (#t) )
(assert '(gimp-image-convert-indexed
                  4
                  CONVERT-DITHER-NONE
                  CONVERT-PALETTE-GENERATE
                  25  ; color count
                  1  ; alpha-dither.  FUTURE: #t
                  1  ; remove-unused. FUTURE: #t
                  "myPalette" ; ignored
                  ))

; conversion was effective:
; basetype of indexed is INDEXED
(assert '(=
            (car (gimp-image-get-base-type 4))
            INDEXED))

; conversion was effective:
; indexed image has-a colormap
; colormap is-a vector of length zero, when image has no drawable.
; FIXME doc says num-bytes is returned, obsolete since GBytes
(assert '(=
            (vector-length
              (car (gimp-image-get-colormap 4)))
            0))

; indexed images have precision PRECISION-U8-NON-LINEAR
; FIXME annotation of PDB procedure says GIMP_PRECISION_U8
(assert '(=
           (car (gimp-image-get-precision 4))
           PRECISION-U8-NON-LINEAR ))

; TODO
; drawable of indexed image is also indexed
;(assert '(car (gimp-drawable-is-indexed
;                 ()
;               4)

; convert precision of indexed images yields error
(assert-error '(car (gimp-image-convert-precision
                  4
                  PRECISION-DOUBLE-GAMMA))
"Procedure execution of gimp-image-convert-precision failed on invalid input arguments: Image '[Untitled]' (4) must not be of type 'indexed'")



