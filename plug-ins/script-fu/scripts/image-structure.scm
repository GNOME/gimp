;;; image-structure.scm -*-scheme-*-
;;; Time-stamp: <1998/03/28 02:46:26 narazaki@InetQ.or.jp>
;;; Author: Shuji Narazaki <narazaki@InetQ.or.jp>
;;; Version 0.7
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************
;;; Code:

(if (not (symbol-bound? 'script-fu-show-image-structure-new-image?
			(the-environment)))
    (define script-fu-show-image-structure-new-image? TRUE))
(if (not (symbol-bound? 'script-fu-show-image-structure-space
			(the-environment)))
    (define script-fu-show-image-structure-space 50))
(if (not (symbol-bound? 'script-fu-show-image-structure-shear-length
			(the-environment)))
    (define script-fu-show-image-structure-shear-length 50))
(if (not (symbol-bound? 'script-fu-show-image-structure-border
			(the-environment)))
    (define script-fu-show-image-structure-border 10))
(if (not (symbol-bound? 'script-fu-show-image-structure-apply-layer-mask?
			(the-environment)))
    (define script-fu-show-image-structure-apply-layer-mask? TRUE))
(if (not (symbol-bound? 'script-fu-show-image-structure-with-layer-name?
			(the-environment)))
    (define script-fu-show-image-structure-with-layer-name? TRUE))
(if (not (symbol-bound? 'script-fu-show-image-structure-with-pad?
			(the-environment)))
    (define script-fu-show-image-structure-with-pad? TRUE))
(if (not (symbol-bound? 'script-fu-show-image-structure-padding-color
			(the-environment)))
    (define script-fu-show-image-structure-padding-color '(255 255 255)))
(if (not (symbol-bound? 'script-fu-show-image-structure-padding-opacity
			(the-environment)))
    (define script-fu-show-image-structure-padding-opacity 25))
(if (not (symbol-bound? 'script-fu-show-image-structure-with-background?
			(the-environment)))
    (define script-fu-show-image-structure-with-background? TRUE))
(if (not (symbol-bound? 'script-fu-show-image-structure-background-color
			(the-environment)))
    (define script-fu-show-image-structure-background-color '(0 0 0)))

(define (script-fu-show-image-structure img drawable new-image? space
					shear-length border apply-layer-mask?
					with-layer-name? with-pad? padding-color
					padding-opacity with-background?
					background-color)
  (if (eq? new-image? TRUE)
      (begin (set! img (car (gimp-image-duplicate img)))
	     (gimp-display-new img)))
  (let* ((layers (gimp-image-get-layers img))
	 (num-of-layers (car layers))
	 (old-width (car (gimp-image-width img)))
	 (old-height (car (gimp-image-height img)))
	 (new-width (+ (* 2 border) (+ old-width (* 2 shear-length))))
	 (new-height (+ (* 2 border) (+ old-height (* space (- num-of-layers 1)))))
	 (new-bg #f)
	 (layer-names '())
	 (layer #f)
	 (index 0))

    (gimp-context-push)

    (gimp-image-resize img new-width new-height 0 0)
    (set! layers (cadr layers))
    (gimp-selection-none img)
    (while (< index num-of-layers)
      (set! layer (aref layers index))
      (if (equal? "Background" (car (gimp-drawable-get-name layer)))
	  (begin
	    (gimp-layer-add-alpha layer)
	    (gimp-drawable-set-name layer "Original Background")))
      (set! layer-names (cons (car (gimp-drawable-get-name layer)) layer-names))
      (if (not (= -1 (car (gimp-layer-get-mask layer))))
	  (gimp-layer-remove-mask layer
				  (if (= TRUE apply-layer-mask?)
				      MASK-APPLY
				      MASK-DISCARD)))
      (if (= TRUE with-pad?)
	  (begin
	    (gimp-selection-layer-alpha layer)
	    (gimp-selection-invert img)
	    (gimp-layer-set-preserve-trans layer FALSE)
	    (gimp-context-set-foreground padding-color)
	    (gimp-edit-bucket-fill layer FG-BUCKET-FILL NORMAL-MODE
                                   padding-opacity 0 0 0 0)
	    (gimp-selection-none img)))

      (gimp-layer-translate layer
			    (+ border shear-length) (+ border (* space index)))
      (gimp-drawable-transform-shear-default layer ORIENTATION-HORIZONTAL
					     (* (/ (car (gimp-drawable-height layer))
						   old-height)
						(* -2 shear-length))
					     TRUE FALSE)
      (set! index (+ index 1)))
    (set! new-bg (- num-of-layers 1))
    (if (= TRUE with-background?)
	(begin
	  (set! new-bg (car (gimp-layer-new img new-width new-height RGBA-IMAGE
					    "New Background" 100 NORMAL-MODE)))
	  (gimp-image-add-layer img new-bg num-of-layers)
	  (gimp-context-set-background background-color)
	  (gimp-edit-fill new-bg BACKGROUND-FILL)))
    (gimp-image-set-active-layer img (aref layers 0))
    (if (= TRUE with-layer-name?)
	(let ((text-layer #f))
	  (gimp-context-set-foreground '(255 255 255))
	  (set! index 0)
	  (set! layer-names (nreverse layer-names))
	  (while (< index num-of-layers)
	    (set! text-layer (car (gimp-text-fontname img -1 (/ border 2)
					     (+ (* space index) old-height)
					     (car layer-names)
					     0 TRUE 14 PIXELS "Sans")))
	    (gimp-layer-set-mode text-layer NORMAL-MODE)
	    (set! index (+ index 1))
	    (set! layer-names (cdr layer-names)))))

    (gimp-image-set-active-layer img new-bg)

    (set! script-fu-show-image-structure-new-image? new-image?)
    (set! script-fu-show-image-structure-space space)
    (set! script-fu-show-image-structure-shear-length shear-length)
    (set! script-fu-show-image-structure-border border)
    (set! script-fu-show-image-structure-apply-layer-mask? apply-layer-mask?)
    (set! script-fu-show-image-structure-with-layer-name? with-layer-name?)
    (set! script-fu-show-image-structure-with-pad? with-pad?)
    (set! script-fu-show-image-structure-padding-color padding-color)
    (set! script-fu-show-image-structure-padding-opacity padding-opacity)
    (set! script-fu-show-image-structure-with-background? with-background?)
    (set! script-fu-show-image-structure-background-color background-color)

    (gimp-displays-flush)

    (gimp-context-pop)))

(script-fu-register "script-fu-show-image-structure"
		    _"Show Image _Structure..."
		    "Show the layer structure of the image"
		    "Shuji Narazaki <narazaki@InetQ.or.jp>"
		    "Shuji Narazaki"
		    "1997"
		    "RGB*, GRAY*"
		    SF-IMAGE       "image" 0
		    SF-DRAWABLE    "Drawable (unused)" 0
		    SF-TOGGLE     _"Create new image" script-fu-show-image-structure-new-image?
		    SF-ADJUSTMENT _"Space between layers" (cons script-fu-show-image-structure-space '(0 1000 1 10 0 1))
		    SF-ADJUSTMENT _"Shear length" (cons script-fu-show-image-structure-shear-length '(1 1000 1 10 0 1))
		    SF-ADJUSTMENT _"Outer border" (cons script-fu-show-image-structure-border '(0 250 1 10 0 1))
		    SF-TOGGLE     _"Apply layer mask (or discard)" script-fu-show-image-structure-apply-layer-mask?
		    SF-TOGGLE     _"Insert layer names" script-fu-show-image-structure-with-layer-name?
		    SF-TOGGLE     _"Padding for transparent regions" script-fu-show-image-structure-with-pad?
		    SF-COLOR      _"Pad color" script-fu-show-image-structure-padding-color
		    SF-ADJUSTMENT _"Pad opacity" (cons script-fu-show-image-structure-padding-opacity '(0 100 1 10 1 0))
		    SF-TOGGLE     _"Make new background" script-fu-show-image-structure-with-background?
		    SF-COLOR      _"Background color" script-fu-show-image-structure-background-color)

(script-fu-menu-register "script-fu-show-image-structure"
			 _"<Image>/Script-Fu/Utils")
