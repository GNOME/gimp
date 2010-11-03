; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Beveled pattern bullet for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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


(define (script-fu-beveled-pattern-bullet diameter pattern transparent)
  (let* (
        (img (car (gimp-image-new diameter diameter RGB)))
        (background (car (gimp-layer-new img diameter diameter RGBA-IMAGE _"Bullet" 100 NORMAL-MODE)))
        (bumpmap (car (gimp-layer-new img diameter diameter RGBA-IMAGE _"Bumpmap" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img background -1 -1)
    (gimp-image-insert-layer img bumpmap -1 -1)

    ; Create pattern layer

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill background BACKGROUND-FILL)
    (gimp-context-set-pattern pattern)
    (gimp-edit-bucket-fill background PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(127 127 127))
    (gimp-ellipse-select img 1 1 (- diameter 2) (- diameter 2) CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(255 255 255))
    (gimp-ellipse-select img 2 2 (- diameter 4) (- diameter 4) CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map RUN-NONINTERACTIVE img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    ; Background

    (gimp-context-set-background '(0 0 0))
    (gimp-ellipse-select img 0 0 diameter diameter CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-selection-invert img)
    (gimp-edit-clear background)
    (gimp-selection-none img)

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)

    (if (= transparent FALSE)
        (gimp-image-flatten img))

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-beveled-pattern-bullet"
  _"_Bullet..."
  _"Create a beveled pattern bullet for webpages"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "July 1997"
  ""
  SF-ADJUSTMENT _"Diameter"               '(16 1 150 1 10 0 1)
  SF-PATTERN    _"Pattern"                "Wood"
  SF-TOGGLE     _"Transparent background" FALSE
)

(script-fu-menu-register "script-fu-beveled-pattern-bullet"
                         "<Image>/File/Create/Web Page Themes/Beveled Pattern")
