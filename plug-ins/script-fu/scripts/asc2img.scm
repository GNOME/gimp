; Chris Gutteridge / ECS Dept. University of Southampton, England
; "ASCII 2 Image" script for the Gimp.
;
; 8th April 1998
;
; Takes a filename of an ASCII file and converts it to a gimp image.
; Does sensible things to preserve indents (gimp-text strips them)
;
; cjg@ecs.soton.ac.uk

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

; Define the function:

(define (script-fu-asc-2-img inFile
                           inFont
                           inFontSize
                           inTextColor
                           inTrans
                           inBackColor
                           inBufferAmount)

  (let* (
        (theImage (car (gimp-image-new 10 10 RGB) ) )
        (theLayer (car (gimp-layer-new theImage
                                       10
                                       10
                                       RGBA-IMAGE
                                       "layer 1"
                                       100
                                       NORMAL-MODE) ) )
        (theImageWidth 0)
        (theImageHeight 0)
        (theBuffer)
        )

  (gimp-context-push)
  (gimp-context-set-background inBackColor)
  (gimp-drawable-set-name theLayer "Background")
  (gimp-image-add-layer theImage theLayer 0)

  (script-fu-asc-2-img-layer theImage theLayer inFile inFont inFontSize
                           inTextColor inBufferAmount)

  (set! theImageWidth (car (gimp-drawable-width
                             (car (gimp-image-get-active-layer theImage)))))
  (set! theImageHeight (car (gimp-drawable-height
                              (car (gimp-image-get-active-layer theImage)))))
  (set! theBuffer (* inFontSize (/ inBufferAmount 100) ) )
  (set! theImageWidth (+ theImageWidth theBuffer theBuffer ))
  (set! theImageHeight (+ theImageHeight theBuffer ))

  (gimp-image-resize theImage theImageWidth theImageHeight theBuffer theBuffer)
  (gimp-layer-resize theLayer theImageWidth theImageHeight theBuffer theBuffer)

  (gimp-selection-all theImage)
  (if (= inTrans TRUE)
      (gimp-edit-clear theLayer)
      (gimp-edit-fill theLayer BACKGROUND-FILL)
      )
  (gimp-selection-none theImage)
 
  (gimp-display-new theImage)
 
  (gimp-displays-flush)
  (gimp-context-pop)
  )
)

(define (script-fu-asc-2-img-layer inImage
                                 inLayer
                                 inFile
                                 inFont
                                 inFontSize
                                 inTextColor)

  (let* (
        (theImage inImage)
        (theLayer inLayer)
        (theFile (fopen inFile))

        (otherLayers (cadr (gimp-image-get-layers theImage)))
        (nLayers (car (gimp-image-get-layers theImage)))
        (n nLayers)
        (theImageWidth 0)
        (theImageHeight 0)
        (theData)
        (theIndentList)
        (theChar)
        (allspaces)
        (theIndent)
        (theLine)
        (theChar)
        (theText)
        (theCharWidth)
        )

    (define (cjg-add-text inData inIndentList inFont inFontSize)
      (if (equal? () inData)
        ()
        (let (
             (theLine (car inData))
             (theIndent (car inIndentList))
             (theLineHeight 0)
             (theText)
             )

          (if (equal? "" theLine)
            ()
            (begin
              (set! theText (car (gimp-text-fontname inImage
                                                     -1
                                                     0
                                                     0
                                                     theLine
                                                     0
                                                     TRUE
                                                     inFontSize
                                                     PIXELS
                                                     inFont)))
              (set! theLineHeight (car (gimp-drawable-height theText)))
              (gimp-layer-set-offsets theText
                                      (* theCharWidth theIndent)
                                      (+ theImageHeight
                                         (- inFontSize theLineHeight)))
              (set! theImageWidth (max
                                   (+ (car (gimp-drawable-width  theText))
                                      (* theCharWidth theIndent))
                                   theImageWidth ))
              (if (= (car (gimp-layer-is-floating-sel theText)) TRUE)
                  (gimp-floating-sel-anchor theText)
              )
              (gimp-drawable-set-name theText theLine)
            )
          )

          (cjg-add-text (cdr inData) (cdr inIndentList) inFont inFontSize)
        )
      )
    )

  (gimp-context-push)
  (gimp-context-set-foreground inTextColor)
  (gimp-selection-none theImage)
  (set! theData ())
  (set! theIndentList ())
  (set! theChar "X")
  (while (not (equal? () theChar))
         (set! allspaces TRUE)
         (set! theIndent 0)
         (set! theLine "")
         (while (begin
                  (set! theChar (fread 1 theFile))
                  (and (not (equal? "\n" theChar))
                       (not (equal? () theChar))
                  )
                )
                (cond
                  (
                    (equal? theChar "\t")
                    (set! theChar "        ")
                    (if (= allspaces TRUE)
                        (set! theIndent (+ theIndent 8))
                    )
                  )
                  (
                    (equal? theChar " ")
                    (if (= allspaces TRUE)
                        (set! theIndent (+ theIndent 1))
                    )
                  )
                  (TRUE (set! allspaces FALSE))
                )
                (set! theLine (string-append theLine theChar))
         )
         (if (= allspaces TRUE)
            (set! theLine "")
         )
         (if (and (equal? () theChar)
                  (equal? "" theLine)
             )
             ()
             (begin
               (set! theData (cons theLine theData))
               (set! theIndentList (cons theIndent theIndentList))
             )
         )
  )
 
  (set! theText (car (gimp-text-fontname theImage
                                         -1
                                         0
                                         0
                                         "X"
                                         0
                                         TRUE
                                         inFontSize
                                         PIXELS
                                         inFont)))
  (set! theCharWidth (car (gimp-drawable-width  theText)))
  (gimp-edit-cut theText)

  (cjg-add-text (reverse theData) (reverse theIndentList) inFont inFontSize)

  (gimp-context-pop)
  (gimp-displays-flush)
  )
)

; Register the function with the GIMP:

(script-fu-register
    "script-fu-asc-2-img"
    _"<Toolbox>/Xtns/Script-Fu/Utils/_ASCII to Image..."
    "Create a new image containing text from a simple text file"
    "Chris Gutteridge: cjg@ecs.soton.ac.uk"
    "8th April 1998"
    "Chris Gutteridge / ECS @ University of Southampton, England"
    ""
    SF-FILENAME   _"Filename"               "afile"
    SF-FONT       _"Font"                   "Bitstream Charter"
    SF-ADJUSTMENT _"Font size (pixels)"     '(45 2 1000 1 10 0 1)
    SF-COLOR      _"Text color"             '(0 0 0)
    SF-TOGGLE     _"Transparent background" FALSE
    SF-COLOR      _"Background color"       '(255 255 255)
    SF-ADJUSTMENT _"Buffer amount (% height of text)" '(35 0 100 1 10 0 0)
)

(script-fu-register "script-fu-asc-2-img-layer"
    _"<Image>/Script-Fu/Utils/_ASCII to Layer..."
    "Create a new layer of text from a simple text file"
    "Chris Gutteridge: cjg@ecs.soton.ac.uk"
    "30th April 1998"
    "Chris Gutteridge / ECS @ University of Southampton, England"
    "*"
    SF-IMAGE      "Image"               0
    SF-DRAWABLE   "Layer"               0
    SF-FILENAME   _"File name"          "afile"
    SF-FONT       _"Font"               "Bitstream Charter"
    SF-ADJUSTMENT _"Font size (pixels)" '(45 2 1000 1 10 0 1)
    SF-COLOR      _"Text color"         '(0 0 0)
)
