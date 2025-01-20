;
; distress selection
;
;
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

; Define the function:

(define (script-fu-distress-selection inImage
                                      inDrawables
                                      inThreshold
                                      inSpread
                                      inGranu
                                      inSmooth
                                      inSmoothH
                                      inSmoothV)

  (let (
       (theImage inImage)
       (inDrawable (vector-ref inDrawables 0))
       (theWidth (car (gimp-image-get-width inImage)))
       (theHeight (car (gimp-image-get-height inImage)))
       (theLayer 0)
       (theMode (car (gimp-image-get-base-type inImage)))
       (prevLayers (car (gimp-image-get-selected-layers inImage)))
       (horizontalRadius (* 0.32 inSmooth))
       (verticalRadius (* 0.32 inSmooth))
       )

    (if (= inSmoothH FALSE)
        (set! horizontalRadius 0)
    )
    (if (= inSmoothV FALSE)
        (set! verticalRadius 0)
    )

    (gimp-context-push)
    (gimp-context-set-defaults)
    (gimp-image-undo-group-start theImage)

    (if (= theMode GRAY)
      (set! theMode GRAYA-IMAGE)
      (set! theMode RGBA-IMAGE)
    )
    (set! theLayer (car (gimp-layer-new theImage
                                        "Distress Scratch Layer"
                                        theWidth
                                        theHeight
                                        theMode
                                        100
                                        LAYER-MODE-NORMAL)))

    (gimp-image-insert-layer theImage theLayer 0 0)

    (if (= FALSE (car (gimp-selection-is-empty theImage)))
        (gimp-drawable-edit-fill theLayer FILL-BACKGROUND)
    )

    (gimp-selection-invert theImage)

    (if (= FALSE (car (gimp-selection-is-empty theImage)))
        (gimp-drawable-edit-clear theLayer)
    )

    (gimp-selection-invert theImage)
    (gimp-selection-none inImage)

    (gimp-layer-scale theLayer
                      (/ theWidth inGranu)
                      (/ theHeight inGranu)
                      TRUE)

    (gimp-drawable-merge-new-filter theLayer "gegl:noise-spread" 0 LAYER-MODE-REPLACE 1.0 "amount-x" inSpread "amount-y" inSpread "seed" (msrg-rand))

    (gimp-drawable-merge-new-filter theLayer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" horizontalRadius "std-dev-y" verticalRadius "filter" "auto")
    (gimp-layer-scale theLayer theWidth theHeight TRUE)
    (gimp-drawable-merge-new-filter theLayer "gimp:threshold-alpha" 0 LAYER-MODE-REPLACE 1.0 "value" inThreshold)
    (gimp-drawable-merge-new-filter theLayer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" 0.32 "std-dev-y" 0.32 "filter" "auto")
    (gimp-image-select-item inImage CHANNEL-OP-REPLACE theLayer)
    (gimp-image-remove-layer theImage theLayer)
    (if (and (= (car (gimp-item-id-is-channel inDrawable)) TRUE)
             (= (car (gimp-item-id-is-layer-mask inDrawable)) FALSE))
      (gimp-image-set-selected-channels theImage (make-vector 1 inDrawable))
      )
    (gimp-image-undo-group-end theImage)

    (gimp-image-set-selected-layers theImage prevLayers)

    (gimp-displays-flush)
    (gimp-context-pop)
  )
)


(script-fu-register-filter "script-fu-distress-selection"
  _"_Distort..."
  _"Distress the selection"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "23rd April 1998"
  "RGB*,GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"_Threshold"              '(0.5 0 1 0.1 0.2 1 0)
  SF-ADJUSTMENT _"_Spread"                 '(8 0 512 1 10 0 1)
  SF-ADJUSTMENT _"_Granularity (1 is low)" '(4 1 25 1 10 0 1)
  SF-ADJUSTMENT _"S_mooth"                 '(2 1 150 1 10 0 1)
  SF-TOGGLE     _"Smooth hor_izontally"    TRUE
  SF-TOGGLE     _"Smooth _vertically"      TRUE
)

(script-fu-menu-register "script-fu-distress-selection"
                         "<Image>/Select/[Modify]")
