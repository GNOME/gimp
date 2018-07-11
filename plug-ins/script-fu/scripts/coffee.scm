; Chris Gutteridge (cjg@ecs.soton.ac.uk)
; At ECS Dept, University of Southampton, England.

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


(define (script-fu-coffee-stain inImage inLayer inNumber inDark)
  (let* (
        (theImage inImage)
        (theHeight (car (gimp-image-height theImage)))
        (theWidth (car (gimp-image-width theImage)))
        (theNumber inNumber)
        (theSize (min theWidth theHeight))
        (theStain 0)
        )

    (gimp-context-push)
    (gimp-context-set-defaults)

    (gimp-image-undo-group-start theImage)

    (while (> theNumber 0)
      (set! theNumber (- theNumber 1))
      (set! theStain (car (gimp-layer-new theImage theSize theSize
                                          RGBA-IMAGE _"Stain" 100
                                          (if (= inDark TRUE)
                                              LAYER-MODE-DARKEN-ONLY LAYER-MODE-NORMAL))))

      (gimp-image-insert-layer theImage theStain 0 0)
      (gimp-selection-all theImage)
      (gimp-drawable-edit-clear theStain)

      (let ((blobSize (/ (rand (- theSize 40)) (+ (rand 3) 1))))
        (gimp-image-select-ellipse theImage
				   CHANNEL-OP-REPLACE
				   (/ (- theSize blobSize) 2)
				   (/ (- theSize blobSize) 2)
				   blobSize blobSize)
      )

      (script-fu-distress-selection theImage theStain
                                    (- (* (+ (rand 15) 1) (+ (rand 15) 1)) 1)
                                    (/ theSize 25) 4 2 TRUE TRUE)

      (gimp-context-set-gradient "Coffee")

      (gimp-drawable-edit-gradient-fill theStain
					GRADIENT-SHAPEBURST-DIMPLED 0
					FALSE 0 0
					TRUE
					0 0 0 0)

      (gimp-layer-set-offsets theStain
                              (- (rand theWidth) (/ theSize 2))
                              (- (rand theHeight) (/ theSize 2)))
    )

    (gimp-selection-none theImage)

    (gimp-image-undo-group-end theImage)

    (gimp-displays-flush)

    (gimp-context-pop)
  )
)

; Register the function with GIMP:

(script-fu-register "script-fu-coffee-stain"
  _"_Coffee Stain..."
  _"Add realistic looking coffee stains to the image"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "25th April 1998"
  "RGB*"
  SF-IMAGE       "The image"   0
  SF-DRAWABLE    "The layer"   0
  SF-ADJUSTMENT _"Stains"      '(3 1 10 1 1 0 0)
  SF-TOGGLE     _"Darken only" TRUE
)

(script-fu-menu-register "script-fu-coffee-stain" "<Image>/Filters/Decor")
