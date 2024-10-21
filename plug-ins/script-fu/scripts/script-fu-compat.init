; This file provides some compatibility for older scripts.
; By defining pure Scheme functions.
; See also plug-in-compat.init.
;
; The functions in the first half are NOT deprecated.
; They are random number functions, and other functions generally useful.
; The functions are not in Scheme R5RS.
; They should be in a separate file.
;
; The last half of this file are deprecated functions.
; You should not use them in new scripts.
; GIMP maintainers may remove deprecated functions in the future.


;The random number generator routines below have been slightly reformatted.
;A couple of define blocks which are not needed have been commented out.
;It has also been extended to enable it to generate numbers with exactly 31
;bits or more.
;The original file was called rand2.scm and can be found in:
;http://www-2.cs.cmu.edu/afs/cs/project/ai-repository/ai/lang/scheme/code/math/random/

; Minimal Standard Random Number Generator
; Park & Miller, CACM 31(10), Oct 1988, 32 bit integer version.
; better constants, as proposed by Park.
; By Ozan Yigit

;(define *seed* 1)

(define (srand seed)
  (set! *seed* seed)
  *seed*
)

(define (msrg-rand)
  (let (
       (A 48271)
       (M 2147483647)
       (Q 44488)
       (R 3399)
       )
    (let* (
          (hi (quotient *seed* Q))
          (lo (modulo *seed* Q))
          (test (- (* A lo) (* R hi)))
          )
      (if (> test 0)
        (set! *seed* test)
        (set! *seed* (+ test M))
      )
    )
  )
  *seed*
)

; poker test
; seed 1
; cards 0-9 inclusive (random 10)
; five cards per hand
; 10000 hands
;
; Poker Hand     Example    Probability  Calculated
; 5 of a kind    (aaaaa)      0.0001      0
; 4 of a kind    (aaaab)      0.0045      0.0053
; Full house     (aaabb)      0.009       0.0093
; 3 of a kind    (aaabc)      0.072       0.0682
; two pairs      (aabbc)      0.108       0.1104
; Pair           (aabcd)      0.504       0.501
; Bust           (abcde)      0.3024      0.3058

(define (random n)
  (define (internal-random n)
    (let* (
          (n (inexact->exact (truncate n)))
          (M 2147483647)
          (slop (modulo M (abs n)))
          )
      (let loop ((r (msrg-rand)))
        (if (>= r slop)
          (modulo r n)
          (loop (msrg-rand))
        )
      )
    )
  )

  ; Negative numbers have a bigger range in twos complement platforms
  ; (nearly all platforms out there) than positive ones, so we deal with
  ; the numbers in negative form.
  (if (> n 0)
    (+ n (random (- n)))

    (if (>= n -2147483647)
      (internal-random n)

      ; 31-or-more-bits number requested - needs multiple extractions
      ; because we don't generate enough random bits.
      (if (>= n -1152921504606846975)
        ; Up to 2^60-1, two extractions are enough
        (let ((q (- (quotient (+ n 1) 1073741824) 1))) ; q=floor(n/2^30)
          (let loop ()
            (let ((big (+ (* (internal-random q) 1073741824)
                          (internal-random -1073741824)
                       )
                 ))
              (if (> big n)
                big
                (loop)
              )
            )
          )
        )

        ; From 2^60 up, we do three extractions.
        ; The code is better understood if seen as generating three
        ; digits in base 2^30. q is the maximum value the first digit
        ; can take. The other digits can take the full range.
        ;
        ; The strategy is to generate a random number digit by digit.
        ; Here's an example in base 10. Say the input n is 348
        ; (thus requesting a number between 0 and 347). Then the algorithm
        ; first calls (internal-random 4) to get a digit between 0 and 3,
        ; then (internal-random 10) twice to get two more digits between
        ; 0 and 9. Say the result is 366: since it is greater than 347,
        ; it's discarded and the process restarted. When the result is
        ; <= 347, that's the returned value. The probability of it being
        ; greater than the max is always strictly less than 1/2.
        ;
        ; This is the same idea but in base 2^30 (1073741824). The
        ; first digit's weight is (2^30)^2 = 1152921504606846976,
        ; similarly to how in our base 10 example, the first digit's
        ; weight is 10^2 = 100. In the base 10 example we first divide
        ; the target number 348 by 100, taking the ceiling, to get 4.
        ; Here we divide by (2^30)^2 instead, taking the ceiling too.
        ;
        ; The math is a bit obscured by the fact that we generate
        ; the digits as negative, so that the result is negative as
        ; well, but it's really the same thing. Changing the sign of
        ; every digit just changes the sign of the result.
        ;
        ; This method works for n up to (2^30)^2*(2^31-1) which is
        ; 2475880077417839045191401472 (slightly under 91 bits). That
        ; covers the 64-bit range comfortably, and some more. If larger
        ; numbers are needed, they'll have to be composed with a
        ; user-defined procedure.

        (if (>= n -2475880077417839045191401472)
          (let ((q (- (quotient (+ n 1) 1152921504606846976) 1))) ; q=floor(n/2^60)
            (let loop ()
              (let ((big (+ (* (internal-random q) 1152921504606846976)
                            (* (internal-random -1073741824) 1073741824)
                            (internal-random -1073741824)
                         )
                   ))
                (if (> big n)
                  big
                  (loop)
                )
              )
            )
          )
          (error "requested (random n) range too large")
        )
      )
    )
  )
)

;(define (rngtest)
;  (display "implementation ")
;  (srand 1)
;  (do
;    ( (n 0 (+ n 1)) )
;    ( (>= n 10000) )
;    (msrg-rand)
;  )
;  (if (= *seed* 399268537)
;      (display "looks correct.")
;      (begin
;        (display "failed.")
;        (newline)
;        (display "   current seed ") (display *seed*)
;        (newline)
;        (display "   correct seed 399268537")
;      )
;  )
;  (newline)
;)


;This macro defines a while loop which is needed by some older scripts.
;This is here since it is not defined in R5RS and could be handy to have.

;This while macro was found at:
;http://www.aracnet.com/~briand/scheme_eval.html
(define-macro (while test . body)
  `(let loop ()
     (cond
       (,test
         ,@body
         (loop)
       )
     )
   )
)


;The following define block(s) require the tsx extension to be loaded

(define (realtime)
  (car (gettimeofday))
)


; Items below this line are for compatibility.
; The history of their inclusion in ScriptFu can not be easily ascertained.
; Some older ScriptFu scripts may use them.
; Generally speaking "useful but not in R5RS."
; At some point GIMP maintainers may remove them.
; For example if replacements are found in an SRFI,
; and ScriptFu supports use of SRFI's.

(define (delq item lis)
  (let ((l '()))
    (unless (null? lis)
      (while (pair? lis)
        (if (<> item (car lis))
          (set! l (append l (list (car lis))))
        )
        (set! lis (cdr lis))
      )
    )

    l
  )
)

(define (make-list count fill)
  (vector->list (make-vector count fill))
)

(define (strbreakup str sep)
  (let* (
        (seplen (string-length sep))
        (start 0)
        (end (string-length str))
        (i start)
        (l '())
        )

    (if (= seplen 0)
      (set! l (list str))
      (begin
        (while (<= i (- end seplen))
          (if (substring-equal? sep str i (+ i seplen))
            (begin
               (if (= start 0)
                 (set! l (list (substring str start i)))
                 (set! l (append l (list (substring str start i))))
               )
               (set! start (+ i seplen))
               (set! i (+ i seplen -1))
            )
          )

          (set! i (+ i 1))
        )

        (set! l (append l (list (substring str start end))))
      )
    )

    l
  )
)

(define (string-downcase str)
  (list->string (map char-downcase (string->list str)))
)

(define (string-trim str)
  (string-trim-right (string-trim-left str))
)

(define (string-trim-left str)
  (let (
       (strlen (string-length str))
       (i 0)
       )

    (while (and (< i strlen)
                (char-whitespace? (string-ref str i))
           )
      (set! i (+ i 1))
    )

    (substring str i (string-length str))
  )
)

(define (string-trim-right str)
  (let ((i (- (string-length str) 1)))

    (while (and (>= i 0)
                (char-whitespace? (string-ref str i))
           )
      (set! i (- i 1))
    )

    (substring str 0 (+ i 1))
  )
)

(define (string-upcase str)
  (list->string (map char-upcase (string->list str)))
)

(define (substring-equal? str str2 start end)
  (string=? str (substring str2 start end))
)

(define (unbreakupstr stringlist sep)
  (let ((str (car stringlist)))

    (set! stringlist (cdr stringlist))
    (while (not (null? stringlist))
      (set! str (string-append str sep (car stringlist)))
      (set! stringlist (cdr stringlist))
    )

    str
  )
)

; Functions said to be deprecated in GIMP 2 and left still in GIMP 3.
; Infrequently used, but not a one-word substitution.

; symbol pi is not in TinyScheme
(define *pi*
  (* 4 (atan 1.0))
)

; unknown provenance for this function: SIOD?
(define (cons-array count type)
  (case type
    ((long)   (make-vector count 0))
    ((short)  (make-vector count 0))
    ((byte)   (make-vector count 0))
    ((double) (make-vector count 0.0))
    ((string) (vector->list (make-vector count "")))
    (else type)
  )
)


; Functions obsolete as of GIMP v3.0.
; Generally speaking, from the SIOD dialect of Scheme.
; The signatures (or the full definition) are left as comments,
; to aid in debugging old scripts.
; You can recover the definitions from the GIMP repo, say the oldest GIMP 2 version.

;(define aset vector-set!)
;(define aref vector-ref)
;(define fopen open-input-file)
;(define mapcar map)
;(define nil '())
;(define nreverse reverse)
;(define pow expt)
;(define prin1 write)
;(define (print obj . port)
;(define strcat string-append)
;(define string-lessp string<?)
;(define symbol-bound? defined?)
;(define the-environment current-environment)
;(define (fmod a b)
;(define (fread arg1 file)
;(define (fread-get-chars count file)
;(define (last x)
;(define (nth k list)
;(define (prog1 form1 . form2)
;(define (rand . modulus)
;(define (strcmp str1 str2)
;(define (trunc n)
;(define verbose


;Items below this line are deprecated and should not be used in new scripts.

; INTENDED TO BE EMPTY FOR GIMP 3.0
