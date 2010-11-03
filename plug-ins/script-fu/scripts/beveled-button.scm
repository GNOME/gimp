; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Button00 --- create a simple beveled Web button
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

(define (blend-bumpmap img
                       drawable
                       x1
                       y1
                       x2
                       y2)
  (gimp-edit-blend drawable FG-BG-RGB-MODE DARKEN-ONLY-MODE
                   GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                   FALSE 0 0 TRUE
                   x1 y1 x2 y2))

(define (script-fu-button00 text
                            size
                            font
                            ul-color
                            lr-color
                            text-color
                            padding
                            bevel-width
                            pressed)
  (let* (
        (text-extents (gimp-text-get-extents-fontname text
                                                      size
                                                      PIXELS
                                                      font))
        (ascent (text-ascent text-extents))
        (descent (text-descent text-extents))

        (img-width (+ (* 2 (+ padding bevel-width))
                      (text-width text-extents)))
        (img-height (+ (* 2 (+ padding bevel-width))
                       (+ ascent descent)))

        (img (car (gimp-image-new img-width img-height RGB)))

        (bumpmap (car (gimp-layer-new img
                                      img-width img-height RGBA-IMAGE
                                      _"Bumpmap" 100 NORMAL-MODE)))
        (gradient (car (gimp-layer-new img
                                       img-width img-height RGBA-IMAGE
                                       _"Gradient" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    ; Create bumpmap layer

    (gimp-image-insert-layer img bumpmap -1 -1)
    (gimp-context-set-foreground '(0 0 0))
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-rect-select img 0 0 bevel-width img-height CHANNEL-OP-REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 0 (- bevel-width 1) 0)

    (gimp-rect-select img 0 0 img-width bevel-width CHANNEL-OP-REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 0 0 (- bevel-width 1))

    (gimp-rect-select img (- img-width bevel-width) 0 bevel-width img-height CHANNEL-OP-REPLACE FALSE 0)
    (blend-bumpmap img bumpmap (- img-width 1) 0 (- img-width bevel-width) 0)

    (gimp-rect-select img 0 (- img-height bevel-width) img-width bevel-width CHANNEL-OP-REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 (- img-height 1) 0 (- img-height bevel-width))

    (gimp-selection-none img)

    ; Create gradient layer

    (gimp-image-insert-layer img gradient -1 -1)
    (gimp-context-set-foreground ul-color)
    (gimp-context-set-background lr-color)

    (gimp-edit-blend gradient FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 (- img-width 1) (- img-height 1))

    (plug-in-bump-map RUN-NONINTERACTIVE img gradient bumpmap
                      135 45 bevel-width 0 0 0 0 TRUE pressed 0)

    ; Create text layer

    (gimp-context-set-foreground text-color)
    (let ((textl (car (gimp-text-fontname
                       img -1 0 0 text 0 TRUE size PIXELS font))))
      (gimp-layer-set-offsets textl
                              (+ bevel-width padding)
                              (+ bevel-width padding descent)))

    ; Done

    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-button00"
  _"Simple _Beveled Button..."
  _"Create a simple, beveled button graphic for webpages"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "June 1997"
  ""
  SF-STRING     _"Text"               "Hello world!"
  SF-ADJUSTMENT _"Font size (pixels)" '(16 2 100 1 1 0 1)
  SF-FONT       _"Font"               "Sans"
  SF-COLOR      _"Upper-left color"   '(0 255 127)
  SF-COLOR      _"Lower-right color"  '(0 127 255)
  SF-COLOR      _"Text color"         "black"
  SF-ADJUSTMENT _"Padding"            '(2 1 100 1 10 0 1)
  SF-ADJUSTMENT _"Bevel width"        '(4 1 100 1 10 0 1)
  SF-TOGGLE     _"Pressed"            FALSE
)

(script-fu-menu-register "script-fu-button00"
                         "<Image>/File/Create/Buttons")
