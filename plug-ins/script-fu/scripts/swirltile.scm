;
; Swirl-tile
;  produces a (hope-fully) seamlessly tiling swirling effect
;
;  Adrian Likins  <aklikins@eos.ncsu.edu>
;
;  http://www4.ncsu.edu/eos/users/a/aklikins/pub/gimp/
;


(define (script-fu-swirl-tile depth azimuth elevation blurRadius height width whirl-amount noise-level bg-color)
  (let* ((img (car (gimp-image-new width height RGB)))
	 (layer-one (car (gimp-layer-new img width height RGB "TEST" 100 NORMAL)))
	 (cx (/ width 2))
	 (cy (/ height 2))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-palette-set-background bg-color)
    (gimp-edit-fill img layer-one)
    (gimp-image-add-layer img layer-one 0)
    (plug-in-noisify 1 img layer-one FALSE noise-level noise-level noise-level 1.0)

    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)

    (gimp-channel-ops-offset img layer-one TRUE 0 cx cy)

    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch 1 img layer-one whirl-amount 0.0 1.0)

    (plug-in-gauss-rle 1 img layer-one blurRadius TRUE TRUE)

    (plug-in-bump-map 1 img layer-one layer-one azimuth elevation depth 0 0 0 0 FALSE FALSE 0)

    (gimp-display-new img)
    (gimp-image-enable-undo img)))

(script-fu-register "script-fu-swirl-tile"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/Swirl-Tile"
		    "Create an interesting swirled tile"
		    "Adrian Likins <aklikins@eos.ncsu.edu>"
		    "Adrian Likins"
		    "1997"
		    ""
		    SF-VALUE "Depth" "10"
		    SF-VALUE "Azimuth" "135"
		    SF-VALUE "Elevation" "45"
		    SF-VALUE "Blur Radius" "3"
		    SF-VALUE "Height" "256"
		    SF-VALUE "Width" "256"
		    SF-VALUE "Whirl Amount" "320"
		    SF-VALUE "Roughness" ".5"
		    SF-COLOR "Backgound Color" '(255 255 255))
