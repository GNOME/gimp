;;; hsv-graph.scm -*-scheme-*-
;;; Author: Shuji Narazaki <narazaki@InetQ.or.jp>
;;; Time-stamp: <1997/07/31 21:10:53 narazaki@InetQ.or.jp>
;;; Version: 0.7
;;; Code:

(define (script-fu-hsv-graph img drawable scale opacity bounds?
			     left2right? beg-x beg-y end-x end-y)
  (define (floor x) (- x (fmod x 1)))
  (define *pos* #f)
  (define (set-point fvec index x y)
    (aset! fvec (* 2 index) x)
    (aset! fvec (+ (* 2 index) 1) y)
    fvec)

  (define (plot-dot img drawable x y)
    (gimp-pencil img drawable 1 (set-point! *pos* 0 x y)))

  (define (rgb-to-hsv rgb hsv)
    (let* ((red (floor (nth 0 rgb)))
	   (green (floor (nth 1 rgb)))
	   (blue (floor (nth 2 rgb)))
	   (h 0.0)
	   (s 0.0)
	   (minv (min red (min green blue)))
	   (maxv (max red (max green blue)))
	   (v maxv)
	   (delta 0))
      (if (not (= 0 maxv))
	  (set! s (/ (* (- maxv minv) 255.0) maxv))
	  (set! s 0.0))
      (if (= 0.0 s)
	  (set! h 0.0)
	  (begin
	    (set! delta (- maxv minv))
	    (cond ((= maxv red)
		   (set! h (/ (- green blue) delta)))
		  ((= maxv green)
		   (set! h (+ 2.0 (/ (- blue red) delta))))
		  ((= maxv blue)
		   (set! h (+ 4.0 (/ (- red green) delta)))))
	    (set! h (* 42.5 h))
	    (if (< h 0.0)
		(set! h (+ h 255.0)))
	    (if (< 255 h)
		(set! h (- h 255.0)))))
      (set-car! hsv (floor h))
      (set-car! (cdr hsv) (floor s))
      (set-car! (cddr hsv) (floor v))))

  ;; segment is
  ;;   filled-index (integer)
  ;;   size as number of points (integer)
  ;;   vector (which size is 2 * size)
  (define (make-segment length x y)
    (if (< 64 length)
	(set! length 64))
    (if (< length 5)
	(set! length 5))
    (let ((vec (cons-array (* 2 length) 'double)))
      (aset vec 0 x)
      (aset vec 1 y)
      (list 1 length vec)))

  ;; accessors
  (define (segment-filled-size segment) (car segment))
  (define (segment-max-size segment) (cadr segment))
  (define (segment-strokes segment) (caddr segment))

  (define (fill-segment! segment new-x new-y)
    (define (shift-segment! segment)
      (let ((base 0)
	    (size (cadr segment))
	    (vec (caddr segment))
	    (offset 2))
	(while (< base offset)
	       (aset vec (* 2 base)
		     (aref vec (* 2 (- size (- offset base)))))
	       (aset vec (+ (* 2 base) 1)
		     (aref vec (+ (* 2 (- size (- offset base))) 1)))
	       (set! base (+ base 1)))
	(set-car! segment base)))  
    (let ((base (car segment))
	  (size (cadr segment))
	  (vec (caddr segment)))
      (if (= base 0)
	  (begin
	    (shift-segment! segment)
	    (set! base (segment-filled-size segment))))
      (if (and (= new-x (aref vec (* 2 (- base 1))))
	       (= new-y (aref vec (+ (* 2 (- base 1)) 1))))
	  #f
	  (begin
	    (aset vec (* 2 base) new-x)
	    (aset vec (+ (* 2 base) 1) new-y)
	    (set! base (+ base 1))
	    (if (= base size)
		(begin
		  (set-car! segment 0)
		  #t)
		(begin
		  (set-car! segment base)
		  #f))))))

  (define (draw-segment img drawable segment limit rgb)
    (gimp-palette-set-foreground rgb)
    (gimp-airbrush img drawable 100 (* 2 limit) (segment-strokes segment)))

  (define red-color '(255 10 10))
  (define green-color '(10 255 10))
  (define blue-color '(10 10 255))
  (define hue-segment #f)
  (define saturation-segment #f)
  (define value-segment #f)
  (define red-segment #f)
  (define green-segment #f)
  (define blue-segment #f)
  (define border-size 10)

  (define (fill-dot img drawable x y segment color)
    (if (fill-segment! segment x y)
	(begin
	  (gimp-palette-set-foreground color)	
	  (draw-segment img drawable segment (segment-max-size segment) color)
	  #t)
	#f))

  (define (fill-color-band img drawable x scale x-base y-base color)
    (gimp-palette-set-foreground color)
    (gimp-rect-select img (+ x-base (* scale x)) 0 scale y-base REPLACE FALSE 0)
    (gimp-bucket-fill img drawable FG-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img))

  (define (plot-hsv img drawable x scale x-base y-base hsv)
    (let ((real-x (* scale x))
	  (h (car hsv))
	  (s (cadr hsv))
	  (v (caddr hsv)))
      (fill-dot img drawable (+ x-base real-x) (- y-base h)
		hue-segment red-color)
      (fill-dot img drawable (+ x-base real-x) (- y-base s)
		saturation-segment green-color)
      (if (fill-dot img drawable (+ x-base real-x) (- y-base v)
		    value-segment blue-color)
	  (gimp-displays-flush))))

  (define (plot-rgb img drawable x scale x-base y-base hsv)
    (let ((real-x (* scale x))
	  (h (car hsv))
	  (s (cadr hsv))
	  (v (caddr hsv)))
      (fill-dot img drawable (+ x-base real-x) (- y-base h)
		red-segment red-color)
      (fill-dot img drawable (+ x-base real-x) (- y-base s)
		green-segment green-color)
      (if (fill-dot img drawable (+ x-base real-x) (- y-base v)
		    blue-segment blue-color)
	  (gimp-displays-flush))))

  (define (clamp-value x minv maxv)
    (if (< x minv)
	(set! x minv))
    (if (< maxv x)
	(set! x maxv))
    x)

  ;; start of script-fu-hsv-graph
  (if (= TRUE bounds?)
      (if (= TRUE (car (gimp-selection-bounds img)))
	  (let ((results (gimp-selection-bounds img)))
	    (set! beg-x (nth (if (= TRUE left2right?) 1 3) results))
	    (set! beg-y (nth 2 results))
	    (set! end-x (nth (if (= TRUE left2right?) 3 1) results))
	    (set! end-y (nth 4 results)))
	  (begin 
	    (set! beg-x (if (= TRUE left2right?)
			    0
			    (- (car (gimp-drawable-width drawable)) 1)))
	    (set! beg-y 0)
	    (set! end-x (if (= TRUE left2right?)
			    (- (car (gimp-drawable-width drawable)) 1)
			    0))
	    (set! end-y (- (car (gimp-drawable-height drawable)) 1))))
      (begin
	(set! beg-x (clamp-value beg-x 0 (gimp-drawable-width drawable)))
	(set! end-x (clamp-value end-x 0 (gimp-drawable-width drawable)))
	(set! beg-y (clamp-value beg-y 0 (gimp-drawable-height drawable)))
	(set! end-y (clamp-value beg-y 0 (gimp-drawable-heigth drawable)))))
  (set! opacity (clamp-value opacity 0 100))
  (let* ((x-len (- end-x beg-x))
	 (y-len (- end-y beg-y))
	 (limit (pow (+ (pow x-len 2) (pow y-len 2)) 0.5))
	 (gimg-width (* limit scale))
	 (gimg-height 256)
	 (gimg (car (gimp-image-new (+ (* 2 border-size) gimg-width)
				    (+ (* 2 border-size) gimg-height) RGB)))
	 (bglayer (car (gimp-layer-new gimg
				       (+ (* 2 border-size) gimg-width)
				       (+ (* 2 border-size) gimg-height)
				       RGB_IMAGE "Background" 100 NORMAL)))
	 (hsv-layer (car (gimp-layer-new gimg
				       (+ (* 2 border-size) gimg-width)
				       (+ (* 2 border-size) gimg-height)
				      RGBA_IMAGE "HSV Graph" 100 NORMAL)))
	 (rgb-layer (car (gimp-layer-new gimg
				       (+ (* 2 border-size) gimg-width)
				       (+ (* 2 border-size) gimg-height)
				      RGBA_IMAGE "RGB Graph" 100 NORMAL)))
	 (clayer (car (gimp-layer-new gimg gimg-width 40 RGBA_IMAGE
				       "Color Sampled" opacity NORMAL)))
	 (rgb '(255 255 255))
	 (hsv '(254 255 255))
	 (x-base border-size)
	 (y-base (+ gimg-height border-size))
	 (index 0)
	 (old-foreground (car (gimp-palette-get-foreground)))
	 (old-background (car (gimp-palette-get-background)))
	 (old-paint-mode (car (gimp-brushes-get-paint-mode)))
	 (old-brush (car (gimp-brushes-get-brush)))
	 (old-opacity (car (gimp-brushes-get-opacity))))
    (gimp-image-disable-undo gimg)
    (gimp-image-add-layer gimg bglayer -1)
    (gimp-selection-all gimg)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill gimg bglayer)
    (gimp-image-add-layer gimg hsv-layer -1)
    (gimp-edit-clear gimg hsv-layer)
    (gimp-image-add-layer gimg rgb-layer -1)
    (gimp-layer-set-visible rgb-layer FALSE)
    (gimp-edit-clear gimg rgb-layer)
    (gimp-image-add-layer gimg clayer -1)
    (gimp-edit-clear gimg clayer)
    (gimp-layer-translate clayer border-size 0)
    (gimp-selection-none gimg)
    (set! red-segment (make-segment 64 x-base y-base))
    (set! green-segment (make-segment 64 x-base y-base))
    (set! blue-segment (make-segment 64 x-base y-base))
    (set! hue-segment (make-segment 64 x-base y-base))
    (set! saturation-segment (make-segment 64 x-base y-base))
    (set! value-segment (make-segment 64 x-base y-base))
    (gimp-brushes-set-brush "Circle (01)")
    (gimp-brushes-set-paint-mode NORMAL)
    (gimp-brushes-set-opacity 70)
    (gimp-display-new gimg)
    (while (< index limit)
      (set! rgb (car (gimp-color-picker img drawable
					(+ beg-x (* x-len (/ index limit)))
					(+ beg-y (* y-len (/ index limit)))
					TRUE FALSE)))
      (fill-color-band gimg clayer index scale x-base 40 rgb)
      (rgb-to-hsv rgb hsv)
      (plot-hsv gimg hsv-layer index scale x-base y-base hsv)
      (plot-rgb gimg rgb-layer index scale x-base y-base rgb)
      (set! index (+ index 1)))
    (mapcar
     (lambda (segment color)
       (if (< 1 (segment-filled-size segment))
	(begin 
	  (gimp-palette-set-foreground color)
	  (draw-segment gimg hsv-layer segment (segment-filled-size segment)
			color))))
     (list hue-segment saturation-segment value-segment)
     (list red-color green-color blue-color))
    (mapcar
     (lambda (segment color)
       (if (< 1 (segment-filled-size segment))
	(begin 
	  (gimp-palette-set-foreground color)
	  (draw-segment gimg rgb-layer segment (segment-filled-size segment)
			color))))
     (list red-segment green-segment blue-segment)
     (list red-color green-color blue-color))
    (gimp-palette-set-foreground '(255 255 255))
    (let ((text-layer (car (gimp-text gimg -1 0 0 
				      "Red: Hue, Green: Sat, Blue: Val"
				      1 1 12 PIXELS "*" 
				      "helvetica" "*" "*" "*" "*")))
	  (offset-y (- y-base (car (gimp-drawable-height clayer)))))
      (gimp-layer-set-mode text-layer DIFFERENCE)
      (gimp-layer-translate clayer 0 offset-y)
      (gimp-layer-translate text-layer border-size (+ offset-y 15)))
    (gimp-image-set-active-layer gimg bglayer)
    ;; return back the state
    (gimp-palette-set-foreground old-foreground)
    (gimp-palette-set-foreground old-background)
    (gimp-brushes-set-brush old-brush)
    (gimp-brushes-set-paint-mode old-paint-mode)
    (gimp-brushes-set-opacity old-opacity)
    (gimp-image-enable-undo gimg)
    (gimp-displays-flush)))

(script-fu-register "script-fu-hsv-graph"
		    "<Image>/Script-Fu/Utils/Draw HSV Graph"
		    "Draph the graph of H/S/V values on the drawable"
		    "Shuji Narazaki (narazaki@InetQ.or.jp)"
		    "Shuji Narazaki"
		    "1997"
		    "RGB*"
		    SF-IMAGE "Image to analyze" 0
		    SF-DRAWABLE "Drawable to analyze" 0
		    SF-VALUE "Graph Scale" "1"
		    SF-VALUE "BG Opacity" "50"
		    SF-TOGGLE "Use Selection Bounds instead of belows" TRUE
		    SF-TOGGLE "from Top-Left to Bottom-Right" TRUE
		    SF-VALUE "Start X" "0"
		    SF-VALUE "Start Y" "0"
		    SF-VALUE "End X" "100"
		    SF-VALUE "End Y" "100"
)

;;; hsv-graph.scm ends here
