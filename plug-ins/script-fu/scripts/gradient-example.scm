; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Gradient example script --- create an example image of a custom gradient
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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

(define (script-fu-gradient-example width height)
  (let* ((img (car (gimp-image-new width height RGB)))
	 (drawable (car (gimp-layer-new img width height RGB "Gradient example" 100 NORMAL)))

	 ; Save old foreground and background colors

	 (old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))

	 ; Calculate colors for checkerboard... just like in the gradient editor

	 (fg-color (* 255 (/ 2 3)))
	 (bg-color (* 255 (/ 1 3))))

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

    ; Render background checkerboard

    (gimp-palette-set-foreground (list fg-color fg-color fg-color))
    (gimp-palette-set-background (list bg-color bg-color bg-color))
    (plug-in-checkerboard 1 img drawable 0 8)

    ; Render gradient

    (gimp-blend drawable CUSTOM NORMAL LINEAR 100 0 REPEAT-NONE FALSE 0 0 0 0 (- width 1) 0)

    ; Terminate

    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-gradient-example"
		    "<Toolbox>/Xtns/Script-Fu/Utils/Custom Gradient"
		    "Create an example image of a custom gradient"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    ""
		    SF-VALUE "Width" "400"
		    SF-VALUE "Height" "32")
