; Test various operations on image


;              setup
(define testImage (car (gimp-image-new 21 22 RGB)))


;           transformations

; flip
(assert `(gimp-image-flip ,testImage ORIENTATION-HORIZONTAL))
(assert `(gimp-image-flip ,testImage ORIENTATION-VERTICAL))
; TODO rotate scale resize policy

(assert-error `(gimp-image-flip ,testImage ORIENTATION-UNKNOWN)
    (string-append
      "Procedure execution of gimp-image-flip failed on invalid input arguments: "
      "Procedure 'gimp-image-flip' has been called with value 'GIMP_ORIENTATION_UNKNOWN'"
      " for argument 'flip-type' (#2, type GimpOrientationType). This value is out of range."))

; rotate
(assert `(gimp-image-rotate ,testImage ROTATE-90))
(assert `(gimp-image-rotate ,testImage ROTATE-180))
(assert `(gimp-image-rotate ,testImage ROTATE-270))

; scale
; up
(assert `(gimp-image-scale ,testImage 100 100))

; down to min
(assert `(gimp-image-scale ,testImage 1 1))

; up to max
; Performance:
; This seems to work fast when previous scaled to 1,1
; but then seems to slow down testing
; unless we scale down afterwards.
; This seems glacial if not scaled to 1,1 prior.
(assert `(gimp-image-scale ,testImage 524288 524288))

; down to min
(assert `(gimp-image-scale ,testImage 1 1))


;          policy ops

; 0 means non-interactive
(assert `(gimp-image-policy-color-profile ,testImage 0))
(assert `(gimp-image-policy-rotate ,testImage 0))



; freezing and unfreezing (avoid updates to dialogs)
; Used for performance.
(assert `(gimp-image-freeze-channels ,testImage))
(assert `(gimp-image-freeze-layers ,testImage))
(assert `(gimp-image-freeze-vectors ,testImage))
(assert `(gimp-image-thaw-channels ,testImage))
(assert `(gimp-image-thaw-layers ,testImage))
(assert `(gimp-image-thaw-vectors ,testImage))

; clean-all makes image not dirty
(assert `(gimp-image-clean-all ,testImage))
(assert `(=
            (car (gimp-image-is-dirty ,testImage))
            0))

; TODO test flatten is effective
; crop


;                painting ops
; TODO
; heal
; erase
; smudge
; pencil
; clone
; airbrush

; cannot flatten empty image
(assert-error `(gimp-image-flatten ,testImage)
  "Procedure execution of gimp-image-flatten failed: Cannot flatten an image without any visible layer.")
