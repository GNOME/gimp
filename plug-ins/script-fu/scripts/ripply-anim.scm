; "Rippling Image" animation generator (ripply-anim.scm)
; Adam D. Moss (adam@foxbox.org)
; 97/05/18
; Revised by Saul Goode April 2015.
;
; Designed to be used in conjunction with a plugin capable
; of saving animations (i.e. the GIF plugin).
;

(define (script-fu-ripply-anim image drawables displacement num-frames edge-type)
  (let* ((drawable (vector-ref (car (gimp-image-get-selected-drawables image)) 0))
         (width (car (gimp-drawable-get-width drawable)))
         (height (car (gimp-drawable-get-height drawable)))
         (work-image (car (gimp-image-new width
                                          height
                                          (quotient (car (gimp-drawable-type drawable))
                                                    2))))
         (map-layer (car (gimp-layer-new work-image
                                         "Ripple Map"
                                         width
                                         height
                                         (car (gimp-drawable-type drawable))
                                         100
                                         LAYER-MODE-NORMAL))))
    (gimp-context-push)
    (gimp-context-set-paint-mode LAYER-MODE-NORMAL)
    (gimp-context-set-opacity 100.0)
    (gimp-image-undo-disable work-image)

    ; Create a tile-able displacement map in the first layer
    (gimp-context-set-background '(127 127 127))
    (gimp-image-insert-layer work-image map-layer 0 0)
    (gimp-drawable-edit-fill map-layer FILL-BACKGROUND)
    (gimp-drawable-merge-new-filter map-layer "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                    "independent" FALSE "red" 1.0 "alpha" 0.0
                                    "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)
    (plug-in-tile #:run-mode RUN-NONINTERACTIVE #:image work-image #:drawables (vector map-layer) #:new-width (* width 3) #:new-height (* height 3) #:new-image FALSE)
    (gimp-drawable-merge-new-filter map-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 11.2 "std-dev-y" 11.2 "filter" "auto")
    (gimp-drawable-equalize map-layer TRUE)
    (gimp-drawable-merge-new-filter map-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 1.6 "std-dev-y" 1.6 "filter" "auto")
    (gimp-drawable-equalize map-layer TRUE)
    (gimp-image-crop work-image width height width height)

    ; Create the frame layers
    (let loop ((remaining-frames num-frames))
      (unless (zero? remaining-frames)
        (let ((frame-layer (car (gimp-layer-new-from-drawable drawable work-image))))
          (gimp-image-insert-layer work-image frame-layer 0 0)
          (gimp-item-set-name frame-layer
                              (string-append "Frame "
                                             (number->string (+ 1 (- num-frames
                                                                     remaining-frames)))
                                             " (replace)"))
          (let* ((abyss "black")
                 (filter (car (gimp-drawable-filter-new frame-layer "gegl:displace" ""))))

            (if (= edge-type 0) (set! abyss "loop"))
            (if (= edge-type 1) (set! abyss "clamp"))

            (gimp-drawable-filter-configure filter LAYER-MODE-REPLACE 1.0
                                            "amount-x" displacement "amount-x" displacement "abyss-policy" abyss
                                            "sampler-type" "cubic" "displace-mode" "cartesian")
            (gimp-drawable-filter-set-aux-input filter "aux" map-layer)
            (gimp-drawable-filter-set-aux-input filter "aux2" map-layer)
            (gimp-drawable-merge-filter frame-layer filter)
          )
          (gimp-item-set-visible frame-layer TRUE))
        (gimp-drawable-offset map-layer
                              TRUE
                              OFFSET-COLOR
                              (car (gimp-context-get-background))
                              (/ width num-frames)
                              (/ height num-frames))
        (loop (- remaining-frames 1))))

    (gimp-image-remove-layer work-image map-layer)
    (gimp-image-undo-enable work-image)
    (gimp-display-new work-image)

    (gimp-context-pop)))

(script-fu-register-filter "script-fu-ripply-anim"
  _"_Rippling..."
  _"Create a multi-layer image by adding a ripple effect to the current layer"
  "Adam D. Moss (adam@foxbox.org), Saul Goode"
  "Adam D. Moss, Saul Goode"
  "1997, 2015"
  "RGB* GRAY*"
  SF-ONE-DRAWABLE
  SF-ADJUSTMENT _"Ri_ppling strength"  '(3 0 256 1 10 1 0)
  SF-ADJUSTMENT _"_Number of frames"   '(15 0 256 1 10 0 1)
  SF-OPTION     _"Edge _behavior"      '(_"Wrap" _"Smear" _"Black")
  )

(script-fu-menu-register "script-fu-ripply-anim"
                         "<Image>/Filters/Animation/")
