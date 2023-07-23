; Test methods of Channel class of the PDB



; setup
; Reusing image 10
(define testImage 10)


; new image has no custom channels
(assert `(= (car (gimp-image-get-channels ,testImage))
            0))

; vectors-new succeeds
(assert `(car (gimp-channel-new
            ,testImage    ; image
            23 24          ; width, height
            "Test Channel" ; name
            50.0           ; opacity
            "red" )))      ; compositing color

(define testChannel 20)

; new returns a valid ID
(assert `(= (car (gimp-item-id-is-channel ,testChannel))
            1))  ; #t

; new channel is not in image until inserted
(assert `(= (car (gimp-image-get-channels ,testImage))
            0))


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
(assert `(= (car (gimp-image-get-channels ,testImage))
            1))

