;
;
;
; Chris Gutteridge (cjg@ecs.soton.ac.uk)
; At ECS Dept, University of Southampton, England.

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


(define (script-fu-render-map inSize inGrain inGrad inWiden)

	(set! old-gradient (car (gimp-gradients-get-active)))
	(set! old-fg (car (gimp-palette-get-foreground)))
	(set! old-bg (car (gimp-palette-get-background)))

        (set! theWidth inSize)
	(set! theHeight inSize)
        (set! theImage (car(gimp-image-new theWidth theHeight RGB)))

        (gimp-selection-all theImage)

        (set! theLayer (car (gimp-layer-new theImage theWidth theHeight RGBA_IMAGE "I've got more rubber ducks than you!" 100 NORMAL)))
        (gimp-image-add-layer theImage theLayer 0)
        (plug-in-solid-noise TRUE theImage theLayer 1 0 (rand 65536) inGrain inGrain inGrain)

	(if (= inWiden TRUE) (begin
        	(set! thinLayer (car (gimp-layer-new theImage theWidth theHeight RGBA_IMAGE "Camo Thin Layer" 100 NORMAL)))
        	(gimp-image-add-layer theImage thinLayer 0)
		(let 	((theBigGrain (min 15 (* 2 inGrain))))
             		(plug-in-solid-noise TRUE theImage thinLayer 1 0 (rand 65536) theBigGrain theBigGrain theBigGrain)
		)
		(gimp-palette-set-background '(255 255 255))
		(gimp-palette-set-foreground '(0 0 0))
		(let 	((theMask (car(gimp-layer-create-mask thinLayer 0))))
			(gimp-image-add-layer-mask theImage thinLayer theMask)
			(gimp-blend theMask FG-BG-RGB NORMAL LINEAR 100
				0 REPEAT-TRIANGULAR FALSE 0 0 0 0 0 (/ theHeight 2) )
		)
		(set! theLayer (car(gimp-image-flatten theImage)))
	))
	
	(gimp-selection-none theImage)
	(gimp-gradients-set-active inGrad)
	(plug-in-gradmap TRUE theImage theLayer)
        (gimp-gradients-set-active old-gradient)
        (gimp-palette-set-background old-bg)
        (gimp-palette-set-foreground old-fg)
        (gimp-display-new theImage)
)



; Register the function with the GIMP:

(script-fu-register
 "script-fu-render-map"
 "<Toolbox>/Xtns/Script-Fu/Patterns/Render Map..."
 "foo"
 "Chris Gutteridge: cjg@ecs.soton.ac.uk"
 "28th April 1998"
 "Chris Gutteridge / ECS @ University of Southampton, England"
 ""
 SF-ADJUSTMENT "Image Size" '(256 0 2048 1 10 0 0)
 SF-ADJUSTMENT "Granularity (0 - 15)" '(4 0 15 1 1 0 0)
 SF-GRADIENT "Gradient" "Land_and_Sea"
 SF-TOGGLE "TRUE = Detail in middle, FALSE = tile" FALSE
)

