;;; unsharp-mask.scm
;;; Time-stamp: <1998/11/17 13:18:39 narazaki@gimp.org>
;;; Author: Narazaki Shuji <narazaki@gimp.org>
;;; Version 0.8

(define (script-fu-unsharp-mask img drw mask-size mask-opacity)
  (let* ((drawable-width (car (gimp-drawable-width drw)))
	 (drawable-height (car (gimp-drawable-height drw)))
	 (new-image (car (gimp-image-new drawable-width drawable-height RGB)))
	 (original-layer (car (gimp-layer-new new-image
					      drawable-width drawable-height
					      RGB "Original" 100 NORMAL)))
	 (original-layer-for-darker #f)
	 (original-layer-for-lighter #f)
	 (blured-layer-for-darker #f)
	 (blured-layer-for-lighter #f)
	 (darker-layer #f)
	 (lighter-layer #f))
    (gimp-selection-all img)
    (gimp-edit-copy drw)
    (gimp-image-undo-disable new-image)
    (gimp-floating-sel-anchor
     (car (gimp-edit-paste original-layer FALSE)))
    (gimp-image-add-layer new-image original-layer 0)
    (set! original-layer-for-darker (car (gimp-layer-copy original-layer TRUE)))
    (set! original-layer-for-lighter (car (gimp-layer-copy original-layer TRUE)))
    (set! blured-layer-for-darker (car (gimp-layer-copy original-layer TRUE)))
    (gimp-layer-set-visible original-layer FALSE)
    (gimp-display-new new-image)
    ;; make darker mask
    (gimp-image-add-layer new-image blured-layer-for-darker -1)
    (plug-in-gauss-iir TRUE new-image blured-layer-for-darker mask-size
		       TRUE TRUE)
    (set! blured-layer-for-lighter
	  (car (gimp-layer-copy blured-layer-for-darker TRUE)))
    (gimp-image-add-layer new-image original-layer-for-darker -1)
    (gimp-layer-set-mode original-layer-for-darker SUBTRACT)
    (set! darker-layer
	  (car (gimp-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (gimp-layer-set-name darker-layer "darker mask")
    (gimp-layer-set-visible darker-layer FALSE)
    ;; make lighter mask
    (gimp-image-add-layer new-image original-layer-for-lighter -1)
    (gimp-image-add-layer new-image blured-layer-for-lighter -1)
    (gimp-layer-set-mode blured-layer-for-lighter SUBTRACT)
    (set! lighter-layer
	  (car (gimp-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (gimp-layer-set-name lighter-layer "lighter mask")
    ;; combine them
    (gimp-layer-set-visible original-layer TRUE)
    (gimp-layer-set-mode darker-layer SUBTRACT)
    (gimp-layer-set-opacity darker-layer mask-opacity)
    (gimp-layer-set-visible darker-layer TRUE)
    (gimp-layer-set-mode lighter-layer ADDITION)
    (gimp-layer-set-opacity lighter-layer mask-opacity)
    (gimp-layer-set-visible lighter-layer TRUE)
    (gimp-image-undo-enable new-image)
    (gimp-displays-flush)))

(script-fu-register
 "script-fu-unsharp-mask"
 "<Image>/Script-Fu/Alchemy/Unsharp Mask..."
 "Make a sharp image of IMAGE's DRAWABLE by applying unsharp mask method"
 "Shuji Narazaki <narazaki@gimp.org>"
 "Shuji Narazaki"
 "1997,1998"
 ""
 SF-IMAGE "Image" 0
 SF-DRAWABLE "Drawable to apply" 0
 SF-ADJUSTMENT "Mask size" '(5 1 100 1 1 0 1)
 SF-ADJUSTMENT "Mask opacity" '(50 0 100 1 1 0 1)
)

;;; unsharp-mask.scm ends here
