; Test methods of Buffer class of the PDB

; aka NamedBuffer i.e. the clipboard saved with a name.

; Edit methods that create buffers is tested elsewhere.
; The names of those methods is hard to understand:
; because they used "named" to mean "buffer"
; E.G. gimp-edit-named-copy might be better named: "edit-copy-to-named-buffer"

; The API has no method to determine if the clipboard is empty
; buffers-get-list only gets the named buffers



; Prereq:  no buffer exists yet.

(script-fu-use-v3)

; setup
; Load test image that already has drawable
(define testImage (testing:load-test-image-basic-v3))

; the layer is the zeroeth element in the vector which is the second element
; but cadr returns the second element!!
; TODO make this a library routine: get-first-layer
; (1 #(<layerID>))
(define testDrawable (vector-ref (cadr (gimp-image-get-layers testImage ))
                                  0))

(test! "Create new named buffer")

; There is no gimp-buffer-new method,
; instead it is a method of the Edit class so-to-speak
; You can't: #(testDrawable)
(define testBuffer (gimp-edit-named-copy
                              1
                              (make-vector 1 testDrawable)
                              "bufferName"))
; Since no selection, the buffer is same size as image

; Creation was effective: gimp knows the buffer
; get-list takes a regex, here empty ""
; get-list returns  (("bufferName")) : a list of strings
; and the first string is "bufferName"
(assert `(string=? (car (gimp-buffers-get-list ""))
                    "bufferName"))

; buffer has same size as image when created with no selection
; test image is 128x128
(assert `(= (gimp-buffer-get-width "bufferName")
            128))
(assert `(= (gimp-buffer-get-height "bufferName")
            128))

; new buffer has alpha: the image is RGB but the buffer has bpp 4
; This is not well documented.
; FIXME the docs and the method name should say "bpp"
; or "bytes per pixel" instead of "bytes"
(assert `(= (gimp-buffer-get-bytes "bufferName")
            4))

; image type is RGBA
; FIXME: the docs erroneously say "ImageBaseType" => "ImageType"
(assert `(= (gimp-buffer-get-image-type "bufferName")
            RGBA-IMAGE))



(test! "buffer-rename")

; Renaming returns the given name if it doesn't clash with existing name.
(assert `(string=? (gimp-buffer-rename "bufferName" "renamedName")
                  "renamedName"))

; Effect renaming: gimp knows the renamed name
(assert `(string=? (car (gimp-buffers-get-list ""))
                    "renamedName"))

; Renaming does not add another buffer
(assert `(= (length (gimp-buffers-get-list ""))
            1))


(test! "buffer-delete")

; Delete evaluates but is void
(assert `(gimp-buffer-delete "renamedName"))

; Delete was effective: gimp no longer knows
; and returns nil i.e. empty list (())
(assert `(null? (gimp-buffers-get-list "")))


; TODO test two buffers

; TODO test renaming when name already in use

(script-fu-use-v2)