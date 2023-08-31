
; Test methods of vector class of the PDB

; aka Path.  Image has set of Paths.  Path has strokes.


;            setup

(define testImage (car (gimp-image-new 21 22 RGB)))
(gimp-message "testImage is:" (number->string testImage))





;      ID methods

; ensure ID 0 and negative are not vectors
; FIXME #f/#t
(assert '(= (car (gimp-item-id-is-vectors 0))
            0))  ; FUTURE #f

; Test valid ID is tested drive-by



;         image get/set vectors methods
; This sequence of tests requires image 6 has no vectors yet

; ensure get-vectors from image having no vectors yields zero vectors
; FUTURE: returns just #(), not (0 #())
(assert `(= (car (gimp-image-get-vectors ,testImage))
            0))


; setup, not an assert
; vectors-new succeeds
(define testPath (car (gimp-vectors-new
                        testImage
                        "Test Path")))


; !!! id is valid even though vectors is not inserted in image
(assert `(= (car (gimp-item-id-is-vectors ,testPath))
            1))  ; #t

; new path name is as given
(assert `(string=?
            (car (gimp-item-get-name ,testPath))
            "Test Path"))

; new vectors is not in image yet
; image still has count of vectors == 0
(assert `(= (car (gimp-image-get-vectors ,testImage))
            0))

; new path has no strokes
; path has stroke count == 0
(assert `(= (car (gimp-vectors-get-strokes ,testPath))
            0))


; insert vector in image yields (#t)
(assert  `(car (gimp-image-insert-vectors
                  ,testImage
                  ,testPath
                  0 0))) ; parent=0 position=0

; image with inserted vectors now has count of vectors == 1
(assert `(= (car (gimp-image-get-vectors ,testImage))
            1))

; FIXME: crashes in gimpvectors-export.c line 234
; possibly because path has no strokes?
; export to string succeeds
;(assert `(gimp-vectors-export-to-string
;            ,testImage
;            ,testPath))

; export-to-string all
; FAIL: crashes
; PDB doc says 0 should work, and ScriptFu is marshalling to a null GimpVectors*
; so the PDB function in C is at fault?
;(assert `(gimp-vectors-export-to-string
;            ,testImage
;            0))
