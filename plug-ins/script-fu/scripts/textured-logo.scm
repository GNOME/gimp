;  BLENDED-DROP-SHADOW-LOGO
;  draw the specified text over a blended background using current gimp fg
;   and bg colors.  The finished blend has a drop shadow underneath that blends
;   to the specified bg-color
;  if the blend colors are specified as high intensity, the sharp option
;   should be enabled or the logo will come out blurry

(define (scale size percent) (* size percent))

(define (apply-textured-logo-effect img
                                    logo-layer
                                    b-size
                                    text-pattern
                                    tile-type
                                    bg-color
                                    blend-fg
                                    blend-bg)
  (let* (
        (b-size-2 (scale b-size 0.5))
        (f-size (scale b-size 0.75))
        (ds-size (scale b-size 0.5))
        (ts-size (- b-size-2 3))
        (width (car (gimp-drawable-width logo-layer)))
        (height (car (gimp-drawable-height logo-layer)))
        (blend-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                          "Blend" 100 NORMAL-MODE)))
        (shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                           "Shadow" 100 NORMAL-MODE)))
        (text-shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                                "Text Shadow" 100 MULTIPLY-MODE)))
        (tsl-layer-mask (car (gimp-layer-create-mask text-shadow-layer
                                                     ADD-BLACK-MASK)))
        (drop-shadow-layer (car (gimp-layer-new img width height RGBA-IMAGE
                                                "Drop Shadow" 100 MULTIPLY-MODE)))
        (dsl-layer-mask (car (gimp-layer-create-mask drop-shadow-layer
                                                     ADD-BLACK-MASK)))
        )

    (gimp-context-push)

    (script-fu-util-image-resize-from-layer img logo-layer)
    (gimp-image-add-layer img shadow-layer 1)
    (gimp-image-add-layer img blend-layer 1)
    (gimp-image-add-layer img drop-shadow-layer 1)
    (gimp-image-add-layer img text-shadow-layer 0)
    (gimp-selection-all img)
    (gimp-context-set-pattern text-pattern)
    (gimp-layer-set-lock-alpha logo-layer TRUE)
    (gimp-edit-bucket-fill logo-layer PATTERN-BUCKET-FILL NORMAL-MODE 100 0 FALSE 0 0)
    (gimp-selection-none img)
    (gimp-edit-clear text-shadow-layer)
    (gimp-edit-clear drop-shadow-layer)
    (gimp-context-set-background bg-color)
    (gimp-drawable-fill shadow-layer BACKGROUND-FILL)
    (gimp-rect-select img b-size-2 b-size-2 (- width b-size) (- height b-size)
                      CHANNEL-OP-REPLACE TRUE b-size-2)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill shadow-layer BACKGROUND-FILL)
    (gimp-selection-layer-alpha logo-layer)
    (gimp-layer-add-mask text-shadow-layer tsl-layer-mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill tsl-layer-mask BACKGROUND-FILL)
    (gimp-selection-feather img f-size)
    (gimp-context-set-background '(63 63 63))
    (gimp-edit-fill drop-shadow-layer BACKGROUND-FILL)
    (gimp-context-set-background '(0 0 0))
    (gimp-edit-fill text-shadow-layer BACKGROUND-FILL)
    (gimp-context-set-foreground '(255 255 255))

    (gimp-edit-blend text-shadow-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-SHAPEBURST-ANGULAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 1 1)

    (gimp-selection-none img)
    (gimp-context-set-foreground blend-fg)
    (gimp-context-set-background blend-bg)

    (gimp-edit-blend blend-layer FG-BG-RGB-MODE NORMAL-MODE
                     GRADIENT-LINEAR 100 0 REPEAT-NONE FALSE
                     FALSE 0 0 TRUE
                     0 0 width 0)

    (plug-in-mosaic RUN-NONINTERACTIVE img blend-layer 12 1 1 0.7 TRUE 135 0.2 TRUE FALSE
                    tile-type 1 0)

    (gimp-layer-translate logo-layer (- b-size-2) (- b-size-2))
    (gimp-layer-translate blend-layer (- b-size) (- b-size))
    (gimp-layer-translate text-shadow-layer (- ts-size) (- ts-size))
    (gimp-layer-translate drop-shadow-layer ds-size ds-size)
    (gimp-selection-layer-alpha blend-layer)
    (gimp-layer-add-mask drop-shadow-layer dsl-layer-mask)
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-fill dsl-layer-mask BACKGROUND-FILL)
    (gimp-layer-remove-mask drop-shadow-layer MASK-APPLY)
    (gimp-selection-none img)

    (gimp-context-pop)
  )
)

(define (script-fu-textured-logo-alpha img
                                       logo-layer
                                       b-size
                                       text-pattern
                                       tile-type
                                       bg-color
                                       blend-fg
                                       blend-bg)
  (begin
    (gimp-image-undo-group-start img)
    (apply-textured-logo-effect img logo-layer b-size text-pattern tile-type
                                bg-color blend-fg blend-bg)
    (gimp-image-undo-group-end img)
    (gimp-displays-flush))
)

(script-fu-register "script-fu-textured-logo-alpha"
  _"_Textured..."
  _"Fill the selected region (or alpha) with a texture and add highlights, shadows, and a mosaic background"
  "Spencer Kimball"
  "Spencer Kimball"
  "1996"
  "RGBA"
  SF-IMAGE      "Image"                 0
  SF-DRAWABLE   "Drawable"              0
  SF-ADJUSTMENT _"Border size (pixels)" '(20 1 100 1 10 0 1)
  SF-PATTERN    _"Pattern"              "Fibers"
  SF-OPTION     _"Mosaic tile type"     '(_"Squares"
                                          _"Hexagons"
                                          _"Octagons")
  SF-COLOR      _"Background color"     "white"
  SF-COLOR      _"Starting blend"       '(32 106 0)
  SF-COLOR      _"Ending blend"         '(0 0 106)
)

(script-fu-menu-register "script-fu-textured-logo-alpha"
                         "<Image>/Filters/Alpha to Logo")

(define (script-fu-textured-logo text
                                 size
                                 fontname
                                 text-pattern
                                 tile-type
                                 bg-color
                                 blend-fg
                                 blend-bg)
  (let* ((img (car (gimp-image-new 256 256 RGB)))
         (b-size (scale size 0.1))
         (text-layer (car (gimp-text-fontname img -1 0 0 text b-size
                                              TRUE size PIXELS fontname))))
    (gimp-image-undo-disable img)
    (apply-textured-logo-effect img text-layer b-size text-pattern tile-type
                                bg-color blend-fg blend-bg)
    (gimp-image-undo-enable img)
    (gimp-display-new img)
  )
)

(script-fu-register "script-fu-textured-logo"
  _"_Textured..."
  _"Create a textured logo with highlights, shadows, and a mosaic background"
  "Spencer Kimball"
  "Spencer Kimball"
  "1996"
  ""
  SF-STRING     _"Text"               "GIMP"
  SF-ADJUSTMENT _"Font size (pixels)" '(200 1 1000 1 10 0 1)
  SF-FONT       _"Font"               "CuneiFont"
  SF-PATTERN    _"Text pattern"       "Fibers"
  SF-OPTION     _"Mosaic tile type"   '(_"Squares"
                                        _"Hexagons"
                                        _"Octagons")
  SF-COLOR      _"Background color"   "white"
  SF-COLOR      _"Starting blend"     '(32 106 0)
  SF-COLOR      _"Ending blend"       '(0 0 106)
)

(script-fu-menu-register "script-fu-textured-logo"
                         "<Toolbox>/Xtns/Logos")
