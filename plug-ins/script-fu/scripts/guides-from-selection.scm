;; -*-scheme-*- 

(define (script-fu-guides-from-selection image
					 drawable) 
  (let* ((boundries (gimp-selection-bounds image))
	 ;; non-empty INT32 TRUE if there is a selection
	 (selection (car boundries)) 
	 (x1 (cadr boundries))
	 (y1 (caddr boundries))
	 (x2 (cadr (cddr boundries))) 
	 (y2 (caddr (cddr boundries))))
        
    ;; need to check for a selection or we get guides right at edges of the image
    (if (= selection TRUE) 
	(begin
	  (gimp-image-undo-group-start image) 

	  (gimp-image-add-vguide image x1)
	  (gimp-image-add-hguide image y1)
	  (gimp-image-add-vguide image x2)
	  (gimp-image-add-hguide image y2)

	  (gimp-image-undo-group-end image)       
	  (gimp-displays-flush)))))

(script-fu-register "script-fu-guides-from-selection" 
		    _"New Guides from _Selection"
		    _"Create four guides around the bounding box of the current selection"
		    "Alan Horkan"
		    "Alan Horkan, 2004.  Public Domain."
		    "2004-08-13"
		    ""
		    SF-IMAGE    "Image"    0 
		    SF-DRAWABLE "Drawable" 0)

(script-fu-menu-register "script-fu-guides-from-selection" 
			 "<Image>/Image/Guides")
