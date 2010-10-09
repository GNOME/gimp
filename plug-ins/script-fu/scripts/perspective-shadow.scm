; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
;
; perspective-shadow.scm   version 1.2   2000/11/08
;
; Copyright (C) 1997-2000 Sven Neumann <sven@gimp.org>
;
;
; Adds a perspective shadow of the current selection or alpha-channel
; as a layer below the active layer
;

(define (script-fu-perspective-shadow image
                                      drawable
                                      alpha
                                      rel-distance
                                      rel-length
                                      shadow-blur
                                      shadow-color
                                      shadow-opacity
                                      interpolation
                                      allow-resize)
  (let* (
        (shadow-blur (max shadow-blur 0))
        (shadow-opacity (min shadow-opacity 100))
        (shadow-opacity (max shadow-opacity 0))
        (rel-length (abs rel-length))
        (alpha (* (/ alpha 180) *pi*))
        (type (car (gimp-drawable-type-with-alpha drawable)))
        (image-width (car (gimp-image-width image)))
        (image-height (car (gimp-image-height image)))
        (from-selection 0)
        (active-selection 0)
        (shadow-layer 0)
        )

  (gimp-context-push)

  (if (> rel-distance 24) (set! rel-distance 999999))
  (if (= rel-distance rel-length) (set! rel-distance (+ rel-distance 0.01)))

  (gimp-image-undo-group-start image)

  (gimp-layer-add-alpha drawable)
  (if (= (car (gimp-selection-is-empty image)) TRUE)
      (begin
        (gimp-selection-layer-alpha drawable)
        (set! from-selection FALSE))
      (begin
        (set! from-selection TRUE)
        (set! active-selection (car (gimp-selection-save image)))))

  (let* ((selection-bounds (gimp-selection-bounds image))
         (select-offset-x (cadr selection-bounds))
         (select-offset-y (caddr selection-bounds))
         (select-width (- (cadr (cddr selection-bounds)) select-offset-x))
         (select-height (- (caddr (cddr selection-bounds)) select-offset-y))

         (abs-length (* rel-length select-height))
         (abs-distance (* rel-distance select-height))
         (half-bottom-width (/ select-width 2))
         (half-top-width (* half-bottom-width
                          (/ (- rel-distance rel-length) rel-distance)))

         (x0 (+ select-offset-x (+ (- half-bottom-width half-top-width)
                                 (* (cos alpha) abs-length))))
         (y0 (+ select-offset-y (- select-height
                                 (* (sin alpha) abs-length))))
         (x1 (+ x0 (* 2 half-top-width)))
         (y1 y0)
         (x2 select-offset-x)
         (y2 (+ select-offset-y select-height))
         (x3 (+ x2 select-width))
         (y3 y2)

         (shadow-width (+ (- (max x1 x3) (min x0 x2)) (* 2 shadow-blur)))
         (shadow-height (+ (- (max y1 y3) (min y0 y2)) (* 2 shadow-blur)))
         (shadow-offset-x (- (min x0 x2) shadow-blur))
         (shadow-offset-y (- (min y0 y2) shadow-blur)))


    (set! shadow-layer (car (gimp-layer-new image
                                            select-width
                                            select-height
                                            type
                                            "Perspective Shadow"
                                            shadow-opacity
                                            NORMAL-MODE)))


    (gimp-image-insert-layer image shadow-layer -1 -1)
    (gimp-layer-set-offsets shadow-layer select-offset-x select-offset-y)
    (gimp-drawable-fill shadow-layer TRANSPARENT-FILL)
    (gimp-context-set-background shadow-color)
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)
    (gimp-selection-none image)

    (if (= allow-resize TRUE)
        (let* ((new-image-width image-width)
               (new-image-height image-height)
               (image-offset-x 0)
               (image-offset-y 0))

          (if (< shadow-offset-x 0)
              (begin
                (set! image-offset-x (abs shadow-offset-x))
                (set! new-image-width (+ new-image-width image-offset-x))
                ; adjust to new coordinate system
                (set! x0 (+ x0 image-offset-x))
                (set! x1 (+ x1 image-offset-x))
                (set! x2 (+ x2 image-offset-x))
                (set! x3 (+ x3 image-offset-x))
              ))

          (if (< shadow-offset-y 0)
              (begin
                (set! image-offset-y (abs shadow-offset-y))
                (set! new-image-height (+ new-image-height image-offset-y))
                ; adjust to new coordinate system
                (set! y0 (+ y0 image-offset-y))
                (set! y1 (+ y1 image-offset-y))
                (set! y2 (+ y2 image-offset-y))
                (set! y3 (+ y3 image-offset-y))
              ))

          (if (> (+ shadow-width shadow-offset-x) new-image-width)
              (set! new-image-width (+ shadow-width shadow-offset-x)))

          (if (> (+ shadow-height shadow-offset-y) new-image-height)
              (set! new-image-height (+ shadow-height shadow-offset-y)))
          (gimp-image-resize image
                             new-image-width
                             new-image-height
                             image-offset-x
                             image-offset-y)))

    (gimp-drawable-transform-perspective shadow-layer
                      x0 y0
                      x1 y1
                      x2 y2
                      x3 y3
                      TRANSFORM-FORWARD
                      interpolation
                      TRUE 3 TRANSFORM-RESIZE-ADJUST)

    (if (>= shadow-blur 1.0)
        (begin
          (gimp-layer-set-lock-alpha shadow-layer FALSE)
          (gimp-layer-resize shadow-layer
                             shadow-width
                             shadow-height
                             shadow-blur
                             shadow-blur)
          (plug-in-gauss-rle RUN-NONINTERACTIVE
                             image
                             shadow-layer
                             shadow-blur
                             TRUE
                             TRUE))))

  (if (= from-selection TRUE)
      (begin
        (gimp-item-to-selection active-selection 2)
        (gimp-edit-clear shadow-layer)
        (gimp-image-remove-channel image active-selection)))

  (if (and
       (= (car (gimp-layer-is-floating-sel drawable)) 0)
       (= from-selection FALSE))
      (gimp-image-raise-layer image drawable))

  (gimp-image-set-active-layer image drawable)
  (gimp-image-undo-group-end image)
  (gimp-displays-flush)

  (gimp-context-pop)
  )
)

(script-fu-register "script-fu-perspective-shadow"
  _"_Perspective..."
  _"Add a perspective shadow to the selected region (or alpha)"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "2000/11/08"
  "RGB* GRAY*"
  SF-IMAGE       "Image"                        0
  SF-DRAWABLE    "Drawable"                     0
  SF-ADJUSTMENT _"Angle"                        '(45 0 180 1 10 1 0)
  SF-ADJUSTMENT _"Relative distance of horizon" '(5 0.1 24.1 0.1 1 1 1)
  SF-ADJUSTMENT _"Relative length of shadow"    '(1 0.1 24   0.1 1 1 1)
  SF-ADJUSTMENT _"Blur radius"                  '(3 0 1024 1 10 0 0)
  SF-COLOR      _"Color"                        '(0 0 0)
  SF-ADJUSTMENT _"Opacity"                      '(80 0 100 1 10 0 0)
  SF-ENUM       _"Interpolation"                '("InterpolationType" "linear")
  SF-TOGGLE     _"Allow resizing"               FALSE
)

(script-fu-menu-register "script-fu-perspective-shadow"
                         "<Image>/Filters/Light and Shadow/Shadow")
