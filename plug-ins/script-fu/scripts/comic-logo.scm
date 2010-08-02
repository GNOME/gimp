; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
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
;
;  Comic Book Logo v0.1  04/08/98
;  by Brian McFee
;  Creates snazzy-looking text, inspired by watching a Maxx marathon :)

(define (apply-comic-logo-effect img
                                 logo-layer
                                 gradient
                                 gradient-reverse
                                 ol-width
                                 ol-color
                                 bg-color)
  (let* (
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (posx (- (car (gimp-drawable-offsets logo-layer))))
        (posy (- (cadr (gimp-drawable-offsets logo-layer))))
        (bg-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                       "Background" 100 NORMAL-MODE)))
        (white-layer (car (gimp-layer-copy logo-layer 1)))
        (black-layer (car (gimp-layer-copy logo-layer 1)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (script-fu-util-image-add-layers img black-layer white-layer bg-layer)
    (gimp-layer-translate white-layer posx posy)
    (gimp-item-set-name white-layer "White")
    (gimp-layer-translate black-layer posx posy)
    (gimp-item-set-name black-layer "Black")

    (gimp-selection-all img)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-selection-none img)

    (gimp-layer-set-lock-alpha white-layer TRUE)
    (gimp-context-set-background ol-color)
    (gimp-selection-all img)
    (gimp-edit-fill white-layer BACKGROUND-FILL)
    (gimp-layer-set-lock-alpha white-layer FALSE)
    (plug-in-spread RUN-NONINTERACTIVE img white-layer (* 3 ol-width) (* 3 ol-width))
    (plug-in-gauss-rle RUN-NONINTERACTIVE img white-layer (* 2 ol-width) 1 1)
    (plug-in-threshold-alpha RUN-NONINTERACTIVE img white-layer 0)
    (gimp-layer-set-lock-alpha white-layer TRUE)
    (gimp-edit-fill white-layer BACKGROUND-FILL)
    (gimp-selection-none img)

    (gimp-context-set-background '(0 0 0))
    (gimp-layer-set-lock-alpha black-layer TRUE)
    (gimp-selection-all img)
    (gimp-edit-fill black-layer BACKGROUND-FILL)
    (gimp-selection-none img)
    (gimp-layer-set-lock-alpha black-layer FALSE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img black-layer ol-width 1 1)
    (plug-in-threshold-alpha RUN-NONINTERACTIVE img black-layer 0)

    (gimp-context-set-gradient gradient)
    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-selection-all img)

    (gimp-edit-blend logo-layer CUSTOM-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE gradient-reverse
                     FALSE 0 0 TRUE
                     0 (* height 0.3) 0 (* height 0.78))

    (plug-in-noisify RUN-NONINTERACTIVE img logo-layer 0 0.20 0.20 0.20 0.20)
    (gimp-selection-none img)
    (gimp-layer-set-lock-alpha logo-layer FALSE)
    (gimp-brightness-contrast logo-layer 0 30)
    (plug-in-threshold-alpha RUN-NONINTERACTIVE img logo-layer 60)
    (gimp-image-set-active-layer img logo-layer)

    (gimp-context-pop)
  )
)

(define (script-fu-comic-logo-alpha img
                                    logo-layer
                                    gradient
                                    gradient-reverse
                                    ol-width
                                    ol-color
                                    bg-color)
  (begin
    (gimp-image-undo-group-start img)
    (apply-comic-logo-effect img logo-layer
                             gradient gradient-reverse
                             ol-width ol-color bg-color)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-comic-logo-alpha"
  _"Comic Boo_k..."
  _"Add a comic-book effect to the selected region (or alpha) by outlining and filling with a gradient"
  "Brian McFee <keebler@wco.com>"
  "Brian McFee"
  "April 1998"
  "RGBA"
  SF-IMAGE      "Image"             0
  SF-DRAWABLE   "Drawable"          0
  SF-GRADIENT   _"Gradient"         "Incandescent"
  SF-TOGGLE     _"Gradient reverse" FALSE
  SF-ADJUSTMENT _"Outline size"     '(5 1 100 1 10 0 1)
  SF-COLOR      _"Outline color"    "white"
  SF-COLOR      _"Background color" "white"
)

(script-fu-menu-register "script-fu-comic-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")


(define (script-fu-comic-logo text
                              size
                              font
                              gradient
                              gradient-reverse
                              ol-width
                              ol-color
                              bg-color)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (border (/ size 4))
         (text-layer (car (gimp-text-fontname
                           img -1 0 0 text border TRUE size PIXELS font))))
    (gimp-image-undo-disable img)
    (apply-comic-logo-effect img text-layer gradient gradient-reverse
                             ol-width ol-color bg-color)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-comic-logo"
  _"Comic Boo_k..."
  _"Create a comic-book style logo by outlining and filling with a gradient"
  "Brian McFee <keebler@wco.com>"
  "Brian McFee"
  "April 1998"
  ""
  SF-STRING     _"Text"               "Moo"
  SF-ADJUSTMENT _"Font size (pixels)" '(85 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Tribeca"
  SF-GRADIENT   _"Gradient"           "Incandescent"
  SF-TOGGLE     _"Gradient reverse"   FALSE
  SF-ADJUSTMENT _"Outline size"       '(5 1 100 1 10 0 1)
  SF-COLOR      _"Outline color"      "white"
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-comic-logo"
                         "<Image>/File/Create/Logos")
