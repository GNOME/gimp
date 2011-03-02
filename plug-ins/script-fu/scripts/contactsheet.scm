; "Contact Sheet" v1.2 September 5, 2007
; by Kevin Cozens <kcozens@interlog.com>
;
; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
; Version 1.0 (July 27, 2004)
; Created
;
; Version 1.1 (September 2, 2004)
; Added ability to select sheet size, set font used for sheet and image
;
; Version 1.2 (September 5, 2007)
; Preserve aspect ratio of original image. Center thumbnail in the area
; allowed for the thumbnail. Added disable/enable of undo operations.
; Added 1600x1200 sheet size.

(define (script-fu-contactsheet dir sheet-size
         title-font legend-font text-color bg-color)

  (define (init-sheet-data size)
    (let (
         (sheet-w 0)
         (sheet-h 0)
         (thumb-w 0)
         (thumb-h 0)
         (border-x 0) ;Space between rows and at top and bottom of thumbnails
         (border-y 0) ;Space between columns and at left and right of thumbnails
         (off-x 0)  ; Additional X shift to properly center a row of thumbnails
         (off-y 0)  ; Additional Y shift to properly center rows of thumbnails
         (count 0)
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

        ((4) (set! sheet-w 1600)
             (set! sheet-h 1200)
             (set! thumb-w 120)
             (set! thumb-h 90)
             (set! border-x 36)
             (set! border-y 25)
             (set! off-x 2)
             (set! off-y 0)
             (set! count 9)
        )
      )

      (list sheet-w sheet-h thumb-w thumb-h border-x border-y off-x off-y count)
    )
  )

  (define (init-sheet-img img num img-width border-y off-y)
    (let* (
          (text-layer 0)
          (text-width 0)
          (text-height 0)
          )
      (gimp-selection-all img)
      (gimp-drawable-fill (car (gimp-image-get-active-layer img))
                          BACKGROUND-FILL)
      (gimp-selection-none img)
      (set! text-layer (car (gimp-text-fontname img -1 0 0
                              (string-append _"Contact Sheet "
                                             (number->string num)
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

  (define (make-thumbnail-size img thumb-w thumb-h)
    (let* (
          (file-height (car (gimp-image-height img)))
          (file-width  (car (gimp-image-width img)))
          (aspect-ratio (/ file-width file-height))
          )

      ;Preserve the aspect ratio of the original image
      (if (> file-width file-height)
        (set! thumb-h (/ thumb-w aspect-ratio))
        (set! thumb-w (* thumb-h aspect-ratio))
      )

      (gimp-image-scale img thumb-w thumb-h)
    )
  )

  (let* (
        (dir-stream (dir-open-stream dir))
        (sheet-num 1)
        (img-count 0)
        (pos-x 0)
        (pos-y 0)

        (sheet-data 0)
        (sheet-width 0)
        (sheet-height 0)
        (thumb-w 0)
        (thumb-h 0)
        (border-x 0)
        (border-y 0)
        (off-x 0)
        (off-y 0)
        (max-x 0)
        (max-y 0)

        (sheet-img 0)
        (sheet-layer 0)

        (new-img 0)
        (file 0)
        (file-path 0)
        (tmp-layer 0)
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

    (gimp-image-undo-disable sheet-img)

    (set! sheet-layer (car (gimp-layer-new sheet-img sheet-width sheet-height
                            RGB-IMAGE "Background"
                            100 NORMAL-MODE)))
    (gimp-image-insert-layer sheet-img sheet-layer 0 0)

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
            (catch ()
              (set! new-img
                    (car (gimp-file-load RUN-NONINTERACTIVE file-path file)))

              (make-thumbnail-size new-img thumb-w thumb-h)

              (if (> (car (gimp-image-get-layers new-img)) 1)
                (gimp-image-flatten new-img)
              )
              (set! tmp-layer (car (gimp-layer-new-from-drawable
                            (car (gimp-image-get-active-drawable new-img))
                                  sheet-img)))

              (gimp-image-insert-layer sheet-img tmp-layer 0 0)

              ;Move thumbnail in to position and center it in area available.
              (gimp-layer-set-offsets tmp-layer
                (+ border-x off-x (* pos-x (+ thumb-w border-x))
                   (/ (- thumb-w (car (gimp-image-width new-img))) 2)
                )
                (+ border-y off-y (* pos-y (+ thumb-h border-y))
                   (/ (- thumb-h (car (gimp-image-height new-img))) 2)
                )
              )

              (gimp-image-delete new-img)

              (set! tmp-layer (car (gimp-text-fontname sheet-img -1 0 0 file
                                     0 TRUE 12 PIXELS legend-font)))
              (gimp-layer-set-offsets tmp-layer
                (+ border-x off-x (* pos-x (+ thumb-w border-x))
                   (/ (- thumb-w (car (gimp-drawable-width tmp-layer))) 2))
                (+ border-y off-y (* pos-y (+ thumb-h border-y)) thumb-h 6)
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

      (gimp-image-undo-enable sheet-img)
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
    SF-DIRNAME _"Images Directory" "/tmp/test"
    SF-OPTION  _"Sheet size"       '("640 x 480"
                                     "800 x 600"
                                     "1024 x 768"
                                     "1280 x 1024"
                                     "1600 x 1200")
    SF-FONT    _"Title font"       "Sans Bold Italic"
    SF-FONT    _"Legend font"      "Sans Bold"
    SF-COLOR   _"Text color"       "white"
    SF-COLOR   _"Background color" "black"
)

(script-fu-menu-register "script-fu-contactsheet" "<Image>/Filters/Combine")
