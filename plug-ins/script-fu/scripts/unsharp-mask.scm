;;; unsharp-mask.scm
;;; Time-stamp: <1998/11/17 13:18:39 narazaki@gimp.org>
;;; Author: Narazaki Shuji <narazaki@gimp.org>
;;; Version 0.8

; This script-fu-unsharp-mask is not in the menus.
; There is an equivalent GEGL filter at Filters>Enhance>Sharpen (Unsharp)
; and a GEGL wrapper in the PDB, plug-in-unsharp-mask.
; This might be kept for compatibility and used by third party scripts.

; Seems not used by any script in the repo.
; FUTURE move to gimp-data-extras or to scripts/test
; and maintain it with low priorty.

; unsharp-mask is a filter AND renderer, creating a new, visible, dirty image
; from the given image.


(define (script-fu-unsharp-mask img drws mask-size mask-opacity)
  (let* (
        (drw (vector-ref drws 0))
        (drawable-width (car (gimp-drawable-get-width drw)))
        (drawable-height (car (gimp-drawable-get-height drw)))
        (new-image (car (gimp-image-new drawable-width drawable-height RGB)))
        (original-layer (car (gimp-layer-new new-image
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

    (gimp-selection-all img)
    (gimp-edit-copy (vector drw))

    (gimp-image-undo-disable new-image)

    (gimp-image-insert-layer new-image original-layer 0 0)

    (let* (
           (pasted (car (gimp-edit-paste original-layer FALSE)))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )

    (set! original-layer-for-darker (car (gimp-layer-copy original-layer TRUE)))
    (set! original-layer-for-lighter (car (gimp-layer-copy original-layer TRUE)))
    (set! blurred-layer-for-darker (car (gimp-layer-copy original-layer TRUE)))
    (gimp-item-set-visible original-layer FALSE)
    (gimp-display-new new-image)

    ;; make darker mask
    (gimp-image-insert-layer new-image blurred-layer-for-darker 0 -1)
    (plug-in-gauss RUN-NONINTERACTIVE
		   new-image blurred-layer-for-darker (* 0.32 mask-size) (* 0.32 mask-size) 0)
    (set! blurred-layer-for-lighter
          (car (gimp-layer-copy blurred-layer-for-darker TRUE)))
    (gimp-image-insert-layer new-image original-layer-for-darker 0 -1)
    (gimp-layer-set-mode original-layer-for-darker LAYER-MODE-SUBTRACT)
    (set! darker-layer
          (car (gimp-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (gimp-item-set-name darker-layer "darker mask")
    (gimp-item-set-visible darker-layer FALSE)

    ;; make lighter mask
    (gimp-image-insert-layer new-image original-layer-for-lighter 0 -1)
    (gimp-image-insert-layer new-image blurred-layer-for-lighter 0 -1)
    (gimp-layer-set-mode blurred-layer-for-lighter LAYER-MODE-SUBTRACT)
    (set! lighter-layer
          (car (gimp-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (gimp-item-set-name lighter-layer "lighter mask")

    ;; combine them
    (gimp-item-set-visible original-layer TRUE)
    (gimp-layer-set-mode darker-layer LAYER-MODE-SUBTRACT)
    (gimp-layer-set-opacity darker-layer mask-opacity)
    (gimp-item-set-visible darker-layer TRUE)
    (gimp-layer-set-mode lighter-layer LAYER-MODE-ADDITION)
    (gimp-layer-set-opacity lighter-layer mask-opacity)
    (gimp-item-set-visible lighter-layer TRUE)

    (gimp-image-undo-enable new-image)
    (gimp-displays-flush)
  )
)

(script-fu-register-filter "script-fu-unsharp-mask"
  "Unsharp Mask..."
  "Make a new image from the current layer by applying the unsharp mask method"
  "Shuji Narazaki <narazaki@gimp.org>"
  "Shuji Narazaki"
  "1997,1998"
  "*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Mask size"        '(5 1 100 1 1 0 1)
  SF-ADJUSTMENT _"Mask opacity"     '(50 0 100 1 1 0 1)
)
