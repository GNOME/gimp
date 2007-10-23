; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
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
;
;  Bovinated Logos v0.1 04/08/98
;  by Brian McFee <keebler@wco.com>
;  Creates Cow-spotted logs.. what else?

(define (apply-bovinated-logo-effect img
                                     logo-layer
                                     spots-x
                                     spots-y
                                     bg-color)
  (let* (
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (bg-layer (car (gimp-layer-new img
                                       width height RGBA-IMAGE
                                       "Background" 100 NORMAL-MODE)))
        (blur-layer (car (gimp-layer-new img
                                         width height RGBA-IMAGE
                                         "Blur" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img blur-layer 1)

    (gimp-selection-all img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-lock-alpha blur-layer TRUE)
    (gimp-context-set-background '(255 255 255))
    (gimp-selection-all img)
    (gimp-edit-fill blur-layer BACKGROUND-FILL)
    (gimp-edit-clear blur-layer)
    (gimp-context-set-background '(191 191 191))
    (gimp-selection-none img)
    (gimp-layer-set-lock-alpha blur-layer FALSE)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-edit-fill blur-layer BACKGROUND-FILL)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img blur-layer 5.0 1 1)
    (gimp-selection-none img)
    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-selection-all img)
    (plug-in-solid-noise RUN-NONINTERACTIVE img logo-layer 0 0 23 1 spots-x spots-y)
    (gimp-brightness-contrast logo-layer 0 127)
    (gimp-selection-none img)
    (gimp-layer-set-lock-alpha logo-layer FALSE)
    (plug-in-bump-map RUN-NONINTERACTIVE img logo-layer blur-layer
                      135 50 10 0 0 0 30 TRUE FALSE 0)
    (gimp-layer-set-offsets blur-layer 5 5)
    (gimp-invert blur-layer)
    (gimp-layer-set-opacity blur-layer 50.0)
    (gimp-image-set-active-layer img logo-layer)

    (gimp-context-pop)
  )
)

(define (script-fu-bovinated-logo-alpha img
                                        logo-layer
                                        spots-x
                                        spots-y
                                        bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-bovinated-logo-effect img logo-layer spots-x spots-y bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-bovinated-logo-alpha"
  _"Bo_vination..."
  _"Add 'cow spots' to the selected region (or alpha)"
  "Brian McFee <keebler@wco.com>"
  "Brian McFee"
  "April 1998"
  "RGBA"
  SF-IMAGE      "Image"             0
  SF-DRAWABLE   "Drawable"          0
  SF-ADJUSTMENT _"Spots density X"  '(16 1 16 1 10 0 1)
  SF-ADJUSTMENT _"Spots density Y"  '(4 1 16 1 10 0 1)
  SF-COLOR      _"Background Color" "white"
)

(script-fu-menu-register "script-fu-bovinated-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-bovinated-logo text
                                  size
                                  font
                                  spots-x
                                  spots-y
                                  bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
         (text-layer (car (gimp-text-fontname img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (apply-bovinated-logo-effect img text-layer spots-x spots-y bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-bovinated-logo"
  _"Bo_vination..."
  _"Create a logo with text in the style of 'cow spots'"
  "Brian McFee <keebler@wco.com>"
  "Brian McFee"
  "April 1998"
  ""
  SF-STRING     _"Text"               "Fear the Cow"
  SF-ADJUSTMENT _"Font size (pixels)" '(80 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "RoostHeavy"
  SF-ADJUSTMENT _"Spots density X"    '(16 1 16 1 10 0 1)
  SF-ADJUSTMENT _"Spots density Y"    '(4 1 16 1 10 0 1)
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-bovinated-logo"
                         "<Toolbox>/Xtns/Logos")
