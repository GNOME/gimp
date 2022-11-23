; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Weave script --- make an image look as if it were woven
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


; Copies the specified rectangle from/to the specified drawable

(define (copy-rectangle img
                        drawable
                        x1
                        y1
                        width
                        height
                        dest-x
                        dest-y)
  (ligma-image-select-rectangle img CHANNEL-OP-REPLACE x1 y1 width height)
  (ligma-edit-copy 1 (vector drawable))
  (let* (
         (pasted (ligma-edit-paste drawable FALSE))
         (num-pasted (car pasted))
         (floating-sel (aref (cadr pasted) (- num-pasted 1)))
        )
   (ligma-layer-set-offsets floating-sel dest-x dest-y)
   (ligma-floating-sel-anchor floating-sel)
  )
  (ligma-selection-none img))

; Creates a single weaving tile

(define (create-weave-tile ribbon-width
                           ribbon-spacing
                           shadow-darkness
                           shadow-depth)
  (let* ((tile-size (+ (* 2 ribbon-width) (* 2 ribbon-spacing)))
         (darkness (* 255 (/ (- 100 shadow-darkness) 100)))
         (img (car (ligma-image-new tile-size tile-size RGB)))
         (drawable (car (ligma-layer-new img tile-size tile-size RGB-IMAGE
                                        "Weave tile" 100 LAYER-MODE-NORMAL))))

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(0 0 0))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    ; Create main horizontal ribbon

    (ligma-context-set-foreground '(255 255 255))
    (ligma-context-set-background (list darkness darkness darkness))

    (ligma-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 0
                                 ribbon-spacing
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 ribbon-width)

    (ligma-context-set-gradient-fg-bg-rgb)
    (ligma-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 0 0
				      TRUE
				      (/ (+ (* 2 ribbon-spacing) ribbon-width -1) 2) 0
				      0 0)

    ; Create main vertical ribbon

    (ligma-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 0
                                 ribbon-width
                                 (+ (* 2 ribbon-spacing) ribbon-width))

    (ligma-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 0 0
				      TRUE
				      0 (/ (+ (* 2 ribbon-spacing) ribbon-width -1) 2)
				      0 0)

    ; Create the secondary horizontal ribbon

    (copy-rectangle img
                    drawable
                    0
                    ribbon-spacing
                    (+ ribbon-width ribbon-spacing)
                    ribbon-width
                    (+ ribbon-width ribbon-spacing)
                    (+ (* 2 ribbon-spacing) ribbon-width))

    (copy-rectangle img
                    drawable
                    (+ ribbon-width ribbon-spacing)
                    ribbon-spacing
                    ribbon-spacing
                    ribbon-width
                    0
                    (+ (* 2 ribbon-spacing) ribbon-width))

    ; Create the secondary vertical ribbon

    (copy-rectangle img
                    drawable
                    (+ (* 2 ribbon-spacing) ribbon-width)
                    0
                    ribbon-width
                    (+ ribbon-width ribbon-spacing)
                    ribbon-spacing
                    (+ ribbon-width ribbon-spacing))

    (copy-rectangle img
                    drawable
                    (+ (* 2 ribbon-spacing) ribbon-width)
                    (+ ribbon-width ribbon-spacing)
                    ribbon-width
                    ribbon-spacing
                    ribbon-spacing
                    0)

    ; Done

    (ligma-image-undo-enable img)
    (list img drawable)))

; Creates a complete weaving mask

(define (create-weave width
                      height
                      ribbon-width
                      ribbon-spacing
                      shadow-darkness
                      shadow-depth)
  (let* ((tile (create-weave-tile ribbon-width ribbon-spacing shadow-darkness
                                  shadow-depth))
         (tile-img (car tile))
         (tile-layer (cadr tile))
          (weaving (plug-in-tile RUN-NONINTERACTIVE tile-img 1 (vector tile-layer) width height TRUE)))
    (ligma-image-delete tile-img)
    weaving))

; Creates a single tile for masking

(define (create-mask-tile ribbon-width
                          ribbon-spacing
                          r1-x1
                          r1-y1
                          r1-width
                          r1-height
                          r2-x1
                          r2-y1
                          r2-width
                          r2-height
                          r3-x1
                          r3-y1
                          r3-width
                          r3-height)
  (let* ((tile-size (+ (* 2 ribbon-width) (* 2 ribbon-spacing)))
         (img (car (ligma-image-new tile-size tile-size RGB)))
         (drawable (car (ligma-layer-new img tile-size tile-size RGB-IMAGE
                                        "Mask" 100 LAYER-MODE-NORMAL))))
    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    (ligma-context-set-background '(0 0 0))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)

    (ligma-image-select-rectangle img CHANNEL-OP-REPLACE r1-x1 r1-y1 r1-width r1-height)
    (ligma-image-select-rectangle img CHANNEL-OP-ADD r2-x1 r2-y1 r2-width r2-height)
    (ligma-image-select-rectangle img CHANNEL-OP-ADD r3-x1 r3-y1 r3-width r3-height)

    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)
    (ligma-selection-none img)

    (ligma-image-undo-enable img)

    (list img drawable)))

; Creates a complete mask image

(define (create-mask final-width
                     final-height
                     ribbon-width
                     ribbon-spacing
                     r1-x1
                     r1-y1
                     r1-width
                     r1-height
                     r2-x1
                     r2-y1
                     r2-width
                     r2-height
                     r3-x1
                     r3-y1
                     r3-width
                     r3-height)
  (let* ((tile (create-mask-tile ribbon-width ribbon-spacing
                                 r1-x1 r1-y1 r1-width r1-height
                                 r2-x1 r2-y1 r2-width r2-height
                                 r3-x1 r3-y1 r3-width r3-height))
         (tile-img (car tile))
         (tile-layer (cadr tile))
         (mask (plug-in-tile RUN-NONINTERACTIVE tile-img 1 (vector tile-layer) final-width final-height
                             TRUE)))
    (ligma-image-delete tile-img)
    mask))

; Creates the mask for horizontal ribbons

(define (create-horizontal-mask ribbon-width
                                ribbon-spacing
                                final-width
                                final-height)
  (create-mask final-width
               final-height
               ribbon-width
               ribbon-spacing
               0
               ribbon-spacing
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-width
               0
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-spacing
               ribbon-width
               (+ ribbon-width ribbon-spacing)
               (+ (* 2 ribbon-spacing) ribbon-width)
               (+ ribbon-width ribbon-spacing)
               ribbon-width))

; Creates the mask for vertical ribbons

(define (create-vertical-mask ribbon-width
                              ribbon-spacing
                              final-width
                              final-height)
  (create-mask final-width
               final-height
               ribbon-width
               ribbon-spacing
               (+ (* 2 ribbon-spacing) ribbon-width)
               0
               ribbon-width
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-spacing
               0
               ribbon-width
               ribbon-spacing
               ribbon-spacing
               (+ ribbon-width ribbon-spacing)
               ribbon-width
               (+ ribbon-width ribbon-spacing)))

; Adds a threads layer at a certain orientation to the specified image

(define (create-threads-layer img
                              width
                              height
                              length
                              density
                              orientation)
  (let* ((drawable (car (ligma-layer-new img width height RGBA-IMAGE
                                        "Threads" 100 LAYER-MODE-NORMAL)))
         (dense (/ density 100.0)))
    (ligma-image-insert-layer img drawable 0 -1)
    (ligma-context-set-background '(255 255 255))
    (ligma-drawable-edit-fill drawable FILL-BACKGROUND)
    (plug-in-noisify RUN-NONINTERACTIVE img drawable FALSE dense dense dense dense)
    (plug-in-c-astretch RUN-NONINTERACTIVE img drawable)
    (cond ((eq? orientation 'horizontal)
           (plug-in-gauss-rle RUN-NONINTERACTIVE img drawable length TRUE FALSE))
          ((eq? orientation 'vertical)
           (plug-in-gauss-rle RUN-NONINTERACTIVE img drawable length FALSE TRUE)))
    (plug-in-c-astretch RUN-NONINTERACTIVE img drawable)
    drawable))

(define (create-complete-weave width
                               height
                               ribbon-width
                               ribbon-spacing
                               shadow-darkness
                               shadow-depth
                               thread-length
                               thread-density
                               thread-intensity)
  (let* ((weave (create-weave width height ribbon-width ribbon-spacing
                              shadow-darkness shadow-depth))
         (w-img (car weave))
         (w-layer (cadr weave))

         (h-layer (create-threads-layer w-img width height thread-length
                                        thread-density 'horizontal))
         (h-mask (car (ligma-layer-create-mask h-layer ADD-MASK-WHITE)))

         (v-layer (create-threads-layer w-img width height thread-length
                                        thread-density 'vertical))
         (v-mask (car (ligma-layer-create-mask v-layer ADD-MASK-WHITE)))

         (hmask (create-horizontal-mask ribbon-width ribbon-spacing
                                        width height))
         (hm-img (car hmask))
         (hm-layer (cadr hmask))

         (vmask (create-vertical-mask ribbon-width ribbon-spacing width height))
         (vm-img (car vmask))
         (vm-layer (cadr vmask)))

    (ligma-layer-add-mask h-layer h-mask)
    (ligma-selection-all hm-img)
    (ligma-edit-copy 1 (vector hm-layer))
    (ligma-image-delete hm-img)
    (let* (
           (pasted (ligma-edit-paste h-mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (ligma-floating-sel-anchor floating-sel)
    )
    (ligma-layer-set-opacity h-layer thread-intensity)
    (ligma-layer-set-mode h-layer LAYER-MODE-MULTIPLY)

    (ligma-layer-add-mask v-layer v-mask)
    (ligma-selection-all vm-img)
    (ligma-edit-copy 1 (vector vm-layer))
    (ligma-image-delete vm-img)
    (let* (
           (pasted (ligma-edit-paste v-mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (ligma-floating-sel-anchor floating-sel)
    )
    (ligma-layer-set-opacity v-layer thread-intensity)
    (ligma-layer-set-mode v-layer LAYER-MODE-MULTIPLY)

    ; Uncomment this if you want to keep the weaving mask image
    ; (ligma-display-new (car (ligma-image-duplicate w-img)))

    (list w-img
          (car (ligma-image-flatten w-img)))))

; The main weave function

(define (script-fu-weave img
                         drawable
                         ribbon-width
                         ribbon-spacing
                         shadow-darkness
                         shadow-depth
                         thread-length
                         thread-density
                         thread-intensity)
  (ligma-context-push)
  (ligma-image-undo-group-start img)

  (let* (
        (d-img (car (ligma-item-get-image drawable)))
        (d-width (car (ligma-drawable-get-width drawable)))
        (d-height (car (ligma-drawable-get-height drawable)))
        (d-offsets (ligma-drawable-get-offsets drawable))

        (weaving (create-complete-weave d-width
                                        d-height
                                        ribbon-width
                                        ribbon-spacing
                                        shadow-darkness
                                        shadow-depth
                                        thread-length
                                        thread-density
                                        thread-intensity))
        (w-img (car weaving))
        (w-layer (cadr weaving))
        )

    (ligma-context-set-paint-mode LAYER-MODE-NORMAL)
    (ligma-context-set-opacity 100.0)
    (ligma-context-set-feather FALSE)

    (ligma-selection-all w-img)
    (ligma-edit-copy 1 (vector w-layer))
    (ligma-image-delete w-img)
    (let* (
           (pasted (ligma-edit-paste drawable FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (ligma-layer-set-offsets floating-sel
                                  (car d-offsets)
                                  (cadr d-offsets))
          (ligma-layer-set-mode floating-sel LAYER-MODE-MULTIPLY)
          (ligma-floating-sel-to-layer floating-sel)
    )
  )
  (ligma-context-pop)
  (ligma-image-undo-group-end img)
  (ligma-displays-flush)
)

(script-fu-register "script-fu-weave"
  _"_Weave..."
  _"Create a new layer filled with a weave effect to be used as an overlay or bump map"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "June 1997"
  "RGB* GRAY*"
  SF-IMAGE       "Image to Weave"    0
  SF-DRAWABLE    "Drawable to Weave" 0
  SF-ADJUSTMENT _"Ribbon width"     '(30  0 256 1 10 1 1)
  SF-ADJUSTMENT _"Ribbon spacing"   '(10  0 256 1 10 1 1)
  SF-ADJUSTMENT _"Shadow darkness"  '(75  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Shadow depth"     '(75  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Thread length"    '(200 0 256 1 10 1 1)
  SF-ADJUSTMENT _"Thread density"   '(50  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Thread intensity" '(100 0 100 1 10 1 1)
)

(script-fu-menu-register "script-fu-weave"
                         "<Image>/Filters/Artistic")
