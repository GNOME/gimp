;  CARVE-IT
;   Carving, embossing, & stamping
;   Process taken from "The Photoshop 3 WOW! Book"
;   http://www.peachpit.com
;   This script requires a grayscale image containing a single layer.
;   This layer is used as the mask for the carving effect
;   NOTE: This script requires the image to be carved to either be an
;   RGB color or grayscale image with a single layer. An indexed file
;   can not be used due to the use of gimp-drawable-histogram and
;   gimp-drawable-levels.


(define (carve-scale val scale)
  (* (sqrt val) scale))

(define (calculate-inset-gamma img layer)
  (let* ((stats (gimp-drawable-histogram layer 0 0.0 1.0))
         (mean (car stats)))
    (cond ((< mean 127) (+ 1.0 (* 0.5 (/ (- 127 mean) 127.0))))
          ((>= mean 127) (- 1.0 (* 0.5 (/ (- mean 127) 127.0)))))))


(define (copy-layer-carve-it dest-image dest-drawable source-image source-drawable)
  (gimp-selection-all dest-image)
  (gimp-drawable-edit-clear dest-drawable)
  (gimp-selection-none dest-image)
  (gimp-selection-all source-image)
  (gimp-edit-copy source-drawable)
      (let ((floating-sel (car (gimp-edit-paste dest-drawable FALSE))))
        (gimp-floating-sel-anchor floating-sel)))



(define (script-fu-carve-it mask-img mask-drawable bg-layer carve-white)
  (let* (
        (width (car (gimp-drawable-width mask-drawable)))
        (height (car (gimp-drawable-height mask-drawable)))
        (type (car (gimp-drawable-type bg-layer)))
        (img (car (gimp-image-new width height (cond ((= type RGB-IMAGE) RGB)
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
        (brush-name (car (gimp-brush-new "Carve It")))
        (mask-fs 0)
        (mask (car (gimp-channel-new img width height "Engraving Mask" 50 '(0 0 0))))
        (inset-gamma (calculate-inset-gamma (car (gimp-item-get-image bg-layer)) bg-layer))
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
        (bg-width (car (gimp-drawable-width bg-layer)))
        (bg-height (car (gimp-drawable-height bg-layer)))
        (bg-type (car (gimp-drawable-type bg-layer)))
        (bg-image (car (gimp-item-get-image bg-layer)))
        (layer1 (car (gimp-layer-new img bg-width bg-height bg-type "Layer1" 100 LAYER-MODE-NORMAL)))
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-disable img)

    (gimp-image-insert-layer img layer1 0 0)

    (gimp-selection-all img)
    (gimp-drawable-edit-clear layer1)
    (gimp-selection-none img)
    (copy-layer-carve-it img layer1 bg-image bg-layer)

    (gimp-edit-copy mask-drawable)
    (gimp-image-insert-channel img mask -1 0)

    (plug-in-tile RUN-NONINTERACTIVE img layer1 width height FALSE)
    (set! mask-fs (car (gimp-edit-paste mask FALSE)))
    (gimp-floating-sel-anchor mask-fs)
    (if (= carve-white FALSE)
        (gimp-drawable-invert mask FALSE))

    (set! mask-fat (car (gimp-channel-copy mask)))
    (gimp-image-insert-channel img mask-fat -1 0)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask-fat)

    (gimp-brush-set-shape brush-name BRUSH-GENERATED-CIRCLE)
    (gimp-brush-set-spikes brush-name 2)
    (gimp-brush-set-hardness brush-name 1.0)
    (gimp-brush-set-spacing brush-name 25)
    (gimp-brush-set-aspect-ratio brush-name 1)
    (gimp-brush-set-angle brush-name 0)
    (cond (<= brush-size 17) (gimp-brush-set-radius brush-name (\ brush-size 2))
	  (else gimp-brush-set-radius brush-name (\ 19 2)))
    (gimp-context-set-brush brush-name)

    (gimp-context-set-foreground '(255 255 255))
    (gimp-drawable-edit-stroke-selection mask-fat)
    (gimp-selection-none img)

    (set! mask-emboss (car (gimp-channel-copy mask-fat)))
    (gimp-image-insert-channel img mask-emboss -1 0)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img mask-emboss feather TRUE TRUE)
    (plug-in-emboss RUN-NONINTERACTIVE img mask-emboss 315.0 45.0 7 TRUE)

    (gimp-context-set-background '(180 180 180))
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask-fat)
    (gimp-selection-invert img)
    (gimp-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (gimp-selection-none img)

    (set! mask-highlight (car (gimp-channel-copy mask-emboss)))
    (gimp-image-insert-channel img mask-highlight -1 0)
    (gimp-drawable-levels mask-highlight 0
			  0.7056 1.0 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (set! mask-shadow mask-emboss)
    (gimp-drawable-levels mask-shadow 0
			  0.0 0.70586 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (gimp-edit-copy mask-shadow)
    (set! shadow-layer (car (gimp-edit-paste layer1 FALSE)))
    (gimp-floating-sel-to-layer shadow-layer)
    (gimp-layer-set-mode shadow-layer LAYER-MODE-MULTIPLY)

    (gimp-edit-copy mask-highlight)
    (set! highlight-layer (car (gimp-edit-paste shadow-layer FALSE)))
    (gimp-floating-sel-to-layer highlight-layer)
    (gimp-layer-set-mode highlight-layer LAYER-MODE-SCREEN)

    (gimp-edit-copy mask)
    (set! cast-shadow-layer (car (gimp-edit-paste highlight-layer FALSE)))
    (gimp-floating-sel-to-layer cast-shadow-layer)
    (gimp-layer-set-mode cast-shadow-layer LAYER-MODE-MULTIPLY)
    (gimp-layer-set-opacity cast-shadow-layer 75)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img cast-shadow-layer feather TRUE TRUE)
    (gimp-item-transform-translate cast-shadow-layer offx offy)

    (set! csl-mask (car (gimp-layer-create-mask cast-shadow-layer ADD-MASK-BLACK)))
    (gimp-layer-add-mask cast-shadow-layer csl-mask)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill csl-mask FILL-BACKGROUND)

    (set! inset-layer (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-insert-layer img inset-layer 0 1)

    (set! il-mask (car (gimp-layer-create-mask inset-layer ADD-MASK-BLACK)))
    (gimp-layer-add-mask inset-layer il-mask)
    (gimp-image-select-item img CHANNEL-OP-REPLACE mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill il-mask FILL-BACKGROUND)
    (gimp-selection-none img)
    (gimp-selection-none bg-image)
    (gimp-drawable-levels inset-layer 0 0.0 1.0 TRUE inset-gamma 0.0 1.0 TRUE)
    (gimp-image-remove-channel img mask)
    (gimp-image-remove-channel img mask-fat)
    (gimp-image-remove-channel img mask-highlight)
    (gimp-image-remove-channel img mask-shadow)

    (gimp-item-set-name layer1 _"Carved Surface")
    (gimp-item-set-name shadow-layer _"Bevel Shadow")
    (gimp-item-set-name highlight-layer _"Bevel Highlight")
    (gimp-item-set-name cast-shadow-layer _"Cast Shadow")
    (gimp-item-set-name inset-layer _"Inset")

    (gimp-brush-delete brush-name)

    (gimp-display-new img)
    (gimp-image-undo-enable img)

    (gimp-context-pop)
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
