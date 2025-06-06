; test Image SamplePoint methods of PDB

; Note similar API for guides and tattoos, not yet tested.

; Using numeric equality operator '=' on numeric ID's

(script-fu-use-v3)


; setup
(define testImage (gimp-image-new 21 22 RGB))


(test! "new image has no sample points")
; Passing 0 means get first.
; Returns 0 if no sample points are present
(assert `(= (gimp-image-find-next-sample-point ,testImage 0)
            0 ))


(test! "add sample point")
(define testSamplePoint
  (gimp-image-add-sample-point testImage 10 11))

(test! "New sample point is at position given")
; tests gimp-image-get-sample-point-position
; Returns a list of two elements, the x and y coordinates of the sample point.
(assert `(= (car (gimp-image-get-sample-point-position ,testImage ,testSamplePoint))
            10))
; cdr is still a list, so we need to use cadr to get the second element.
(assert `(= (cadr (gimp-image-get-sample-point-position ,testImage ,testSamplePoint))
            11))

(test! "sample point is added to image")
(assert `(= (gimp-image-find-next-sample-point ,testImage 0)
            ,testSamplePoint))

(test! "First sample point is last sample point")
(assert `(= (gimp-image-find-next-sample-point ,testImage ,testSamplePoint)
            0))

(test! "add another sample point")
(define testSamplePoint2
  (gimp-image-add-sample-point testImage 12 13))

(test! "Next from first sample point is second")
(assert `(= (gimp-image-find-next-sample-point ,testImage ,testSamplePoint)
            ,testSamplePoint2))

(test! "Next from second sample point is 0")
(assert `(= (gimp-image-find-next-sample-point ,testImage ,testSamplePoint2)
            0))

(test! "delete first sample point")
(gimp-image-delete-sample-point testImage testSamplePoint)

(test! "First sample point is now the second one we added")
(assert `(= (gimp-image-find-next-sample-point ,testImage 0)
            ,testSamplePoint2))

(test! "delete second added sample point using its ID")
(gimp-image-delete-sample-point testImage testSamplePoint2)

(test! "No sample points left")
(assert `(= (gimp-image-find-next-sample-point ,testImage 0)
            0))

(test! "Delete sample point using non-existing ID")
; testSamplePoint is already deleted.
; The error is not from Script-Fu pre-flight, but from the PDB i.e. the GIMP core.
(assert-error `(gimp-image-delete-sample-point ,testImage ,testSamplePoint)
              "Procedure execution of gimp-image-delete-sample-point failed on invalid input arguments")

(test! "Delete sample point using zero ID")
; 0 is not a valid sample point ID.
; The error is not from Script-Fu pre-flight, but from the PDB i.e. the GIMP core.
(assert-error `(gimp-image-delete-sample-point ,testImage 0)
              "Procedure execution of gimp-image-delete-sample-point failed on invalid input arguments")

(test! "Delete sample point using negative ID")
; Negative ID is not a valid sample point ID.
; The error is not from Script-Fu pre-flight, but from the PDB i.e. the GIMP core.
; The preflight check in Script-Fu does not catch this,
; because it is a flawed signed to unsigned comparison,
; and C coerces the negative value to a large positive value.
(assert-error `(gimp-image-delete-sample-point ,testImage -1)
              "Procedure execution of gimp-image-delete-sample-point failed on invalid input arguments")

(script-fu-use-v2)