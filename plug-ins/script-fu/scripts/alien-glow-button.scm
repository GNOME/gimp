; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Alien Glow themed button
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on code from Frederico Mena Quintero (Quartic)
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


(define (script-fu-alien-glow-button text
                                     font
                                     size
                                     text-color
                                     glow-color
                                     bg-color
                                     padding
                                     glow-radius
                                     flatten)

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
					 x1 y1 x2 y2)
  )

  (let* (
        (text-extents (gimp-text-get-extents-fontname text
                                                      size
                                                      PIXELS
                                                      font))
        (ascent (text-ascent text-extents))
        (descent (text-descent text-extents))

        (img-width (+ (* 2  padding)
                      (text-width text-extents)))
        (img-height (+ (* 2 padding)
                       (+ ascent descent)))
        (layer-height img-height)
        (layer-width img-width)
        (img-width (+ img-width glow-radius))
        (img-height (+ img-height glow-radius))
        (img (car (gimp-image-new img-width img-height RGB)))
        (bg-layer (car (gimp-layer-new img
                                       img-width img-height RGBA-IMAGE
                                       _"Background" 100 NORMAL-MODE)))
        (glow-layer (car (gimp-layer-new img
                                         img-width img-height RGBA-IMAGE
                                         _"Glow" 100 NORMAL-MODE)))
        (button-layer (car (gimp-layer-new img
                                           layer-width layer-height RGBA-IMAGE
                                           _"Button" 100 NORMAL-MODE)))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    ; Create bumpmap layer

    (gimp-image-insert-layer img bg-layer 0 -1)
    (gimp-context-set-foreground '(0 0 0))
    (gimp-context-set-background bg-color)
    (gimp-edit-fill bg-layer BACKGROUND-FILL)
    (gimp-image-insert-layer img glow-layer 0 -1)

    ; Create text layer

    (gimp-image-insert-layer img button-layer 0 -1)
    (gimp-layer-set-offsets button-layer (/ glow-radius 2) (/ glow-radius 2))
    (gimp-selection-none img)
    (gimp-rect-select img 0 0 img-width img-height CHANNEL-OP-REPLACE FALSE 0)
    (gimp-context-set-foreground '(100 100 100))
    (gimp-context-set-background '(0 0 0))

    (gimp-edit-blend button-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 img-height img-width)

    (gimp-edit-clear glow-layer)

    (gimp-rect-select img
                      (/ glow-radius 4)
                      (/ glow-radius 4)
                      (- img-width (/ glow-radius 2))
                      (- img-height (/ glow-radius 2))
                      CHANNEL-OP-REPLACE FALSE 0 )

    (gimp-context-set-foreground glow-color)
    (gimp-edit-fill glow-layer FOREGROUND-FILL)
    (gimp-selection-none img)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img glow-layer glow-radius TRUE TRUE)
    (gimp-context-set-foreground text-color)
    (let (
         (textl (car (gimp-text-fontname
                      img -1 0 0 text 0 TRUE size PIXELS font)))
         )
      (gimp-layer-set-offsets textl
                              (+  padding (/ glow-radius 2))
                              (+ (+ padding descent) (/ glow-radius 2)))
    )
    ; Done
    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (if (= flatten TRUE)
        (gimp-image-flatten img)
    )

    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-alien-glow-button"
  _"B_utton..."
  _"Create a button graphic with an eerie glow for web pages"
  "Adrian Likins"
  "Adrian Likins"
  "July 1997"
  ""
  SF-STRING     _"Text"               "Hello world!"
  SF-FONT       _"Font"               "Sans Bold"
  SF-ADJUSTMENT _"Font size (pixels)" '(22 2 100 1 1 0 1)
  SF-COLOR      _"Text color"         "black"
  SF-COLOR      _"Glow color"         '(63 252 0)
  SF-COLOR      _"Background color"   "black"
  SF-ADJUSTMENT _"Padding"            '(6 1 100 1 10 0 1)
  SF-ADJUSTMENT _"Glow radius"        '(10 1 200 1 10 0 1)
  SF-TOGGLE     _"Flatten image"      TRUE
)

(script-fu-menu-register "script-fu-alien-glow-button"
                         "<Image>/File/Create/Web Page Themes/Alien Glow")
