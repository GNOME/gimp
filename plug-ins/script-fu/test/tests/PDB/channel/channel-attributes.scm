; Test get/set methods of Channel class of the PDB


(script-fu-use-v3)


; setup (not in an assert and not quoted)

; new, empty image
(define testImage (gimp-image-new 21 22 RGB))

(define testChannel
      (gimp-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" ))       ; compositing color

(test! "insert-channel")

; new channel is not in image until inserted
(assert `(gimp-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack


(test! "set/get channel attributes")

; color
(assert `(gimp-channel-set-color ,testChannel "red"))
; effective, getter returns same color: red
(assert `(equal?
            (gimp-channel-get-color ,testChannel)
            '(255 0 0 255)))

; opacity
(assert `(gimp-channel-set-opacity ,testChannel 0.7))
; effective
; numeric equality
; Compare floats to some fixed epsilon precision
; Otherwise, the test is fragile to changes in the tested code babl, gimp etc.
; Actual result is 0.7000000216
(assert `(equal-relative?
            (gimp-channel-get-opacity ,testChannel)
            0.7
            0.0001))

; show-masked
(assert `(gimp-channel-set-show-masked ,testChannel TRUE))
; effective
; procedure returns boolean, #t
(assert `(gimp-channel-get-show-masked ,testChannel))




(test! "item methods applied to channel")

; gimp-channel-set-name is deprecated
; gimp-channel-set-visible is deprecated
; etc.

; name
(assert `(gimp-item-set-name ,testChannel "New Name"))
; effective
(assert `(string=?
            (gimp-item-get-name ,testChannel)
            "New Name"))

; visible
(assert `(gimp-item-set-visible ,testChannel FALSE))
; effective
; procedure returns boolean #f
(assert `(not (gimp-item-get-visible ,testChannel)))

; tattoo
(assert `(gimp-item-set-tattoo ,testChannel 999))
; effective
(assert `(=
            (gimp-item-get-tattoo ,testChannel)
            999))

; TODO  other item methods

(script-fu-use-v2)
