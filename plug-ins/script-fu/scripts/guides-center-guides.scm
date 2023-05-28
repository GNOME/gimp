;; -*-scheme-*-

;; Alx Sa 2023.  No copyright.  Public Domain.

(define (script-fu-guide-center-guides image drawable)
  (script-fu-guide-new-percent image drawable 0 50)
  (script-fu-guide-new-percent image drawable 1 50)
)

(script-fu-register "script-fu-guide-center-guides"
  _"New Guides (_Center)..."
  _"Add guides at the vertical and horizontal center of the image"
  "Alx Sa"
  "Alx Sa, 2023"
  "May 2023"
  "*"
  SF-IMAGE    "Input Image"    0
  SF-DRAWABLE "Input Drawable" 0
)

(script-fu-menu-register "script-fu-guide-center-guides"
                         "<Image>/Image/Guides")
