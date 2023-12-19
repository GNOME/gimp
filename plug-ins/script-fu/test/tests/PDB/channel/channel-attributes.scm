; Test get/set methods of Channel class of the PDB



; setup (not in an assert and not quoted)

; new, empty image
(define testImage (car (gimp-image-new 21 22 RGB)))

(define testChannel (car (gimp-channel-new
            testImage    ; image
            23 24          ; width, height
            "Test Channel" ; name
            50.0           ; opacity
            "red" )))      ; compositing color

; new channel is not in image until inserted
(gimp-image-insert-channel
            testImage
            testChannel
            0            ; parent, moot since channel groups not supported
            0)           ; position in stack


; tests

; color
(assert `(gimp-channel-set-color ,testChannel "red"))
; effective
; FIXME, SF is broken, should be red
(assert `(equal?
            (car (gimp-channel-get-color ,testChannel))
            '(0 0 0)))

; opacity
(assert `(gimp-channel-set-opacity ,testChannel 0.7))
; effective
(assert `(equal?
            (car (gimp-channel-get-opacity ,testChannel))
            0.7))

; show-masked
(assert `(gimp-channel-set-show-masked ,testChannel TRUE))
; effective
(assert `(=
            (car (gimp-channel-get-show-masked ,testChannel))
            TRUE))





;               item methods applied to channel

; gimp-channel-set-name is deprecated
; gimp-channel-set-visible is deprecated
; etc.

; name
(assert `(gimp-item-set-name ,testChannel "New Name"))
; effective
(assert `(string=?
            (car (gimp-item-get-name ,testChannel))
            "New Name"))

; visible
(assert `(gimp-item-set-visible ,testChannel FALSE))
; effective
(assert `(=
            (car (gimp-item-get-visible ,testChannel))
            FALSE))

; tattoo
(assert `(gimp-item-set-tattoo ,testChannel 999))
; effective
(assert `(=
            (car (gimp-item-get-tattoo ,testChannel))
            999))

; TODO  other item methods