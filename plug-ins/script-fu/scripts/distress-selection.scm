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
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
       (theWidth (car (gimp-image-width inImage)))
       (theHeight (car (gimp-image-height inImage)))
       (theLayer 0)
       (theMode (car (gimp-image-base-type inImage)))
       )

  (gimp-image-undo-group-start theImage)
  (if (= theMode GRAY)
    (set! theMode GRAYA-IMAGE)
    (set! theMode RGBA-IMAGE)
    )
  (set! theLayer (car (gimp-layer-new theImage
                                      theWidth
                                      theHeight
                                      theMode
                                      "Distress Scratch Layer"
                                      100
                                      NORMAL-MODE)))

  (gimp-image-insert-layer theImage theLayer -1 0)

  (if (= FALSE (car (gimp-selection-is-empty theImage)))
      (gimp-edit-fill theLayer BACKGROUND-FILL)
  )

  (gimp-selection-invert theImage)

  (if (= FALSE (car (gimp-selection-is-empty theImage)))
      (gimp-edit-clear theLayer)
  )

  (gimp-selection-invert theImage)
  (gimp-selection-none inImage)

  (gimp-layer-scale theLayer
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
  (gimp-layer-scale theLayer theWidth theHeight TRUE)
  (plug-in-threshold-alpha RUN-NONINTERACTIVE theImage theLayer inThreshold)
  (plug-in-gauss-iir RUN-NONINTERACTIVE theImage theLayer 1 TRUE TRUE)
  (gimp-image-select-item inImage CHANNEL-OP-REPLACE theLayer)
  (gimp-image-remove-layer theImage theLayer)
  (if (and (= (car (gimp-item-is-channel inDrawable)) TRUE)
           (= (car (gimp-item-is-layer-mask inDrawable)) FALSE))
    (gimp-image-set-active-channel theImage inDrawable)
    )
  (gimp-image-undo-group-end theImage)

  (gimp-displays-flush)
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
  SF-ADJUSTMENT _"Threshold (bigger 1<-->255 smaller)" '(127 1 255 1 10 0 0)
  SF-ADJUSTMENT _"Spread"                 '(8 0 1000 1 10 0 1)
  SF-ADJUSTMENT _"Granularity (1 is low)" '(4 1 25 1 10 0 1)
  SF-ADJUSTMENT _"Smooth"                 '(2 1 150 1 10 0 1)
  SF-TOGGLE     _"Smooth horizontally"    TRUE
  SF-TOGGLE     _"Smooth vertically"      TRUE
)

(script-fu-menu-register "script-fu-distress-selection"
                         "<Image>/Select/Modify")
