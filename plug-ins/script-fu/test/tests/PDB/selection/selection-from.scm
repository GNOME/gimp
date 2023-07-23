; test PDB methods that change selection from existing selection


; setup
; Reusing image 10
(define testImage 10)


; Test a selection-changing function
; starting from selection None.
;
; The testFunction takes a "step" arg
; and does not change the selection bounds.

; {none <func> is-empty} yields true
; {none <func>} is not an error

(define (test-selection-change-from-none testFunction testImage)
  ; Starting state: selection none
  (assert `(gimp-selection-none ,testImage))
  ; test the testFunction
  (assert `(,testFunction
            ,testImage
            4 )) ; radius or step
  ; expect selection is still empty
  (assert `(= (car (gimp-selection-is-empty ,testImage))
            1))
  ; expect since there is no selection, the bounds are the entire image
  (assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                  '(0 0 21 22)))
)

(define (test-selection-change-from-all testFunction testImage isIdempotent)
  ; Starting state: selection all
  (assert `(gimp-selection-all ,testImage))
  ; test the testFunction
  (assert `(,testFunction
            ,testImage
            4 )) ; radius or step

  (if isIdempotent
    (begin
      ; expect selection is still not empty
      (assert `(= (car (gimp-selection-is-empty ,testImage))
                  0))
      ; expect selection bounds are still entire image
      (assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                        '(0 0 21 22)))))
)





; test selection methods that change by a pixel amount
(test-selection-change-from-none gimp-selection-feather testImage)
(test-selection-change-from-none gimp-selection-grow    testImage)
(test-selection-change-from-none gimp-selection-shrink  testImage)
(test-selection-change-from-none gimp-selection-border  testImage)

; feather and grow from all are idempotent
(test-selection-change-from-all gimp-selection-feather testImage #t)
(test-selection-change-from-all gimp-selection-grow    testImage #t)

(test-selection-change-from-all gimp-selection-shrink  testImage #f)
; shrink from all changes bounds
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(4 4 17 18)))
(test-selection-change-from-all gimp-selection-border  testImage #f)
; border from all empties the selection
(assert `(= (car (gimp-selection-is-empty ,testImage))
                  1))




;                 Effectiveness
; When starting from a typical selection (not empty, not all)

; TODO feather effective?
; Might feather change bounds?

; grow is effective
; bounds are larger
; TODO
(assert `(equal? (cdr (gimp-selection-bounds ,testImage))
                 '(0 0 21 22)))

; TODO test flood effective: holes were filled
; Can't do it without knowing how many pixels are selected?
; Knowing bounds is not adequate.

; Simple tests of success
(assert `(gimp-selection-flood ,testImage))
(assert `(gimp-selection-invert ,testImage))
(assert `(gimp-selection-sharpen ,testImage))
(assert `(gimp-selection-translate
            ,testImage
            4 4))

; TODO invert none is all and vice versa

; TODO translate effective
; TODO translate by large offset is empty selection
; TODO sharpen is effective at removing antialiasing

; save creates a new channel