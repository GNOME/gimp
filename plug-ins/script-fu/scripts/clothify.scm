; CLOTHIFY version 1.02
; Gives the current layer in the indicated image a cloth-like texture.
; Process invented by Zach Beane (Xath@irc.gimp.net)
;
; Tim Newsome <drz@froody.bloke.com> 4/11/97

(define (script-fu-clothify timg tdrawable bx by azimuth elevation depth)
	(let* (
		(width (car (gimp-drawable-width tdrawable)))
		(height (car (gimp-drawable-height tdrawable)))
		(img (car (gimp-image-new width height RGB)))
;		(layer-two (car (gimp-layer-new img width height RGB "Y Dots" 100 MULTIPLY)))
		(layer-one (car (gimp-layer-new img width height RGB "X Dots" 100 NORMAL))))

	(gimp-image-undo-disable img)
	(gimp-edit-fill layer-one)
;	(gimp-edit-fill img layer-two)
	(gimp-image-add-layer img layer-one 0)

	(plug-in-noisify 1 img layer-one FALSE 0.7 0.7 0.7 0.7)

	(set! layer-two (car (gimp-layer-copy layer-one 0)))
	(gimp-layer-set-mode layer-two MULTIPLY)
	(gimp-image-add-layer img layer-two 0)

	(plug-in-gauss-rle 1 img layer-one bx TRUE FALSE)
	(plug-in-gauss-rle 1 img layer-two by FALSE TRUE)
	(gimp-image-flatten img)
	(set! bump-layer (car (gimp-image-get-active-layer img)))

	(plug-in-c-astretch 1 img bump-layer)
	(plug-in-noisify 1 img bump-layer FALSE 0.2 0.2 0.2 0.2)

	(plug-in-bump-map 1 img tdrawable bump-layer azimuth elevation depth 0 0 0 0 FALSE FALSE 0)
	(gimp-image-delete img)
	(gimp-displays-flush)
))


(script-fu-register "script-fu-clothify"
		    "<Image>/Script-Fu/Alchemy/Clothify..."
		    "Render the specified text along the perimeter of a circle"
		    "Tim Newsome <drz@froody.bloke.com>"
		    "Tim Newsome"
		    "4/11/97"
		    "RGB* GRAY*"
		    SF-IMAGE "Input Image" 0
		    SF-DRAWABLE "Input Drawable" 0
		    SF-VALUE "X Blur" "9"
		    SF-VALUE "Y Blur" "9"
		    SF-VALUE "Azimuth" "135"
		    SF-VALUE "Elevation" "45"
		    SF-VALUE "Depth" "3")
