;;; unsharp-mask.scm
;;; Time-stamp: <1998/11/17 13:18:39 narazaki@ligma.org>
;;; Author: Narazaki Shuji <narazaki@ligma.org>
;;; Version 0.8

(define (script-fu-unsharp-mask img drw mask-size mask-opacity)
  (let* (
        (drawable-width (car (ligma-drawable-get-width drw)))
        (drawable-height (car (ligma-drawable-get-height drw)))
        (new-image (car (ligma-image-new drawable-width drawable-height RGB)))
        (original-layer (car (ligma-layer-new new-image
                                             drawable-width drawable-height
                                             RGB-IMAGE "Original"
                                             100 LAYER-MODE-NORMAL)))
        (original-layer-for-darker 0)
        (original-layer-for-lighter 0)
        (blurred-layer-for-darker 0)
        (blurred-layer-for-lighter 0)
        (darker-layer 0)
        (lighter-layer 0)
        )

    (ligma-selection-all img)
    (ligma-edit-copy 1 (vector drw))

    (ligma-image-undo-disable new-image)

    (ligma-image-insert-layer new-image original-layer 0 0)

    (let* (
           (pasted (ligma-edit-paste original-layer FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (ligma-floating-sel-anchor floating-sel)
    )

    (set! original-layer-for-darker (car (ligma-layer-copy original-layer TRUE)))
    (set! original-layer-for-lighter (car (ligma-layer-copy original-layer TRUE)))
    (set! blurred-layer-for-darker (car (ligma-layer-copy original-layer TRUE)))
    (ligma-item-set-visible original-layer FALSE)
    (ligma-display-new new-image)

    ;; make darker mask
    (ligma-image-insert-layer new-image blurred-layer-for-darker 0 -1)
    (plug-in-gauss-iir RUN-NONINTERACTIVE
		       new-image blurred-layer-for-darker mask-size TRUE TRUE)
    (set! blurred-layer-for-lighter
          (car (ligma-layer-copy blurred-layer-for-darker TRUE)))
    (ligma-image-insert-layer new-image original-layer-for-darker 0 -1)
    (ligma-layer-set-mode original-layer-for-darker LAYER-MODE-SUBTRACT)
    (set! darker-layer
          (car (ligma-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (ligma-item-set-name darker-layer "darker mask")
    (ligma-item-set-visible darker-layer FALSE)

    ;; make lighter mask
    (ligma-image-insert-layer new-image original-layer-for-lighter 0 -1)
    (ligma-image-insert-layer new-image blurred-layer-for-lighter 0 -1)
    (ligma-layer-set-mode blurred-layer-for-lighter LAYER-MODE-SUBTRACT)
    (set! lighter-layer
          (car (ligma-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (ligma-item-set-name lighter-layer "lighter mask")

    ;; combine them
    (ligma-item-set-visible original-layer TRUE)
    (ligma-layer-set-mode darker-layer LAYER-MODE-SUBTRACT)
    (ligma-layer-set-opacity darker-layer mask-opacity)
    (ligma-item-set-visible darker-layer TRUE)
    (ligma-layer-set-mode lighter-layer LAYER-MODE-ADDITION)
    (ligma-layer-set-opacity lighter-layer mask-opacity)
    (ligma-item-set-visible lighter-layer TRUE)

    (ligma-image-undo-enable new-image)
    (ligma-displays-flush)
  )
)

(script-fu-register "script-fu-unsharp-mask"
  "Unsharp Mask..."
  "Make a new image from the current layer by applying the unsharp mask method"
  "Shuji Narazaki <narazaki@ligma.org>"
  "Shuji Narazaki"
  "1997,1998"
  ""
  SF-IMAGE      "Image"             0
  SF-DRAWABLE   "Drawable to apply" 0
  SF-ADJUSTMENT _"Mask size"        '(5 1 100 1 1 0 1)
  SF-ADJUSTMENT _"Mask opacity"     '(50 0 100 1 1 0 1)
)
