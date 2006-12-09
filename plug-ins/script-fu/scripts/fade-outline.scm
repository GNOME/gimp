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
; GIMP - The GNU Image Manipulation Program
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

(define (script-fu-fade-outline inImage
                                inLayer
                                inBorderSize
                                inFadeFrom
                                inFadeTo
                                inGrowingSelection
                                inApplyMask
                                inClearUnselected)

  (let* (
        (l-idx 0)
        (l-has-selection TRUE)
        (l-from-gray)
        (l-to-gray)
        (l-step)
        (l-gray)
        (l-mask)
        (l-orig-selection)
        )

    (gimp-context-push)

    ; check Fade from and To Values (and force values from 0% to 100%)
    (if (> inFadeFrom 100) (set! inFadeFrom 100) )
    (if (< inFadeFrom 0)   (set! inFadeFrom 0) )
    (if (> inFadeTo 100)   (set! inFadeTo 100) )
    (if (< inFadeTo 0)     (set! inFadeTo 0) )

    (set! l-from-gray (* inFadeFrom 255))
    (set! l-to-gray (* inFadeTo 255))
    (set! l-step (/  (- l-from-gray l-to-gray) (+ inBorderSize 1)))
    (set! l-gray l-to-gray)

    ; do nothing if the layer is a layer mask
    (if (= (car (gimp-drawable-is-layer-mask inLayer)) 0)
        (begin

          (gimp-image-undo-group-start inImage)

          ; if the layer has no alpha add alpha channel
          (if (= (car (gimp-drawable-has-alpha inLayer)) FALSE)
              (gimp-layer-add-alpha inLayer)
          )

          ; if the layer is the floating selection convert to normal layer
          ; because floating selection cant have a layer mask
          (if (> (car (gimp-layer-is-floating-sel inLayer)) 0)
              (gimp-floating-sel-to-layer inLayer)
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
          (gimp-layer-remove-mask inLayer 0)

          (if (= inClearUnselected  TRUE)
              (set! l-mask (car (gimp-layer-create-mask inLayer ADD-BLACK-MASK)))
              (set! l-mask (car (gimp-layer-create-mask inLayer ADD-WHITE-MASK)))
          )

          (gimp-layer-add-mask inLayer l-mask)

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
                 (set! l-gray l-from-gray)
             )
             (gimp-context-set-background (list (/ l-gray 100) (/ l-gray 100) (/ l-gray 100)))
             (gimp-edit-fill l-mask BACKGROUND-FILL)
             (set! l-idx (+ l-idx 1))
             (set! l-gray (+ l-gray l-step))
             (gimp-selection-shrink inImage 1)
             ; check if selection has shrinked to none
             (if (= (car (gimp-selection-is-empty inImage)) TRUE)
                 (set! l-idx (+ inBorderSize 100))     ; break the while loop
             )
          )

          (if (= inGrowingSelection  TRUE)
              (begin
                (gimp-selection-load l-orig-selection)
                (gimp-context-set-background (list (/ l-to-gray 100) (/ l-to-gray 100) (/ l-to-gray 100)))
                (gimp-edit-fill l-mask BACKGROUND-FILL)
                (gimp-selection-grow inImage inBorderSize)
                (gimp-selection-invert inImage)
                (if (= inClearUnselected  TRUE)
                    (begin
                      ;(gimp-context-set-background (list (/ l-from-gray 100) (/ l-from-gray 100) (/ l-from-gray 100)))
                      (gimp-context-set-background (list 0 0 0))
                    )
                    (gimp-context-set-background (list 255 255 255))
                 )
                (gimp-edit-fill l-mask BACKGROUND-FILL)
                (gimp-image-remove-channel inImage l-orig-selection)
              )
          )

          (if (=  inApplyMask TRUE)
              (gimp-layer-remove-mask inLayer 0)
          )

          (if (= l-has-selection FALSE)
              (gimp-selection-none inImage)
          )

          (gimp-image-undo-group-end inImage)
          (gimp-displays-flush)
        )
    )

    (gimp-context-pop)
  )
)


(script-fu-register
  "script-fu-fade-outline"
  _"_Fade to Layer Mask..."
  _"Create a layermask that fades the edges of the selected region (or alpha)"
  "Wolfgang Hofer <hof@hotbot.com>"
  "Wolfgang Hofer"
  "10 Nov 1999"
  "RGB* GRAY*"
  SF-IMAGE      "The image"                  0
  SF-DRAWABLE   "The layer"                  0
  SF-ADJUSTMENT _"Border size"               '(10 1 300 1 10 0 1)
  SF-ADJUSTMENT _"Fade from %"               '(100 0 100 1 10 0 0)
  SF-ADJUSTMENT _"Fade to %"                 '(0 0 100 1 10 0 0)
  SF-TOGGLE     _"Use growing selection"     FALSE
  SF-TOGGLE     _"Apply generated layermask" FALSE
  SF-TOGGLE     _"Clear unselected maskarea" TRUE
)

(script-fu-menu-register "script-fu-fade-outline"
                         "<Image>/Filters/Selection")
