; fade-outline.scm
; version 1.1
;
; This GIMP script_fu operates on a single Layer
; It makes it's outline boarder transparent by
; adding a layer mask that is black (transparent) at the outline
; and blends into white (opaque) at the inner regions of the
; shape. The user can specify the thickness of the fading border
; that is used to blend from transparent to opaque.
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
                                  inApplyMask
                                  inClearUnselected
        )

        (let* ((l-idx 0)
               (l-step (/ 25500 (+ inBorderSize 1)))
               (l-gray l-step)
               (l-old-bg-color (car (gimp-palette-get-background)))
               (l-has-selection TRUE)            

              )

        ; do nothing if the layer is a layer mask
        (if (= (car (gimp-drawable-layer-mask inLayer)) 0)
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
                     (gimp-selection-layer-alpha inImage inLayer)
                  )
               )

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
                                           
              (while (<= l-idx inBorderSize)
                 (if (= l-idx inBorderSize)
                     (begin
                        (set! l-gray 25500)
                      )
                  )
                  (gimp-palette-set-background (list (/ l-gray 100) (/ l-gray 100) (/ l-gray 100)))
                  (gimp-edit-fill inImage l-mask)
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
    "<Image>/Script-Fu/Selection/Fade Outline"
    "Blend the Layers outline border to transparent"
    "Wolfgang Hofer <wolfgang.hofer@siemens.at>"
    "Wolfgang Hofer"
    "19 May 1998"
    "RGB RGBA GRAY GRAYA"
    SF-IMAGE "The Image" 0
    SF-DRAWABLE "The Layer" 0
    SF-VALUE "Border Size" "10"
    SF-TOGGLE "Apply generated Layermask?" FALSE
    SF-TOGGLE "Clear unselected Maskarea?" TRUE
)
