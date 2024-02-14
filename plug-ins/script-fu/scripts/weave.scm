; GIMP - The GNU Image Manipulation Program
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
  (gimp-image-select-rectangle img CHANNEL-OP-REPLACE x1 y1 width height)
  (gimp-edit-copy 1 (vector drawable))
  (let* (
         (pasted (gimp-edit-paste drawable FALSE))
         (num-pasted (car pasted))
         (floating-sel (aref (cadr pasted) (- num-pasted 1)))
        )
   (gimp-layer-set-offsets floating-sel dest-x dest-y)
   (gimp-floating-sel-anchor floating-sel)
  )
  (gimp-selection-none img))

; Creates a single weaving tile

(define (create-weave-tile ribbon-width
                           ribbon-spacing
                           shadow-darkness
                           shadow-depth)
  (let* ((tile-size (+ (* 2 ribbon-width) (* 2 ribbon-spacing)))
         (darkness (* 255 (/ (- 100 shadow-darkness) 100)))
         (img (car (gimp-image-new tile-size tile-size RGB)))
         (drawable (car (gimp-layer-new img tile-size tile-size RGB-IMAGE
                                        "Weave tile" 100 LAYER-MODE-NORMAL))))

    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    ; Create main horizontal ribbon

    (gimp-context-set-foreground '(255 255 255))
    (gimp-context-set-background (list darkness darkness darkness))

    (gimp-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 0
                                 ribbon-spacing
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 ribbon-width)

    (gimp-context-set-gradient-fg-bg-rgb)
    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 1 0
				      TRUE
				      (/ (+ (* 2 ribbon-spacing) ribbon-width -1) 2) 0
				      0 0)

    ; Create main vertical ribbon

    (gimp-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 0
                                 ribbon-width
                                 (+ (* 2 ribbon-spacing) ribbon-width))

    (gimp-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 1 0
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

    (gimp-image-undo-enable img)
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
    (gimp-image-delete tile-img)
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
         (img (car (gimp-image-new tile-size tile-size RGB)))
         (drawable (car (gimp-layer-new img tile-size tile-size RGB-IMAGE
                                        "Mask" 100 LAYER-MODE-NORMAL))))
    (gimp-image-undo-disable img)
    (gimp-image-insert-layer img drawable 0 0)

    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)

    (gimp-image-select-rectangle img CHANNEL-OP-REPLACE r1-x1 r1-y1 r1-width r1-height)
    (gimp-image-select-rectangle img CHANNEL-OP-ADD r2-x1 r2-y1 r2-width r2-height)
    (gimp-image-select-rectangle img CHANNEL-OP-ADD r3-x1 r3-y1 r3-width r3-height)

    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)
    (gimp-selection-none img)

    (gimp-image-undo-enable img)

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
    (gimp-image-delete tile-img)
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
  (let* ((drawable (car (gimp-layer-new img width height RGBA-IMAGE
                                        "Threads" 100 LAYER-MODE-NORMAL)))
         (dense (/ density 100.0)))
    (gimp-image-insert-layer img drawable 0 -1)
    (gimp-context-set-background '(255 255 255))
    (gimp-drawable-edit-fill drawable FILL-BACKGROUND)
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
         (h-mask (car (gimp-layer-create-mask h-layer ADD-MASK-WHITE)))

         (v-layer (create-threads-layer w-img width height thread-length
                                        thread-density 'vertical))
         (v-mask (car (gimp-layer-create-mask v-layer ADD-MASK-WHITE)))

         (hmask (create-horizontal-mask ribbon-width ribbon-spacing
                                        width height))
         (hm-img (car hmask))
         (hm-layer (cadr hmask))

         (vmask (create-vertical-mask ribbon-width ribbon-spacing width height))
         (vm-img (car vmask))
         (vm-layer (cadr vmask)))

    (gimp-layer-add-mask h-layer h-mask)
    (gimp-selection-all hm-img)
    (gimp-edit-copy 1 (vector hm-layer))
    (gimp-image-delete hm-img)
    (let* (
           (pasted (gimp-edit-paste h-mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )
    (gimp-layer-set-opacity h-layer thread-intensity)
    (gimp-layer-set-mode h-layer LAYER-MODE-MULTIPLY)

    (gimp-layer-add-mask v-layer v-mask)
    (gimp-selection-all vm-img)
    (gimp-edit-copy 1 (vector vm-layer))
    (gimp-image-delete vm-img)
    (let* (
           (pasted (gimp-edit-paste v-mask FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
     (gimp-floating-sel-anchor floating-sel)
    )
    (gimp-layer-set-opacity v-layer thread-intensity)
    (gimp-layer-set-mode v-layer LAYER-MODE-MULTIPLY)

    ; Uncomment this if you want to keep the weaving mask image
    ; (gimp-display-new (car (gimp-image-duplicate w-img)))

    (list w-img
          (car (gimp-image-flatten w-img)))))

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
  (gimp-context-push)
  (gimp-image-undo-group-start img)

  (let* (
        (d-img (car (gimp-item-get-image drawable)))
        (d-width (car (gimp-drawable-get-width drawable)))
        (d-height (car (gimp-drawable-get-height drawable)))
        (d-offsets (gimp-drawable-get-offsets drawable))

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

    (gimp-context-set-paint-mode LAYER-MODE-NORMAL)
    (gimp-context-set-opacity 100.0)
    (gimp-context-set-feather FALSE)

    (gimp-selection-all w-img)
    (gimp-edit-copy 1 (vector w-layer))
    (gimp-image-delete w-img)
    (let* (
           (pasted (gimp-edit-paste drawable FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
          (gimp-layer-set-offsets floating-sel
                                  (car d-offsets)
                                  (cadr d-offsets))
          (gimp-layer-set-mode floating-sel LAYER-MODE-MULTIPLY)
          (gimp-floating-sel-to-layer floating-sel)
    )
  )
  (gimp-context-pop)
  (gimp-image-undo-group-end img)
  (gimp-displays-flush)
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
