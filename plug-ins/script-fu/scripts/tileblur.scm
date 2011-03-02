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


(define (script-fu-tile-blur inImage inLayer inRadius inVert inHoriz inType)

  (let* (
        (theImage inImage)
        (theLayer inLayer)
        (theHeight (car (gimp-drawable-height theLayer)))
        (theWidth (car (gimp-drawable-width theLayer)))
        )

    (define (pasteat xoff yoff)
      (let ((theFloat (car(gimp-edit-paste theLayer 0))))
        (gimp-layer-set-offsets theFloat (* xoff theWidth) (* yoff theHeight) )
        (gimp-floating-sel-anchor theFloat)
       )
    )

    (gimp-image-undo-group-start theImage)
    (gimp-layer-resize theLayer (* 3 theWidth) (* 3 theHeight) 0 0)

    (gimp-context-set-feather 0)
    (gimp-context-set-feather-radius 0 0)
    (gimp-image-select-rectangle theImage CHANNEL-OP-REPLACE 0 0 theWidth theHeight)
    (gimp-edit-cut theLayer)

    (gimp-selection-none theImage)
    (gimp-layer-set-offsets theLayer theWidth theHeight)

    (pasteat 1 1) (pasteat 1 2) (pasteat 1 3)
    (pasteat 2 1) (pasteat 2 2) (pasteat 2 3)
    (pasteat 3 1) (pasteat 3 2) (pasteat 3 3)

    (gimp-selection-none theImage)
    (if (= inType 0)
        (plug-in-gauss-iir RUN-NONINTERACTIVE
                           theImage theLayer inRadius inHoriz inVert)
        (plug-in-gauss-rle RUN-NONINTERACTIVE
                           theImage theLayer inRadius inHoriz inVert)
    )

    (gimp-layer-resize theLayer
                       theWidth theHeight (- 0 theWidth) (- 0 theHeight))
    (gimp-layer-set-offsets theLayer 0 0)
    (gimp-image-undo-group-end theImage)
    (gimp-displays-flush)
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
