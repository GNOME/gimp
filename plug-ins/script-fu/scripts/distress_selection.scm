;
; distress selection
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

; Define the function:

(define (script-fu-distress-selection	inImage
					inLayer
					inThreshold
					inSpread
					inGranu
					inSmooth
                                        inSmoothH
                                        inSmoothV
	)

	(set! theImage inImage)
        (set! theWidth (car (gimp-image-width inImage)))
	(set! theHeight (car (gimp-image-height inImage)))

	(gimp-image-undo-disable theImage)

	(set! theLayer (car (gimp-layer-new 	theImage
						theWidth
						theHeight
						RGBA_IMAGE
						"Distress Scratch Layer"
						100
						NORMAL
	) ) )

	(gimp-image-add-layer theImage theLayer 0)
        (if (= TRUE (car (gimp-selection-is-empty theImage)))
            ()
	    (gimp-edit-fill theLayer BG-IMAGE-FILL)
        )
	(gimp-selection-invert theImage)
        (if (= TRUE (car (gimp-selection-is-empty theImage)))
            ()
            (gimp-edit-clear theLayer)
        )
	(gimp-selection-invert theImage)
        (gimp-selection-none inImage)

	(gimp-layer-scale 	theLayer
				(/ theWidth inGranu)
				(/ theHeight inGranu)
				TRUE
	)

	(plug-in-spread 	TRUE
				theImage
				theLayer
				inSpread
				inSpread
	)
	(plug-in-gauss-iir TRUE theImage theLayer inSmooth inSmoothH inSmoothV)
;	(plug-in-gauss-iir TRUE theImage theLayer 2 TRUE TRUE)
	(gimp-layer-scale theLayer theWidth theHeight TRUE)
	(plug-in-threshold-alpha TRUE theImage theLayer inThreshold)
        (plug-in-gauss-iir TRUE theImage theLayer 1 TRUE TRUE)
	(gimp-selection-layer-alpha theLayer)
	(gimp-image-remove-layer theImage theLayer)
;	(gimp-layer-delete theLayer)
	(gimp-image-undo-enable theImage)
	(gimp-displays-flush)
)


; Register the function with the GIMP:

(script-fu-register
    "script-fu-distress-selection"
    "<Image>/Script-Fu/Selection/Distress Selection..."
    "No description"
    "Chris Gutteridge"
    "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
    "23rd April 1998"
    "RGB*"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-VALUE "Threshold (bigger 1<-->255 smaller)" "127"
    SF-VALUE "Spread" "8"
    SF-VALUE "Granularity (1 is low)" "4"
    SF-VALUE "Smooth" "2"
    SF-TOGGLE "Smooth Horizontally?" TRUE
    SF-TOGGLE "Smooth Vertically?" TRUE
)
