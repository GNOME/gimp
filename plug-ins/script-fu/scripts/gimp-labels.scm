; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; www.gimp.org web labels
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by jtl (Jens  Lautenbacher)
; and improved by jtl
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
;
;  **NOTE**  This fonts use some very common fonts, that are typically
;  bitmap fonts on most installations. If you want better quality versions
;  you need to grab the urw font package form www.gimp.org/fonts.html
;  and install as indicated. This will replace the some current bitmap fonts
;  with higher quality vector fonts. This is how the actual www.gimp.org
;  logos were created.
;
; ************************************************************************
; Changed on Feb 4, 1999 by Piet van Oostrum <piet@cs.uu.nl>
; For use with GIMP 1.1.
; All calls to gimp-text-* have been converted to use the *-fontname form.
; The corresponding parameters have been replaced by an SF-FONT parameter.
; ************************************************************************


(define (script-fu-labels-gimp-org text font font-size text-color shadow-color bg-color rm-bg index num-colors color-thresh yoff xoff height)
  (let* ((img (car (gimp-image-new 125 height RGB)))
	 (text-layer (car (gimp-text-fontname img -1 xoff yoff text 0
				     TRUE font-size PIXELS
				     font)))
	 (bg-layer (car (gimp-layer-new  img 125 height
					 RGB_IMAGE "Background" 100 NORMAL)))
	 (shadow-layer (car (gimp-layer-copy text-layer TRUE)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    
    (gimp-image-undo-disable img)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img bg-layer 2)
    
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-layer-set-preserve-trans shadow-layer TRUE)
    
    (gimp-palette-set-background text-color)
    (gimp-edit-fill text-layer)

    (gimp-palette-set-background bg-color)
    (gimp-edit-fill bg-layer)

    (gimp-palette-set-background shadow-color)
    (gimp-edit-fill shadow-layer)
    (gimp-layer-translate shadow-layer 1 1)

    (set! text-layer (car (gimp-image-flatten img)))
    (gimp-layer-add-alpha text-layer)

    (if (= rm-bg TRUE)
	(begin
	  (gimp-by-color-select text-layer bg-color
				color-thresh REPLACE TRUE FALSE 0 FALSE)
	  (gimp-edit-clear text-layer)
	  (gimp-selection-clear img)))
    
    (if (= index TRUE)
	(gimp-convert-indexed img TRUE num-colors))
   
    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)))


;;;(define (script-fu-tube-button-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 14 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 8 0 30))
;;;
;;;(define (script-fu-tube-subbutton-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 12 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 7 0 24))
;;;  
;;;(define (script-fu-tube-subsubbutton-label-gimp-org text rm-bg index)
;;;  (script-fu-labels-gimp-org text "nimbus sans" 10 "medium" "r" "normal" '(151 177 192) '(0 0 0) '(255 255 255) rm-bg index 15 1 6 0 18))
;;;

(define (script-fu-tube-button-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 14 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 8 0 30))

(define (script-fu-tube-subbutton-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 12 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 7 10 24))
  
(define (script-fu-tube-subsubbutton-label-gimp-org text rm-bg index)
  (script-fu-labels-gimp-org text "helvetica" 10 '(86 114 172) '(255 255 255) '(255 255 255) rm-bg index 15 1 6 20 18))


(script-fu-register "script-fu-tube-button-label-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/Tube Button Label"
		    "Tube Button Label Header for gimp.org"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "?"
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index Image" TRUE)

(script-fu-register "script-fu-tube-subbutton-label-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/Tube Sub-Button Label"
		    "Tube Button Label Header for gimp.org"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "?"
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index Image" TRUE)

(script-fu-register "script-fu-tube-subsubbutton-label-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/Tube Sub-Sub-Button Label"
		    "Tube Button Label Header for gimp.org"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "?"
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index Image" TRUE)


(script-fu-register "script-fu-labels-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/General Tube Labels"
		    "Tube Button Label Header for gimp.org"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "Gimp.Org"
		    SF-FONT "Font" "-*-helvetica-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(18 2 1000 1 10 0 1)
 		    SF-COLOR "Text Color" '(130 165 235)
	 	    SF-COLOR "Shadow Color" '(0 0 0)
		    SF-COLOR "Background Color" '(255 255 255)
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index Image" TRUE
		    SF-VALUE "# of colors" "15"
		    SF-VALUE "select-by-color threshold" "1"
		    SF-VALUE "Y-Offset" "8"
		    SF-VALUE "X-Offset" "0"
		    SF-VALUE "Height"   "30")
