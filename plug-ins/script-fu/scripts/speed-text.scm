; Speed text
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


(define (script-fu-speed-text string font font-size density text-color bg-color)
  (let* (
        (text-ext (gimp-text-get-extents-fontname string font-size PIXELS font))
        (wid (+ (car text-ext) 20))
        (hi  (+ (list-ref text-ext 1) 20))
        (img (car (gimp-image-new wid hi RGB)))
        (bg-layer (car (gimp-layer-new img wid hi  RGB-IMAGE "Background" 100 NORMAL-MODE)))
        (text-layer (car (gimp-layer-new img wid hi  RGBA-IMAGE "Text layer" 100 NORMAL-MODE)))
        (text-mask 0)
        (saved-select 0)
        (cell-size (/ font-size 8))
        (grey (/ (* density 255) 100))
        (saved-sel 0)
        (text-mask 0)
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img bg-layer 0 1)
    (gimp-image-insert-layer img text-layer 0 -1)

    (gimp-context-set-background bg-color)
    (gimp-edit-clear bg-layer)
    (gimp-edit-clear text-layer)

    (gimp-floating-sel-anchor (car (gimp-text-fontname img text-layer 10 10 string 0 TRUE font-size PIXELS font)))

    ; save the selection for later
    (gimp-selection-layer-alpha text-layer)
    (set! saved-sel (car (gimp-selection-save img)))

    ; add layer mask
    (set! text-mask (car (gimp-layer-create-mask text-layer ADD-ALPHA-MASK)))
    (gimp-layer-add-mask text-layer text-mask)

    ; grow the layer
    (gimp-layer-set-edit-mask text-layer FALSE)
    (gimp-selection-grow img 10)
    (gimp-context-set-foreground text-color)
    (gimp-edit-fill text-layer FOREGROUND-FILL)

    ; feather the mask
    (gimp-layer-set-edit-mask text-layer TRUE)
    (gimp-selection-load saved-sel)
    (gimp-selection-feather img 10)
    (gimp-context-set-background (list grey grey grey))
    (gimp-edit-fill text-mask BACKGROUND-FILL)
    (gimp-edit-fill text-mask BACKGROUND-FILL)
    (gimp-edit-fill text-mask BACKGROUND-FILL)
    (gimp-selection-none img)

    (plug-in-newsprint RUN-NONINTERACTIVE img text-mask cell-size 0 0 0.0 1 45.0 0 45.0 0 45.0 0 5)

    (gimp-layer-remove-mask text-layer MASK-APPLY)

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-speed-text"
  _"Speed Text..."
  _"Create a logo with a speedy text effect"
  "Austin Donnelly"
  "Austin Donnelly"
  "1998"
  ""
  SF-STRING     _"Text"               "Speed!"
  SF-FONT       _"Font"               "Charter"
  SF-ADJUSTMENT _"Font size (pixels)" '(100 2 1000 1 10 0 1)
  SF-ADJUSTMENT _"Density (%)"        '(80 0 100 1 10 0 0)
  SF-COLOR      _"Text color"         "black"
  SF-COLOR      _"Background color"   "white"
)

(script-fu-menu-register "script-fu-speed-text"
                         "<Image>/File/Create/Logos")
