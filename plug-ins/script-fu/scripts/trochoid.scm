;;; trochoid.scm -*-scheme-*-
;;; Time-stamp: <1997/06/13 23:15:23 narazaki@InetQ.or.jp>
;;; This file is a part of:
;;;   The GIMP (Copyright (C) 1995 Spencer Kimball and Peter Mattis)
;;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)
;;; Version 1.0

;;; Code:

(define (script-fu-trochoid base-radius-f wheel-radius-f pen-pos hue-rate
			    erase-before-draw brush-details)
  (if 'not-guile (define modulo fmod))
  (define (floor x) (- x (fmod x 1)))
  (define *prime-table*
    '(2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97))

  (define (LCM x y)			; Least Common Multiple
    (define (divide? x y) (= 0 (modulo x y)))
    (define (factorize x)
      (define (f-aux x p-list result)
	(cond ((= x 1)
	       (nreverse result))
	      ((null? p-list)
	       (nreverse (cons (list x 1) result)))
	      ((divide? x (car p-list))
	       (let ((times 1)
		     (p (car p-list)))
		 (set! x (/ x p))
		 (while (divide? x p)
			(set! times (+ times 1))
			(set! x (/ x p)))
		 (f-aux x (cdr p-list) (cons (list p times) result))))
	      ('else (f-aux x (cdr p-list) result))))
      (f-aux x *prime-table* '()))
    (define (extend-prime-table limit)
      (let ((index (+ (car *prime-table*) 1)))
	(while (< index limit)
	       (let ((prime? #t)
		     (table *prime-table*))
		 (while (and (not (null? table)) prime?)
			(if (divide? index (car table))
			    (set! prime? #f)
			    (set! table (cdr table))))
		 (if prime?
		     (set! *prime-table*
			   (nreverse (cons index (nreverse *prime-table*))))))
	       (set! index (+ index 1)))))
    (define (aux l1 l2 result)
      (cond ((and (null? l1) (null? l2)) result)
	    ((null? l1) (append l2 result))
	    ((null? l2) (append l1 result))
	    ((= (car (car l1)) (car (car l2)))
	     (aux (cdr l1) (cdr l2) (cons (list (car (car l1))
						(max (cadr (car l1))
						     (cadr (car l2))))
					  result)))
	    ((< (car (car l1)) (car (car l2)))
	     (aux (cdr l1) l2 (cons (car l1) result)))
	    ('else
	     (aux l1 (cdr l2) (cons (car l2) result)))))
    (if  (< (pow (car (reverse *prime-table*)) 2) (max x y))
	 (extend-prime-table (sqrt (max x y))))
    (let ((f-list (aux (factorize x) (factorize y) '()))
	  (result 1))
      (while (not (null? f-list))
	     (set! result (* (pow (car (car f-list)) (cadr (car f-list))) result))
	     (set! f-list (cdr f-list)))
      result))

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

;;; hsv-to-rgb that does not consume new cons cell
  (define (hsv-to-rgb hsv rgb)
    (let ((h (nth 0 hsv))
	  (s (nth 1 hsv))
	  (v (nth 2 hsv))
	  (hue 0)
	  (saturation 0)
	  (value 0))
      (if (= s 0)
	  (begin
	    (set! h v)
	    (set! s v))
	  (let ((f 0)
		(p 0)
		(q 0)
		(t 0))
	    (set! hue (/ (* 6 h) 255))
	    (if (= hue 6.0)
		(set! hue 0.0))
	    (set! saturation (/ s  255.0))
	    (set! value (/ v 255.0))
	    (set! f (- hue (floor hue)))
	    (set! p (* value (- 1.0 saturation)))
	    (set! q (* value (- 1.0 (* saturation f))))
	    (set! t (* value (- 1.0 (* saturation (- 1.0 f)))))
	    (let ((tmp (floor hue)))
	      (cond ((= 0 tmp)
		     (set! h (* value 255))
		     (set! s (* t 255))
		     (set! v (* p 255)))
		    ((= 1 tmp)
		     (set! h (* q 255))
		     (set! s (* value 255))
		     (set! v (* p 255)))
		    ((= 2 tmp)
		     (set! h (* p 255))
		     (set! s (* value 255))
		     (set! v (* t 255)))
		    ((= 3 tmp)
		     (set! h (* p 255))
		     (set! s (* q 255))
		     (set! v (* value 255)))
		    ((= 4 tmp)
		     (set! h (* t 255))
		     (set! s (* p 255))
		     (set! v (* value 255)))
		    ((= 5 tmp)
		     (set! h (* value 255))
		     (set! s (* p 255))
		     (set! v (* q 255)))))))
      (set-car! rgb h)
      (set-car! (cdr rgb) s)
      (set-car! (cddr rgb) v)))
  
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

  (define (update-segment! center-x center-y angle1 rad1 angle2 rad2 segment)
    (define (fill-segment! new-x new-y segment)
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
    (set! angel1 (fmod angle1 (* 2 *pi*)))
    (set! angel2 (fmod angle2 (* 2 *pi*)))
    (fill-segment! (+ center-x
		      (* rad1 (sin angle1))
		      (* rad2 (- (sin angle2))))
		   (+ center-y
		      (* rad1 (cos angle1))
		      (* rad2 (cos angle2)))
		   segment))

  ; (set-brush-color! index total-step hue-rate rgb hsv)
  ; (set! drawable-to-erase drawable)
  (define (draw-segment img drawable drawable-to-erase single-drawable?
			segment limit rgb background-color
			stroke-overwrite keep-opacity paint-mode)
    (if (not stroke-overwrite)
	(begin		; erase crossover region
	  (if (< keep-opacity 100) (gimp-brushes-set-opacity 100))
	  (if single-drawable?
	      (begin
		(gimp-brushes-set-paint-mode NORMAL)
		(gimp-palette-set-foreground background-color)
		(gimp-airbrush drawable-to-erase 100
			       (* 2 limit) (segment-strokes segment))
		(gimp-brushes-set-paint-mode paint-mode))
	      (gimp-eraser drawable-to-erase (* 2 limit)
			   (segment-strokes segment)))
	  (if (< keep-opacity 100) (gimp-brushes-set-opacity keep-opacity))))
    (gimp-palette-set-foreground rgb)
    (gimp-airbrush drawable 100 (* 2 limit) (segment-strokes segment)))

  (define (set-brush-color! index max-index hue-rate rgb hsv)
    (if (= 0 hue-rate)
	rgb
	(let* ((max-hue 254)
	       (hue (* max-hue (fmod (/ (* index (abs hue-rate)) max-index) 1))))
	  (if (< hue-rate 0)
	      (set! hue (- max-hue hue)))
	  (set-car! hsv hue)
	  (hsv-to-rgb hsv rgb)
	  rgb)))

  (define (trochoid-rotate-gear total-distance img use-this-drawable center-x
				center-y base-radius wheel-radius pen-pos hue-rate
				layer-paint-mode stroke-overwrite brush-details)
    (let* ((rad-of-wheel 0)
	   (steps-for-circle 100.0)
	   (wheel-spin (/ total-distance (abs wheel-radius)))
	   (total-step (* wheel-spin steps-for-circle))
	   (loop-num (max (* 2 (/ total-distance base-radius))
			  (/ total-distance (abs wheel-radius))))
	   (steps-for-a-loop (/ total-step loop-num))
	   (w2r (/ (abs wheel-radius) base-radius))
	   (rad-of-step (/ (* 2.0 *pi*) steps-for-circle))
	   (brush-opacity (car (gimp-brushes-get-opacity)))
	   (rgb (car (gimp-palette-get-foreground)))
	   (drawable use-this-drawable)
	   (drawable-to-erase use-this-drawable)
	   (paint-mode (car (gimp-brushes-get-paint-mode)))
	   (background-color (car (gimp-palette-get-background)))
	   (hsv '(0 255 255))
	   (index 0)
	   (iindex 0)
	   (center2wheel (+ base-radius wheel-radius))
	   (wheel2pen (* (abs wheel-radius) pen-pos))
	   (segment (make-segment
		     (if (= 0 hue-rate)
			 32
			 (max 4 (floor (/ (/ total-step (abs hue-rate)) 255.0))))
		     center-x (+ center-y center2wheel wheel2pen))))
      (while (< 0 loop-num)
	     (set! iindex 0)
	     (if (null? use-this-drawable)
		 (begin
		   (if drawable (set! drawable-to-erase drawable))
		   (set! drawable (car (gimp-layer-copy
					(or drawable
					    (car (gimp-image-get-active-layer img)))
					1)))
		   (if (not drawable-to-erase) (set! drawable-to-erase drawable))
		   (gimp-image-add-layer img drawable 0)
		   (gimp-layer-set-mode drawable layer-paint-mode)
		   (gimp-layer-set-name drawable
					(string-append "cricle "
						       (number->string loop-num)))
		   (gimp-edit-clear drawable)))
	     (while (< iindex steps-for-a-loop) ; draw a circle
		    (set! rad-of-wheel (* rad-of-step index))
		    (if (update-segment! center-x center-y
					 (* w2r rad-of-wheel) center2wheel
					 rad-of-wheel wheel2pen
					 segment)
			(begin
			  (draw-segment img drawable drawable-to-erase use-this-drawable
					segment (segment-max-size segment)
					(set-brush-color! index total-step hue-rate rgb hsv)
					background-color
					stroke-overwrite brush-opacity paint-mode)
			  (set! drawable-to-erase drawable)))
		    (set! index (+ index 1))
		    (set! iindex (+ iindex 1)))
	     (if use-this-drawable (gimp-displays-flush))
	     (set! loop-num (- loop-num 1)))
      (while (<= index total-step)
	     (set! rad-of-wheel (* rad-of-step index))
	     (if (update-segment! center-x center-y
				  (* w2r rad-of-wheel) center2wheel
				  rad-of-wheel wheel2pen
				  segment)
		 (begin
		   (draw-segment img drawable drawable-to-erase use-this-drawable
				 segment (segment-max-size segment)
				 (set-brush-color! index total-step hue-rate rgb hsv)
				 background-color
				 stroke-overwrite brush-opacity paint-mode)
		   (set! drawable-to-erase drawable)))
	     (set! index (+ index 1)))
      (if (< 1 (segment-filled-size segment))
	  (draw-segment img drawable drawable-to-erase use-this-drawable
			segment (segment-filled-size segment)
			(set-brush-color! index total-step hue-rate rgb hsv)
			background-color
			stroke-overwrite brush-opacity paint-mode))))
  ;; start of script-fu-trochoid
  (gimp-brushes-set-brush (car brush-details))
  (let* ((base-radius (floor (abs base-radius-f))) ; to int
	 (wheel-radius (floor wheel-radius-f)) ; to int
	 (total-step-num (if (or (= 0 base-radius) (= 0 wheel-radius))
			     1
			     (LCM base-radius (abs wheel-radius))))
	 (brush-size (gimp-brushes-get-brush))
	 (drawable-size (if (or (= 0 base-radius) (= 0 wheel-radius))
			    256
			    (* 2.0 (+ base-radius
				      (max (* 2 wheel-radius) 0)
				      (max (nth 1 brush-size)
					   (nth 2 brush-size))))))
	 (img (car (gimp-image-new drawable-size drawable-size RGB)))
	 (BG-layer (car (gimp-layer-new img drawable-size drawable-size
					RGBA_IMAGE "background" 100 NORMAL)))
	 (layer-paint-mode 0)
	 (the-layer #f)
	 (old-paint-mode (car (gimp-brushes-get-paint-mode)))
	 (old-rgb (car (gimp-palette-get-foreground))))
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img BG-layer 0)
    (gimp-edit-fill BG-layer)
    (if (<= 0 erase-before-draw)	; HDDN FTR (2SLW)
	(begin
	  (set! the-layer (car (gimp-layer-new img drawable-size drawable-size
					       RGBA_IMAGE "the curve"
					       100 NORMAL)))
	  (gimp-image-add-layer img the-layer 0)
	  (if (= NORMAL old-paint-mode)
	      (gimp-edit-clear the-layer)
	      (gimp-edit-fill the-layer)))
	(begin
	  (set! layer-paint-mode (- 1 erase-before-draw))
	  (gimp-image-set-active-layer img BG-layer)))
    (gimp-display-new img)
    (gimp-displays-flush)
    (if (or (= base-radius 0) (= wheel-radius 0))
	(gimp-text-fontname img -1 0 0
		   "`Base-radius'\n and\n`Rad.;hyp<0<epi'\n require\n non zero values."
		   1 1 18 PIXELS "-*-helvetica-*-r-*-*-*-*-*-*-p-*-*-*")
	(trochoid-rotate-gear total-step-num img the-layer
			      (/ drawable-size 2) (/ drawable-size 2)
			      base-radius wheel-radius pen-pos hue-rate
			      layer-paint-mode (= 0 erase-before-draw) brush-details))
    (gimp-palette-set-foreground old-rgb)
    (gimp-brushes-set-paint-mode old-paint-mode)
    (gimp-image-undo-enable img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-trochoid"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/Trochoid..."
		    "Draw Trochoid Curve"
		    "Shuji Narazaki <narazaki@InetQ.or.jp>"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-ADJUSTMENT "Base radius (pixel)" '(40 0 512 1 1 0 0)
		    SF-ADJUSTMENT "Wheel Radius (hypo < 0 < epi)" '(60 0 512 1 1 0 0)
		    SF-ADJUSTMENT "Pen rad./wheel rad. [0.0:1.0]" '(0.8 0 1 .01 .01 2 0)
		    SF-ADJUSTMENT "Hue Rate" '(1.0 0 1 .01 .01 2 0)
		    SF-VALUE "Erase before draw? [0/1]" "0"
	            SF-BRUSH "Use brush" '("Circle (05)" 1.0 44 2)
)

;;; trochoid.scm ends here
