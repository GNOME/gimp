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


(define (script-fu-tile-blur inImage inLayer inRadius inVert inHoriz inType)

  (let* (
        (theImage inImage)
        (theLayer inLayer)
        (theHeight (car (ligma-drawable-get-height theLayer)))
        (theWidth (car (ligma-drawable-get-width theLayer)))
        )

    (define (pasteat xoff yoff)
      (let* (
             (pasted (ligma-edit-paste theLayer FALSE))
             (num-pasted (car pasted))
             (floating-sel (aref (cadr pasted) (- num-pasted 1)))
            )
        (ligma-layer-set-offsets floating-sel (* xoff theWidth) (* yoff theHeight) )
        (ligma-floating-sel-anchor floating-sel)
      )
    )

    (ligma-context-push)
    (ligma-context-set-feather FALSE)
    (ligma-image-undo-group-start theImage)

    (ligma-layer-resize theLayer (* 3 theWidth) (* 3 theHeight) 0 0)

    (ligma-image-select-rectangle theImage CHANNEL-OP-REPLACE 0 0 theWidth theHeight)
    (ligma-edit-cut 1 (vector theLayer))

    (ligma-selection-none theImage)
    (ligma-layer-set-offsets theLayer theWidth theHeight)

    (pasteat 1 1) (pasteat 1 2) (pasteat 1 3)
    (pasteat 2 1) (pasteat 2 2) (pasteat 2 3)
    (pasteat 3 1) (pasteat 3 2) (pasteat 3 3)

    (ligma-selection-none theImage)
    (if (= inType 0)
        (plug-in-gauss-iir RUN-NONINTERACTIVE
                           theImage theLayer inRadius inHoriz inVert)
        (plug-in-gauss-rle RUN-NONINTERACTIVE
                           theImage theLayer inRadius inHoriz inVert)
    )

    (ligma-layer-resize theLayer
                       theWidth theHeight (- 0 theWidth) (- 0 theHeight))
    (ligma-layer-set-offsets theLayer 0 0)
    (ligma-image-undo-group-end theImage)
    (ligma-displays-flush)
    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-tile-blur"
  _"_Tileable Blur..."
  _"Blur the edges of an image so the result tiles seamlessly"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "25th April 1998"
  "RGB*"
  SF-IMAGE       "The Image"         0
  SF-DRAWABLE    "The Layer"         0
  SF-ADJUSTMENT _"Radius"            '(5 0 128 1 1 0 0)
  SF-TOGGLE     _"Blur vertically"   TRUE
  SF-TOGGLE     _"Blur horizontally" TRUE
  SF-OPTION     _"Blur type"         '(_"IIR" _"RLE")
)

(script-fu-menu-register "script-fu-tile-blur"
                         "<Image>/Filters/Blur")
