
; Test methods of Path class of the PDB

; Formerly known as "Vectors".
; Model: Image has set of Paths.  Path has strokes.


(script-fu-use-v3)

;            setup

(define testImage (gimp-image-new 21 22 RGB))
(gimp-message "testImage is:" (number->string testImage))





;      ID methods

; ensure ID 0 and negative are not paths
(assert '(not (gimp-item-id-is-path 0)))


; Test valid ID is tested drive-by



(test! "image get/set paths methods")

; This sequence of tests requires testImage has no paths yet

; ensure get-paths from image having no paths yields list length zero
; FUTURE: returns just #(), not (0 #())
(assert `(= (car (gimp-image-get-paths ,testImage))
            0))


; setup, not an assert
; path-new succeeds and returns a single ID.
(define testPath (gimp-path-new
                        testImage
                        "Test Path"))


; !!! id is valid even though path is not inserted in image
(assert `(gimp-item-id-is-path ,testPath))

; new path's name is as given
(assert `(string=?
            (gimp-item-get-name ,testPath)
            "Test Path"))

; new path is not in image yet
; image still has count of paths == 0
(assert `(= (car (gimp-image-get-paths ,testImage))
            0))

; new path has no strokes
; path has stroke count == 0
(assert `(= (car (gimp-path-get-strokes ,testPath))
            0))


; insert path in image succeeds
; C returns void, yields #t in scheme
(assert  `(gimp-image-insert-path
                  ,testImage
                  ,testPath
                  0 0)) ; parent=0 position=0

; image with one inserted path now has count of paths == 1
(assert `(= (car (gimp-image-get-paths ,testImage))
            1))


(test! "path export methods")

; export single path to string succeeds
(assert `(gimp-path-export-to-string
            ,testImage
            ,testPath))

; export all paths to string all succeeds
; passing 0 for path means "all"
; FIXME this is wierd, should be a separate method gimp-image-export-paths-to-string
; The name implies a single path
(assert `(gimp-path-export-to-string
            ,testImage
            0))

; export single path to file succeeds
(assert `(gimp-path-export-to-file
            ,testImage
            "tmp.svg"
            ,testPath))

; export all paths to file succeeds
(assert `(gimp-path-export-to-file
            ,testImage
            "tmp2.svg"
            0))

(script-fu-use-v2)

