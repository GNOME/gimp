;; kanji-circle a script for The GIMP (漢字いり) -*-scheme-*-
;; Shuji Narazaki (narazaki@InetQ.or.jp)
;; Time-stamp: <1997/06/09 22:18:23 narazaki@InetQ.or.jp>
;; Version 0.8

(define (script-fu-kanji-circle text radius start-angle fill-angle 
				font font-size antialias)
  (let* ((drawable-size (* 2.0 (+ radius (* 2 font-size)))) ; with padding
	 (img (car (gimp-image-new drawable-size drawable-size RGB)))
	 (BG-layer (car (gimp-layer-new img drawable-size drawable-size
					RGBA_IMAGE "background" 100 NORMAL)))
	 (merged-layer #f)
	 (char-num (string-length text))
	 (radians (/ (* 2 *pi*) (/ char-num 2)))
	 (rad-90 (/ *pi* 2))
	 (center-x (/ drawable-size 2))
	 (center-y (/ drawable-size 2))
	 (desc 0)
	 (angle-list #f)
	 (letter "")
	 (new-layer #f)
	 (index 0))
    (gimp-image-disable-undo img)
    (gimp-image-add-layer img BG-layer 0)
    (gimp-edit-fill img BG-layer)
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
      (while (< index (/ char-num 2))
	(set! angle-list (cons font-size angle-list))
	(set! index (+ index 1)))
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
    (while (< index (/ char-num 2))
      (set! letter (substring text (* 2 index) (+ (* 2 index) 2)))
      (if (not (equal? " " letter))
	  ;; Running gimp-text with " " causes an error!
	  (let* ((new-layer (car (plug-in-vftext 1 img -1 0 0
						 letter
						 1 antialias 
						 font-size PIXELS
						 "*" font "*" "*" "*" 0)))
		 (width (car (gimp-drawable-width new-layer)))
		 (height (car (gimp-drawable-height new-layer)))
		 (rotate-radius (- (/ height 2) desc))
		 (angle (+ start-angle (- (nth index angle-list) rad-90))))
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
    (gimp-layer-set-name merged-layer "kanji circle")
    (gimp-layer-set-visible BG-layer 1)
    (gimp-image-enable-undo img)
    (gimp-display-new img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-kanji-circle"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Kanji Circle"
		    "Kanji Circle (version 0.3)"
		    "Shuji Narazaki <narazaki@InetQ.or.jp>"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-VALUE "Kanji Text" "\"ねうしとらうたつみうまひつじさるとりいぬい\""
		    SF-VALUE "Radius" "100"
		    SF-VALUE "Start-angle" "0"
		    SF-VALUE "Fill-angle" "360"
		    SF-VALUE "Font name" "\"min\""
		    SF-VALUE "Font Size" "24"
		    SF-TOGGLE "Antialias" TRUE
)

;; Local Variables:
;; buffer-file-coding-system: euc-japan
;; End:
;; end of kanji-circle.scm