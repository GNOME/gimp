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
      (begin (set! img (car (gimp-channel-ops-duplicate img)))
	     (gimp-display-new img)))
  (let* ((layers (gimp-image-get-layers img))
	 (num-of-layers (car layers))
	 (old-width (car (gimp-image-width img)))
	 (old-height (car (gimp-image-height img)))
	 (new-width (+ (* 2 border) (+ old-width (* 2 shear-length))))
	 (new-height (+ (* 2 border) (+ old-height (* space (- num-of-layers 1)))))
	 (new-bg #f)
	 (old-foreground (car (gimp-palette-get-foreground)))
	 (old-background (car (gimp-palette-get-background)))
	 (layer-names '())
	 (layer #f)
	 (index 0))
    (gimp-image-resize img new-width new-height 0 0)
    (set! layers (cadr layers))
    (gimp-selection-none img)
    (while (< index num-of-layers)
      (set! layer (aref layers index))
      (if (equal? "Background" (car (gimp-layer-get-name layer)))
	  (begin
	    (gimp-layer-add-alpha layer)
	    (gimp-layer-set-name layer "Original Background")))
      (set! layer-names (cons (car (gimp-layer-get-name layer)) layer-names))
      (if (not (= -1 (car (gimp-layer-mask layer))))
	  (gimp-image-remove-layer-mask img layer
					(if (= TRUE apply-layer-mask?)
					    APPLY
					    DISCARD)))
      (if (= TRUE with-pad?)
	  (begin
	    (gimp-selection-layer-alpha layer)
	    (gimp-selection-invert img)
	    (gimp-layer-set-preserve-trans layer FALSE)
	    (gimp-palette-set-foreground padding-color)
	    (gimp-bucket-fill layer FG-BUCKET-FILL NORMAL
			      padding-opacity 0 0 0 0)
	    (gimp-selection-none img)))
      
      (gimp-layer-translate layer
			    (+ border shear-length) (+ border (* space index)))
      (gimp-shear layer TRUE 0 (* (/ (car (gimp-drawable-height layer))
					 old-height)
				      (* -2 shear-length)))
      (set! index (+ index 1)))
    (set! new-bg (- num-of-layers 1))
    (if (= TRUE with-background?)
	(begin
	  (set! new-bg (car (gimp-layer-new img new-width new-height RGBA_IMAGE
					    "New Background" 100 NORMAL)))
	  (gimp-image-add-layer img new-bg num-of-layers)
	  (gimp-palette-set-background background-color)
	  (gimp-edit-fill new-bg)))
    (gimp-image-set-active-layer img (aref layers 0))
    (if (= TRUE with-layer-name?)
	(let ((text-layer #f))
	  (gimp-palette-set-foreground '(255 255 255))
	  (set! index 0)
	  (set! layer-names (nreverse layer-names))
	  (while (< index num-of-layers)
	    (set! text-layer (car (gimp-text-fontname img -1 (/ border 2)
					     (+ (* space index) old-height)
					     (car layer-names)
					     0 TRUE 14 PIXELS "-*-helvetica-*-r-*-*-14-*-*-*-p-*-*-*")))
	    (gimp-layer-set-mode text-layer NORMAL)
	    (set! index (+ index 1))
	    (set! layer-names (cdr layer-names)))))
    (gimp-image-set-active-layer img new-bg)
    (gimp-palette-set-background old-background)
    (gimp-palette-set-foreground old-foreground)
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
    (gimp-displays-flush)))

(script-fu-register
 "script-fu-show-image-structure"
 "<Image>/Script-Fu/Utils/Show Image Structure..."
 "Show the layer structure of the image"
 "Shuji Narazaki <narazaki@InetQ.or.jp>"
 "Shuji Narazaki"
 "1997"
 "RGB*, GRAY*"
 SF-IMAGE "image" 0
 SF-DRAWABLE "Drawable (unused)" 0
 SF-TOGGLE "Make new image" script-fu-show-image-structure-new-image?
 SF-VALUE "Space between layers" (number->string script-fu-show-image-structure-space)
 SF-VALUE "Shear length (> 0)" (number->string script-fu-show-image-structure-shear-length)
 SF-VALUE "Outer Border (>= 0)" (number->string script-fu-show-image-structure-border)
 SF-TOGGLE "Apply layer mask (or discard)" script-fu-show-image-structure-apply-layer-mask?
 SF-TOGGLE "Insert layer names" script-fu-show-image-structure-with-layer-name?
 SF-TOGGLE "Padding for transparent regions" script-fu-show-image-structure-with-pad?
 SF-COLOR "Pad Color" script-fu-show-image-structure-padding-color
 SF-VALUE "Pad Opacity [0:100]" (number->string script-fu-show-image-structure-padding-opacity)
 SF-TOGGLE "Make New Background" script-fu-show-image-structure-with-background?
 SF-COLOR "Background Color" script-fu-show-image-structure-background-color
)

;;; image-structure.scm ends here
