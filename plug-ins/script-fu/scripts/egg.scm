; Aspirin and Yosh present: Fun with eggs
;

(define (script-fu-egg-catcher coolfile)
  (let* ((image (car (gimp-file-load 1 coolfile coolfile)))
	 (drawable (car (gimp-image-active-drawable image))))
    (plug-in-the-egg 1 image drawable)
    (gimp-display-new image)))

(script-fu-register "script-fu-egg-catcher" 
		    "<None>"
		    "Invoke The Egg!"
		    "Adam D. Moss & Manish Singh"
		    "Adam D. Moss & Manish Singh"
		    "1998/04/19"
		    ""
		    SF-VALUE "Cool File" "foo.xcf")
