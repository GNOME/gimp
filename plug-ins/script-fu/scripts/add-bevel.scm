; GIMP - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; add-bevel.scm version 1.04
; Time-stamp: <2004-02-09 17:07:06 simon>
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
; Copyright (C) 1997 Andrew Donkin  (ard@cs.waikato.ac.nz)
; Contains code from add-shadow.scm by Sven Neumann
; (neumanns@uni-duesseldorf.de) (thanks Sven).
;
; Adds a bevel to an image.  See http://www.cs.waikato.ac.nz/~ard/gimp/
;
; If there is a selection, it is bevelled.
; Otherwise if there is an alpha channel, the selection is taken from it
; and bevelled.
; Otherwise the part of the layer inside the image boundries is bevelled.
;
; The selection is set on exit, so Select->Invert then Edit->Clear will
; leave a cut-out.  Then use Sven's add-shadow for that
; floating-bumpmapped-texture cliche.

;
; 1.01: now works on offset layers.
; 1.02: has crop-pixel-border option to trim one pixel off each edge of the
;       bevelled image.  Bumpmapping leaves edge pixels unchanged, which
;       looks bad.  Oddly, this is not apparant in GIMP - you have to
;       save the image and load it into another viewer.  First noticed in
;       Nutscrape.
;       Changed path (removed "filters/").
; 1.03: adds one-pixel border before bumpmapping, and removes it after.
;       Got rid of the crop-pixel-border option (no longer reqd).
; 1.04: Fixed undo handling, ensure that bumpmap is big enough,
;       (instead of resizing the image). Removed references to outdated
;       bumpmap plugin.     (Simon)
; 1.05  When there is no selection, bevel the whole layer instead of the
;       whole image (which was broken in the first place).
;       Also fixed some bugs with setting the selection when there is no
;       initial selection.     (Barak Itkin)
;

(define (script-fu-add-bevel img
                             drawable
                             thickness
                             work-on-copy
                             keep-bump-layer)

  (let* (
        (index 1)
        (greyness 0)
        (thickness (abs thickness))
        (type (car (gimp-drawable-type-with-alpha drawable)))
        (image (if (= work-on-copy TRUE) (car (gimp-image-duplicate img)) img))
        (pic-layer (car (gimp-image-get-active-drawable image)))
        (offsets (gimp-drawable-offsets pic-layer))
        (width (car (gimp-drawable-width pic-layer)))
        (height (car (gimp-drawable-height pic-layer)))

        ; Bumpmap has a one pixel border on each side
        (bump-layer (car (gimp-layer-new image
                                         (+ width 2)
                                         (+ height 2)
                                         RGB-IMAGE
                                         _"Bumpmap"
                                         100
                                         NORMAL-MODE)))

        (selection-exists (car (gimp-selection-bounds image)))
        (selection 0)
        )

    (gimp-context-push)

    ; disable undo on copy, start group otherwise
    (if (= work-on-copy TRUE)
      (gimp-image-undo-disable image)
      (gimp-image-undo-group-start image)
    )

    (gimp-image-insert-layer image bump-layer -1 1)

    ; If the layer we're bevelling is offset from the image's origin, we
    ; have to do the same to the bumpmap
    (gimp-layer-set-offsets bump-layer (- (car offsets) 1)
                                       (- (cadr offsets) 1))

    ;------------------------------------------------------------
    ;
    ; Set the selection to the area we want to bevel.
    ;
    (if (= selection-exists 0)
        (gimp-image-select-item image CHANNEL-OP-REPLACE pic-layer)
    )

    ; Store it for later.
    (set! selection (car (gimp-selection-save image)))
    ; Try to lose the jaggies
    (gimp-selection-feather image 2)

    ;------------------------------------------------------------
    ;
    ; Initialise our bumpmap
    ;
    (gimp-context-set-background '(0 0 0))
    (gimp-drawable-fill bump-layer BACKGROUND-FILL)

    (while (< index thickness)
           (set! greyness (/ (* index 255) thickness))
           (gimp-context-set-background (list greyness greyness greyness))
           ;(gimp-selection-feather image 1) ;Stop the slopey jaggies?
           (gimp-edit-bucket-fill bump-layer BG-BUCKET-FILL NORMAL-MODE
                                  100 0 FALSE 0 0)
           (gimp-selection-shrink image 1)
           (set! index (+ index 1))
    )
    ; Now the white interior
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-bucket-fill bump-layer BG-BUCKET-FILL NORMAL-MODE
                           100 0 FALSE 0 0)

    ;------------------------------------------------------------
    ;
    ; Do the bump.
    ;
    (gimp-selection-none image)

    ; To further lessen jaggies?
    ;(plug-in-gauss-rle RUN-NONINTERACTIVE image bump-layer thickness TRUE TRUE)


    ;
    ; BUMPMAP INVOCATION:
    ;
    (plug-in-bump-map RUN-NONINTERACTIVE image pic-layer bump-layer 125 45 3 0 0 0 0 TRUE FALSE 1)

    ;------------------------------------------------------------
    ;
    ; Restore things
    ;
    (if (= selection-exists 0)
        (gimp-selection-none image)        ; No selection to start with
        (gimp-image-select-item image CHANNEL-OP-REPLACE selection)
    )
    ; If they started with a selection, they can Select->Invert then
    ; Edit->Clear for a cutout.

    ; clean up
    (gimp-image-remove-channel image selection)
    (if (= keep-bump-layer TRUE)
        (gimp-item-set-visible bump-layer 0)
        (gimp-image-remove-layer image bump-layer)
    )

    (gimp-image-set-active-layer image pic-layer)

    ; enable undo / end undo group
    (if (= work-on-copy TRUE)
      (begin
        (gimp-display-new image)
        (gimp-image-undo-enable image)
      )
      (gimp-image-undo-group-end image)
    )

    (gimp-displays-flush)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-add-bevel"
  _"Add B_evel..."
  _"Add a beveled border to an image"
  "Andrew Donkin <ard@cs.waikato.ac.nz>"
  "Andrew Donkin"
  "1997/11/06"
  "RGB*"
  SF-IMAGE       "Image"           0
  SF-DRAWABLE    "Drawable"        0
  SF-ADJUSTMENT _"Thickness"       '(5 0 30 1 2 0 0)
  SF-TOGGLE     _"Work on copy"    TRUE
  SF-TOGGLE     _"Keep bump layer" FALSE
)

(script-fu-menu-register "script-fu-add-bevel" "<Image>/Filters/Decor")
