;; circle-logo -- a script for The GIMP 1.0
;; Author: Shuji Narazaki (narazaki@InetQ.or.jp)
;; Time-stamp: <1997/06/09 22:16:20 narazaki@InetQ.or.jp>
;; Version 1.3

(define modulo fmod)			; in R4RS way

(define (script-fu-circle-logo text radius start-angle fill-angle
			       font font-size slant antialias)
  (let* ((drawable-size (* 2.0 (+ radius (* 2 font-size))))
	 (img (car (gimp-image-new drawable-size drawable-size RGB)))
	 (BG-layer (car (gimp-layer-new img drawable-size drawable-size
					RGBA_IMAGE "background" 100 NORMAL)))
	 (merged-layer #f)
	 (char-num (string-length text))
	 (radian-step 0)
	 (rad-90 (/ *pi* 2))
	 (center-x (/ drawable-size 2))
	 (center-y center-x)
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
    (gimp-image-add-layer img BG-layer 0)
    (gimp-edit-fill img BG-layer)
    ;; change units
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
    (gimp-layer-set-visible BG-layer 0)
    (set! merged-layer (car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
    (gimp-layer-set-name merged-layer "text circle")
    (gimp-layer-set-visible BG-layer 1)
    (gimp-image-enable-undo img)
    (gimp-display-new img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-circle-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Text Circle"
		    "Render the specified text along the perimeter of a circle"
		    "Shuji Narazaki (narazaki@InetQ.or.jp)"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-VALUE "Text" "\"Ring World again! Tiger! Tiger! Tiger! \""
		    SF-VALUE "Radius" "80"
		    SF-VALUE "Start-angle" "0"
		    SF-VALUE "Fill-angle" "360"
		    SF-VALUE "Family" "\"helvetica\""
		    SF-VALUE "Font Size (pixel)" "18"
		    SF-VALUE "Slant" "\"r\""
		    SF-TOGGLE "Antialias" TRUE
)

;; circle-logo.scm ends here
