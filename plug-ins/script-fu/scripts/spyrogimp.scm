;; spyrogimp.scm -*-scheme-*-
;; Draws Spirographs, Epitrochoids and Lissajous Curves.
;; More info at http://www.wisdom.weizmann.ac.il/~elad/spyrogimp/
;; Version 1.2
;;
;; Copyright (C) 2003 by Elad Shahar <elad@wisdom.weizmann.ac.il>
;;
;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License
;; as published by the Free Software Foundation; either version 2
;; of the License, or (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; This routine is invoked by a dialog.
; It is the main routine in this file.
(define (script-fu-spyrogimp img drw
                             type shape
                             oteeth iteeth
                             margin hole-ratio start-angle
                             tool brush
                             color-method color grad)

  ; Internal function to draw the spyro.
  (define (script-fu-spyrogimp-internal img drw
               x1 y1 x2 y2   ; Bounding box.
               type          ; = 0 (Spirograph), 1 (Epitrochoid), 2(Lissajous) .
               shape         ; = 0 (Circle), 1 (Frame), >2 (Polygons) .
               oteeth iteeth ; Outer and inner teeth.
               margin hole-ratio
               start-angle   ; 0 <= start-angle < 360 .
               tool          ; = 0 (Pencil), 1 (Brush), 2 (Airbrush) .
               brush
               color-method  ; = 0 (Single color), 1 (Grad. Loop Sawtooth),
                             ;   2 (Grad. Loop triangle) .
               color         ; Used when color-method = Single color .
               grad          ; Gradient used in Gradient color methods.
          )


    ; This function returns a list of samples according to the gradient.
    (define (get-gradient steps color-method grad)
      (if (= color-method 1)
          ; option 1
          ; Just return the gradient
          (gimp-gradient-get-uniform-samples grad (min steps 50) FALSE)

          ; option 2
          ; The returned list is such that the gradient appears two times, once
          ; in the normal order and once in reverse. This way there are no color
          ; jumps if we go beyond the edge
          (let* (
                ; Sample the gradient into array "gr".
                (gr (gimp-gradient-get-uniform-samples grad
                                                       (/ (min steps 50) 2)
                                                       FALSE))

                (grn (car gr))  ; length of sample array.
                (gra (cadr gr)) ; array of color samples (R1,G1,B1,A1, R2,....)

                ; Allocate array gra-new of size  (2 * grn) - 8,
                ; but since each 4 items is actually one (RGBA) tuple,
                ; it contains 2x - 2 entries.
                (grn-new (+ grn grn -8))
                (gra-new (cons-array grn-new 'double))

                (gr-index 0)
                (gr-index2 0)
                )

            ; Copy original array gra to gra_new.
            (while (< gr-index grn)
               (aset gra-new gr-index (aref gra gr-index))
               (set! gr-index (+ 1 gr-index))
            )

            ; Copy second time, but in reverse
            (set! gr-index2 (- gr-index 8))
            (while (< gr-index grn-new)
               (aset gra-new gr-index (aref gra gr-index2))
               (set! gr-index (+ 1 gr-index))
               (set! gr-index2 (+ 1 gr-index2))

               (if (= (fmod gr-index 4) 0)
                 (set! gr-index2 (- gr-index2 8))
               )
            )

            ; Return list.
            (list grn-new gra-new)
          )
      )
    )


    (let* (
          (steps (+ 1 (lcm oteeth iteeth)))
          (*points* (cons-array (* steps 2) 'double))

          (ot 0)                         ; current outer tooth
          (cx 0)                         ; Current x,y
          (cy 0)

          ; If its a polygon or frame, how many sides does it have.
          (poly (if (= shape 1) 4   ; A frame has four sides.
                                (if (> shape 1) (+ shape 1) 0)))

          (2pi (* 2 *pi*))

          (drw-width (- x2 x1))
          (drw-height (- y2 y1))
          (half-width (/ drw-width 2))
          (half-height (/ drw-height 2))
          (midx (+ x1 half-width))
          (midy (+ y1 half-height))

          (hole (* hole-ratio
                   (- (/ (min drw-width drw-height) 2) margin)
                )
          )
          (irad (+ hole margin))

          (radx (- half-width irad))  ;
          (rady (- half-height irad)) ;

          (gradt (get-gradient steps color-method grad))
          (grada (cadr gradt)) ; Gradient array.
          (gradn (car gradt))  ; Number of entries of gradients.

          ; Indexes
          (grad-index 0)  ; for array: grada
          (point-index 0) ; for array: *points*
          (index 0)
          )

      ; Do one step of the loop.
      (define (calc-and-step!)
        (let* (
              (oangle (* 2pi (/ ot oteeth)) )
              (shifted-oangle (+ oangle (* 2pi (/ start-angle 360))) )
              (xfactor (cos shifted-oangle))
              (yfactor (sin shifted-oangle))
              (lenfactor 1)
              (ofactor (/ (+ oteeth iteeth) iteeth))

              ; The direction of the factor changes according
              ; to whether the type is a sypro or an epitcorhoid.
              (mfactor (if (= type 0) (- ofactor) ofactor))
              )

          ; If we are drawing a polygon then compute a contortion
          ; factor "lenfactor" which deforms the standard circle.
          (if (> poly 2)
            (let* (
                  (pi4 (/ *pi* poly))
                  (pi2 (* pi4 2))

                  (oanglemodpi2 (fmod (+ oangle
                                        (if (= 1 (fmod poly 2))
                                           0 ;(/ pi4 2)
                                           0
                                        )
                                      )
                                      pi2))
                  )

                  (set! lenfactor (/ ( if (= shape 1) 1 (cos pi4) )
                                     (cos
                                       (if (< oanglemodpi2 pi4)
                                         oanglemodpi2
                                         (- pi2 oanglemodpi2)
                                       )
                                     )
                                  )
                  )
            )
          )

          (if (= type 2)
            (begin  ; Lissajous
              (set! cx (+ midx
                          (* half-width (cos shifted-oangle)) ))
              (set! cy (+ midy
                          (* half-height (cos (* mfactor oangle))) ))
            )
            (begin  ; Spyrograph or Epitrochoid
             (set! cx (+ midx
                         (* radx xfactor lenfactor)
                         (* hole (cos (* mfactor oangle) ) ) ))
             (set! cy (+ midy
                         (* rady yfactor lenfactor)
                         (* hole (sin (* mfactor oangle) ) ) ))
            )
          )

        ;; Advance teeth
        (set! ot (+ ot 1))
        )
      )


      ;; Draw all the points in *points* with appropriate tool.
      (define (flush-points len)
        (if (= tool 0)
          (gimp-pencil drw len *points*)              ; Use pencil
          (if (= tool 1)
            (gimp-paintbrush-default drw len *points*); use paintbrush
            (gimp-airbrush-default drw len *points*)  ; use airbrush
          )
        )

        ; Reset points array, but copy last point to first
        ; position so it will connect the next time.
        (aset *points* 0 (aref *points* (- point-index 2)))
        (aset *points* 1 (aref *points* (- point-index 1)))
        (set! point-index 2)
      )

   ;;
   ;; Execution starts here.
   ;;

      (gimp-context-push)

      (gimp-image-undo-group-start img)

      ; Set new color, brush, opacity, paint mode.
      (gimp-context-set-foreground color)
      (gimp-context-set-brush (car brush))
      (gimp-context-set-opacity (* 100 (car (cdr brush))))
      (gimp-context-set-paint-mode (car (cdr (cdr (cdr brush)))))

      (while (< index steps)

          (calc-and-step!)

          (aset *points* point-index cx)
          (aset *points* (+ point-index 1) cy)
          (set! point-index (+ point-index 2))

          ; Change color and draw points if using gradient.
          (if (< 0 color-method)  ; use gradient.
             (if (< (/ (+ grad-index 4) gradn) (/ index steps))
               (begin
                (gimp-context-set-foreground
                  (list
                    (* 255 (aref grada grad-index))
                    (* 255 (aref grada (+ 1 grad-index)) )
                    (* 255 (aref grada (+ 2 grad-index)) )
                  )
                )
                (gimp-context-set-opacity (* 100 (aref grada (+ 3 grad-index) ) )  )
                (set! grad-index (+ 4 grad-index))

                ; Draw points
                (flush-points point-index)
               )
             )
          )

          (set! index (+ index 1))
      )


      ; Draw remaining points.
      (flush-points point-index)

      (gimp-image-undo-group-end img)
      (gimp-displays-flush)

      (gimp-context-pop)
    )
  )

  (let* (
        ; Get current selection to determine where to draw.
        (bounds (cdr (gimp-selection-bounds img)))
        (x1 (car bounds))
        (y1 (cadr bounds))
        (x2 (caddr bounds))
        (y2 (car (cdddr bounds)))
        )

    (set! oteeth (trunc (+ oteeth 0.5)))
    (set! iteeth (trunc (+ iteeth 0.5)))

    (script-fu-spyrogimp-internal img drw
             x1 y1 x2 y2
             type shape
             oteeth iteeth
             margin hole-ratio start-angle
             tool brush
             color-method color grad)
  )
)



(script-fu-register "script-fu-spyrogimp"
  _"_Spyrogimp..."
  _"Add Spirographs, Epitrochoids, and Lissajous Curves to the current layer"
  "Elad Shahar <elad@wisdom.weizmann.ac.il>"
  "Elad Shahar"
  "June 2003"
  "RGB*, INDEXED*, GRAY*"
  SF-IMAGE       "Image"         0
  SF-DRAWABLE    "Drawable"      0

  SF-OPTION     _"Type"          '(_"Spyrograph"
                                   _"Epitrochoid"
                                   _"Lissajous")
  SF-OPTION     _"Shape"         '(_"Circle"
                                    _"Frame"
                                   _"Triangle"
                                   _"Square"
                                   _"Pentagon"
                                   _"Hexagon"
                                   _"Polygon: 7 sides"
                                   _"Polygon: 8 sides"
                                   _"Polygon: 9 sides"
                                   _"Polygon: 10 sides")
  SF-ADJUSTMENT _"Outer teeth"   '(86 1 120 1 10 0 0)
  SF-ADJUSTMENT _"Inner teeth"   '(70 1 120 1 10 0 0)
  SF-ADJUSTMENT _"Margin (pixels)" '(0 -10000 10000 1 10 0 1)
  SF-ADJUSTMENT _"Hole ratio"    '(0.4 0.0 1.0 0.01 0.1 2 0)
  SF-ADJUSTMENT _"Start angle"   '(0 0 359 1 10 0 0)

  SF-OPTION     _"Tool"          '(_"Pencil"
                                   _"Brush"
                                   _"Airbrush")
  SF-BRUSH      _"Brush"         '("Circle (01)" 1.0 -1 0)

  SF-OPTION     _"Color method"  '(_"Solid Color"
                                   _"Gradient: Loop Sawtooth"
                                   _"Gradient: Loop Triangle")
  SF-COLOR      _"Color"         '(0 0 0)
  SF-GRADIENT   _"Gradient"       "Deep Sea"
)

;; End of syprogimp.scm

(script-fu-menu-register "script-fu-spyrogimp"
                         "<Image>/Filters/Render")
