;; font-map
;; Spencer Kimball

;; To test, open the Font tool dialog,
;; press right mouse button in the list of fonts, choose "Render Font Map"

;; Test cases for font filter regex
;;   ".*"  expect render all installed fonts
;;   "foo" expect render blank image (no matching fonts)
;;   "Sans" expect render subset of installed fonts

(define (script-fu-font-map text
                            use-name
                            labels
                            font-filter
                            font-size
                            border
                            colors)

  (define (max-font-width text use-name list-cnt list font-size)
    (let* ((count        0)
           (width        0)
           (maxwidth     0)
           (font         "")
           (font-object '())
           (extents     '()))
      (while (< count list-cnt)
        (set! font (car list))
        (set! font-object (car (gimp-font-get-by-name font)))

        (if (= use-name TRUE)
            (set! text font))
        (set! extents (gimp-text-get-extents-font     text
                                                      font-size
                                                      font-object))
        (set! width (car extents))
        (if (> width maxwidth)
            (set! maxwidth width))

        (set! list (cdr list))
        (set! count (+ count 1))
      )

      maxwidth
    )
  )

  (define (max-font-height text use-name list-cnt list font-size)
    (let* ((count       0)
           (height      0)
           (maxheight   0)
           (font        "")
           (font-object '())
           (extents     '()))
      (while (< count list-cnt)
        (set! font (car list))
        (set! font-object (car (gimp-font-get-by-name font)))

        (if (= use-name TRUE)
            (set! text font)
        )
        (set! extents (gimp-text-get-extents-font     text
                                                      font-size
                                                      font-object))
        (set! height (cadr extents))
        (if (> height maxheight)
            (set! maxheight height)
        )

        (set! list (cdr list))
        (set! count (+ count 1))
      )

      maxheight
    )
  )

  (let* (
        ; gimp-fonts-get-list returns a one element list of results,
        ; the only element is itself a list of fonts, possibly empty.
        (font-list   (car (gimp-fonts-get-list font-filter)))
        (num-fonts   (length font-list))
        (label-size  (/ font-size 2))
        (border      (+ border (* labels (/ label-size 2))))
        (y           border)
        (maxheight   (max-font-height text use-name num-fonts font-list font-size))
        (maxwidth    (max-font-width  text use-name num-fonts font-list font-size))
        (width       (+ maxwidth (* 2 border)))
        (height      (+ (+ (* maxheight num-fonts) (* 2 border))
                        (* labels (* label-size num-fonts))))
        (img         (car (gimp-image-new width height (if (= colors 0)
                                                           GRAY RGB))))
        (drawable    (car (gimp-layer-new img width height (if (= colors 0)
                                                               GRAY-IMAGE RGB-IMAGE)
                                          "Background" 100 LAYER-MODE-NORMAL)))
        (count       0)
        (font        "")
        (font-object '())
        )

    (gimp-context-push)

    (gimp-image-undo-disable img)

    (if (= colors 0)
        (begin
          (gimp-context-set-background '(255 255 255))
          (gimp-context-set-foreground '(0 0 0))))

    (gimp-image-insert-layer img drawable 0 0)
    (gimp-drawable-edit-clear drawable)

    (if (= labels TRUE)
        (begin
          (set! drawable (car (gimp-layer-new img width height
                                              (if (= colors 0)
                                                  GRAYA-IMAGE RGBA-IMAGE)
                                              "Labels" 100 LAYER-MODE-NORMAL)))
          (gimp-image-insert-layer img drawable 0 -1)))
          (gimp-drawable-edit-clear drawable)

    (while (< count num-fonts)
      (set! font (car font-list))
      (set! font-object (car (gimp-font-get-by-name font)))

      (if (= use-name TRUE)
          (set! text font))

      (gimp-text-font     img -1
                          border
                          y
                          text
                          0 TRUE font-size
                          font-object)

      (set! y (+ y maxheight))

      (if (= labels TRUE)
          (begin
            (gimp-floating-sel-anchor (car (gimp-text-font     img drawable
                                                               (- border
                                                                  (/ label-size 2))
                                                               (- y
                                                                  (/ label-size 2))
                                                               font
                                                               0 TRUE
                                                               label-size
                                                               font-object)))
          (set! y (+ y label-size))
          )
      )

      (set! font-list (cdr font-list))
      (set! count (+ count 1))
    )

    (gimp-image-set-selected-layers img 1 (vector drawable))

    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-font-map"
  _"Render _Font Map..."
  _"Create an image filled with previews of fonts matching a fontname filter"
  "Spencer Kimball"
  "Spencer Kimball"
  "1997"
  ""
  SF-STRING     _"_Text"                  "How quickly daft jumping zebras vex."
  SF-TOGGLE     _"Use font _name as text" FALSE
  SF-TOGGLE     _"_Labels"                TRUE
  SF-STRING     _"_Filter (regexp)"       "Sans"
  SF-ADJUSTMENT _"Font _size (pixels)"    '(32 2 1000 1 10 0 1)
  SF-ADJUSTMENT _"_Border (pixels)"       '(10 0  200 1 10 0 1)
  SF-OPTION     _"_Color scheme"          '(_"Black on white" _"Active colors")
)

(script-fu-menu-register "script-fu-font-map"
                         "<Fonts>/Fonts Menu")
