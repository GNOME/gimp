; Test various operations on image

; The operand is the total image itself.
; For paint operations (changing a subset of the image) see paint.scm


(script-fu-use-v3)


;              setup
(define testImage (gimp-image-new 21 22 RGB))


(test! "image transformations")

; flip
(assert `(gimp-image-flip ,testImage ORIENTATION-HORIZONTAL))
(assert `(gimp-image-flip ,testImage ORIENTATION-VERTICAL))

(assert-error `(gimp-image-flip ,testImage ORIENTATION-UNKNOWN)
    (string-append
      "Procedure execution of gimp-image-flip failed on invalid input arguments: "
      "Procedure 'gimp-image-flip' has been called with value 'GIMP_ORIENTATION_UNKNOWN'"
      " for argument 'flip-type' (#2, type GimpOrientationType). This value is out of range."))

; rotate
; v2 ROTATE-90 => v3 ROTATE-DEGREES90
(assert `(gimp-image-rotate ,testImage ROTATE-DEGREES90))
(assert `(gimp-image-rotate ,testImage ROTATE-DEGREES180))
(assert `(gimp-image-rotate ,testImage ROTATE-DEGREES270))

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
; FIXME throws GLib-GObject-CRITICAL value "524288.000000" of type 'gdouble'
; is invalid or out of range for property 'x' of type 'gdouble'
; but docs say 524288 is the max
; (assert `(gimp-image-scale ,testImage 524288 524288))

; down to min does not throw
(assert `(gimp-image-scale ,testImage 1 1))
; effective
(assert `(= (gimp-image-get-height ,testImage)
            1))
; Note there is no get-size, only get-height and width, the origin is always (0,0)


; resize does not throw
(assert `(gimp-image-resize ,testImage
            30 30 ; width height
            0 0)) ; offset
; effective
(assert `(= (gimp-image-get-height ,testImage)
            30))

; resize to layers when image is empty of layers does not throw
(assert `(gimp-image-resize-to-layers ,testImage))
; not effective: height remains the same
; effective
(assert `(= (gimp-image-get-height ,testImage)
            30))

; TODO resize to layers when there is a layer smaller than canvas



; TODO crops that are plugins plug-in-zealouscrop et al

(test! "crop")

(assert `(gimp-image-crop ,testImage
    2 2 ; width height
    2 2 ; x y offset
    ))



(test! "image transformation by policy ops")
; These perform operations (convert or rotate) using a policy in preferences

; 0 means non-interactive, else shows dialog in some cases
(assert `(gimp-image-policy-color-profile ,testImage 0))

(assert `(gimp-image-policy-rotate ,testImage 0))



(test! "freezing and unfreezing (avoid updates to dialogs)")

; Used for performance.
(assert `(gimp-image-freeze-channels ,testImage))
(assert `(gimp-image-freeze-layers ,testImage))
(assert `(gimp-image-freeze-paths ,testImage))
(assert `(gimp-image-thaw-channels ,testImage))
(assert `(gimp-image-thaw-layers ,testImage))
(assert `(gimp-image-thaw-paths ,testImage))

(test! "clean-all makes image not dirty")
(assert `(gimp-image-clean-all ,testImage))
(assert `(not (gimp-image-is-dirty ,testImage)))


; flatten is tested in layer-ops.scm

; cannot flatten empty image
(assert-error `(gimp-image-flatten ,testImage)
  "Procedure execution of gimp-image-flatten failed: Cannot flatten an image without any visible layer.")

(script-fu-use-v2)