;; text-circle -- a scheme script for The GIMP
;; Shuji Narazaki (narazaki@InetQ.or.jp)
;; Time-stamp: <1997/05/16 23:44:21 narazaki@InetQ.or.jp>
;; Version 1.0
;; 1.0 official release with the following modifications:
;; 0.7 implement reverse fill mode (try minus fill-angle)
;; 0.6 handle proportional font correctly
;; 0.5 fix the bug in angle calculation
;; 0.4 add start/fill angle parameters
;; 0.3 included in 0.99.8

(define modulo fmod)			; in R4RS way

(define (text-circle text radius start-angle fill-angle
		     font font-size slant antialias)
  (let* ((width (* radius 2.5))
	 (height (* radius 2.5))
	 (img (car (gimp-image-new width height RGB)))
	 (drawable (car (gimp-layer-new img width height RGBA_IMAGE
					"text-circle" 100 NORMAL)))
	 (char-num (string-length text))
	 (radian-step 0)
	 (rad-90 (/ *pi* 2))
	 (center-x (/ width 2))
	 (center-y (/ height 2))
	 (fixed-pole " ]Ag")		; some fonts have no "]" "g" has desc.
	 (font-infos (gimp-text-get-extents fixed-pole font-size PIXELS
					    "*" font "*" slant "*" "*"))
	 (extra (max 0 (- (nth 0 font-infos) 5))) ; why 5? See text_tool.c.
	 (desc (nth 3 font-infos))
	 (angle-list #f)
	 (letter "")
	 (new-layer #f)
	 (index 0))
    (gimp-image-disable-undo img)
    ;(gimp-display-new img)
    (gimp-image-add-layer img drawable 0)
    (gimp-edit-fill img drawable)
    ;; change unit
    (set! start-angle (* (/ (modulo start-angle 360) 360) 2 *pi*))
    (set! fill-angle (* (/ fill-angle 360) 2 *pi*))
    (set! radian-step (/ fill-angle char-num))
    ;; make width-list
    (let ((temp-list '())
	  (temp-str #f)
	  (scale 0)
	  (temp #f))
      (set! index 0)
      (while (< index char-num)
	(set! temp-str (substring text index (+ index 1)))
	(if (equal? " " temp-str)
	    (set! temp-str "]"))
	(set! temp (gimp-text-get-extents temp-str font-size PIXELS
					  "*" font "*" slant "*" "*"))
	(set! temp-list (cons (nth 0 temp) temp-list))
	(set! index (+ index 1)))
      (set! angle-list (nreverse temp-list))
      (set! temp 0)
      (set! angle-list
	    (mapcar (lambda (angle) 
		      (let ((tmp temp))
			(set! temp (+ angle temp))
			(+ tmp (/ angle 2))))
		    angle-list))
      (set! scale (/ fill-angle temp))
      (set! angle-list (mapcar (lambda (angle) (* scale angle)) angle-list)))
    (set! index 0)
    (while (< index char-num)
      (set! letter (substring text index (+ index 1)))
      (if (not (equal? " " letter))
	  ;; Running gimp-text with " " causes an error!
	  (let* ((new-layer (car (gimp-text img -1 0 0
					    (string-append letter fixed-pole)
					    1 antialias 
					    font-size PIXELS
					    "*" font "*" slant "*" "*")))
		 (width (car (gimp-drawable-width new-layer)))
		 (height (car (gimp-drawable-height new-layer)))
		 (rotate-radius (- (/ height 2) desc))
		 (angle (+ start-angle (- (nth index angle-list) rad-90))))
	    ;; delete fixed-pole
	    (gimp-layer-resize new-layer (- width extra 1) height 0 0)
	    (set! width (car (gimp-drawable-width new-layer)))
	    (gimp-layer-translate new-layer
				  (+ center-x
				     (* radius (cos angle))
				     (* rotate-radius
					(cos (if (< 0 fill-angle)
						 angle
						 (+ angle *pi*))))
				     (- (/ width 2)))
				  (+ center-y
				     (* radius (sin angle))
				     (* rotate-radius
					(sin (if (< 0 fill-angle) 
						 angle
						 (+ angle *pi*))))
				     (- (/ height 2))))
	    (gimp-rotate img new-layer 1 
			 ((if (< 0 fill-angle) + -) angle rad-90))))
      (set! index (+ index 1)))
    (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "text-circle" "Text Circle"
		    SF-VALUE "Text" "\"Ring World again! Tiger! Tiger! Tiger! \""
		    SF-VALUE "Radius" "80"
		    SF-VALUE "Start-angle" "0"
		    SF-VALUE "Fill-angle" "360"
		    SF-VALUE "Family" "\"helvetica\""
		    SF-VALUE "Font Size (pixel)" "18"
		    SF-VALUE "Slant" "\"r\""
		    SF-VALUE "Antialias [0/1]" "1"
)

;; text-circle.scm ends here

