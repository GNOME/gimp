; glossy-patterned-shadowed-and-bump-mapped-logo
; creates anything you can create with it :)
; (use it wisely, use it in peace...) 
;
; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; glossy gives a glossy outlook to your fonts (unlogical name, isn't it?)
; Copyright (C) 1998 Hrvoje Horvat
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

(define (script-fu-glossy-logo text size font blend-gradient-text blend-gradient-outline grow-size bg-color use-pattern-text pattern-text use-pattern-outline pattern-outline use-pattern-overlay pattern-overlay noninteractive shadow-toggle s-offset-x s-offset-y flatten-toggle)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (text-layer (car (gimp-text-fontname img -1 0 0 text 30 TRUE size PIXELS font)))
         (width (car (gimp-drawable-width text-layer)))
         (height (car (gimp-drawable-height text-layer)))
         (bg-layer (car (gimp-layer-new img width height RGBA_IMAGE "Background" 100 NORMAL)))
         (grow-me (car (gimp-layer-copy text-layer TRUE)))

         (old-gradient (car (gimp-gradients-get-active)))
         (old-patterns (car (gimp-patterns-get-pattern)))
         (old-fg (car (gimp-palette-get-foreground)))
         (old-bg (car (gimp-palette-get-background))))

    (gimp-image-undo-disable img)
    (gimp-image-resize img width height 0 0)

    (gimp-layer-set-name grow-me "Grow-me")
    (gimp-image-add-layer img grow-me 1)
    (gimp-image-add-layer img bg-layer 2)

    (gimp-palette-set-background bg-color)
    (gimp-selection-all img)
    (gimp-bucket-fill bg-layer BG-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (gimp-selection-none img)
    (gimp-palette-set-background old-bg)

    (gimp-selection-layer-alpha text-layer)

; if we are going to use transparent gradients for text, we will (maybe) need to uncomment this
; this clears black letters first so you don't end up with black where the transparent should be
;    (gimp-edit-clear img text-layer)

    (if (= use-pattern-text TRUE)
      (begin
        (gimp-patterns-set-pattern pattern-text)
        (gimp-bucket-fill text-layer PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
        (gimp-patterns-set-pattern old-patterns)))

    (if (= use-pattern-text FALSE)
      (begin
        (gimp-gradients-set-active blend-gradient-text)
        (gimp-blend text-layer CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 0 (+ height 5))))

    (gimp-selection-none img)

    (gimp-selection-layer-alpha grow-me)
    (gimp-selection-grow img grow-size)

; if we are going to use transparent gradients for outline, we will (maybe) need to uncomment this
; I didn't put it in the options because there are already enough settings there and anyway, transparent
; gradients will be used very rarely (if ever)
;    (gimp-edit-clear img grow-me)

    (if (= use-pattern-outline TRUE)
      (begin
        (gimp-patterns-set-pattern pattern-outline)
        (gimp-bucket-fill grow-me PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
        (gimp-patterns-set-pattern old-patterns)))

    (if (= use-pattern-outline FALSE)
      (begin
        (gimp-gradients-set-active blend-gradient-outline)
        (gimp-blend grow-me CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 0 (+ height 5))))

    (gimp-selection-none img)

    (plug-in-bump-map noninteractive img grow-me text-layer 110.0 45.0 3 0 0 0 0 TRUE FALSE 0)
    (gimp-layer-set-mode text-layer SCREEN)

    (if (= use-pattern-overlay TRUE)
      (begin
        (gimp-selection-layer-alpha grow-me)
        (gimp-patterns-set-pattern pattern-overlay)
        (gimp-bucket-fill grow-me PATTERN-BUCKET-FILL OVERLAY 100 0 FALSE 0 0)
        (gimp-patterns-set-pattern old-patterns)
        (gimp-selection-none img)))
   
  (if (= shadow-toggle TRUE)
      (begin
        (gimp-selection-layer-alpha text-layer)
        (set! dont-drop-me (car (script-fu-drop-shadow img text-layer s-offset-x s-offset-y 15 '(0 0 0) 80 TRUE)))
        (set! width (car (gimp-image-width img)))
        (set! height (car (gimp-image-height img)))
        (gimp-selection-none img)))

  (if (= flatten-toggle TRUE)
      (begin
      (set! final (car (gimp-image-flatten img)))))

    (gimp-gradients-set-active old-gradient)
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))

(script-fu-register "script-fu-glossy-logo"
                    "<Toolbox>/Xtns/Script-Fu/Logos/Glossy"
                    "Creates anything you can create with it :)"

                    "Hrvoje Horvat (hhorvat@open.hr)"
                    "Hrvoje Horvat"
                    "14/04/1998"
                    ""
                    SF-STRING "Text String" "Galaxy"
                    SF-ADJUSTMENT "Font Size (pixels)" '(100 2 1000 1 10 0 1)
                    SF-FONT "Font" "-*-Eras-*-r-*-*-24-*-*-*-p-*-*-*"
                    SF-GRADIENT "Blend Gradient (text)" "Shadows_2"
                    SF-GRADIENT "Blend Gradient (outline)" "Shadows_2"
                    SF-VALUE "How big outline?" "5"
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-TOGGLE "Use pattern for text instead of gradient" FALSE
		    SF-PATTERN "Pattern (text)" "Electric Blue"
		    SF-TOGGLE "Use pattern for outline instead of gradient" FALSE
		    SF-PATTERN "Pattern (outline)" "Electric Blue"
		    SF-TOGGLE "Use pattern overlay" FALSE
		    SF-PATTERN "Pattern (overlay)" "Parque #1"
		    SF-TOGGLE "Default bump-map settings" TRUE
		    SF-TOGGLE "Shadow?" TRUE
		    SF-VALUE "Shadow X offset" "8"
                    SF-VALUE "Shadow Y offset" "8"
		    SF-TOGGLE "Flatten image?" FALSE)

