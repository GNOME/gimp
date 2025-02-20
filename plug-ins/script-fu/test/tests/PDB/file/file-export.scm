; Test gimp-<image format>-export PDB procedures

; Tests image export methods of the PDB

(script-fu-use-v3)





; setup

(define testImage (testing:load-test-image-basic-v3))
(assert `(gimp-image-id-is-valid ,testImage))



; random value generators

(define dct-method-strings '("integer" "fixed" "float"))
(define subsampling-method-strings
  '("sub-sampling-1x1" "sub-sampling-2x1" "sub-sampling-1x2" "sub-sampling-2x2"))
(define png-format-strings
  '("auto" "rgb8" "gray8" "rgba8" "graya8" "rgb16" "gray16" "rgba16" "graya16"))
(define bmp-format-strings
  '( "rgb-565" "rgba-5551" "rgb-555" "rgb-888" "rgba-8888" "rgbx-8888"))
(define webp-encoder-strings
  '("default" "picture" "photo" "drawing" "icon" "text"))
; omitting two compresison strings not compatible with all imgage types
; "ccittfax3" "ccittfax4" only for B/W images?
(define tiff-compression-strings
  '("none" "lzw" "packbits" "adobe_deflate" "jpeg" ))

(define raw-planarity-strings
  '("contiguous" "planar" ))
(define raw-palette-type-strings
  '("rgb" "bgr" ))

(define (random-dct-method)
  ; (random 3) is [1 2 3], list index is [0 1 2]
  (list-ref dct-method-strings (- (random 3) 1)))

(define (random-subsampling-method)
  ; (random 3) is [1 2 3 4], list index is [0 1 2 3]
  (list-ref subsampling-method-strings (- (random 4) 1)))

(define (random-png-format)
  (list-ref png-format-strings (- (random 9) 1)))

(define (random-bmp-format)
  (list-ref bmp-format-strings (- (random 6) 1)))

(define (random-webp-encoder)
  (list-ref webp-encoder-strings (- (random 6) 1)))

(define (random-tiff-compression)
  (list-ref tiff-compression-strings (- (random 5) 1)))

(define (random-raw-planarity)
  (list-ref raw-planarity-strings (- (random 2) 1)))

(define (random-raw-palette-type)
  (list-ref raw-planarity-strings (- (random 2) 1)))

; returns 0 or 1
(define (random-bool)
  ; (random 2) returns 1 or 2
  ; (random 1) does not ever return 0?
  (- (random 2) 1))




; exporter functions, randomized

; Call file-jpg-export with randomized args
(define (randomize-jpg-export sourceImage tempTestFile)
  (file-jpeg-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random 1.0)       ; quality
    (random 1.0)       ; smoothing
    (random-bool)      ; optimize
    (random-bool)      ; progressive
    (random-bool)      ; cmyk softproofing
    (random-subsampling-method) ;
    (random-bool)      ; baseline
    (random 64)        ; restart markers
    (random-dct-method)  ; e.g. "integer"
    (random-bool)      ; include-exif
    ))

(define (randomize-png-export sourceImage tempTestFile)
  (file-png-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random-bool)      ; interlaced
    (random 9)         ; compression
    (random-bool)      ; write metadata bKGD
    (random-bool)      ; write metadata oFFs
    (random-bool)      ; write metadata pHYs
    (random-bool)      ; write metadata tIMe
    (random-bool)      ; save transparent
    (random-bool)      ; optimize palette
    (random-png-format)
    (random-bool)      ; include exif
    ; rest default
    ))

(define (randomize-bmp-export sourceImage tempTestFile)
  (file-bmp-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random-bool)      ; rle compression
    (random-bool)      ; write colorspace
    (random-bmp-format)
    ))

(define (randomize-webp-export sourceImage tempTestFile)
  (file-webp-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random-webp-encoder)
    (random-bool)      ; lossless
    (random 100)       ; quality
    (random 100)       ; alpha quality
    (random-bool)      ; use-sharp-yuv
    (random-bool)      ; loop animation
    (random-bool)      ; minimize size
    ; rest are defaulted
    ))

; Not totally randomizeable: certain combinations not permitted.
(define (randomize-tiff-export sourceImage tempTestFile)
  (file-tiff-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random-bool)      ; BigTiff
    (random-tiff-compression)
    (random-bool)      ; keep transparent, pixels not alpha
    ; Not randomizing, not compatible with all compression choices.
    #f                 ; cmyk
    (random-bool)      ; include exif
    (random-bool)      ; include iptc
    (random-bool)      ; include xmp
    (random-bool)      ; include color profile
    (random-bool)      ; include thumbnail
    (random-bool)      ; include comment
    ))


; gih is essentially sequence of gbr.
; we don't yet test gbr separately
(define (randomize-gih-export sourceImage tempTestFile)
  (file-gih-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random 10)        ; spacing.  Actual limit is 1000, but save time.
    "dummy description"
    (random 64)        ; cell width.  Actual limit is huge.
    ; rest default
  ))

; TODO randomize
(define (randomize-gif-export sourceImage tempTestFile)
  (file-gif-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    ; rest default
  ))

; TODO randomize
(define (randomize-pdf-export sourceImage tempTestFile)
  (file-pdf-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    ; rest default
  ))

(define (randomize-raw-export sourceImage tempTestFile)
  (file-pdf-export
    RUN-NONINTERACTIVE
    sourceImage
    tempTestFile       ; file destination
    -1                 ; NULL for export options
    (random-raw-planarity)
    (random-raw-palette-type)
  ))




; Repeatedly call a randomizing exporter function
(define (iterate-exporter exporter testImage tempTestFile count)
  (while (>= count 0)
    (test! (string-append tempTestFile ": " (number->string count)))
    ; ? wrapping in assert does not work
    (exporter testImage tempTestFile)

    ; eat our own dog food
    ; exported file is importable.
    (gimp-image-delete (gimp-file-load RUN-NONINTERACTIVE tempTestFile))

    (set! count (- count 1))))



(test! "file export jpg")

; Testing is not comparing output file to a known good result.
; Test that exported file can be imported.
; Only testing that no errors are thrown.


; Iterate, calling first-class function that is an image exporter.
; This overwrites the created file.
; Create a temp file name with proper suffix.
; Not sure that is necessary, but the file loader might examine it.

; JPG
(define tempTestFile (gimp-temp-file "jpg"))
(iterate-exporter randomize-jpg-export testImage tempTestFile 10)

; PNG
(define tempTestFile (gimp-temp-file "png"))
(iterate-exporter randomize-png-export testImage tempTestFile 10)

; BMP
(define tempTestFile (gimp-temp-file "bmp"))
(iterate-exporter randomize-bmp-export testImage tempTestFile 10)

; WEBP
(define tempTestFile (gimp-temp-file "webp"))
(iterate-exporter randomize-webp-export testImage tempTestFile 10)

; TIFF
; Passes, but warns to GIMP Error Console
(define tempTestFile (gimp-temp-file "tiff"))
(iterate-exporter randomize-tiff-export testImage tempTestFile 10)

; GIH brush pipe
(define tempTestFile (gimp-temp-file "gih"))
(iterate-exporter randomize-gih-export testImage tempTestFile 10)

; TODO until randomized, only call once
; GIF
(define tempTestFile (gimp-temp-file "gif"))
(iterate-exporter randomize-gif-export testImage tempTestFile 1)

; PDF
(define tempTestFile (gimp-temp-file "pdf"))
(iterate-exporter randomize-pdf-export testImage tempTestFile 1)

; RAW
; requires a separate loader app installed: RawTherapee or darktable
;(define tempTestFile (gimp-temp-file "raw"))
;(iterate-exporter randomize-raw-export testImage tempTestFile 10)

; XCF has no options yet, and is tested elsewhere

; TODO: GBR etc.

; Only the last created file still exists (of each image format.)
; We were overwriting it.


; cleanup
; Delete image but not the created file.
(gimp-image-delete testImage)


(script-fu-use-v2)