;;; line-nova.scm -*-scheme-*-
;;; Time-stamp: <1998/01/17 21:15:38 narazaki@InetQ.or.jp>
;;; Author Shuji Narazaki <narazaki@inetq.or.jp>
;;; Version 0.6

(if (not (symbol-bound? 'script-fu-line-nova-num-of-lines (the-environment)))
    (define script-fu-line-nova-num-of-lines 200))
(if (not (symbol-bound? 'script-fu-line-nova-corn-deg (the-environment)))
    (define script-fu-line-nova-corn-deg 1.0))
(if (not (symbol-bound? 'script-fu-line-nova-offset (the-environment)))
    (define script-fu-line-nova-offset 100))
(if (not (symbol-bound? 'script-fu-line-nova-variation (the-environment)))
    (define script-fu-line-nova-variation 30))

(define (script-fu-line-nova img drw num-of-lines corn-deg offset variation)
  (let* ((*points* (cons-array (* 3 2) 'double))
	 (modulo fmod)			; in R4RS way
	 (pi/2 (/ *pi* 2))
	 (pi/4 (/ *pi* 4))
	 (pi3/4 (* 3 pi/4))
	 (pi5/4 (* 5 pi/4))
	 (pi3/2 (* 3 pi/2))
	 (pi7/4 (* 7 pi/4))
	 (2pi (* 2 *pi*))
	 (rad/deg (/ 2pi 360))
	 (variation/2 (/ variation 2))
	 (drw-width (car (gimp-drawable-width drw)))
	 (drw-height (car (gimp-drawable-height drw)))
	 (drw-offsets (gimp-drawable-offsets drw))
	 (old-selection (car (gimp-selection-save img)))
	 (radius (max drw-height drw-width))
	 (index 0)
	 (dir-deg/line (/ 360 num-of-lines)))
    (define (draw-vector beg-x beg-y direction)
      (define (set-point! index x y)
	(aset *points* (* 2 index) x)
	(aset *points* (+ (* 2 index) 1) y))
      (define (deg->rad rad)
	(* (modulo rad 360) rad/deg))
      (define (set-marginal-point beg-x beg-y direction)
	(let ((dir1 (deg->rad (+ direction corn-deg)))
	      (dir2 (deg->rad (- direction corn-deg))))
	  (define (aux dir index)
	    (set-point! index 
			(+ beg-x (* (cos dir) radius))
			(+ beg-y (* (sin dir) radius))))
	  (aux dir1 1)
	  (aux dir2 2)))
      (let ((dir0 (deg->rad direction))
	    (of (+ offset (- (modulo (rand) variation) variation/2))))
	(set-point! 0 
		    (+ beg-x (* of (cos dir0)))
		    (+ beg-y (* of (sin dir0))))
	(set-marginal-point beg-x beg-y direction)
	(gimp-free-select img 6 *points* ADD
			  TRUE		; antialias
			  FALSE		; feather
			  0		; feather radius 
			  )))

    (gimp-undo-push-group-start img)
    (gimp-selection-none img)
    (srand (realtime))
    (while (< index num-of-lines)
      (draw-vector (+ (nth 0 drw-offsets) (/ drw-width 2))
		   (+ (nth 1 drw-offsets) (/ drw-height 2))
		   (* index dir-deg/line))
      (set! index (+ index 1)))
    (gimp-bucket-fill img drw FG-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-load img old-selection)
    ;; (gimp-image-set-active-layer img drw)
    ;; delete extra channel by Sven Neumann <neumanns@uni-duesseldorf.de>
    (gimp-image-remove-channel img old-selection)
    (gimp-undo-push-group-end img)
    (set! script-fu-line-nova-num-of-lines num-of-lines)
    (set! script-fu-line-nova-corn-deg corn-deg)
    (set! script-fu-line-nova-offset offset)
    (set! script-fu-line-nova-variation variation)
    (gimp-displays-flush)))

(script-fu-register
 "script-fu-line-nova"
 "<Image>/Script-Fu/Render/Line Nova"
 "Line Nova. Draw lines with Foreground color from the center of image to the edges. 1st undo cancels bucket-fill. 2nd undo gets orignal selection."
 "Shuji Narazaki <narazaki@InetQ.or.jp>"
 "Shuji Narazaki"
 "1997"
 "RGB*, INDEXED*, GRAY*"
 SF-IMAGE "Image to use" 0
 SF-DRAWABLE "Drawable to draw line" 0
 SF-VALUE "Number of lines" (number->string script-fu-line-nova-num-of-lines)
 SF-VALUE "Sharpness (deg.)" (number->string script-fu-line-nova-corn-deg)
 SF-VALUE "Offset radius" (number->string script-fu-line-nova-offset)
 SF-VALUE "- randomness" (number->string script-fu-line-nova-variation)
)
;;; line-nova.scm ends here
