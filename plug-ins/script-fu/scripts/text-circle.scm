;; text-circle.scm -- a script for The GIMP 1.0
;; Author: Shuji Narazaki <narazaki@InetQ.or.jp>
;; Time-stamp: <1998/04/30 22:00:40 narazaki@InetQ.or.jp>
;; Version 2.4
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

(if (not (symbol-bound? 'script-fu-text-circle-text (the-environment)))
    (define script-fu-text-circle-text
      "\"The GNU Image Manipulation Program Version 1.0 \""))
(if (not (symbol-bound? 'script-fu-text-circle-radius (the-environment)))
    (define script-fu-text-circle-radius 80))
(if (not (symbol-bound? 'script-fu-text-circle-start-angle (the-environment)))
    (define script-fu-text-circle-start-angle 0))
(if (not (symbol-bound? 'script-fu-text-circle-fill-angle (the-environment)))
    (define script-fu-text-circle-fill-angle 360))
(if (not (symbol-bound? 'script-fu-text-circle-font-size (the-environment)))
    (define script-fu-text-circle-font-size 18))
(if (not (symbol-bound? 'script-fu-text-circle-antialias (the-environment)))
    (define script-fu-text-circle-antialias TRUE))
(if (not (symbol-bound? 'script-fu-text-circle-extra-pole (the-environment)))
    (define script-fu-text-circle-extra-pole TRUE))
(if (not (symbol-bound? 'script-fu-text-circle-font-foundry (the-environment)))
    (define script-fu-text-circle-font-foundry "\"*\""))
(if (not (symbol-bound? 'script-fu-text-circle-font-family (the-environment)))
    (define script-fu-text-circle-font-family "\"helvetica\""))
(if (not (symbol-bound? 'script-fu-text-circle-font-weight (the-environment)))
    (define script-fu-text-circle-font-weight "\"*\""))
(if (not (symbol-bound? 'script-fu-text-circle-font-slant (the-environment)))
    (define script-fu-text-circle-font-slant "\"r\""))
(if (not (symbol-bound? 'script-fu-text-circle-font-width (the-environment)))
    (define script-fu-text-circle-font-width "\"*\""))
(if (not (symbol-bound? 'script-fu-text-circle-font-spacing (the-environment)))
    (define script-fu-text-circle-font-spacing "\"*\""))
(if (not (symbol-bound? 'script-fu-text-circle-debug? (the-environment)))
    (define script-fu-text-circle-debug? #f))

(define (script-fu-text-circle text radius start-angle fill-angle
			       font-size antialias
			       foundry family weight slant width spacing)
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
	 (font-infos (gimp-text-get-extents fixed-pole font-size PIXELS
					    "*" family "*" slant "*" "*"))
	 (desc (nth 3 font-infos))
	 (extra 0)			; extra is calculated from real layer
	 (angle-list #f)
	 (letter "")
	 (new-layer #f)
	 (index 0))
    (gimp-image-disable-undo img)
    (gimp-image-add-layer img BG-layer 0)
    (gimp-edit-fill BG-layer)
    ;; change units
    (set! start-angle-rad (* (/ (modulo start-angle 360) 360) 2 *pi*))
    (set! fill-angle-rad (* (/ fill-angle 360) 2 *pi*))
    (set! radian-step (/ fill-angle-rad char-num))
    ;; set extra
    (if (eq? extra-pole TRUE)
	(let ((temp-pole-layer (car (gimp-text img -1 0 0
					       fixed-pole0
					       1 antialias
					       font-size PIXELS
					       "*" family "*" slant "*" "*"))))
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
	(set! temp-layer (car (gimp-text img -1 0 0
					 temp-str
					 1 antialias
					 font-size PIXELS
					 "*" family "*" slant "*" "*")))
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
	  (let* ((new-layer (car (gimp-text img -1 0 0
					    (if (eq? extra-pole TRUE)
						(string-append letter fixed-pole)
						letter)
					    1 antialias
					    font-size PIXELS
					    "*" family "*" slant "*" "*")))
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
    (gimp-image-enable-undo img)
    (gimp-image-clean-all img)
    (gimp-display-new img)
    (set! script-fu-text-circle-text (wrap-string text))
    (set! script-fu-text-circle-radius radius)
    (set! script-fu-text-circle-start-angle start-angle)
    (set! script-fu-text-circle-fill-angle fill-angle)
    (set! script-fu-text-circle-font-size font-size)
    (set! script-fu-text-circle-antialias antialias)
    (set! script-fu-text-circle-extra-pole extra-pole)
    (set! script-fu-text-circle-font-foundry (wrap-string foundry))
    (set! script-fu-text-circle-font-family (wrap-string family))
    (set! script-fu-text-circle-font-weight (wrap-string weight))
    (set! script-fu-text-circle-font-slant (wrap-string slant))
    (set! script-fu-text-circle-font-width (wrap-string width))
    (set! script-fu-text-circle-font-spacing (wrap-string spacing))
    (gimp-displays-flush)))

(script-fu-register
 "script-fu-text-circle"
 "<Toolbox>/Xtns/Script-Fu/Logos/Text Circle"
 "Render the specified text along the perimeter of a circle"
 "Shuji Narazaki <narazaki@InetQ.or.jp>"
 "Shuji Narazaki"
 "1997-1998"
 ""
 SF-VALUE "Text" script-fu-text-circle-text
 SF-VALUE "Radius" (number->string script-fu-text-circle-radius)
 SF-VALUE "Start-angle[-180:180]" (number->string script-fu-text-circle-start-angle)
 SF-VALUE "Fill-angle [-360:360]" (number->string script-fu-text-circle-fill-angle)
 SF-VALUE "Font Size (pixel)" (number->string script-fu-text-circle-font-size)
 SF-TOGGLE "Antialias" script-fu-text-circle-antialias
 SF-VALUE "Font Foundry" script-fu-text-circle-font-foundry
 SF-VALUE " - Family" script-fu-text-circle-font-family
 SF-VALUE " - Weight" script-fu-text-circle-font-weight
 SF-VALUE " - Slant" script-fu-text-circle-font-slant
 SF-VALUE " - Width" script-fu-text-circle-font-width
 SF-VALUE " - Spacing" script-fu-text-circle-font-spacing
)

;; text-circle.scm ends here
