; Newsprint text
; Copyright (c) 1998 Austin Donnelly <austin@greenend.org.uk>
;
;
; Based on alien glow code from Adrian Likins
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


(define (script-fu-speed-text string font font-size density)
  (let* ((text-ext (gimp-text-get-extents-fontname string font-size PIXELS font))
	 (wid (+ (car text-ext) 20))
	 (hi  (+ (nth 1 text-ext) 20))
	 (img (car (gimp-image-new wid hi RGB)))
	 (bg-layer (car (gimp-layer-new img wid hi  RGB_IMAGE "Background" 100 NORMAL)))
	 (text-layer (car (gimp-layer-new img wid hi  RGBA_IMAGE "Text layer" 100 NORMAL)))
	 (text-mask 0)
	 (saved-select 0)
	 (cell-size (/ font-size 8))
	 (grey (/ (* density 255) 100))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-image-add-layer img text-layer -1)

    (gimp-edit-clear bg-layer)
    (gimp-edit-clear text-layer)

    (gimp-floating-sel-anchor (car (gimp-text-fontname img text-layer 10 10 string 0 TRUE font-size PIXELS font)))

    ; save the selection for later
    (gimp-selection-layer-alpha text-layer)
    (set! saved-sel (car (gimp-selection-save img)))

    ; add layer mask
    (set! text-mask (car (gimp-layer-create-mask text-layer ALPHA-MASK)))
    (gimp-image-add-layer-mask img text-layer text-mask)

    ; grow the layer
    (gimp-layer-set-edit-mask text-layer FALSE)
    (gimp-selection-grow img 10)
    (gimp-palette-set-background old-fg)
    (gimp-edit-fill text-layer)

    ; feather the mask
    (gimp-layer-set-edit-mask text-layer TRUE)
    (gimp-selection-load saved-sel)
    (gimp-selection-feather img 10)
    (gimp-palette-set-background (list grey grey grey))
    (gimp-edit-fill text-mask)
    (gimp-edit-fill text-mask)
    (gimp-edit-fill text-mask)
    (gimp-selection-clear img)

    (plug-in-newsprint 1 img text-mask cell-size 0 0 0.0 1 45.0 0 45.0 0 45.0 0 5)

    (gimp-image-remove-layer-mask img text-layer APPLY)

    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)

    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-speed-text"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Speed text"
		    "give text a speedy effect"
		    "Austin Donnelly"
		    "Austin Donnelly"
		    "1998"
		    ""
		    SF-STRING "Text String" "Speed!"
		    SF-FONT "Font" "-*-Charter-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(100 2 1000 1 10 0 1)
		    SF-ADJUSTMENT "Density (%)" '(80 0 100 1 10 0 0))
