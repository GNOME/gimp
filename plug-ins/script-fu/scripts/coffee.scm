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


(define (script-fu-coffee-stain inImage inLayer inNumber inDark)

   (set! old-gradient (car (gimp-gradients-get-active)))
   (set! theImage inImage)
   (set! theHeight (car (gimp-image-height theImage)))
   (set! theWidth (car (gimp-image-width theImage)))
   (set! theNumber inNumber)
   (set! theSize (min theWidth theHeight) )

   (while (> theNumber 0)
       (set! theNumber (- theNumber 1))
       (set! theStain (car (gimp-layer-new theImage theSize theSize RGBA_IMAGE "Stain" 100
             (if (= inDark TRUE) DARKEN-ONLY NORMAL)          )))

  
                              
       (gimp-image-add-layer theImage theStain 0)
       (gimp-selection-all theImage)
       (gimp-edit-clear theStain)
       (let ((blobSize (/ (rand (- theSize 40)) (+ (rand 3) 1)  ) ) )
            (gimp-ellipse-select theImage
                 (/ (- theSize blobSize) 2)
                 (/ (- theSize blobSize) 2)
		 blobSize blobSize REPLACE TRUE 0 FALSE)
       )
       (script-fu-distress-selection theImage theStain (* (+ (rand 15) 1) (+ (rand 15) 1)) (/ theSize 25) 4 2 TRUE TRUE   )
       (gimp-gradients-set-active "Coffee")
       (gimp-blend theStain CUSTOM NORMAL SHAPEBURST-DIMPLED 100 0 REPEAT-NONE FALSE 0 0 0 0 0 0)
       (gimp-layer-set-offsets theStain (- (rand theWidth) (/ theSize 2)) (- (rand theHeight) (/ theSize 2)) theSize)
    )
   (gimp-selection-none theImage)
   (gimp-gradients-set-active old-gradient)
   (gimp-displays-flush)
)

; Register the function with the GIMP:

(script-fu-register
    "script-fu-coffee-stain"
    "<Image>/Script-Fu/Decor/Coffee Stain..."
    "Draws realistic looking coffee stains"
    "Chris Gutteridge"
    "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
    "25th April 1998"
    "RGB*"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-VALUE "Stains" "3"
    SF-TOGGLE "Darken Only? (better but only for images with alot of white)" TRUE
)
