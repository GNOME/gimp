; 3d-outlined-patterned-shadowed-and-bump-mapped-logo :)
; creates outlined border of a text with patterns
;
; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; 3d-outline creates outlined border of a text with patterns
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

(define (script-fu-3d-outline-logo text-pattern text size font outline-blur-radius shadow-blur-radius bump-map-blur-radius noninteractive s-offset-x s-offset-y)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (text-layer (car (gimp-text-fontname img -1 0 0 text 30 TRUE size PIXELS font)))
         (width (car (gimp-drawable-width text-layer)))
         (height (car (gimp-drawable-height text-layer)))
         (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
         (pattern (car (gimp-layer-new img width height RGBA_IMAGE "Pattern" 100 NORMAL)))
         (old-fg (car (gimp-palette-get-foreground)))
         (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img pattern 1)
    (gimp-image-add-layer img bg-layer 2)
    (gimp-edit-fill img bg-layer)
    (gimp-edit-clear img pattern)                     
    (gimp-layer-set-preserve-trans text-layer FALSE)
    (plug-in-gauss-iir 1 img text-layer outline-blur-radius TRUE TRUE)

    (gimp-layer-set-visible pattern FALSE)
    (set! layer2 (car (gimp-image-merge-visible-layers img 1)))
    (plug-in-edge 1 img layer2 2 1)
    (set! layer3 (car (gimp-layer-copy layer2 TRUE)))
    (gimp-image-add-layer img layer3 2)
    (plug-in-gauss-iir 1 img layer2 bump-map-blur-radius TRUE TRUE)

    (gimp-selection-all img)
    (gimp-patterns-set-pattern text-pattern)
    (gimp-bucket-fill img pattern PATTERN-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
    (plug-in-bump-map noninteractive img pattern layer2 110.0 45.0 4 0 0 0 0 TRUE FALSE 0)

    (set! pattern-mask (car (gimp-layer-create-mask pattern ALPHA-MASK)))
    (gimp-image-add-layer-mask img pattern pattern-mask)

    (gimp-selection-all img)
    (gimp-edit-copy img layer3)
    (set! floating_sel (car (gimp-edit-paste img pattern-mask 0)))
    (gimp-floating-sel-anchor floating_sel)

    (gimp-image-remove-layer-mask img pattern APPLY)
    (gimp-invert img layer3)
    (plug-in-gauss-iir 1 img layer3 shadow-blur-radius TRUE TRUE)

    (gimp-channel-ops-offset img layer3 0 1 s-offset-x s-offset-y)

    (gimp-layer-set-visible layer2 FALSE)
    (gimp-layer-set-visible pattern TRUE)
    (set! final (car (gimp-image-flatten img)))
    
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))    

(script-fu-register "script-fu-3d-outline-logo"
                    "<Toolbox>/Xtns/Script-Fu/Logos/3D Outline"
                    "Creates outlined texts with drop shadow"
                    "Hrvoje Horvat (hhorvat@open.hr)"
                    "Hrvoje Horvat"
                    "07 April, 1998"
                    ""
		    SF-PATTERN "Pattern" "Parque #1"
                    SF-STRING "Text String" "The Gimp"
                    SF-ADJUSTMENT "Font Size (pixels)" '(100 2 1000 1 10 0 1)
                    SF-FONT "Font" "-*-Roostheavy-*-r-*-*-24-*-*-*-p-*-*-*"
                    SF-VALUE "Outline blur radius" "5"
                    SF-VALUE "Shadow blur radius" "10"
                    SF-VALUE "Bump-map (alpha layer) blur radius" "5"
		    SF-TOGGLE "Default bump-map settings" TRUE
		    SF-VALUE "Shadow X offset" "0"
                    SF-VALUE "Shadow Y offset" "0")
