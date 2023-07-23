
; Test cases for cond-expand in ScriptFu interpreter of GIMP app.

; cond-expand is SRFI-0
; https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/cond_002dexpand-_0028SRFI-0_0029.html

; ScriptFu cond-expand is defined in the tail of script-fu.init

; This tests existing ScriptFu code, which is not a full implementation of cond-expand.
; ScriptFu omits "else" clause.
; GIMP issue #9729 proposes an enhancement that adds else clause to cond-expand, etc.


; *features* is a defined symbol that names features of language
(assert '(equal?
           *features*
           '(srfi-0 tinyscheme)))

; srfi-0 is not a defined symbol
(assert-error '(srfi-0)
              "eval: unbound variable:")
; Note that *error-hook* erroneously omits tail of error message



; simple condition on one supported feature
(assert '(equal?
            (cond-expand (tinyscheme "implements tinyscheme"))
             "implements tinyscheme"))

; simple clause on one unsupported feature
; Since the condition fails there is no expansion.
; Since there is no 'else clause', there is no expansion for false condition.
; The cond-expand doc says:
; "It either expands into the body of one of its clauses or signals an error during syntactic processing."
; Yielding #t is not "signals an error" so is not correct.
; This documents what ScriptFu does, until we decide whether and how to fix it.
(assert '(equal?
            (cond-expand (srfi-38 "implements srfi-38"))
             #t))

; multiple clauses
(assert '(equal?
            (cond-expand
              (srfi-38 "implements srfi-38")
              ((not srfi-38) "not implements srfi-38"))
            "not implements srfi-38"))


; clauses start with 'and', 'or', or 'not'

; 'not clause'
(assert '(equal?
            (cond-expand ((not srfi-38) "not implements srfi-38"))
            "not implements srfi-38"))

; 'and clause' having two logical conditions that are true
(assert '(equal?
            (cond-expand ((and tinyscheme srfi-0) "implements both tinyscheme and srfi-0"))
            "implements both tinyscheme and srfi-0"))

; 'or clause' having two logical conditions, one of which is false
(assert '(equal?
            (cond-expand ((or tinyscheme srfi-38) "implements tinyscheme or srfi-38"))
            "implements tinyscheme or srfi-38"))


; nested logical clauses
(assert '(equal?
            (cond-expand ((or srfi-38 (and tinyscheme srfi-0)) "implements srfi-38 or tinyscheme and srfi-0"))
            "implements srfi-38 or tinyscheme and srfi-0"))







