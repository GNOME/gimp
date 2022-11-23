;
; fuzzy-border
;
; Do a cool fade to a given color at the border of an image (optional shadow)
; Will make image RGB if it isn't already.
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

(define (script-fu-fuzzy-border inImage
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

  (define (chris-color-edge inImage inLayer inColor inSize)
    (ligma-selection-all inImage)
    (ligma-selection-shrink inImage inSize)
    (ligma-selection-invert inImage)
    (ligma-context-set-background inColor)
    (ligma-drawable-edit-fill inLayer FILL-BACKGROUND)
    (ligma-selection-none inImage)
  )

  (let (
       (theWidth (car (ligma-image-get-width inImage)))
       (theHeight (car (ligma-image-get-height inImage)))
       (theImage (if (= inCopy TRUE) (car (ligma-image-duplicate inImage))
                                      inImage))
       (theLayer 0)
       )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (if (= inCopy TRUE)
        (ligma-image-undo-disable theImage)
        (ligma-image-undo-group-start theImage)
    )

    (ligma-selection-all theImage)

    (if (> (car (ligma-drawable-type inLayer)) 1)
        (ligma-image-convert-rgb theImage)
    )

    (set! theLayer (car (ligma-layer-new theImage
                                        theWidth
                                        theHeight
                                        RGBA-IMAGE
                                        "layer 1"
                                        100
                                        LAYER-MODE-NORMAL)))

    (ligma-image-insert-layer theImage theLayer 0 0)


    (ligma-drawable-edit-clear theLayer)
    (chris-color-edge theImage theLayer inColor inSize)

    (ligma-layer-scale theLayer
                      (/ theWidth inGranu)
                      (/ theHeight inGranu)
                      TRUE)

    (plug-in-spread RUN-NONINTERACTIVE
                    theImage
                    theLayer
                    (/ inSize inGranu)
                    (/ inSize inGranu))
    (chris-color-edge theImage theLayer inColor 1)
    (ligma-layer-scale theLayer theWidth theHeight TRUE)

    (ligma-image-select-item theImage CHANNEL-OP-REPLACE theLayer)
    (ligma-selection-invert theImage)
    (ligma-drawable-edit-clear theLayer)
    (ligma-selection-invert theImage)
    (ligma-drawable-edit-clear theLayer)
    (ligma-context-set-background inColor)
    (ligma-drawable-edit-fill theLayer FILL-BACKGROUND)
    (ligma-selection-none theImage)
    (chris-color-edge theImage theLayer inColor 1)

    (if (= inBlur TRUE)
        (plug-in-gauss-rle RUN-NONINTERACTIVE
                           theImage theLayer inSize TRUE TRUE)
    )
    (if (= inShadow TRUE)
        (begin
          (ligma-image-insert-layer theImage
                                   (car (ligma-layer-copy theLayer FALSE)) 0 -1)
          (ligma-layer-scale theLayer
                            (- theWidth inSize) (- theHeight inSize) TRUE)
          (ligma-drawable-desaturate theLayer DESATURATE-LIGHTNESS)
          (ligma-drawable-brightness-contrast theLayer 0.5 0.5)
          (ligma-drawable-invert theLayer FALSE)
          (ligma-layer-resize theLayer
                             theWidth
                             theHeight
                             (/ inSize 2)
                             (/ inSize 2))
          (plug-in-gauss-rle RUN-NONINTERACTIVE
                             theImage
                             theLayer
                             (/ inSize 2)
                             TRUE
                             TRUE)
          (ligma-layer-set-opacity theLayer inShadWeight)
        )
    )
    (if (= inFlatten TRUE)
        (ligma-image-flatten theImage)
    )
    (if (= inCopy TRUE)
        (begin  (ligma-image-clean-all theImage)
                (ligma-display-new theImage)
                (ligma-image-undo-enable theImage)
         )
        (ligma-image-undo-group-end theImage)
    )
    (ligma-displays-flush)

    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-fuzzy-border"
  _"_Fuzzy Border..."
  _"Add a jagged, fuzzy border to an image"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "3rd April 1998"
  "RGB* GRAY*"
  SF-IMAGE      "The image"               0
  SF-DRAWABLE   "The layer"               0
  SF-COLOR      _"Color"                  "white"
  SF-ADJUSTMENT _"Border size"            '(16 1 300 1 10 0 1)
  SF-TOGGLE     _"Blur border"            TRUE
  SF-ADJUSTMENT _"Granularity (1 is Low)" '(4 1 16 0.25 5 2 0)
  SF-TOGGLE     _"Add shadow"             FALSE
  SF-ADJUSTMENT _"Shadow weight (%)"      '(100 0 100 1 10 0 0)
  SF-TOGGLE     _"Work on copy"           TRUE
  SF-TOGGLE     _"Flatten image"          TRUE
)

(script-fu-menu-register "script-fu-fuzzy-border"
                         "<Image>/Filters/Decor")
