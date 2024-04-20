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
(assert-PDB-true `(gimp-item-id-is-channel ,testChannel))


;               attributes

; color attribute is as given during creation
(assert `(equal?
            (car (gimp-channel-get-color ,testChannel))
            '(255 0 0)))  ; red
; gimp-channel-get-name is deprecated
(assert `(string=?
            (car (gimp-item-get-name ,testChannel))
            "Test Channel"))




;               insert

; insert succeeds
(assert `(gimp-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack

; insert was effective: testImage now has one channel
(assert `(= (car (gimp-image-get-channels ,testImage))
            1))

; insert was effective: image now knows by name
; capture the ID of channel we just newed
(assert `(=
           (car (gimp-image-get-channel-by-name
                ,testImage
                "Test Channel"))
           ,testChannel))


;              remove

; Note the difference between remove and delete:
; Docs say that delete is only useful for a channel not added to the image.

; remove does not throw
(assert `(gimp-image-remove-channel ,testImage ,testChannel))

; Effective: image now has zero channels
(assert `(= (car (gimp-image-get-channels ,testImage))
            0))

; After remove, channel ID is NOT valid
(assert-PDB-false `(gimp-item-id-is-channel ,testChannel))

; Delete throws error when channel already removed
; gimp-channel-delete is deprecated
(assert-error `(gimp-item-delete ,testChannel)
              "runtime: invalid item ID"  )


;               delete

; Can delete a new channel not yet added to image

(define testChannel2 (car (gimp-channel-new
            testImage    ; image
            23 24          ; width, height
            "Test Channel" ; name
            50.0           ; opacity
            "red" )))      ; compositing color

; Does not throw
(assert `(gimp-item-delete ,testChannel2))

; Effective: ID is not valid
(assert-PDB-false `(gimp-item-id-is-channel ,testChannel))


