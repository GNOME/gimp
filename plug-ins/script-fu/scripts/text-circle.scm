;; text-circle.scm -- a script for The GIMP 1.1
;; Author: Shuji Narazaki <narazaki@gimp.org>
;; Time-stamp: <1998/11/25 13:26:51 narazaki@gimp.org>
;; Version 2.5
;; Thanks:
;;   jseymour@jimsun.LinxNet.com (Jim Seymour)
;;   Sven Neumann <neumanns@uni-duesseldorf.de>

;; Note:
;;  Please remove /usr/local/share/gimp/scripts/circle-logo.scm, which is
;;  obsolete version of this script.

;; Implementation memo:
;; This script uses "extra-pole".
;; Namely, when rendering a letter, gimp-text is invoked with the letter
;; followed by " lAgy", then strips it by gimp-layer-resize. I call this " lAgy"
;; extra-pole. Why is it needed?
;; Since a text is located by its left-upper corner's position, THERE IS NO WAY
;; TO PLACE LETTERS ON A BASE LINE!
;; (FURTHERMORE, GIMP-TEXT EATS WHITESPACES AT THE BEGINNING/END OF LINE.)
;; Thus, as a dirty trick, by adding tall letters: "lA", and "gy" which have
;; large descent value to each letter temporally, most letters in most fonts
;; are aligned correctly. But don't expect completeness :-<

(if (not (symbol-bound? 'script-fu-text-circle-debug? (the-environment)))
    (define script-fu-text-circle-debug? #f))

(define (script-fu-text-circle text radius start-angle fill-angle
			       font-size antialias font-name)
  ;;(set! script-fu-text-circle-debug? #t)
  (define extra-pole TRUE)		; for debugging purpose
  (define modulo fmod)			; in R4RS way
  (define (wrap-string str) (string-append "\"" str "\""))
  (define (white-space-string? str)
    (or (equal? " " str) (equal? "	" str)))
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
	 ;; widths of " lAgy" and of "l Agy" will be different, because gimp-text
	 ;; strips spaces at the beginning of a string![Mon Apr 27 15:10:39 1998]
	 (fixed-pole0 "l Agy")
	 ;; the following used as real pad.
	 (fixed-pole " lAgy")
	 (font-infos (gimp-text-get-extents-fontname fixed-pole font-size
						     PIXELS font-name))
	 (desc (nth 3 font-infos))
	 (extra 0)			; extra is calculated from real layer
	 (angle-list #f)
	 (letter "")
	 (new-layer #f)
	 (index 0))
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img BG-layer 0)
    (gimp-edit-fill BG-layer BG-IMAGE-FILL)
    ;; change units
    (set! start-angle-rad (* (/ (modulo start-angle 360) 360) 2 *pi*))
    (set! fill-angle-rad (* (/ fill-angle 360) 2 *pi*))
    (set! radian-step (/ fill-angle-rad char-num))
    ;; set extra
    (if (eq? extra-pole TRUE)
	(let ((temp-pole-layer (car (gimp-text-fontname img -1 0 0
					       fixed-pole0
					       1 antialias
					       font-size PIXELS
					       font-name))))
	  (set! extra (car (gimp-drawable-width temp-pole-layer)))
	  (gimp-image-remove-layer img temp-pole-layer))
	(set! extra 0))
    ;; make width-list
    ;;  In a situation,
    ;; (car (gimp-drawable-width (car (gimp-text ...)))
    ;; != (car (gimp-text-get_extent ...))
    ;; Thus, I changed to gimp-text from gimp-text-get-extent at 2.2 !!
    (let ((temp-list '())
	  (temp-str #f)
	  (temp-layer #f)
	  (scale 0)
	  (temp #f))
      (set! index 0)
      (while (< index char-num)
	(set! temp-str (substring text index (+ index 1)))
	(if (white-space-string? temp-str)
	    (set! temp-str "x"))
	(set! temp-layer (car (gimp-text-fontname img -1 0 0
						  temp-str
						  1 antialias
						  font-size PIXELS
						  font-name)))
	(set! temp-list (cons (car (gimp-drawable-width temp-layer)) temp-list))
	(gimp-image-remove-layer img temp-layer)
	(set! index (+ index 1)))
      (set! angle-list (nreverse temp-list))
      (set! temp 0)
      (set! angle-list
	    (mapcar (lambda (angle)
		      (let ((tmp temp))
			(set! temp (+ angle temp))
			(+ tmp (/ angle 2))))
		    angle-list))
      (set! scale (/ fill-angle-rad temp))
      (set! angle-list (mapcar (lambda (angle) (* scale angle)) angle-list)))
    (set! index 0)
    (while (< index char-num)
      (set! letter (substring text index (+ index 1)))
      (if (not (white-space-string? letter))
	  ;; Running gimp-text with " " causes an error!
	  (let* ((new-layer
		  (car (gimp-text-fontname img -1 0 0
					   (if (eq? extra-pole TRUE)
					       (string-append letter fixed-pole)
					       letter)
					   1 antialias
					   font-size PIXELS
					   font-name)))
		 (width (car (gimp-drawable-width new-layer)))
		 (height (car (gimp-drawable-height new-layer)))
		 (rotate-radius (- (/ height 2) desc))
		 (new-width (- width extra))
		 (angle (+ start-angle-rad (- (nth index angle-list) rad-90))))
	    ;; delete fixed-pole
	    (gimp-layer-resize new-layer new-width height 0 0)
	    (set! width (car (gimp-drawable-width new-layer)))
	    (if (not script-fu-text-circle-debug?)
		(begin
		  (gimp-layer-translate new-layer
					(+ center-x
					   (* radius (cos angle))
					   (* rotate-radius
					      (cos (if (< 0 fill-angle-rad)
						       angle
						       (+ angle *pi*))))
					   (- (/ width 2)))
					(+ center-y
					   (* radius (sin angle))
					   (* rotate-radius
					      (sin (if (< 0 fill-angle-rad)
						       angle
						       (+ angle *pi*))))
					   (- (/ height 2))))
		  (gimp-rotate new-layer 1
			       ((if (< 0 fill-angle-rad) + -) angle rad-90))))))
      (set! index (+ index 1)))
    (gimp-layer-set-visible BG-layer 0)
    (if (not script-fu-text-circle-debug?)
	(begin
	  (set! merged-layer
		(car (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)))
	  (gimp-layer-set-name merged-layer
			       (if (< (length text) 16)
				   (wrap-string text)
				   "Text Circle"))))
    (gimp-layer-set-visible BG-layer 1)
    (gimp-image-undo-enable img)
    (gimp-image-clean-all img)
    (gimp-display-new img)
    (gimp-displays-flush)))

(script-fu-register
 "script-fu-text-circle"
 "<Toolbox>/Xtns/Script-Fu/Logos/Text Circle..."
 "Render the specified text along the perimeter of a circle"
 "Shuji Narazaki <narazaki@gimp.org>"
 "Shuji Narazaki"
 "1997-1998"
 ""
 SF-STRING "Text" "The GNU Image Manipulation Program Version 1.1 "
 SF-ADJUSTMENT "Radius" '(80 1 8000 1 1 0 1)
 SF-ADJUSTMENT "Start-angle[-180:180]" '(0 -180 180 1 1 0 1)
 SF-ADJUSTMENT "Fill-angle [-360:360]" '(360 -360 360 1 1 0 1)
 SF-ADJUSTMENT "Font Size (pixel)" '(18 1 1000 1 1 0 1)
 SF-TOGGLE "Antialias" TRUE
 SF-FONT "Font name" "-adobe-helvetica-bold-r-normal-*-30-*-*-*-p-*-*-*"
)

;; text-circle.scm ends here
