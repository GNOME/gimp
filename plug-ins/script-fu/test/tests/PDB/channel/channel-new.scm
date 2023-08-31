; Test methods of Channel class of the PDB



; setup
; new, empty image
(define testImage (car (gimp-image-new 21 22 RGB)))


; new image has no custom channels
(assert `(= (car (gimp-image-get-channels ,testImage))
            0))

; setup (not in an assert and not quoted)
; vectors-new succeeds
(define testChannel (car (gimp-channel-new
            testImage    ; image
            23 24          ; width, height
            "Test Channel" ; name
            50.0           ; opacity
            "red" )))      ; compositing color

; new channel is not in image until inserted
; get-channels yields (0 #())
(assert `(= (car (gimp-image-get-channels ,testImage))
            0))

; channel ID is valid
(assert `(= (car (gimp-item-id-is-channel ,testChannel))
            1))  ; #t


;               attributes

; get-color
; FIXME: this passes but should test return red ???
(assert `(equal?
            (car (gimp-channel-get-color ,testChannel))
            '(0 0 0)))




;               insert

; insert succeeds
(assert `(gimp-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack

; insert was effective
; testImage now has one channel
(assert `(= (car (gimp-image-get-channels ,testImage))
            1))

