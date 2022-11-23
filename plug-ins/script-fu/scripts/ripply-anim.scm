; "Rippling Image" animation generator (ripply-anim.scm)
; Adam D. Moss (adam@foxbox.org)
; 97/05/18
; Revised by Saul Goode April 2015.
;
; Designed to be used in conjunction with a plugin capable
; of saving animations (i.e. the GIF plugin).
;

(define (script-fu-ripply-anim image drawable displacement num-frames edge-type)
  (let* ((width (car (ligma-drawable-get-width drawable)))
         (height (car (ligma-drawable-get-height drawable)))
         (work-image (car (ligma-image-new width
                                          height
                                          (quotient (car (ligma-drawable-type drawable))
                                                    2))))
         (map-layer (car (ligma-layer-new work-image
                                         width
                                         height
                                         (car (ligma-drawable-type drawable))
                                         "Ripple Map"
                                         100
                                         LAYER-MODE-NORMAL))))
    (ligma-context-push)
    (ligma-context-set-paint-mode LAYER-MODE-NORMAL)
    (ligma-context-set-opacity 100.0)
    (ligma-image-undo-disable work-image)

    ; Create a tile-able displacement map in the first layer
    (ligma-context-set-background '(127 127 127))
    (ligma-image-insert-layer work-image map-layer 0 0)
    (ligma-drawable-edit-fill map-layer FILL-BACKGROUND)
    (plug-in-noisify RUN-NONINTERACTIVE work-image map-layer FALSE 1.0 1.0 1.0 0.0)
    (plug-in-tile RUN-NONINTERACTIVE work-image map-layer (* width 3) (* height 3) FALSE)
    (plug-in-gauss-iir RUN-NONINTERACTIVE work-image map-layer 35 TRUE TRUE)
    (ligma-drawable-equalize map-layer TRUE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE work-image map-layer 5 TRUE TRUE)
    (ligma-drawable-equalize map-layer TRUE)
    (ligma-image-crop work-image width height width height)

    ; Create the frame layers
    (let loop ((remaining-frames num-frames))
      (unless (zero? remaining-frames)
        (let ((frame-layer (car (ligma-layer-new-from-drawable drawable work-image))))
          (ligma-image-insert-layer work-image frame-layer 0 0)
          (ligma-item-set-name frame-layer
                              (string-append "Frame "
                                             (number->string (+ 1 (- num-frames
                                                                     remaining-frames)))
                                             " (replace)"))
          (plug-in-displace RUN-NONINTERACTIVE work-image frame-layer
                            displacement displacement
                            TRUE TRUE map-layer map-layer (+ edge-type 1))
          (ligma-item-set-visible frame-layer TRUE))
        (ligma-drawable-offset map-layer
                              TRUE
                              OFFSET-BACKGROUND
                              (/ width num-frames)
                              (/ height num-frames))
        (loop (- remaining-frames 1))))

    (ligma-image-remove-layer work-image map-layer)
    (ligma-image-undo-enable work-image)
    (ligma-display-new work-image)

    (ligma-context-pop)))

(script-fu-register "script-fu-ripply-anim"
  _"_Rippling..."
  _"Create a multi-layer image by adding a ripple effect to the current layer"
  "Adam D. Moss (adam@foxbox.org), Saul Goode"
  "Adam D. Moss, Saul Goode"
  "1997, 2015"
  "RGB* GRAY*"
  SF-IMAGE      "Image to animage"    0
  SF-DRAWABLE   "Drawable to animate" 0
  SF-ADJUSTMENT _"Rippling strength"  '(3 0 256 1 10 1 0)
  SF-ADJUSTMENT _"Number of frames"   '(15 0 256 1 10 0 1)
  SF-OPTION     _"Edge behavior"      '(_"Wrap" _"Smear" _"Black")
  )

(script-fu-menu-register "script-fu-ripply-anim"
                         "<Image>/Filters/Animation/Animators")
