; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Round Button --- create a round beveled Web button.
; Copyright (C) 1998 Federico Mena Quintero & Arturo Espinosa Aldama
; federico@nuclecu.unam.mx arturo@nuclecu.unam.mx
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************
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

(define (script-fu-round-button text
                                size
                                font
                                ul-color
                                lr-color
                                text-color
                                ul-color-high
                                lr-color-high
                                hlight-color
                                xpadding
                                ypadding
                                bevel
                                ratio
                                notpressed
                                notpressed-active
                                pressed)

  (cond ((eqv? notpressed TRUE)
         (do-pupibutton text size font ul-color lr-color
                        text-color xpadding ypadding bevel ratio 0)))
  (cond ((eqv? notpressed-active TRUE)
         (do-pupibutton text size font ul-color-high lr-color-high
                        hlight-color xpadding ypadding bevel ratio 0)))
  (cond ((eqv? pressed TRUE)
         (do-pupibutton text size font ul-color-high lr-color-high
                        hlight-color xpadding ypadding bevel ratio 1))))

(define (do-pupibutton text
                       size
                       font
                       ul-color
                       lr-color
                       text-color
                       xpadding
                       ypadding
                       bevel
                       ratio
                       pressed)

  (define (text-width extents)
    (car extents))

  (define (text-height extents)
    (cadr extents))

  (define (text-ascent extents)
    (caddr extents))

  (define (text-descent extents)
    (cadr (cddr extents)))

  (define (round-select img
                        x
                        y
                        width
                        height
                        ratio)
    (let* ((diameter (* ratio height)))
      (gimp-ellipse-select img x y diameter height CHANNEL-OP-ADD FALSE 0 0)
      (gimp-ellipse-select img (+ x (- width diameter)) y
                           diameter height CHANNEL-OP-ADD FALSE 0 0)
      (gimp-rect-select img (+ x (/ diameter 2)) y
                        (- width diameter) height CHANNEL-OP-ADD FALSE 0)))

  (let* (
        (text-extents (gimp-text-get-extents-fontname text
                                                      size
                                                      PIXELS
                                                      font))
        (ascent (text-ascent text-extents))
        (descent (text-descent text-extents))

        (height (+ (* 2 (+ ypadding bevel))
                       (+ ascent descent)))

        (radius (/ (* ratio height) 4))

        (width (+ (* 2 (+ radius xpadding))
                  bevel
                  (text-width text-extents)))

        (img (car (gimp-image-new width height RGB)))

        (bumpmap (car (gimp-layer-new img width height
                                      RGBA-IMAGE "Bumpmap" 100 NORMAL-MODE)))
        (gradient (car (gimp-layer-new img width height
                                       RGBA-IMAGE "Button" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    ; Create bumpmap layer

    (gimp-image-insert-layer img bumpmap 0 -1)
    (gimp-selection-none img)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (round-select img (/ bevel 2) (/ bevel 2)
                  (- width bevel) (- height bevel) ratio)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bumpmap BACKGROUND-FILL)

    (gimp-selection-none img)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img bumpmap bevel 1 1)

    ; Create gradient layer

    (gimp-image-insert-layer img gradient 0 -1)
    (gimp-edit-clear gradient)
    (round-select img 0 0 width height ratio)
    (gimp-context-set-foreground ul-color)
    (gimp-context-set-background lr-color)

    (gimp-edit-blend gradient FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 0 (- height 1))

    (gimp-selection-none img)

    (plug-in-bump-map RUN-NONINTERACTIVE img gradient bumpmap
                      135 45 bevel 0 0 0 0 TRUE pressed 0)

;     Create text layer

    (cond ((eqv? pressed 1) (set! bevel (+ bevel 1))))

    (gimp-context-set-foreground text-color)
    (let ((textl (car (gimp-text-fontname
                       img -1 0 0 text 0 TRUE size PIXELS
                       font))))
      (gimp-layer-set-offsets textl
                              (+ xpadding radius bevel)
                              (+ ypadding descent bevel)))

;   Delete some fucked-up pixels.

    (gimp-selection-none img)
    (round-select img 1 1 (- width 1) (- height 1) ratio)
    (gimp-selection-invert img)
    (gimp-edit-clear gradient)

;     Done

    (gimp-image-remove-layer img bumpmap)
    (gimp-image-merge-visible-layers img EXPAND-AS-NECESSARY)

    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-round-button"
  _"_Round Button..."
  _"Create images, each containing an oval button graphic"
  "Arturo Espinosa (stolen from quartic's beveled button)"
  "Arturo Espinosa & Federico Mena Quintero"
  "June 1998"
  ""
  SF-STRING     _"Text"                 "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)"   '(16 2 100 1 1 0 1)
  SF-FONT       _"Font"                 "Sans"
  SF-COLOR      _"Upper color"          '(192 192 0)
  SF-COLOR      _"Lower color"          '(128 108 0)
  SF-COLOR      _"Text color"           "black"
  SF-COLOR      _"Upper color (active)" '(255 255 0)
  SF-COLOR      _"Lower color (active)" '(128 108 0)
  SF-COLOR      _"Text color (active)"  '(0 0 192)
  SF-ADJUSTMENT _"Padding X"            '(4 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Padding Y"            '(4 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Bevel width"          '(2 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Round ratio"          '(1 0.05 20 0.05 1 2 1)
  SF-TOGGLE     _"Not pressed"          TRUE
  SF-TOGGLE     _"Not pressed (active)" TRUE
  SF-TOGGLE     _"Pressed"              TRUE
)

(script-fu-menu-register "script-fu-round-button"
                         "<Image>/File/Create/Buttons")
