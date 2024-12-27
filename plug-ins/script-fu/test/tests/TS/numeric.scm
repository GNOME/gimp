;   test numerics of TS

; TinyScheme supports the numeric tower "fixnums and flonums"
; Meaning the tower is number=>real=>integer, omitting actual rational and complex.
; Also meaning the machine representations as integer or float

; Note "machine representation" is not "string representation"
; Former is a machine implementation.
; Latter is how the interpreter prints numbers.

; These tests only show what TinyScheme does.
; These tests might not reflect what a R5RS conforming implementation should do.
; See comments.

; TinyScheme is technically exactness-preserving,
; but silently returns the wrong answers when arithmetic operations overflow.
; Thus TinyScheme is non-conformant.

; !!! This must be also be tested with non-English locale
; e.g. export LANG="de_DE.UTF-8"
; In that locale, comma is used as a decimal point.
; In TinyScheme, string representation of numbers SHOULD use period as decimal point.
; But currently doesn't, in some locales using comma for decimal point.
; Because TinyScheme uses C standard lib snprintf
; which respects locale and can print comma for decimal point.


; float precision

; R5RS allows 1.0[s,e,d,l]0 notation
; meaning short-float, single-float, double-float and long-float.
; TinyScheme only supports e

(test! "e notation is supported")
(assert `(= 1.0e0 1.0))
(assert `(= 1.0e+0 1.0))
(assert `(= 1.0e-0 1.0))

(test! "e[s,d,l] notation is treated as a symbol, and is unbound")
(assert-error `1.0s0
              "eval: unbound variable")



(test! "max integers")

; largest 64-bit integer
;              1234567890123456789 digit counter
;    unsigned 18446744073709551615
;      signed  9223372036854775807
; TinyScheme machine representation is signed 64-bit.
;
; Any int literal greater than 19 digits overflows silently.
; Int literal of 19 digits < max signed 64-bit int do not overflow.

(test! "integers overflow silently.")
; Adding one to the largest representable int yields the wrong result
(assert `(not
            (=
               (+ 9223372036854775807 1)
               9223372036854775808)))
; And is in fact a negative int (overflowing into the sign bit)
(assert `(=
            (+ 9223372036854775807 1)
             -9223372036854775808))
; Numbers larger than the largest representable int
; can be represented as float literals, but not exact.
(assert `(inexact? 9223372036854775808.0))
; and they are not integers
(assert `(not (integer? 9223372036854775808.0)))
; Such numbers can be represented in equal e notation
(assert `(=
           9223372036854775808.0
           9.223372036854775808e+18))
; The string representation loses significant digits
; FIXME arbitrary limiting string repr to 10 decimal places is wrong.
(assert `(string=? (number->string 9223372036854775808.0)
                   "9.223372037e+18"))
; "9.2233720368547758e+18" when fixed, having more digits but still losing digits.



(test! "numeric type predicates")
; These do NOT tell you the machine representation.
; "The behavior of these type predicates on inexact numbers is unreliable, since any inaccuracy may affect the result."
(assert `(integer? 3.0))
(assert `(integer? (/ 8 4)))       ; i.e. 2.0
(assert `(not (integer? (/ 1 3)))) ; i.e. 0.3333

; integer implies rational implies real implies complex implies number
; integer is a subset of all higher types
(assert `(integer?  3))
(assert `(rational? 3))
(assert `(real?     3))
(assert `(complex?  3))
(assert `(number?   3))

(test! "complex type is not implemented")
(assert-error `(imag-part 3)
              "eval: unbound variable")



; exactness predicates

; The usual predicates are defined.
; R5RS says: "One of the predicates must be true.  Exactly one of the two can be true."
(assert `(exact? 1))
(assert `(not (inexact? 1)))

; exact-integer? exact-nonnegative-integer? exact-rational? are not defined
(assert-error `(exact-integer? 1)
              "eval: unbound variable")



; exactness

(test! "exact")

; small integers are exact
(assert `(exact? 1))

; some float notation (with all fractional digits zero) is exact
(assert `(exact? 4.0))
(assert `(real? 4.0))
(assert `(integer? 4.0))

; float notation with fractional digits nonzero is not exact
(assert  `(inexact? 1.1))




; exactness and IEEE representability
;
; Note: exactness is not the same concept as
; "representable in IEEE float without loss of precision"

; Some large numbers seemingly integral
; are not representable in IEEE float:
; some integers having greater than 15 digits.
; This is moot for TinyScheme since if they have no non-zero fractional digits,
; they are integer?

; integers greater than 19 digits are exact but have overflowed
(assert `(exact? 12345678901234567890))

; 3122.55 is not representable in IEEE float
; but is represented by the nearest representable value.
; Which is 3122.5500000000001818989403545856475830078125
; 17 significant digits: 3122.5500000000002, with >10 decimal places.
; TS uses %.10g printf format.
(assert `(inexact? 3122.55))
; The string representation is only 10 digits
; so rounded and truncated trailing zeros.
; FIXME this fails in non-English locale
; Currently prints as 3122,55.0
(assert `(string=? (number->string 3122.55)
                   "3122.55"))





; exactness and operations

; "inexactness is a contagious property of a number", with exceptions
; I.E. any operations with an inexact operand produces an inexact result,
; with exceptions.

; Division of any two exact numbers is exact
; This is only an illustration, not an exhaustive test
(assert `(exact?
            (/ 4.0 2.0)))

; exception:
; multiplication of any number by an exact zero may produce an exact zero result, even if the other argument is inexact.
(assert `(exact?
            (* 0 3122.55)))

; equivalence of exact and inexact numbers
; Scheme says: never equivalent.
; The literals look the same, and are exact, but converting one to inexact.
; FUTURE: this fails because exact->inexact fails to produce an inexact
; This is extremely obscure, so wait for upstream fix.
;; (assert `(not (eqv? 4.0 (exact->inexact 4.0))))


; inexact->exact

; Non-integral numbers cannot be converted to exact
(assert-error `(inexact->exact 3122.55)
              "inexact->exact: not integral:")

; A large integral number (only representable in float machine representation)
; can be converted to exact.
(assert `(exact? (inexact->exact 9223372036854775808.0)))
; But the result is not correct.
; Here the result is the least signed integer representable in signed 64-bit
(assert `(= (inexact->exact 9223372036854775808.0)
             -9223372036854775808))


; exact->inexact

; Is defined
(assert `exact->inexact)

; FUTURE fails, TinyScheme is non-compliant
;; (assert `(inexact? (exact->inexact 4.0)))


(test! "Round-trip (string->number (number->string x))")
; conversions to and from strings
;
; Meets a round-trip conversion predicate.
; A string representation of a number converts back to the same number.
(define (round-trip-repr? number)
  (eqv? number
        (string->number (number->string number))))

; Note, no backtick on calls to predicate
(assert (round-trip-repr? 1.0))
(assert (round-trip-repr? 1.01))
(assert (round-trip-repr? 3122.55))

; The string representation is fixed at 10 digits after decimal point.
; So numbers with more precision don't round-trip
(assert (not (round-trip-repr? 1.01234567890123)))
; FIXME The test is inverted.  This number should round-trip
; Currently prints as 1.012345679, losing 4 significant digits.

; Note that IEEE double can represent about 15 decimal places
; but TinyScheme somewhat arbitrarily limits the string representation
; to 10 decimal places



; Test various literals
(test! "string->number")
(assert `(= (string->number "100")
            100))
(assert `(= (string->number "1e2")
            100.0))
; the notation using # for unspecified digit is not supported
; TinyScheme returns #f, some Schemes return 1500.0
(assert `(not (string->number "15##")))

(test! "radix arg to string->number")
; hexadecimal
(assert `(= (string->number "100" 16)
            256))



; numeric literals

; In Scheme, the #e prefix makes a literal exact
; but TinyScheme returns a syntax error.
; FUTURE: the test framework can't test syntax errors
;;(assert-error `#e1
;;              "syntax: illegal sharp expression")








