; "Rippling Image" animation generator (ripply-anim.scm)
; Adam D. Moss (adam@foxbox.org)
; 97/05/18
;
; Designed to be used in conjunction with a plugin capable
; of saving animations (i.e. the GIF plugin).
;

(define (copy-layer-ripple dest-image dest-drawable source-image source-drawable)
  (gimp-selection-all dest-image)
  (gimp-edit-clear dest-drawable)
  (gimp-selection-none dest-image)
  (gimp-selection-all source-image)
  (gimp-edit-copy source-drawable)
  (gimp-selection-none source-image)
      (let ((floating-sel (car (gimp-edit-paste dest-drawable FALSE))))
	(gimp-floating-sel-anchor floating-sel)))

(define (script-fu-ripply-anim img drawable displacement num-frames)
  (let* ((width (car (gimp-drawable-width drawable)))
	 (height (car (gimp-drawable-height drawable)))
	 (ripple-image (car (gimp-image-new width height GRAY)))
	 (ripple-layer (car (gimp-layer-new ripple-image width height GRAY_IMAGE "Ripple Texture" 100 NORMAL))))

 ; this script generates its own displacement map

    (gimp-image-disable-undo ripple-image)
    (gimp-palette-set-background '(127 127 127) )
    (gimp-image-add-layer ripple-image ripple-layer 0)
    (gimp-edit-fill ripple-layer)
    (plug-in-noisify 1 ripple-image ripple-layer FALSE 1.0 1.0 1.0 0.0)
    ; tile noise
    (set! rippletiled-ret (plug-in-tile 1 ripple-image ripple-layer (* width 3) (* height 3) TRUE))
    (gimp-image-enable-undo ripple-image)
    (gimp-image-delete ripple-image)

    (set! rippletiled-image (car rippletiled-ret))
    (set! rippletiled-layer (cadr rippletiled-ret))
    (gimp-image-disable-undo rippletiled-image)

    ; process tiled noise into usable displacement map
    (plug-in-gauss-iir 1 rippletiled-image rippletiled-layer 35 TRUE TRUE)
    (gimp-equalize rippletiled-layer TRUE)
    (plug-in-gauss-rle 1 rippletiled-image rippletiled-layer 5 TRUE TRUE)
    (gimp-equalize rippletiled-layer TRUE)

    ; displacement map is now in rippletiled-layer of rippletiled-image

    ; loop through the desired frames

    (set! remaining-frames num-frames)
    (set! xpos (/ width 2))
    (set! ypos (/ height 2))
    (set! xoffset (/ width num-frames))
    (set! yoffset (/ height num-frames))

    (let* ((out-imagestack (car (gimp-image-new width height RGB))))

      (gimp-image-disable-undo out-imagestack)
      
      (while (> remaining-frames 0)
	     (set! dup-image (car (gimp-channel-ops-duplicate rippletiled-image)))
	     (gimp-image-disable-undo dup-image)
	     (gimp-crop dup-image width height xpos ypos)
	     
	     (set! layer-name (string-append "Frame "
			(number->string (- num-frames remaining-frames) 10)))
	     (set! this-layer (car (gimp-layer-new out-imagestack
						   width height RGB
						   layer-name 100 NORMAL)))
	     (gimp-image-add-layer out-imagestack this-layer 0)
	     
	     (copy-layer-ripple out-imagestack this-layer img drawable)
	     
	     (set! dup-layer (car (gimp-image-get-active-layer dup-image)))
	     (plug-in-displace 1 out-imagestack this-layer
			       displacement displacement
			       TRUE TRUE dup-layer dup-layer 2)
	     
	     (gimp-image-enable-undo dup-image)
	     (gimp-image-delete dup-image)
	     
	     (set! remaining-frames (- remaining-frames 1))
	     (set! xpos (+ xoffset xpos))
	     (set! ypos (+ yoffset ypos)))
      
      (gimp-image-enable-undo rippletiled-image)
      (gimp-image-delete rippletiled-image)
      (gimp-image-enable-undo out-imagestack)
      (gimp-display-new out-imagestack))))

(script-fu-register "script-fu-ripply-anim"
		    "<Image>/Script-Fu/Animators/Rippling"
		    "Ripple any image by creating animation frames as layers"
		    "Adam D. Moss (adam@foxbox.org)"
		    "Adam D. Moss"
		    "1997"
		    "RGB*, GRAY*"
		    SF-IMAGE "Image to Animage" 0
		    SF-DRAWABLE "Drawable to Animate" 0
		    SF-ADJUSTMENT "Rippling Strentgh" '(3 0 256 1 10 3 0)
		    SF-ADJUSTMENT "Number of Frames" '(15 0 256 1 10 0 1))
