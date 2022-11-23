;  CARVE-IT
;   Carving, embossing, & stamping
;   Process taken from "The Photoshop 3 WOW! Book"
;   http://www.peachpit.com
;   This script requires a grayscale image containing a single layer.
;   This layer is used as the mask for the carving effect
;   NOTE: This script requires the image to be carved to either be an
;   RGB color or grayscale image with a single layer. An indexed file
;   can not be used due to the use of ligma-drawable-histogram and
;   ligma-drawable-levels.


(define (carve-scale val scale)
  (* (sqrt val) scale))

(define (calculate-inset-gamma img layer)
  (let* ((stats (ligma-drawable-histogram layer 0 0.0 1.0))
         (mean (car stats)))
    (cond ((< mean 127) (+ 1.0 (* 0.5 (/ (- 127 mean) 127.0))))
          ((>= mean 127) (- 1.0 (* 0.5 (/ (- mean 127) 127.0)))))))


(define (copy-layer-carve-it dest-image dest-drawable source-image source-drawable)
  (ligma-selection-all dest-image)
  (ligma-drawable-edit-clear dest-drawable)
  (ligma-selection-none dest-image)
  (ligma-selection-all source-image)
  (ligma-edit-copy 1 (vector source-drawable))
  (let* (
         (pasted (ligma-edit-paste dest-drawable FALSE))
         (num-pasted (car pasted))
         (floating-sel (aref (cadr pasted) (- num-pasted 1)))
        )
        (ligma-floating-sel-anchor floating-sel)
  )
)



(define (script-fu-carve-it mask-img mask-drawable bg-layer carve-white)
  (let* (
        (width (car (ligma-drawable-get-width mask-drawable)))
        (height (car (ligma-drawable-get-height mask-drawable)))
        (type (car (ligma-drawable-type bg-layer)))
        (img (car (ligma-image-new width height (cond ((= type RGB-IMAGE) RGB)
                                                     ((= type RGBA-IMAGE) RGB)
                                                     ((= type GRAY-IMAGE) GRAY)
                                                     ((= type GRAYA-IMAGE) GRAY)
                                                     ((= type INDEXED-IMAGE) INDEXED)
                                                     ((= type INDEXEDA-IMAGE) INDEXED)))))
        (size (min width height))
        (offx (carve-scale size 0.33))
        (offy (carve-scale size 0.25))
        (feather (carve-scale size 0.3))
        (brush-size (carve-scale size 0.3))
        (brush-name (car (ligma-brush-new "Carve It")))
        (mask (car (ligma-channel-new img width height "Engraving Mask" 50 '(0 0 0))))
        (inset-gamma (calculate-inset-gamma (car (ligma-item-get-image bg-layer)) bg-layer))
        (mask-fat 0)
        (mask-emboss 0)
        (mask-highlight 0)
        (mask-shadow 0)
        (shadow-layer 0)
        (highlight-layer 0)
        (cast-shadow-layer 0)
        (csl-mask 0)
        (inset-layer 0)
        (il-mask 0)
        (bg-width (car (ligma-drawable-get-width bg-layer)))
        (bg-height (car (ligma-drawable-get-height bg-layer)))
        (bg-type (car (ligma-drawable-type bg-layer)))
        (bg-image (car (ligma-item-get-image bg-layer)))
        (layer1 (car (ligma-layer-new img bg-width bg-height bg-type "Layer1" 100 LAYER-MODE-NORMAL)))
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable img)

    (ligma-image-insert-layer img layer1 0 0)

    (ligma-selection-all img)
    (ligma-drawable-edit-clear layer1)
    (ligma-selection-none img)
    (copy-layer-carve-it img layer1 bg-image bg-layer)

    (ligma-edit-copy 1 (vector mask-drawable))
    (ligma-image-insert-channel img mask -1 0)

    (plug-in-tile RUN-NONINTERACTIVE img 1 (vector layer1) width height FALSE)
    (let* (
           (pasted (ligma-edit-paste mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (ligma-floating-sel-anchor floating-sel)
    )
    (if (= carve-white FALSE)
        (ligma-drawable-invert mask FALSE))

    (set! mask-fat (car (ligma-channel-copy mask)))
    (ligma-image-insert-channel img mask-fat -1 0)
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask-fat)

    (ligma-brush-set-shape brush-name BRUSH-GENERATED-CIRCLE)
    (ligma-brush-set-spikes brush-name 2)
    (ligma-brush-set-hardness brush-name 1.0)
    (ligma-brush-set-spacing brush-name 25)
    (ligma-brush-set-aspect-ratio brush-name 1)
    (ligma-brush-set-angle brush-name 0)
    (cond (<= brush-size 17) (ligma-brush-set-radius brush-name (\ brush-size 2))
	  (else ligma-brush-set-radius brush-name (\ 19 2)))
    (ligma-context-set-brush brush-name)

    (ligma-context-set-foreground '(255 255 255))
    (ligma-drawable-edit-stroke-selection mask-fat)
    (ligma-selection-none img)

    (set! mask-emboss (car (ligma-channel-copy mask-fat)))
    (ligma-image-insert-channel img mask-emboss -1 0)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img mask-emboss feather TRUE TRUE)
    (plug-in-emboss RUN-NONINTERACTIVE img mask-emboss 315.0 45.0 7 TRUE)

    (ligma-context-set-background '(180 180 180))
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask-fat)
    (ligma-selection-invert img)
    (ligma-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask)
    (ligma-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (ligma-selection-none img)

    (set! mask-highlight (car (ligma-channel-copy mask-emboss)))
    (ligma-image-insert-channel img mask-highlight -1 0)
    (ligma-drawable-levels mask-highlight 0
			  0.7056 1.0 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (set! mask-shadow mask-emboss)
    (ligma-drawable-levels mask-shadow 0
			  0.0 0.70586 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (ligma-edit-copy 1 (vector mask-shadow))
    (let* (
           (pasted (ligma-edit-paste layer1 FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (set! shadow-layer floating-sel)
          (ligma-floating-sel-to-layer shadow-layer)
    )
    (ligma-layer-set-mode shadow-layer LAYER-MODE-MULTIPLY)

    (ligma-edit-copy 1 (vector mask-highlight))
    (let* (
           (pasted (ligma-edit-paste shadow-layer FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (set! highlight-layer floating-sel)
          (ligma-floating-sel-to-layer highlight-layer)
    )
    (ligma-layer-set-mode highlight-layer LAYER-MODE-SCREEN)

    (ligma-edit-copy 1 (vector mask))
    (let* (
           (pasted (ligma-edit-paste highlight-layer FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (set! cast-shadow-layer floating-sel)
          (ligma-floating-sel-to-layer cast-shadow-layer)
    )
    (ligma-layer-set-mode cast-shadow-layer LAYER-MODE-MULTIPLY)
    (ligma-layer-set-opacity cast-shadow-layer 75)

    (plug-in-gauss-rle RUN-NONINTERACTIVE img cast-shadow-layer feather TRUE TRUE)
    (ligma-item-transform-translate cast-shadow-layer offx offy)

    (set! csl-mask (car (ligma-layer-create-mask cast-shadow-layer ADD-MASK-BLACK)))
    (ligma-layer-add-mask cast-shadow-layer csl-mask)
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask)
    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-edit-fill csl-mask FILL-BACKGROUND)

    (set! inset-layer (car (ligma-layer-copy layer1 TRUE)))
    (ligma-image-insert-layer img inset-layer 0 1)

    (set! il-mask (car (ligma-layer-create-mask inset-layer ADD-MASK-BLACK)))
    (ligma-layer-add-mask inset-layer il-mask)
    (ligma-image-select-item img CHANNEL-OP-REPLACE mask)
    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-edit-fill il-mask FILL-BACKGROUND)
    (ligma-selection-none img)
    (ligma-selection-none bg-image)
    (ligma-drawable-levels inset-layer 0 0.0 1.0 TRUE inset-gamma 0.0 1.0 TRUE)
    (ligma-image-remove-channel img mask)
    (ligma-image-remove-channel img mask-fat)
    (ligma-image-remove-channel img mask-highlight)
    (ligma-image-remove-channel img mask-shadow)

    (ligma-item-set-name layer1 _"Carved Surface")
    (ligma-item-set-name shadow-layer _"Bevel Shadow")
    (ligma-item-set-name highlight-layer _"Bevel Highlight")
    (ligma-item-set-name cast-shadow-layer _"Cast Shadow")
    (ligma-item-set-name inset-layer _"Inset")

    (ligma-brush-delete brush-name)

    (ligma-display-new img)
    (ligma-image-undo-enable img)

    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-carve-it"
    _"Stencil C_arve..."
    _"Use the specified drawable as a stencil to carve from the specified image."
    "Spencer Kimball"
    "Spencer Kimball"
    "1997"
    "GRAY"
    SF-IMAGE     "Mask image"        0
    SF-DRAWABLE  "Mask drawable"     0
    SF-DRAWABLE _"Image to carve"    0
    SF-TOGGLE   _"Carve white areas" TRUE
)

(script-fu-menu-register "script-fu-carve-it"
                         "<Image>/Filters/Decor")
