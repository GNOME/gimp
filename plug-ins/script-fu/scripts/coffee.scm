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


(define (script-fu-coffee-stain inImage inLayers inNumber inDark inGradient)
  (let* (
        (theImage inImage)
        (theHeight (car (gimp-image-get-height theImage)))
        (theWidth (car (gimp-image-get-width theImage)))
        (theNumber inNumber)
        (theSize (min theWidth theHeight))
        (theStain 0)
        (theThreshold 0)
        (theSpread 0)
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
        (if (< blobSize 32) (set! blobSize 32))
        (gimp-image-select-ellipse theImage
				   CHANNEL-OP-REPLACE
				   (/ (- theSize blobSize) 2)
				   (/ (- theSize blobSize) 2)
				   blobSize blobSize)
      )

      ; Clamp spread value to 'plug-in-spread' limits.
      ; This plugin calls plug-in-spread indirectly via distress-selection.
      ; plug-in-spread is a compatability plug-in to gegl:noise-spread.
      ; gegl:noise-spread seems to have a limit of 512.
      ; Here we limit to 200, for undocumented reasons.
      (set! theSpread (/ theSize 25))
      (if (> theSpread 200) (set! theSpread 200))

      ; Threshold to distress-selection now allows [0, 1.0] including zero.
      ; Formerly, it allowed [1, 254] (strange, but documented.)
      ;
      ; Here we generate a random non-uniform weighted towards center.
      ; The distribution of the product (or sum) of two random numbers is non-uniform.

      ; rand is a compability function for the SIOD dialect, and basically calls random.
      ; random seems not standardized in Scheme, our version is MSRG in script-fu-compat.init.
      ; It yields random uniform integers, except 0.
      ;
      ; (+ (rand 15) 1) yields [2, 16] uniform.
      ; The original formula, a product of random numbers, yields [3, 255] non-uniform.
      ; To accomodate changes to distress-selection, we use the old formula, and divide by 255.
      ; When you instead just generate a single random float, stains are smaller.
      (set! theThreshold (/
                            (- (* (+ (rand 15) 1) (+ (rand 15) 1)) 1) ; original formula
                            255))

      ; !!! This call is not via the PDB so SF does not range check args.
      ; The script is in the interpreter state and doesn't require a PDB call.
      (script-fu-distress-selection theImage (vector theStain)
                                    theThreshold
                                    theSpread 4 2 TRUE TRUE)

      (gimp-context-set-gradient inGradient)

      ; only fill if there is a selection
      (if (> (car (gimp-selection-bounds theImage)) 0)
        (gimp-drawable-edit-gradient-fill theStain
            GRADIENT-SHAPEBURST-DIMPLED 0
            FALSE 1 0
            TRUE
            0 0 0 0)
      )

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

; Register as a filter

; !!! Enabled for any non-zero count of layers.
; Requires an image, to which we add layers.
; Argument inLayers is not used.

(script-fu-register-filter "script-fu-coffee-stain"
  _"_Stain..."
  _"Add layers of stain or blotch marks"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "25th April 1998"
  "RGB*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Number of stains to add"      '(3 1 10 1 2 0 0)
  SF-TOGGLE     _"Darken only" TRUE
  SF-GRADIENT   _"Gradient to color stains"    "Coffee"
)

(script-fu-menu-register "script-fu-coffee-stain" "<Image>/Filters/Decor")
