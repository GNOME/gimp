; Deprecated and should not be used in new scripts.
; GIMP developers strongly recommend you not use this file
; in scripts that you will distribute to other users.

; Mostly definitions from the ancient SIOD dialect of Scheme.

; These were deprecated in GIMP 2 and obsoleted in GIMP 3
; ScriptFu since v3 does not automatically load these definitions.

; This file is NOT automatically loaded by ScriptFu.
; A script can load this file at runtime like this:
;
; (define my-plug-in-run-func
;     (load (string-append
;              script-fu-sys-init-directory
;              DIR-SEPARATOR
;              "SIOD-compat.scm"))
;     ...
; )
;
; Note this puts the definitions in execution-scope, not global scope.
; They will affect all functions called.
; They can affect ScriptFu plugin scripts called.
; They will go out of scope when the run-func completes.

; *pi*, butlast, and cons-array symbols were deprecated in v2.
; They remain in v3, still deprecated.

; See also the companion file PDB-compat-v2.scm
; which defines aliases for obsolete PDB procedure names, from GIMP 2.


(define aset vector-set!)
(define aref vector-ref)
(define fopen open-input-file)
(define mapcar map)
(define nil '())
(define nreverse reverse)
(define pow expt)
(define prin1 write)

(define (print obj . port)
  (apply write obj port)
  (newline)
)

(define strcat string-append)
(define string-lessp string<?)
(define symbol-bound? defined?)
(define the-environment current-environment)



(define (fmod a b)
  (- a (* (truncate (/ a b)) b))
)

(define (fread arg1 file)

  (define (fread-get-chars count file)
    (let (
         (str "")
         (c 0)
         )

      (while (> count 0)
        (set! count (- count 1))
        (set! c (read-char file))
        (if (eof-object? c)
            (set! count 0)
            (set! str (string-append str (make-string 1 c)))
        )
      )

      (if (eof-object? c)
          ()
          str
      )
    )
  )

  (if (number? arg1)
      (begin
        (set! arg1 (inexact->exact (truncate arg1)))
        (fread-get-chars arg1 file)
      )
      (begin
        (set! arg1 (fread-get-chars (string-length arg1) file))
        (string-length arg1)
      )
  )
)

(define (last x)
  (cons (car (reverse x)) '())
)

(define (nth k list)
  (list-ref list k)
)

(define (prog1 form1 . form2)
  (let ((a form1))
    (if (not (null? form2))
      form2
    )
    a
  )
)

(define (rand . modulus)
  (if (null? modulus)
    (msrg-rand)
    (apply random modulus)
  )
)

(define (strcmp str1 str2)
  (if (string<? str1 str2)
      -1
      (if (string>? str1 str2)
          1
          0
      )
  )
)

(define (trunc n)
  (inexact->exact (truncate n))
)

(define verbose
  (lambda n
    (if (or (null? n) (not (number? (car n))))
      0
      (car n)
    )
  )
)
