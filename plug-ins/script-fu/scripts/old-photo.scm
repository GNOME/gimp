;
; old-photo
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

(define (script-fu-old-photo inImage inLayer inDefocus inBorder inSepia inMottle inCopy)
        (gimp-selection-all inImage)
        (set! theImage (if (= inCopy TRUE)
                       (car (gimp-channel-ops-duplicate inImage))
                       inImage)
        )

	(set! theLayer (car(gimp-image-flatten theImage)))
        (if (= inDefocus TRUE)
            (plug-in-gauss-rle TRUE theImage theLayer 1.5 TRUE TRUE)
            ()
        )
        (if (= inBorder TRUE)
            (script-fu-fuzzy-border theImage inLayer '(255 255 255)
				20 TRUE 8 FALSE 100 FALSE TRUE )
            ()
        )
	(set! theLayer (car(gimp-image-flatten theImage)))

	(if (= inSepia TRUE)
            (begin (gimp-desaturate theLayer)
	           (gimp-brightness-contrast theLayer -20 -40)
	           (gimp-color-balance theLayer 0 TRUE 30 0 -30)
            )
            ()
        )
        (set! theWidth (car (gimp-image-width theImage)))
        (set! theHeight (car (gimp-image-height theImage)))
	(if (= inMottle TRUE)
            (begin (set! mLayer (car (gimp-layer-new theImage theWidth theHeight RGBA_IMAGE "Mottle" 100 DARKEN-ONLY)))
                              
                   (gimp-image-add-layer theImage mLayer 0)
                   (gimp-selection-all theImage)
                   (gimp-edit-clear mLayer)
                   (gimp-selection-none theImage)
                   (plug-in-noisify TRUE theImage mLayer TRUE 0 0 0 0.5)
                   (plug-in-gauss-rle TRUE theImage mLayer 5 TRUE TRUE)
	           (set! theLayer (car(gimp-image-flatten theImage)))
            )
            ()
        )



        (if     (= inCopy TRUE)
                (begin  (gimp-image-clean-all theImage)
                        (gimp-display-new theImage)
                )
                ()
        )
        (gimp-selection-none inImage)
	(gimp-displays-flush theImage)
)

; Register the function with the GIMP:

(script-fu-register
    "script-fu-old-photo"
    _"<Image>/Script-Fu/Decor/Old Photo..."
    "Makes the image look like an old photo"
    "Chris Gutteridge"
    "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
    "16th April 1998"
    "RGB* GRAY*"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-TOGGLE _"Defocus" TRUE
    SF-TOGGLE _"Border" TRUE
    SF-TOGGLE _"Sepia" TRUE
    SF-TOGGLE _"Mottle" FALSE
    SF-TOGGLE _"Work on Copy" TRUE
)
