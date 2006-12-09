;; reverse-layers.scm: Reverse the order of layers in the current image.
;; Copyright (C) 2006 by Akkana Peck.
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License.

(define (script-fu-reverse-layers img drawable)
  (let* (
        (layers (gimp-image-get-layers img))
        (num-layers (car layers))
        (layer-array (cadr layers))
        (i (- num-layers 1))
        )

    (gimp-image-undo-group-start img)

    (while (>= i 0)
           (let ((layer (aref layer-array i)))
             (if (= (car (gimp-layer-is-floating-sel layer)) FALSE)
                 (gimp-image-lower-layer-to-bottom img layer))
           )

           (set! i (- i 1))
    )

    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
  )
)

(script-fu-register "script-fu-reverse-layers"
  _"Reverse Layer Order"
  _"Reverse the order of layers in the image"
  "Akkana Peck"
  "Akkana Peck"
  "August 2006"
  "*"
  SF-IMAGE    "Image"    0
  SF-DRAWABLE "Drawable" 0
)

(script-fu-menu-register "script-fu-reverse-layers"
                         "<Image>/Layer/Stack")
