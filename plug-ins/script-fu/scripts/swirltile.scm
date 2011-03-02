;
; Swirl-tile
;  produces a (hope-fully) seamlessly tiling swirling effect
;
;  Adrian Likins  <aklikins@eos.ncsu.edu>
;
;  http://www4.ncsu.edu/eos/users/a/aklikins/pub/gimp/
;


(define (script-fu-swirl-tile depth azimuth elevation blurRadius height width whirl-amount noise-level bg-color)
  (let* (
        (img (car (gimp-image-new width height RGB)))
        (layer-one (car (gimp-layer-new img width height
                                        RGB-IMAGE "TEST" 100 NORMAL-MODE)))
        (cx (/ width 2))
        (cy (/ height 2))
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    (gimp-image-insert-layer img layer-one 0 0)
    (gimp-context-set-background bg-color)
    (gimp-edit-fill layer-one BACKGROUND-FILL)
    (plug-in-noisify RUN-NONINTERACTIVE img layer-one FALSE noise-level noise-level noise-level 1.0)

    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)

    (gimp-drawable-offset layer-one TRUE 0 cx cy)

    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)
    (plug-in-whirl-pinch RUN-NONINTERACTIVE img layer-one whirl-amount 0.0 1.0)

    (plug-in-gauss-rle RUN-NONINTERACTIVE img layer-one blurRadius TRUE TRUE)

    (plug-in-bump-map RUN-NONINTERACTIVE img layer-one layer-one azimuth elevation depth 0 0 0 0 FALSE FALSE 0)

    (gimp-display-new img)
    (gimp-image-undo-enable img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-swirl-tile"
  _"Swirl-_Tile..."
  _"Create an image filled with a swirled tile effect"
  "Adrian Likins <aklikins@eos.ncsu.edu>"
  "Adrian Likins"
  "1997"
  ""
  SF-ADJUSTMENT _"Depth"           '(10 1 64 1 1 0 0)
  SF-ADJUSTMENT _"Azimuth"          '(135 0 360 1 10 0 0)
  SF-ADJUSTMENT _"Elevation"        '(45 0 90 1 10 0 0)
  SF-ADJUSTMENT _"Blur radius"      '(3 0 128 1 10 0 0)
  SF-ADJUSTMENT _"Height"           '(256 0 1024 1 10 0 1)
  SF-ADJUSTMENT _"Width"            '(256 0 1024 1 10 0 1)
  SF-ADJUSTMENT _"Whirl amount"     '(320 0 360 1 10 0 0)
  SF-ADJUSTMENT _"Roughness"        '(0.5 0 1 0.1 0.01 2 1)
  SF-COLOR      _"Background color" "white"
)

(script-fu-menu-register "script-fu-swirl-tile"
                         "<Image>/File/Create/Patterns")
