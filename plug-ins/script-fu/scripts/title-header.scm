; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Bump-mapped title script --- create a bump-mapped title image for web pages
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
;
; The corresponding parameters have been replaced by an SF-FONT parameter.
; The call to gimp-context-set-background has been given a real layer
; (although it is not used) otherwise gimp 1.1 crashed.
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


(define (script-fu-title-header text
                                size
                                fontname
                                gradient-reverse)
  (let* (; Parameters

         (padding 8)
         (fade-width 64)

         ; Image

         (img (car (gimp-image-new 256 256 RGB)))

         ; Text layer

         (text-layer (car (gimp-text-fontname
                           img
                           -1
                           0
                           0
                           text
                           padding
                           TRUE
                           size
                           PIXELS
                           fontname)))
         (text-width (car (gimp-drawable-width text-layer)))
         (text-height (car (gimp-drawable-height text-layer)))

         ; Sizes

         (text-layers-offset (/ text-height 2))

         (img-width (+ text-layers-offset text-width fade-width))
         (img-height text-height)

         ; Additional layers

         (bg-layer (car (gimp-layer-new img img-width img-height RGBA-IMAGE
                                        "bg-layer" 100 NORMAL-MODE)))
         (bumpmap-layer (car (gimp-layer-new img
                                             text-width
                                             text-height
                                             RGBA-IMAGE
                                             "bumpmap-layer"
                                             100
                                             NORMAL-MODE)))
         (fore-layer (car (gimp-layer-new img text-width text-height RGBA-IMAGE
                                          "fore-layer" 100 NORMAL-MODE)))
       )

    (gimp-context-push)

    ; Create image

    (gimp-image-undo-disable img)
    (gimp-image-resize img img-width img-height 0 0)

    (gimp-image-add-layer img bg-layer -1)
    (gimp-image-add-layer img bumpmap-layer -1)
    (gimp-image-add-layer img fore-layer -1)
;    (gimp-image-add-layer img text-layer -1)
    (gimp-image-raise-layer img text-layer)
    (gimp-image-raise-layer img text-layer)
    (gimp-image-raise-layer img text-layer)
    (gimp-layer-set-offsets bg-layer 0 0)
    (gimp-layer-set-offsets text-layer text-layers-offset 0)
    (gimp-layer-set-offsets bumpmap-layer text-layers-offset 0)
    (gimp-layer-set-offsets fore-layer text-layers-offset 0)

    ; Create bumpmap layer

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bumpmap-layer BACKGROUND-FILL)
    (gimp-selection-layer-alpha text-layer)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill bumpmap-layer BACKGROUND-FILL)
    (gimp-selection-none img)
    (plug-in-gauss-rle RUN-NONINTERACTIVE img bumpmap-layer 4.0 TRUE TRUE)

    ; Fore layer, bumpmap

    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill fore-layer BACKGROUND-FILL)
    (plug-in-bump-map RUN-NONINTERACTIVE img fore-layer bumpmap-layer 135.0 45.0 4 0 0 0 0 FALSE FALSE 0)

    ; Text layer

    (gimp-item-set-visible text-layer TRUE)
    (gimp-layer-set-lock-alpha text-layer TRUE)

    (gimp-edit-blend text-layer CUSTOM-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE gradient-reverse
                     FALSE 0.2 3 TRUE
                     padding padding
                     (- text-width padding 1) (- text-height padding 1))

    ; Semicircle at the left

    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    (gimp-ellipse-select img 0 0 text-height text-height CHANNEL-OP-REPLACE TRUE FALSE 0)
    (gimp-context-set-background (car (gimp-image-pick-color img text-layer
                                                             text-layers-offset 0
                                                             TRUE FALSE 0)))
    (gimp-edit-fill bg-layer BACKGROUND-FILL)

    ; Fade-out gradient at the right

    (gimp-rect-select img (- img-width fade-width) 0 fade-width text-height
                      CHANNEL-OP-REPLACE FALSE 0)
    (gimp-context-set-foreground (car (gimp-context-get-background)))
    (gimp-context-set-background '(0 0 0))

    (gimp-edit-blend bg-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                     FALSE 0.2 3 TRUE
                     (- img-width fade-width) 0 (- img-width 1) 0)

    (gimp-selection-none img)

    ; Done

;    (gimp-image-flatten img)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-title-header"
  _"Web Title Header..."
  _"Create a decorative web title header"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "June 1997"
  ""
  SF-STRING     _"Text"               "Hello world!"
  SF-ADJUSTMENT _"Font size (pixels)" '(32 2 256 1 10 0 0)
  SF-FONT       _"Font"               "Sans"
  SF-TOGGLE     _"Gradient reverse"   FALSE
)

(script-fu-menu-register "script-fu-title-header"
                         "<Image>/File/Create/Logos")
