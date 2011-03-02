; "Rippling Image" animation generator (ripply-anim.scm)
; Adam D. Moss (adam@foxbox.org)
; 97/05/18
;
; Designed to be used in conjunction with a plugin capable
; of saving animations (i.e. the GIF plugin).
;

(define (script-fu-ripply-anim img drawable displacement num-frames edge-type)

  (define (copy-layer-ripple dest-image dest-drawable source-image source-drawable)
    (gimp-selection-all dest-image)
    (gimp-edit-clear dest-drawable)
    (gimp-selection-none dest-image)
    (gimp-selection-all source-image)
    (gimp-edit-copy source-drawable)
    (gimp-selection-none source-image)
    (let ((floating-sel (car (gimp-edit-paste dest-drawable FALSE))))
      (gimp-floating-sel-anchor (car (gimp-edit-paste dest-drawable FALSE)))
    )
  )

  (let* (
        (width (car (gimp-drawable-width drawable)))
        (height (car (gimp-drawable-height drawable)))
        (ripple-image (car (gimp-image-new width height GRAY)))
        (ripple-layer (car (gimp-layer-new ripple-image width height GRAY-IMAGE "Ripple Texture" 100 NORMAL-MODE)))
        (rippletiled-ret 0)
        (rippletiled-image 0)
        (rippletiled-layer 0)
        (remaining-frames 0)
        (xpos 0)
        (ypos 0)
        (xoffset 0)
        (yoffset 0)
        (dup-image 0)
        (layer-name 0)
        (this-image 0)
        (this-layer 0)
        (dup-layer 0)
        )

    (gimp-context-push)

    ; this script generates its own displacement map

    (gimp-image-undo-disable ripple-image)
    (gimp-context-set-background '(127 127 127))
    (gimp-image-insert-layer ripple-image ripple-layer 0 0)
    (gimp-edit-fill ripple-layer BACKGROUND-FILL)
    (plug-in-noisify RUN-NONINTERACTIVE ripple-image ripple-layer FALSE 1.0 1.0 1.0 0.0)
    ; tile noise
    (set! rippletiled-ret (plug-in-tile RUN-NONINTERACTIVE ripple-image ripple-layer (* width 3) (* height 3) TRUE))
    (gimp-image-undo-enable ripple-image)
    (gimp-image-delete ripple-image)

    (set! rippletiled-image (car rippletiled-ret))
    (set! rippletiled-layer (cadr rippletiled-ret))
    (gimp-image-undo-disable rippletiled-image)

    ; process tiled noise into usable displacement map
    (plug-in-gauss-iir RUN-NONINTERACTIVE rippletiled-image rippletiled-layer 35 TRUE TRUE)
    (gimp-equalize rippletiled-layer TRUE)
    (plug-in-gauss-rle RUN-NONINTERACTIVE rippletiled-image rippletiled-layer 5 TRUE TRUE)
    (gimp-equalize rippletiled-layer TRUE)

    ; displacement map is now in rippletiled-layer of rippletiled-image

    ; loop through the desired frames

    (set! remaining-frames num-frames)
    (set! xpos (/ width 2))
    (set! ypos (/ height 2))
    (set! xoffset (/ width num-frames))
    (set! yoffset (/ height num-frames))

    (let* ((out-imagestack (car (gimp-image-new width height RGB))))

    (gimp-image-undo-disable out-imagestack)

    (while (> remaining-frames 0)
      (set! dup-image (car (gimp-image-duplicate rippletiled-image)))
      (gimp-image-undo-disable dup-image)
      (gimp-image-crop dup-image width height xpos ypos)

      (set! layer-name (string-append "Frame "
                 (number->string (- num-frames remaining-frames) 10)
                 " (replace)"))
      (set! this-layer (car (gimp-layer-new out-imagestack
                                            width height RGB
                                            layer-name 100 NORMAL-MODE)))
      (gimp-image-insert-layer out-imagestack this-layer 0 0)

      (copy-layer-ripple out-imagestack this-layer img drawable)

      (set! dup-layer (car (gimp-image-get-active-layer dup-image)))
      (plug-in-displace RUN-NONINTERACTIVE out-imagestack this-layer
                        displacement displacement
                        TRUE TRUE dup-layer dup-layer edge-type)

      (gimp-image-undo-enable dup-image)
      (gimp-image-delete dup-image)

      (set! remaining-frames (- remaining-frames 1))
      (set! xpos (+ xoffset xpos))
      (set! ypos (+ yoffset ypos))
    )

    (gimp-image-undo-enable rippletiled-image)
    (gimp-image-delete rippletiled-image)
    (gimp-image-undo-enable out-imagestack)
    (gimp-display-new out-imagestack))

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-ripply-anim"
  _"_Rippling..."
  _"Create a multi-layer image by adding a ripple effect to the current image"
  "Adam D. Moss (adam@foxbox.org)"
  "Adam D. Moss"
  "1997"
  "RGB* GRAY*"
  SF-IMAGE      "Image to animage"    0
  SF-DRAWABLE   "Drawable to animate" 0
  SF-ADJUSTMENT _"Rippling strength"  '(3 0 256 1 10 1 0)
  SF-ADJUSTMENT _"Number of frames"   '(15 0 256 1 10 0 1)
  SF-OPTION     _"Edge behavior"      '(_"Wrap" _"Smear" _"Black")
)

(script-fu-menu-register "script-fu-ripply-anim"
                         "<Image>/Filters/Animation/Animators")
