;
; fuzzy-border
;
; Do a cool fade to a given colour at the border of an image (optional shadow)
; Will make image RGB if it isn't already.
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

(define (script-fu-fuzzy-border	inImage 
				inLayer 
				inColor 
				inSize 
				inBlur 
				inGranu 
				inShadow 
				inShadWeight 
				inCopy
                                inFlatten
	)
         
	(gimp-selection-all inImage)
	(set! theImage (if (= inCopy TRUE)
		       (car (gimp-channel-ops-duplicate inImage))
                       inImage)
        )
	(if 	(> 	(car (gimp-drawable-type inLayer)) 
               		1
            	)
		(gimp-convert-rgb theImage)
	)
        (set! theWidth (car (gimp-image-width inImage))) 
	(set! theHeight (car (gimp-image-height inImage))) 

	(set! theLayer (car (gimp-layer-new 	theImage 
						theWidth 
						theHeight 
						RGBA_IMAGE 
						"layer 1" 
						100 
						NORMAL
	) ) )

	(gimp-image-add-layer theImage theLayer 0)


        (gimp-edit-clear theImage theLayer)
	(chris-color-edge theImage theLayer inColor inSize)

	(gimp-layer-scale 	theLayer 
				(/ theWidth inGranu) 
				(/ theHeight inGranu) 
				TRUE
	)

	(plug-in-spread 	TRUE 
				theImage 
				theLayer 	
				(/ inSize inGranu) 
				(/ inSize inGranu)
	)
	(chris-color-edge theImage theLayer inColor 1)
	(gimp-layer-scale theLayer theWidth theHeight TRUE)
	
	(gimp-selection-layer-alpha theImage theLayer)
	(gimp-selection-invert theImage)
	(gimp-edit-clear theImage theLayer)
	(gimp-selection-invert theImage)
	(gimp-edit-clear theImage theLayer)
	(gimp-palette-set-background inColor)
	(gimp-edit-fill theImage theLayer)
        (gimp-selection-none inImage)
	(chris-color-edge theImage theLayer inColor 1)
        
	(if 	(= inBlur TRUE) 
		(plug-in-gauss-rle TRUE theImage theLayer inSize TRUE TRUE)
		()
	)
	(if (= inShadow FALSE) 
	    ()
            (begin	
                (gimp-selection-none inImage)
	        (gimp-image-add-layer 	theImage 
	 				(car (gimp-layer-copy 	theLayer 
								FALSE
					)) 
					0
	        )
		(gimp-layer-scale 	theLayer (- theWidth inSize) (- theHeight inSize) TRUE)
	        (gimp-desaturate theImage theLayer)
		(gimp-brightness-contrast theImage theLayer 127 127)
        	(gimp-invert theImage theLayer)
		(gimp-layer-resize 	theLayer 
		   			theWidth 
		    			theHeight 
		    			(/ inSize 2) 
		    			(/ inSize 2)
		)
		(plug-in-gauss-rle 	TRUE 
		    			theImage 
		    			theLayer 
		    			(/ inSize 2) 
		    			TRUE 
		    			TRUE
		)
		(gimp-layer-set-opacity theLayer inShadWeight)
            )
	)
	(if 	(= inFlatten TRUE) 
 		(gimp-image-flatten theImage)
		()
	)
	(if 	(= inCopy TRUE) 
		(begin 	(gimp-image-clean-all theImage) 
			(gimp-display-new theImage)
		)
		()
	)
	(gimp-displays-flush)
)

(define (chris-color-edge inImage inLayer inColor inSize)
          (gimp-selection-all inImage)
          (gimp-selection-shrink inImage inSize)
          (gimp-selection-invert inImage)
	  (gimp-palette-set-background inColor)
	(gimp-edit-fill theImage theLayer)
          (gimp-selection-none inImage)
)

; Register the function with the GIMP:

(script-fu-register
    "script-fu-fuzzy-border"
    "<Image>/Script-Fu/Decor/Fuzzy Border"
    "foo"
    "Chris Gutteridge"
    "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
    "3rd April 1998"
    "RGB RGBA GRAY GRAYA"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-COLOR "Color"      '(255 255 255)
    SF-VALUE "Border Size" "16"
    SF-TOGGLE "Blur Border?" TRUE
    SF-VALUE "Granularity (1 is low)" "4"
    SF-TOGGLE "Add Shadow?" FALSE
    SF-ADJUSTMENT "Shadow-Weight (%)" '(100 0 100 1 10 0 0)
    SF-TOGGLE "Work on Copy?" TRUE
    SF-TOGGLE "Flatten Layers?" TRUE
)
