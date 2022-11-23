;
;  burn-in-anim.scm V2.1  -  script-fu for LIGMA 1.1 and higher
;
;  Copyright (C) 9/2000  Roland Berger
;  roland@fuchur.leute.server.de
;  http://fuchur.leute.server.de
;
;  Let text appear and fade out with a "burn-in" like SFX.
;  Works on an image with a text and a background layer
;
;  Copying Policy:  GNU Public License http://www.gnu.org
;

(define (script-fu-burn-in-anim org-img
                                org-layer
                                glow-color
                                fadeout
                                bl-width
                                corona-width
                                after-glow
                                show-glow
                                optimize
                                speed)

  (let* (
        ;--- main variable: "bl-x" runs from 0 to layer-width
        (bl-x 0)
        (frame-nr 0)
        (img 0)
        (source-layer 0)
        (bg-source-layer 0)
        (source-layer-width 0)
        (bg-layer 0)
        (bg-layer-name 0)
        (bl-layer 0)
        (bl-layer-name 0)
        (bl-mask 0)
        (bl-layer-width 0)
        (bl-height 0)
        (bl-x-off 0)
        (bl-y-off 0)
        (nofadeout-bl-x-off 0)
        (nofadeout-bl-width 0)
        (blended-layer 0)
        (img-display 0)
        )

    (if (< speed 1)
        (set! speed (* -1 speed)) )

    ;--- check image and work on a copy
    (if (and (= (car (ligma-image-get-layers org-img)) 2)
             (= (car (ligma-image-get-floating-sel org-img)) -1))

        ;--- main program structure starts here, begin of "if-1"
        (begin
          (ligma-context-push)
          (ligma-context-set-defaults)

          (set! img (car (ligma-image-duplicate org-img)))
          (ligma-image-undo-disable img)
          (if (> (car (ligma-drawable-type org-layer)) 1 )
              (ligma-image-convert-rgb img))
          (set! source-layer    (aref (cadr (ligma-image-get-layers img)) 0 ))
          (set! bg-source-layer (aref (cadr (ligma-image-get-layers img)) 1 ))
          (set! source-layer-width (car (ligma-drawable-get-width  source-layer)))

          ;--- hide layers, cause we want to "merge visible layers" later
          (ligma-item-set-visible source-layer FALSE)
          (ligma-item-set-visible bg-source-layer     FALSE)

          ;--- process image horizontal with pixel-speed
          (while (< bl-x (+ source-layer-width bl-width))
              (set! bl-layer (car (ligma-layer-copy source-layer TRUE)))
              (set! bl-layer-name (string-append "fr-nr"
                                                 (number->string frame-nr 10) ) )

              (ligma-image-insert-layer img bl-layer 0 -2)
              (ligma-item-set-name bl-layer bl-layer-name)
              (ligma-item-set-visible bl-layer TRUE)
              (ligma-layer-set-lock-alpha bl-layer TRUE)
              (ligma-layer-add-alpha bl-layer)

              ;--- add an alpha mask for blending and select it
              (ligma-image-select-item img CHANNEL-OP-REPLACE bl-layer)
              (set! bl-mask (car (ligma-layer-create-mask bl-layer ADD-MASK-BLACK)))
              (ligma-layer-add-mask bl-layer bl-mask)

              ;--- handle layer geometry
              (set! bl-layer-width source-layer-width)
              (set! bl-height      (car (ligma-drawable-get-height bl-layer)))
              (set! bl-x-off (- bl-x     bl-width))
              (set! bl-x-off (+ bl-x-off (car  (ligma-drawable-get-offsets bl-layer))))
              (set! bl-y-off             (cadr (ligma-drawable-get-offsets bl-layer)))

              ;--- select a rectangular area to blend
              (ligma-image-select-rectangle img CHANNEL-OP-REPLACE bl-x-off bl-y-off bl-width bl-height)
              ;--- select at least 1 pixel!
              (ligma-image-select-rectangle img CHANNEL-OP-ADD bl-x-off bl-y-off (+ bl-width 1) bl-height)

              (if (= fadeout FALSE)
                  (begin
                    (set! nofadeout-bl-x-off (car (ligma-drawable-get-offsets bl-layer)))
                    (set! nofadeout-bl-width (+ nofadeout-bl-x-off bl-x))
                    (set! nofadeout-bl-width (max nofadeout-bl-width 1))
                    (ligma-image-select-rectangle img CHANNEL-OP-REPLACE
                                                 nofadeout-bl-x-off bl-y-off
                                                 nofadeout-bl-width bl-height)
                  )
              )

              ;--- alpha blending text to trans (fadeout)
              (ligma-context-set-foreground '(255 255 255))
              (ligma-context-set-background '(  0   0   0))
              (if (= fadeout TRUE)
                  (begin
                    ; blend with 20% offset to get less transparency in the front
		    (ligma-context-set-gradient-fg-bg-rgb)
                    (ligma-drawable-edit-gradient-fill bl-mask
						      GRADIENT-LINEAR 20
						      FALSE 0 0
						      TRUE
						      (+ bl-x-off bl-width) 0
						      bl-x-off 0)
                  )
              )

              (if (= fadeout FALSE)
                  (begin
                    (ligma-context-set-foreground '(255 255 255))
                    (ligma-drawable-edit-fill bl-mask FILL-FOREGROUND)
                  )
              )

              (ligma-layer-remove-mask bl-layer MASK-APPLY)

              ;--- add bright glow in front
              (if (= show-glow TRUE)
                  (begin
                    ;--- add some brightness to whole text
                    (if (= fadeout TRUE)
                        (ligma-drawable-brightness-contrast bl-layer 0.787 0)
                    )

		    ;--- blend glow color inside the letters
		    (ligma-context-set-foreground glow-color)
		    (ligma-context-set-gradient-fg-transparent)
		    (ligma-drawable-edit-gradient-fill bl-layer
						      GRADIENT-LINEAR 0
						      FALSE 0 0
						      TRUE
						      (+ bl-x-off bl-width) 0
						      (- (+ bl-x-off bl-width) after-glow) 0)

                    ;--- add corona effect
		    (ligma-image-select-item img CHANNEL-OP-REPLACE bl-layer)
		    (ligma-selection-sharpen img)
		    (ligma-selection-grow img corona-width)
		    (ligma-layer-set-lock-alpha bl-layer FALSE)
		    (ligma-selection-feather img corona-width)
		    (ligma-context-set-foreground glow-color)
		    (ligma-drawable-edit-gradient-fill bl-layer
						      GRADIENT-LINEAR 0
						      FALSE 0 0
						      TRUE
						      (- (+ bl-x-off bl-width) corona-width) 0
						      (- (+ bl-x-off bl-width) after-glow) 0)
		    )
		  )

              ;--- merge with bg layer
              (set! bg-layer (car (ligma-layer-copy bg-source-layer FALSE)))
              (ligma-image-insert-layer img bg-layer 0 -1)
              (ligma-image-lower-item img bg-layer)
              (set! bg-layer-name (string-append "bg-"
                                                 (number->string frame-nr 10)))
              (ligma-item-set-name bg-layer bg-layer-name)
              (ligma-item-set-visible bg-layer TRUE)
              (set! blended-layer (car (ligma-image-merge-visible-layers img
                                        CLIP-TO-IMAGE)))
              ;(set! blended-layer bl-layer)
              (ligma-item-set-visible blended-layer FALSE)

              ;--- end of "while" loop
              (set! frame-nr (+ frame-nr 1))
              (set! bl-x     (+ bl-x speed))
          )

          ;--- finalize the job
          (ligma-selection-none img)
          (ligma-image-remove-layer img    source-layer)
          (ligma-image-remove-layer img bg-source-layer)

          (ligma-image-set-filename img "burn-in")

          (if (= optimize TRUE)
              (begin
                (ligma-image-convert-indexed img CONVERT-DITHER-FS CONVERT-PALETTE-WEB 250 FALSE TRUE "")
                (set! img (car (plug-in-animationoptimize RUN-NONINTERACTIVE
                                                          img
                                                          blended-layer)))
              )
          )

          (ligma-item-set-visible (aref (cadr (ligma-image-get-layers img)) 0)
                                  TRUE)
          (ligma-image-undo-enable img)
          (ligma-image-clean-all img)
          (set! img-display (car (ligma-display-new img)))

          (ligma-displays-flush)

          (ligma-context-pop)
        )

        ;--- false form of "if-1"
        (ligma-message _"The Burn-In script needs two layers in total. A foreground layer with transparency and a background layer.")
    )
  )
)


(script-fu-register "script-fu-burn-in-anim"
    _"B_urn-In..."
    _"Create intermediate layers to produce an animated 'burn-in' transition between two layers"
    "Roland Berger roland@fuchur.leute.server.de"
    "Roland Berger"
    "January 2001"
    "RGBA GRAYA INDEXEDA"
    SF-IMAGE    "The image"            0
    SF-DRAWABLE "Layer to animate"     0
    SF-COLOR   _"Glow color"           "white"
    SF-TOGGLE  _"Fadeout"              FALSE
    SF-VALUE   _"Fadeout width"        "100"
    SF-VALUE   _"Corona width"         "7"
    SF-VALUE   _"After glow"           "50"
    SF-TOGGLE  _"Add glowing"          TRUE
    SF-TOGGLE  _"Prepare for GIF"      FALSE
    SF-VALUE   _"Speed (pixels/frame)" "50"
)

(script-fu-menu-register "script-fu-burn-in-anim"
                         "<Image>/Filters/Animation/Animators")
