; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; www.gimp.org web headers
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

(define (script-fu-headers-gimp-org text font font-size text-color high-color side-color shadow-color crop rm-bg index num-colors)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (text-layer (car (gimp-text-fontname img -1 0 0
				     text 30 TRUE font-size PIXELS
				     font)))
	 (width (car (gimp-drawable-width text-layer)))
	 (height (car (gimp-drawable-height text-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Background" 100 NORMAL)))
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    
    (gimp-image-disable-undo img)
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)
    (gimp-layer-set-preserve-trans text-layer TRUE)
    (gimp-palette-set-background text-color)
    (gimp-edit-fill text-layer)

    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill bg-layer)

    (let* ((highlight-layer (car (gimp-layer-copy text-layer TRUE)))
	   (side-layer (car (gimp-layer-copy text-layer TRUE)))
	   (shadow-layer (car (gimp-layer-copy text-layer TRUE))))

      (gimp-image-add-layer img highlight-layer 1)
      (gimp-layer-set-preserve-trans highlight-layer TRUE)
      
      (gimp-image-add-layer img side-layer 1)
      (gimp-layer-set-preserve-trans side-layer TRUE)
      
      (gimp-image-add-layer img shadow-layer 1)
      (gimp-layer-set-preserve-trans shadow-layer TRUE)
      
      (gimp-palette-set-background high-color)
      (gimp-edit-fill highlight-layer)
      (gimp-layer-translate highlight-layer -1 -1)
      
      (gimp-palette-set-background side-color)
      (gimp-edit-fill side-layer)
      (gimp-layer-translate side-layer 1 1)

      (gimp-palette-set-background shadow-color)
      (gimp-edit-fill shadow-layer)
      (gimp-layer-translate shadow-layer 5 5)
      
      (gimp-layer-set-preserve-trans shadow-layer FALSE)
      (plug-in-gauss-rle 1 img shadow-layer 5 TRUE TRUE)
      (gimp-layer-set-opacity shadow-layer 60)
      (gimp-image-lower-layer img shadow-layer)
      (gimp-image-lower-layer img shadow-layer))
  

    (set! text-layer (car (gimp-image-flatten img)))
    (gimp-layer-add-alpha text-layer)
	  

    (if (= rm-bg TRUE)
	(begin
	  (gimp-by-color-select text-layer '(255 255 255)
				1 REPLACE TRUE FALSE 0 FALSE)
	  (gimp-edit-clear text-layer)
	  (gimp-selection-clear img)))
        
    (if (= crop TRUE)
	 (plug-in-autocrop 1 img text-layer))

    (if (= index TRUE)
	(gimp-convert-indexed img TRUE num-colors))
    

    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)
    
    ))


(define (script-fu-big-header-gimp-org text font font-size text-color
					high-color side-color shadow-color
					crop rm-bg index num-colors)
  (script-fu-headers-gimp-org (string-append " " text)
			      font font-size
			      text-color high-color side-color shadow-color
			      crop rm-bg index num-colors))


(define (script-fu-small-header-gimp-org text font font-size text-color
					high-color side-color shadow-color
					crop rm-bg index num-colors)
  (script-fu-headers-gimp-org text font
			      font-size text-color high-color
			      side-color shadow-color
			      crop rm-bg index num-colors))


(script-fu-register "script-fu-big-header-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/Big Header"
		    "Big Gimp.org Header"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "gimp.org"
		    SF-FONT "Font" "-*-times-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(50 2 1000 1 10 0 1)
		    SF-COLOR "Text Color" '(82 108 159)
		    SF-COLOR "Higlight Color" '(190 220 250)
		    SF-COLOR "Dark Color" '(46 74 92)
		    SF-COLOR "Shadow Color" '(0 0 0)
		    SF-TOGGLE "AutoCrop?" TRUE
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index image?" TRUE
		    SF-VALUE "# of colors" "15")

(script-fu-register "script-fu-small-header-gimp-org"
		    "<Toolbox>/Xtns/Script-Fu/Web page themes/Gimp.Org/Small Header"
		    "Small Gimp.org Header"
		    "Adrian Likins & Jens Lautenbacher"
		    "Adrian Likins & Jens Lautenbacher"
		    "1997"
		    ""
		    SF-STRING "Text String" "gimp.org"
		    SF-FONT "Font" "-*-helvetica-*-r-*-*-24-*-*-*-p-*-*-*"
		    SF-ADJUSTMENT "Font Size (pixels)" '(24 2 1000 1 10 0 1)
		    SF-COLOR "Text Color" '(135 220 220)
		    SF-COLOR "Higlight Color" '(210 240 245)
		    SF-COLOR "Dark Color" '(46 74 92)
		    SF-COLOR "Shadow Color" '(0 0 0)
		    SF-TOGGLE "AutoCrop?" TRUE
		    SF-TOGGLE "Remove Background" TRUE
		    SF-TOGGLE "Index image?" TRUE
		    SF-VALUE "# of colors" "15"
		    SF-VALUE "select-by-color threshold" "1")

