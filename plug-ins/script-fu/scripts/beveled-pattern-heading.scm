; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Beveled pattern heading for web pages
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
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************


(define (script-fu-beveled-pattern-heading
         text text-size font pattern transparent)
  (let* (
        (img (car (gimp-image-new 10 10 RGB)))
        (textl
          (car
            (gimp-text-fontname img -1 0 0 text 0 TRUE text-size PIXELS font)))

        (width (car (gimp-drawable-width textl)))
        (height (car (gimp-drawable-height textl)))

        (background (car (gimp-layer-new img
                                         width height RGBA-IMAGE
                                         _"Background" 100 NORMAL-MODE)))
        (bumpmap (car (gimp-layer-new img
                                      width height RGBA-IMAGE
                                      _"Bumpmap" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-insert-layer img background -1 1)
    (gimp-image-insert-layer img bumpmap -1 1)

    ; Create pattern layer

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill background BACKGROUND-FILL)
    (gimp-context-set-pattern pattern)
    (gimp-edit-bucket-fill background
                           PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(127 127 127))
    (gimp-item-to-selection textl CHANNEL-OP-REPLACE)
    (gimp-selection-shrink img 1)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(255 255 255))
    (gimp-item-to-selection textl CHANNEL-OP-REPLACE)
    (gimp-selection-shrink img 2)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map RUN-NONINTERACTIVE img background bumpmap 135 45 2 0 0 0 0 TRUE FALSE 0)

    ; Clean up

    (gimp-context-set-background '(0 0 0))
    (gimp-item-to-selection textl CHANNEL-OP-REPLACE)
    (gimp-selection-invert img)
    (gimp-edit-clear background)
    (gimp-selection-none img)

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)
    (gimp-image-remove-layer img textl)

    (if (= transparent FALSE)
        (gimp-image-flatten img))

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-beveled-pattern-heading"
  _"H_eading..."
  _"Create a beveled pattern heading for webpages"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "July 1997"
  ""
  SF-STRING     _"Text"               "Hello world!"
  SF-ADJUSTMENT _"Font size (pixels)" '(72 2 200 1 1 0 1)
  SF-FONT       _"Font"               "Sans"
  SF-PATTERN    _"Pattern"            "Wood"
  SF-TOGGLE     _"Transparent background" FALSE
)

(script-fu-menu-register "script-fu-beveled-pattern-heading"
                         "<Image>/File/Create/Web Page Themes/Beveled Pattern")
