;; font-select
;; Spencer Kimball

(define (max-font-width font-list font-size)
  (let* ((list font-list)
	 (width 0)
	 (maxwidth 0)
	 (font "")
	 (extents '()))
    (while list
	   (set! font (car list))
	   (set! list (cdr list))
	   (set! extents (gimp-text-get-extents-fontname font
							 font-size PIXELS
							 font))
	   (set! width (nth 0 extents))
	   (if (> width maxwidth) (set! maxwidth width)))
    maxwidth))


(define (max-font-height font-list font-size)
  (let* ((list font-list)
	 (height 0)
	 (maxheight 0)
	 (font "")
	 (extents '()))
    (while list
	   (set! font (car list))
	   (set! list (cdr list))
	   (set! extents (gimp-text-get-extents-fontname font
							 font-size PIXELS
							 font))
	   (set! height (nth 1 extents))
	   (if (> height maxheight) (set! maxheight height)))
    maxheight))


(define (script-fu-font-map font-filter font-size border)
  (let* ((font "")
	 (count 0)
	 (text-fs 0)
         (font-list (cadr (gimp-fonts-get-list font-filter)))
	 (num-fonts (length font-list))
	 (maxheight (max-font-height font-list font-size))
	 (maxwidth (max-font-width font-list font-size))
	 (width (+ maxwidth (* 2 border)))
	 (height (+ (* maxheight num-fonts) (* 2 border)))
	 (img (car (gimp-image-new width height GRAY)))
	 (drawable (car (gimp-layer-new img width height GRAY_IMAGE
					"Font List" 100 NORMAL))))
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img drawable 0)
    (gimp-edit-fill drawable BG-IMAGE-FILL)

    (while font-list
	   (set! font (car font-list))
	   (set! font-list (cdr font-list))
	   (set! text-fs (car (gimp-text-fontname img drawable
						  border
						  (+ border (* count maxheight))
						  font
						  0 TRUE font-size PIXELS
						  font)))
	   (set! count (+ count 1))
	   (gimp-floating-sel-anchor text-fs))

    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-font-map"
		    _"<Toolbox>/Xtns/Script-Fu/Utils/_Font Map..."
		    "Generate a listing of fonts matching a filter"
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-STRING     _"Filter (regexp)"    "Sans"
		    SF-ADJUSTMENT _"Font Size (pixels)" '(32 2 1000 1 10 0 1)
		    SF-ADJUSTMENT _"Border"             '(10 0  150 1 10 0 1)
)
