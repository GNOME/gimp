; "Contact Sheet" v1.1 September 2, 2004
; by Kevin Cozens <kcozens@interlog.com>
;
; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
; Created version 1.1
; Added ability to select sheet size, set font used for sheet and image
; titles, background color, and colour used for titles.

(define (script-fu-contactsheet dir sheet-size
         title-font legend-font text-color bg-color)

  (define (init-sheet-data size)
    (let (
         (sheet-w)
         (sheet-h)
         (thumb-w)
         (thumb-h)
         (border-x)
         (border-y)
         (off-x)
         (off-y)
         (count)
         )

      (case size
        ((0) (set! sheet-w 640)
             (set! sheet-h 480)
             (set! thumb-w 90)
             (set! thumb-h 68)
             (set! border-x 32)
             (set! border-y 23)
             (set! off-x -1)
             (set! off-y 0)
             (set! count 4)
        )

        ((1) (set! sheet-w 800)
             (set! sheet-h 600)
             (set! thumb-w 119)
             (set! thumb-h 90)
             (set! border-x 34)
             (set! border-y 25)
             (set! off-x 0)
             (set! off-y 0)
             (set! count 4)
        )

        ((2) (set! sheet-w 1024)
             (set! sheet-h 768)
             (set! thumb-w 133)
             (set! thumb-h 100)
             (set! border-x 32)
             (set! border-y 24)
             (set! off-x 1)
             (set! off-y 0)
             (set! count 5)
        )

        ((3) (set! sheet-w 1280)
             (set! sheet-h 1024)
             (set! thumb-w 133)
             (set! thumb-h 100)
             (set! border-x 24)
             (set! border-y 25)
             (set! off-x 0)
             (set! off-y 0)
             (set! count 7)
        )
      )

      (list sheet-w sheet-h thumb-w thumb-h border-x border-y off-x off-y count)
    )
  )

  (define (init-sheet-img img num img-width border-y off-y)
    (let* (
          (text-layer)
          (text-width)
          (text-height)
          )
      (gimp-selection-all img)
      (gimp-drawable-fill (car (gimp-image-get-active-layer img))
                          BACKGROUND-FILL)
      (gimp-selection-none img)
      (set! text-layer (car (gimp-text-fontname img -1 0 0
                              (string-append _"Sheet " (number->string num)
                                             _" for directory " dir)
                                             0 TRUE 14 PIXELS title-font)))
      (set! text-width (car (gimp-drawable-width text-layer)))
      (set! text-height (car (gimp-drawable-height text-layer)))
      (gimp-layer-set-offsets text-layer
        (/ (- img-width text-width) 2)
        (/ (- (+ border-y off-y) text-height) 2)
      )
      (gimp-image-merge-visible-layers img CLIP-TO-IMAGE)
    )
  )

  ;This routine should preserve the aspect ratio of the original image
  (define (make-thumbnail-size img thumb-w thumb-h)
    (gimp-image-scale new-img thumb-w thumb-h)
  )

  (let* (
        (dir-stream (dir-open-stream dir))
        (sheet-num 1)
        (img-count 0)
        (pos-x 0)
        (pos-y 0)
        (sheet-data)
        (sheet-img)
        (sheet-layer)
        (sheet-width)
        (sheet-height)
        (thumb-w)
        (thumb-h)
        (border-x)
        (border-y)
        (off-x)
        (off-y)
        (max-x)
        (max-y)
        (new-img)
        (file)
        (file-path)
        (tmp-layer)
        )

    (gimp-context-push)
    (gimp-context-set-foreground text-color)
    (gimp-context-set-background bg-color)

    (set! sheet-data (init-sheet-data sheet-size))
    (set! sheet-width (car sheet-data))
    (set! sheet-height (cadr sheet-data))
    (set! sheet-data (cddr sheet-data))
    (set! thumb-w (car sheet-data))
    (set! thumb-h (cadr sheet-data))
    (set! sheet-data (cddr sheet-data))
    (set! border-x (car sheet-data))
    (set! border-y (cadr sheet-data))
    (set! sheet-data (cddr sheet-data))
    (set! off-x (car sheet-data))
    (set! off-y (cadr sheet-data))
    (set! max-x (caddr sheet-data))
    (set! max-y max-x)

    (set! sheet-img (car (gimp-image-new sheet-width sheet-height RGB)))
    (set! sheet-layer (car (gimp-layer-new sheet-img sheet-width sheet-height
                            RGB-IMAGE "Background"
                            100 NORMAL-MODE)))
    (gimp-image-add-layer sheet-img sheet-layer 0)

    (init-sheet-img sheet-img sheet-num sheet-width border-y off-y)

    (if (not dir-stream)
      (gimp-message (string-append _"Unable to open directory " dir))
      (begin
        (do
          ( (file (dir-read-entry dir-stream) (dir-read-entry dir-stream)) )
          ( (eof-object? file) )

          (set! file-path (string-append dir DIR-SEPARATOR file))
          (if (and (not (re-match "index.*" file))
                   (= (file-type file-path) FILE-TYPE-FILE)
              )
            (catch 'errobj
              (set! new-img
                    (car (gimp-file-load RUN-NONINTERACTIVE file-path file)))

              (make-thumbnail-size new-img thumb-w thumb-h)

              (if (> (car (gimp-image-get-layers new-img)) 1)
                (gimp-image-flatten new-img)
              )
              (set! tmp-layer (car (gimp-layer-new-from-drawable
                            (car (gimp-image-get-active-drawable new-img))
                                  sheet-img)))

              (gimp-image-add-layer sheet-img tmp-layer 0)
              (gimp-layer-set-offsets tmp-layer
                (+ border-x off-x (* pos-x (+ thumb-w border-x)))
                (+ border-y off-y (* pos-y (+ thumb-h border-y)))
              )

              (gimp-image-delete new-img)

              (set! tmp-layer (car (gimp-text-fontname sheet-img -1 0 0 file
                                     0 TRUE 12 PIXELS legend-font)))
              (gimp-layer-set-offsets tmp-layer
                (+ border-x off-x (* pos-x (+ thumb-w border-x))
                   (/ (- thumb-w (car (gimp-drawable-width tmp-layer))) 2))
                (+ border-y off-y (* pos-y (+ thumb-h border-y)) thumb-h 2)
              )

              (set! img-count (+ img-count 1))

              (set! pos-x (+ pos-x 1))
              (if (> pos-x max-x)
                (begin
                  (set! pos-x 0)
                  (set! pos-y (+ pos-y 1))
                  (if (> pos-y max-y)
                    (begin
                      (set! pos-y 0)
                      (set! sheet-layer (car (gimp-image-flatten sheet-img)))
                      (gimp-file-save
                        RUN-NONINTERACTIVE
                        sheet-img
                        sheet-layer
                        (string-append dir DIR-SEPARATOR
                            "index" (number->string sheet-num) ".jpg")
                        (string-append
                            "index" (number->string sheet-num) ".jpg")
                      )

                      (set! sheet-num (+ sheet-num 1))
                      (init-sheet-img sheet-img sheet-num sheet-width
                                      border-y off-y)
                      (set! img-count 0)
                    )
                  )
                )
              )
            )
          )
        )

        (dir-close-stream dir-stream)

        (if (> img-count 0)
          (begin
            (set! sheet-layer (car (gimp-image-flatten sheet-img)))
            (gimp-file-save
              RUN-NONINTERACTIVE
              sheet-img
              sheet-layer
              (string-append dir DIR-SEPARATOR
                  "index" (number->string sheet-num) ".jpg")
              (string-append "index" (number->string sheet-num) ".jpg")
            )
          )
        )
      )

      (gimp-image-delete sheet-img)

      (display (string-append _"Created " (number->string sheet-num)
                              _" contact sheets from a total of "
                              (number->string img-count) _" images"))
      (newline)
    )

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-contactsheet"
    _"_Contact Sheet..."
    _"Create a series of images containing thumbnail sized versions of all of the images in a specified directory."
    "Kevin Cozens <kcozens@interlog.com>"
    "Kevin Cozens"
    "July 19, 2004"
    ""
    SF-DIRNAME _"Directory"        "/tmp/test"
    SF-OPTION  _"Sheet size"       '("640 x 480"
                                     "800 x 600"
                                     "1024 x 768"
                                     "1280 x 1024")
    SF-FONT    _"Title font"       "Sans Bold Italic"
    SF-FONT    _"Legend font"      "Sans Bold"
    SF-COLOR   _"Text color"       "white"
    SF-COLOR   _"Background color" "black"
)

(script-fu-menu-register "script-fu-contactsheet" "<Toolbox>/Xtns/Utils")
