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
		    SF-ADJUSTMENT "Depth" '(10 0 64 1 1 0 0)
		    SF-ADJUSTMENT "Azimuth" '(135 0 360 1 10 0 0)
		    SF-ADJUSTMENT "Elevation" '(45 0 90 1 10 0 0)
		    SF-ADJUSTMENT "Blur Radius" '(3 0 128 1 10 0 0)
		    SF-ADJUSTMENT "Height" '(256 0 1024 1 10 0 1)
		    SF-ADJUSTMENT "Width" '(256 0 1024 1 10 0 1)
		    SF-ADJUSTMENT "Whirl Amount" '(320 0 360 1 10 0 0)
		    SF-ADJUSTMENT "Roughness" '(.5 0 1 .1 .01 2 1)
		    SF-COLOR "Backgound Color" '(255 255 255))
