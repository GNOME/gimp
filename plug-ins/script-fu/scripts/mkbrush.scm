; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Make-Brush - a script for the script-fu program
; by Seth Burgess 1997 <sjburges@ou.edu>
;
; ======================================================================
; NOTE: This will crash with an ugly error if home dir is not set correctly,
;       or ~/.gimp/brushes/ is not writable.  To change it from /home/seth
;       permanently, change the value on the next to last line to reflect
;       your home directory.  Be sure to escape the quotes!
; ======================================================================
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


(define (script-fu-make-brush-rectangular description width height spacing )
    (begin
        (let* (
            (img (car (gimp-image-new width height GRAY)))
             (drawable (car (gimp-layer-new img width height GRAY_IMAGE "MakeBrush" 100 NORMAL)))

     ; Save old foregound and background colors

     (old-fg-color (car (gimp-palette-get-foreground)))
     (old-bg-color (car (gimp-palette-get-background)))

     (set! data-dir (car (gimp-gimprc-query "gimp_dir"))) 
     (filename (string-append data-dir
               "/brushes/r" 
               (number->string width) 
               "x" 
               (number->string height) 
               ".gbr")
               )
     (desc (string-append description " " 
                  (number->string width) 
                  "x" 
                  (number->string height)
                  )
      )
    )

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

; Actual code starts...
    (gimp-palette-set-background '(0 0 0))
    (gimp-drawable-fill drawable BG-IMAGE-FILL)
    (gimp-palette-set-background '(255 255 255))
    (gimp-rect-select img 0 0 width height REPLACE FALSE 0)
    
    (gimp-edit-fill    img drawable) 
    (file-gbr-save 1 img drawable filename "" spacing desc)
    
    (gimp-brushes-refresh) ; My own modification!  You'll need my diff.
    (gimp-brushes-set-brush desc)

; Terminate, restoring old bg.

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-image-delete img)
    )
  )
) 

; Register with the GDB

(script-fu-register "script-fu-make-brush-rectangular"
            "<Toolbox>/Xtns/Script-Fu/Make Brush/Rectangular"
            "Create size of brush"
            "Seth Burgess <sjburges@ou.edu>"
            "Seth Burgess"
            "1997"
            ""
            SF-VALUE "Description" "\"Rectangle\""
            SF-VALUE "Width" "20"
            SF-VALUE "Height" "20"
            SF-VALUE "Spacking" "20"
            )


(define (script-fu-make-brush-rectangular-feathered description width height feathering spacing)
    (begin
        (let* (
            (widthplus (+ width feathering))
            (heightplus (+ height feathering))
            (img (car (gimp-image-new widthplus heightplus GRAY)))
             (drawable (car (gimp-layer-new img widthplus heightplus GRAY_IMAGE "MakeBrush" 100 NORMAL)))

     ; Save old foregound and background colors

     (old-fg-color (car (gimp-palette-get-foreground)))
     (old-bg-color (car (gimp-palette-get-background)))
     
    (set! data-dir (car (gimp-gimprc-query "gimp_dir"))) 
    (filename (string-append data-dir
               "/brushes/r" 
               (number->string width) 
               "x" 
               (number->string height) 
               "f"
               (number->string feathering)
               ".gbr")
               )
     (desc (string-append description " " 
                  (number->string width) 
                  "x" 
                  (number->string height)
                  ","
                  (number->string feathering)
                  )
      )
    )

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

; Actual code starts...
    (gimp-palette-set-background '(0 0 0))
    (gimp-drawable-fill drawable BG-IMAGE-FILL)
    (gimp-palette-set-background '(255 255 255))
    (cond ((< 0 feathering) 
           (gimp-rect-select img (/ feathering 2) (/ feathering 2) width height REPLACE TRUE feathering))
          ((>= 0 feathering)
           (gimp-rect-select img 0 0 width height REPLACE FALSE 0))
          )
    (gimp-edit-fill    img drawable) 
    (file-gbr-save 1 img drawable filename "" 25 desc)
    
    (gimp-brushes-refresh) ; My own modification!  You'll need my diff.
    (gimp-brushes-set-brush desc)

; Terminate, restoring old bg.

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-image-delete img)
    )
  )
) 

; Register with the GDB

(script-fu-register "script-fu-make-brush-rectangular-feathered"
            "<Toolbox>/Xtns/Script-Fu/Make Brush/Rectangular, Feathered"
            "Create size of brush"
            "Seth Burgess <sjburges@ou.edu>"
            "Seth Burgess"
            "1997"
            ""
            SF-VALUE "Description" "\"Rectangle\""
            SF-VALUE "Width" "20"
            SF-VALUE "Height" "20"
            SF-VALUE "Feathering" "4"
            SF-VALUE "Spacing" "25" 
            )

(define (script-fu-make-brush-elliptical description width height spacing)
    (begin
        (let* (
            (img (car (gimp-image-new width height GRAY)))
             (drawable (car (gimp-layer-new img width height GRAY_IMAGE "MakeBrush" 100 NORMAL)))

     ; Save old foregound and background colors

     (old-fg-color (car (gimp-palette-get-foreground)))
     (old-bg-color (car (gimp-palette-get-background)))

     ; Construct variables...

     (set! data-dir (car (gimp-gimprc-query "gimp_dir")))
     (filename (string-append data-dir
			      "/brushes/r" 
			      (number->string width) 
			      "x" 
			      (number->string height) 
			      ".gbr"))
     (desc (string-append description " " 
			  (number->string width) 
			  "x" 
			  (number->string height)
			  )
	   )
     )
	  ; End of variables.  Couple of necessary things here.

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

; Actual code starts...
    (gimp-palette-set-background '(0 0 0))
    (gimp-drawable-fill drawable BG-IMAGE-FILL)
    (gimp-palette-set-background '(255 255 255))
    (gimp-ellipse-select img 0 0 width height REPLACE TRUE FALSE 0)
    
    (gimp-edit-fill    img drawable) 
    (file-gbr-save 1 img drawable filename "" spacing desc)
    
    (gimp-brushes-refresh) ; My own modification!  You'll need my diff.
    (gimp-brushes-set-brush desc)

; Terminate, restoring old bg.

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-image-delete img)
    )
  )
) 

; Register with the GDB

(script-fu-register "script-fu-make-brush-elliptical"
            "<Toolbox>/Xtns/Script-Fu/Make Brush/Elliptical"
            "Create size of brush"
            "Seth Burgess <sjburges@ou.edu>"
            "Seth Burgess"
            "1997"
            ""
            SF-VALUE "Description" "\"Ellipse\""
            SF-VALUE "Width" "20"
            SF-VALUE "Height" "20"
            SF-VALUE "Spacing" "25"
            )


(define (script-fu-make-brush-elliptical-feathered description width height feathering spacing)
    (begin
        (let* (
	    (widthplus (+ feathering width)) ; add 3 for blurring
            (heightplus (+ feathering height))
            (img (car (gimp-image-new widthplus heightplus GRAY)))
             (drawable (car (gimp-layer-new img widthplus heightplus GRAY_IMAGE "MakeBrush" 100 NORMAL)))

     ; Save old foregound and background colors

     (old-fg-color (car (gimp-palette-get-foreground)))
     (old-bg-color (car (gimp-palette-get-background)))

     ; Construct variables...
     (set! data-dir (car (gimp-gimprc-query "gimp_dir")))
     (filename (string-append data-dir
			      "/brushes/r" 
			      (number->string width) 
			      "x" 
			      (number->string height) 
			      "f"
			      (number->string feathering)
			      ".gbr"))
     (desc (string-append description " " 
			  (number->string width) 
			  "x" 
			  (number->string height)
			  " f"
			  (number->string feathering)
			  )
	   )
     
     )
; End of variables.  Couple of necessary things here.

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

; Actual code starts...
    (gimp-palette-set-background '(0 0 0))
    (gimp-drawable-fill drawable BG-IMAGE-FILL)
    (gimp-palette-set-background '(255 255 255))
    (cond ((> feathering 0)   ; keep from taking out gimp with stupid entry. 
        (gimp-ellipse-select img (/ feathering 2) (/ feathering 2) width height REPLACE TRUE TRUE feathering))
          ((<= feathering 0) 
        (gimp-ellipse-select img 0 0 width height REPLACE TRUE FALSE 0)) 
	)
    (gimp-edit-fill    img drawable) 
    (file-gbr-save 1 img drawable filename "" spacing desc)
    
    (gimp-brushes-refresh) ; My own modification!  You'll need my diff.
    (gimp-brushes-set-brush desc)

; Terminate, restoring old bg.

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-image-delete img)
    )
  )
) 

; Register with the GDB

(script-fu-register "script-fu-make-brush-elliptical-feathered"
            "<Toolbox>/Xtns/Script-Fu/Make Brush/Elliptical, Feathered"
            "Create size of brush"
            "Seth Burgess <sjburges@ou.edu>"
            "Seth Burgess"
            "1997"
            ""
            SF-VALUE "Description" "\"Ellipse\""
            SF-VALUE "Width" "20"
            SF-VALUE "Height" "20"
            SF-VALUE "Feathering" "4"
            SF-VALUE "Spacing" "25"
            )
