; fade-outline.scm
; version 1.1.11a
;
; This GIMP script_fu operates on a single Layer
; It blends the outline boarder from one to another transparency level
; by adding a layer_mask that follows the shape of the selection.
; usually from 100% (white is full opaque) to 0% (black is full transparent)
;
; The user can specify the thickness of the fading border
; that is used to blend from transparent to opaque
; and the Fading direction (shrink or grow).
;
; The outline is taken from the current selection
; or from the layers alpha channel if no selection is active.
;
; Optional you may keep the generated layermask or apply
; it to the layer 

;
; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


; Define the main function:

(define (script-fu-fade-outline   inImage
                                  inLayer
                                  inBorderSize
                                  inFadeFrom
                                  inFadeTo
                                  inGrowingSelection
                                  inApplyMask
                                  inClearUnselected
        )

        (let* ((l-idx 0)
               (l-old-bg-color (car (gimp-palette-get-background)))
               (l-has-selection TRUE)
              )
              
	; check Fade from and To Values (and force values from 0% to 100%)
	(if (> inFadeFrom 100) (begin (set! inFadeFrom 100 )) )
	(if (< inFadeFrom 0)   (begin (set! inFadeFrom 0 )) )
	(if (> inFadeTo 100) (begin (set! inFadeTo 100 )) )
	(if (< inFadeTo 0)   (begin (set! inFadeTo 0 )) )
         
	(set! l-from-gray (* inFadeFrom 255))
	(set! l-to-gray (* inFadeTo 255))
        (set! l-step (/  (- l-from-gray l-to-gray) (+ inBorderSize 1)))
        (set! l-gray l-to-gray)

        ; do nothing if the layer is a layer mask
        (if (= (car (gimp-drawable-is-layer-mask inLayer)) 0)
            (begin

              (gimp-undo-push-group-start inImage)
              
              ; if the layer has no alpha add alpha channel
              (if (= (car (gimp-drawable-has-alpha inLayer)) FALSE)
                  (begin
                     (gimp-layer-add-alpha inLayer)
                  )
               )

             ; if the layer is the floating selection convert to normal layer
              ; because floating selection cant have a layer mask
              (if (> (car (gimp-layer-is-floating-sel inLayer)) 0)
                  (begin
                     (gimp-floating-sel-to-layer inLayer)
                  )
               )

              ; if there is no selection we use the layers alpha channel
              (if (= (car (gimp-selection-is-empty inImage)) TRUE)
                  (begin
                     (set! l-has-selection FALSE)
                     (gimp-selection-layer-alpha inLayer)
                  )
               )

               ;
              (gimp-selection-sharpen inImage)

               ; apply the existing mask before creating a new one
              (gimp-image-remove-layer-mask inImage inLayer 0)

              (if (= inClearUnselected  TRUE)
                  (begin
                    (set! l-mask (car (gimp-layer-create-mask inLayer BLACK-MASK)))
                   )
                  (begin
                    (set! l-mask (car (gimp-layer-create-mask inLayer WHITE-MASK)))
                  )
               )

              (gimp-image-add-layer-mask inImage inLayer l-mask)

              (if (= inGrowingSelection  TRUE)
                  (begin
	            (set! l-gray l-from-gray)
	            (set! l-from-gray l-to-gray)
                    (set! l-to-gray l-gray)
                    (set! l-step (/  (- l-from-gray l-to-gray) (+ inBorderSize 1)))
                    (set! l-orig-selection  (car (gimp-selection-save inImage)))
                    (gimp-selection-invert inImage)
                  )
               )
                                           
              (while (<= l-idx inBorderSize)
                 (if (= l-idx inBorderSize)
                     (begin
                        (set! l-gray l-from-gray)
                      )
                  )
                  (gimp-palette-set-background (list (/ l-gray 100) (/ l-gray 100) (/ l-gray 100)))
                  (gimp-edit-fill l-mask BG-IMAGE-FILL)
                  (set! l-idx (+ l-idx 1))
                  (set! l-gray (+ l-gray l-step))
                  (gimp-selection-shrink inImage 1)
                  ; check if selection has shrinked to none
                  (if (= (car (gimp-selection-is-empty inImage)) TRUE)
                      (begin
                         (set! l-idx (+ inBorderSize 100))     ; break the while loop
                      )
                   )

              )
              
              (if (= inGrowingSelection  TRUE)
                  (begin
                    (gimp-selection-load l-orig-selection)
                    (gimp-palette-set-background (list (/ l-to-gray 100) (/ l-to-gray 100) (/ l-to-gray 100)))
                    (gimp-edit-fill l-mask BG-IMAGE-FILL)
                    (gimp-selection-grow inImage inBorderSize)
                    (gimp-selection-invert inImage)
        	    (if (= inClearUnselected  TRUE)
                	(begin
                          ;(gimp-palette-set-background (list (/ l-from-gray 100) (/ l-from-gray 100) (/ l-from-gray 100)))
                          (gimp-palette-set-background (list 0 0 0))
                	 )
                	(begin
                          (gimp-palette-set-background (list 255 255 255))
                	)
        	     )
                    (gimp-edit-fill l-mask BG-IMAGE-FILL)
                    (gimp-image-remove-channel inImage l-orig-selection)
                  )
               )

              (if (=  inApplyMask TRUE)
                  (begin
                    (gimp-image-remove-layer-mask inImage inLayer 0)
                  )
               )

              (if (= l-has-selection FALSE)
                  (gimp-selection-none inImage)
              )

             (gimp-palette-set-background l-old-bg-color)
             (gimp-undo-push-group-end inImage)
             (gimp-displays-flush)
             )
        ))
)




; Register the function with the GIMP:
; ------------------------------------

(script-fu-register
    "script-fu-fade-outline"
    _"<Image>/Script-Fu/Selection/Fade Outline..."
    "Blend the Layers outline border from one alpha value (opaque) to another (transparent) by generating a Layermask"
    "Wolfgang Hofer <hof@hotbot.com>"
    "Wolfgang Hofer"
    "10 Nov 1999"
    "RGB* GRAY*"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-VALUE _"Border Size" "10"
    SF-VALUE _"Fade From (100%-0%)" "100"
    SF-VALUE _"Fade To   (0%-100%)" "0"
    SF-TOGGLE _"Use Growing Selection" FALSE
    SF-TOGGLE _"Apply Generated Layermask" FALSE
    SF-TOGGLE _"Clear Unselected Maskarea" TRUE
)
