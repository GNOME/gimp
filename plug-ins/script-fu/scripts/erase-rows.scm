(define (script-fu-erase-rows img drawable orientation which)
  (let* ((width (car (gimp-drawable-width drawable)))
	 (height (car (gimp-drawable-height drawable))))
    (gimp-image-undo-disable img)
    (letrec ((loop (lambda (i max)
		     (if (< i max)
			 (begin
			   (if (eq? orientation 'rows)
			       (gimp-rect-select img 0 i width 1 REPLACE FALSE 0)
			       (gimp-rect-select img i 0 1 height REPLACE FALSE 0))
			   (gimp-edit-fill drawable)
			   (loop (+ i 2) max))))))
      (loop (if (eq? which 'even)
		0
		1)
	    (if (eq? orientation 'rows)
		height
		width)))
    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-erase-rows"
		    "<Image>/Script-Fu/Alchemy/Erase every other row"
		    "Erase every other row/column with the background color"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    "RGB*, GRAY*, INDEXED*"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Rows/cols" "'rows"
		    SF-VALUE "Even/odd" "'even")
