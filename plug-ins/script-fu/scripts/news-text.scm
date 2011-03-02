; Newsprint text
; Copyright (c) 1998 Austin Donnelly <austin@greenend.org.uk>
;
;
; Based on alien glow code from Adrian Likins
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


(define (script-fu-newsprint-text string font font-size cell-size
                                  density blur-radius text-color bg-color)
  (let* (
        (text-ext (gimp-text-get-extents-fontname string font-size PIXELS font))
        (width (+ (car text-ext) 20 blur-radius))
        (height (+ (list-ref text-ext 1) 20 blur-radius))
        (img (car (gimp-image-new width height RGB)))
        (bg-layer (car (gimp-layer-new img width height RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (text-layer (car (gimp-layer-new img width height RGBA-IMAGE "Text layer" 100 NORMAL-MODE)))
        (text-mask 0)
        (grey (/ (* density 255) 100))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img bg-layer 0 1)
    (gimp-image-insert-layer img text-layer 0 -1)

    (gimp-context-set-background bg-color)
    (gimp-edit-clear bg-layer)
    (gimp-edit-clear text-layer)

    (gimp-context-set-foreground text-color)
    (gimp-floating-sel-anchor
      (car (gimp-text-fontname img text-layer
                               (/ (+ 20 blur-radius) 2) (/ (+ 20 blur-radius) 2)
                               string 0 TRUE font-size PIXELS font)))

    (set! text-mask (car (gimp-layer-create-mask text-layer ADD-ALPHA-MASK)))
    (gimp-layer-add-mask text-layer text-mask)

    (gimp-selection-layer-alpha text-layer)
    (gimp-context-set-background (list grey grey grey))
    (gimp-edit-fill text-mask BACKGROUND-FILL)
    (gimp-selection-none img)
    (if (> blur-radius 0)
        (plug-in-gauss-iir RUN-NONINTERACTIVE img text-mask blur-radius 1 1)
    )

    (plug-in-newsprint RUN-NONINTERACTIVE img text-mask cell-size
                       0 0 45.0 3 45.0 0 45.0 0 45.0 0 3)

    (gimp-edit-fill text-layer FOREGROUND-FILL)
    (gimp-layer-remove-mask text-layer MASK-APPLY)

    (gimp-image-undo-enable img)

    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-newsprint-text"
  _"Newsprint Te_xt..."
  _"Create a logo in the style of newspaper printing"
  "Austin Donnelly"
  "Austin Donnelly"
  "1998"
  ""
  SF-STRING     _"Text"               "Newsprint"
  SF-FONT       _"Font"               "Sans"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-ADJUSTMENT _"Cell size (pixels)" '(7 1 100 1 10 0 1)
  SF-ADJUSTMENT _"Density (%)"        '(60 0 100 1 10 0 0)
  SF-ADJUSTMENT _"Blur radius"        '(0 0 100 1 5 0 0)
  SF-COLOR      _"Text color"         "black"
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-newsprint-text"
                         "<Image>/File/Create/Logos")
