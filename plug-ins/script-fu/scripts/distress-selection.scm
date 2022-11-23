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
                                      inDrawable
                                      inThreshold
                                      inSpread
                                      inGranu
                                      inSmooth
                                      inSmoothH
                                      inSmoothV)

  (let (
       (theImage inImage)
       (theWidth (car (ligma-image-get-width inImage)))
       (theHeight (car (ligma-image-get-height inImage)))
       (theLayer 0)
       (theMode (car (ligma-image-get-base-type inImage)))
       (prevLayers (ligma-image-get-selected-layers inImage))
       )

    (ligma-context-push)
    (ligma-context-set-defaults)
    (ligma-image-undo-group-start theImage)

    (if (= theMode GRAY)
      (set! theMode GRAYA-IMAGE)
      (set! theMode RGBA-IMAGE)
    )
    (set! theLayer (car (ligma-layer-new theImage
                                        theWidth
                                        theHeight
                                        theMode
                                        "Distress Scratch Layer"
                                        100
                                        LAYER-MODE-NORMAL)))

    (ligma-image-insert-layer theImage theLayer 0 0)

    (if (= FALSE (car (ligma-selection-is-empty theImage)))
        (ligma-drawable-edit-fill theLayer FILL-BACKGROUND)
    )

    (ligma-selection-invert theImage)

    (if (= FALSE (car (ligma-selection-is-empty theImage)))
        (ligma-drawable-edit-clear theLayer)
    )

    (ligma-selection-invert theImage)
    (ligma-selection-none inImage)

    (ligma-layer-scale theLayer
                      (/ theWidth inGranu)
                      (/ theHeight inGranu)
                      TRUE)

    (plug-in-spread RUN-NONINTERACTIVE
                    theImage
                    theLayer
                    inSpread
                    inSpread)

    (plug-in-gauss-iir RUN-NONINTERACTIVE
           theImage theLayer inSmooth inSmoothH inSmoothV)
    (ligma-layer-scale theLayer theWidth theHeight TRUE)
    (plug-in-threshold-alpha RUN-NONINTERACTIVE theImage theLayer inThreshold)
    (plug-in-gauss-iir RUN-NONINTERACTIVE theImage theLayer 1 TRUE TRUE)
    (ligma-image-select-item inImage CHANNEL-OP-REPLACE theLayer)
    (ligma-image-remove-layer theImage theLayer)
    (if (and (= (car (ligma-item-is-channel inDrawable)) TRUE)
             (= (car (ligma-item-is-layer-mask inDrawable)) FALSE))
      (ligma-image-set-active-channel theImage inDrawable)
      )
    (ligma-image-undo-group-end theImage)

    (ligma-image-set-selected-layers theImage (car prevLayers) (cadr prevLayers))

    (ligma-displays-flush)
    (ligma-context-pop)
  )
)


(script-fu-register "script-fu-distress-selection"
  _"_Distort..."
  _"Distress the selection"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "23rd April 1998"
  "RGB*,GRAY*"
  SF-IMAGE       "The image"              0
  SF-DRAWABLE    "The layer"              0
  SF-ADJUSTMENT _"_Threshold (bigger 1<-->254 smaller)" '(127 1 254 1 10 0 0)
  SF-ADJUSTMENT _"_Spread"                 '(8 0 1000 1 10 0 1)
  SF-ADJUSTMENT _"_Granularity (1 is low)" '(4 1 25 1 10 0 1)
  SF-ADJUSTMENT _"S_mooth"                 '(2 1 150 1 10 0 1)
  SF-TOGGLE     _"Smooth hor_izontally"    TRUE
  SF-TOGGLE     _"Smooth _vertically"      TRUE
)

(script-fu-menu-register "script-fu-distress-selection"
                         "<Image>/Select/Modify")
