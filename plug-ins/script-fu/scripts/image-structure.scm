;;; image-structure.scm -*-scheme-*-
;;; Time-stamp: <1997/06/30 00:21:41 narazaki@InetQ.or.jp>
;;; Author: Shuji Narazaki <narazaki@InetQ.or.jp>
;;; Version 0.3
;;; Code:

(define (script-fu-show-image-structure img drawable space shear-length border
					apply-layer-mask? with-layer-name?
					with-pad? padding-color padding-opacity
					with-background? background-color)
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
	    (gimp-selection-layer-alpha img layer)
	    (gimp-selection-invert img)
	    (gimp-layer-set-preserve-trans layer FALSE)
	    (gimp-palette-set-foreground padding-color)
	    (gimp-bucket-fill img layer FG-BUCKET-FILL NORMAL
			      padding-opacity 0 0 0 0)
	    (gimp-selection-none img)))
      
      (gimp-layer-translate layer
			    (+ border shear-length) (+ border (* space index)))
      (gimp-shear img layer TRUE 0 (* (/ (car (gimp-drawable-height layer))
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
	  (gimp-edit-fill img new-bg)))
    (gimp-image-set-active-layer img (aref layers 0))
    (if (= TRUE with-layer-name?)
	(let ((text-layer #f))
	  (gimp-palette-set-foreground '(255 255 255))
	  (set! index 0)
	  (set! layer-names (nreverse layer-names))
	  (while (< index num-of-layers)
	    (set! text-layer (car (gimp-text img -1 (/ border 2)
					     (+ (* space index) old-width)
					     (car layer-names)
					     0 TRUE 14 PIXELS "*" "helvetica"
					     "*" "*" "*" "*")))
	    (gimp-layer-set-mode text-layer NORMAL)
	    (set! index (+ index 1))
	    (set! layer-names (cdr layer-names)))))
    (gimp-image-set-active-layer img new-bg)
    (gimp-palette-set-background old-background)
    (gimp-palette-set-foreground old-foreground)
    (gimp-displays-flush)))

(script-fu-register "script-fu-show-image-structure"
		    "<Image>/Script-Fu/Utils/Show Image Structure Destructively"
		    "Show the layer structure of the image DESTRACTIVELY(the original image was modified)"
		    "Shuji Narazaki (narazaki@InetQ.or.jp)"
		    "Shuji Narazaki"
		    "1997"
		    "RGB*, GRAY*"
		    SF-IMAGE "image" 0
		    SF-DRAWABLE "Drawable (unused)" 0
		    SF-VALUE "Space between layers" "50"
		    SF-VALUE "Shear length (> 0)" "50"
		    SF-VALUE "Outer Border (>= 0)" "10"
		    SF-TOGGLE "Apply layer mask (otherwise discard)" TRUE 
		    SF-TOGGLE "Insert layer names" TRUE
		    SF-TOGGLE "Padding for transparent regions" TRUE
		    SF-COLOR "Pad Color" '(255 255 255)
		    SF-VALUE "Pad Opacity [0:100]" "25"
		    SF-TOGGLE "Make New Background" TRUE
		    SF-COLOR "Background Color" '(0 0 0)
)

;;; image-structure.scm ends here
