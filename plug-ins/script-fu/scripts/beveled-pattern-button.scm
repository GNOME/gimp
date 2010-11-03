; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Beveled pattern button for web pages
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

(define (text-width extents)
  (car extents))

(define (text-height extents)
  (cadr extents))

(define (text-ascent extents)
  (caddr extents))

(define (text-descent extents)
  (cadr (cddr extents)))

(define (script-fu-beveled-pattern-button
         text text-size font text-color pattern pressed)
  (let* (
        (text-extents (gimp-text-get-extents-fontname
                       text text-size PIXELS font))
        (ascent (text-ascent text-extents))
        (descent (text-descent text-extents))

        (xpadding 8)
        (ypadding 6)

        (width (+ (* 2 xpadding)
                  (text-width text-extents)))
        (height (+ (* 2 ypadding)
                   (+ ascent descent)))

        (img (car (gimp-image-new width height RGB)))
        (background (car (gimp-layer-new img width height RGBA-IMAGE _"Background" 100 NORMAL-MODE)))
        (bumpmap (car (gimp-layer-new img width height RGBA-IMAGE _"Bumpmap" 100 NORMAL-MODE)))
        (textl (car
                (gimp-text-fontname
                 img -1 0 0 text 0 TRUE text-size PIXELS font)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img background -1 1)
    (gimp-image-insert-layer img bumpmap -1 1)

    ; Create pattern layer

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill background BACKGROUND-FILL)
    (gimp-context-set-pattern pattern)
    (gimp-edit-bucket-fill background PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)

    ; Create bumpmap layer

    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(127 127 127))
    (gimp-rect-select img 1 1 (- width 2) (- height 2) CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-context-set-background '(255 255 255))
    (gimp-rect-select img 2 2 (- width 4) (- height 4) CHANNEL-OP-REPLACE FALSE 0)
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)

    ; Bumpmap

    (plug-in-bump-map RUN-NONINTERACTIVE img background bumpmap 135 45 2 0 0 0 0 TRUE pressed 0)

    ; Color and position text

    (gimp-context-set-background text-color)
    (gimp-layer-set-lock-alpha textl TRUE)
    (gimp-edit-fill textl BACKGROUND-FILL)

    (gimp-layer-set-offsets textl
                            xpadding
                            (+ ypadding descent))

    ; Clean up

    (gimp-image-set-active-layer img background)
    (gimp-image-remove-layer img bumpmap)
    (gimp-image-flatten img)

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)


(script-fu-register "script-fu-beveled-pattern-button"
  _"B_utton..."
  _"Create a beveled pattern button for webpages"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "July 1997"
  ""
  SF-STRING     _"Text"               "Hello world!"
  SF-ADJUSTMENT _"Font size (pixels)" '(32 2 1000 1 10 0 1)
  SF-FONT       _"Font"               "Sans"
  SF-COLOR      _"Text color"         "black"
  SF-PATTERN    _"Pattern"            "Wood"
  SF-TOGGLE     _"Pressed"            FALSE
)

(script-fu-menu-register "script-fu-beveled-pattern-button"
                         "<Image>/File/Create/Web Page Themes/Beveled Pattern")
