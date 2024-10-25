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
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

; Resizes the image so as to include the selected layer.
; The resulting image has the selected layer size.
; Copyright (C) 2002 Chauk-Mean PROUM
;
(define (script-fu-util-image-resize-from-layer image layer)
  (let* (
        (width (car (gimp-drawable-get-width layer)))
        (height (car (gimp-drawable-get-height layer)))
        (posx (- (car (gimp-drawable-get-offsets layer))))
        (posy (- (cadr (gimp-drawable-get-offsets layer))))
        )

    (gimp-image-resize image width height posx posy)
  )
)

; Add the specified layers to the image.
; The layers will be added in the given order below the
; active layer.
;
(define (script-fu-util-image-add-layers image . layers)
  (while (not (null? layers))
    (let ((layer (car layers)))
      (set! layers (cdr layers))
      (gimp-image-insert-layer image layer 0 -1)
      (gimp-image-lower-item image layer)
    )
  )
)



; Allow command line usage of GIMP such as:
;
;     gimp -i -b '(with-files "*.png" <body>)'
;
; where <body> is the code that handles whatever processing you want to
; perform on the files. There are four variables that are available
; within the <body>: 'basename', 'image', 'filename' and 'layer'.
; The 'basename' is the name of the file with its extension removed,
; while the other three variables are self-explanatory.
; You basically write your code as though it were processing a single
; 'image' and the 'with-files' macro applies it to all of the files
; matching the pattern.
;
; For example, to invert the colors of all of the PNG files in the
; start directory:
;
;    gimp -i -b '(with-files "*.png" (gimp-drawable-invert layer FALSE) \
;                 (gimp-file-save 1 image layer filename))'
;
; To do the same thing, but saving them as jpeg instead:
;
;    gimp -i -b '(with-files "*.png" (gimp-drawable-invert layer FALSE) \
;                 (gimp-file-save 1 image layer \
;                  (string-append basename ".jpg") ))'

; butlast is needed below.
; Not in R5RS but in simply-scheme and chicken dialects.
; aka drop-right.
; Returns new list with right element deleted.
(define (butlast x)
  (if (= (length x) 1)
    '()
    (reverse (cdr (reverse x)))
  )
)

(define-macro (with-files pattern . body)
  (let ((loop (gensym))
        (filenames (gensym))
        (filename (gensym)))
    `(begin
       (let ,loop ((,filenames (cadr (file-glob ,pattern 1))))
         (unless (null? ,filenames)
           (let* ((filename (car ,filenames))
                  (image (catch #f (car (gimp-file-load RUN-NONINTERACTIVE
                                                        filename))))
                  (layer (if image (vector-ref (gimp-image-get-selected-layers image) 0) #f))
                  (basename (unbreakupstr (butlast (strbreakup filename ".")) ".")))
             (when image
               ,@body
               (gimp-image-delete image)))
           (,loop (cdr ,filenames))
         )
       )
     )
  )
)
