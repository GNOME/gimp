; Tests of sharp char expressions in ScriptFu

; This only tests:
;  "sharp character"     #\<c>
;  "sharp character hex" #\x<hex digits>
;   sharp expressions for whitespace
; See also:
;   sharp-expr.scm
;   sharp-expr-number.scm

; This also only tests a subset: the ASCII subset.
; See also: sharp-expr-unichar.scm

; #\<char> denotes a character constant where <char> is one character
; The one character may be multiple bytes in UTF-8,
; but should appear in the display as a single glyph,
; but may appear as a box glyph for unichar chars outside ASCII.

; #\x<x> denotes a character constant where <x> is a sequence of hex digits
; See mk_sharp_const()

; #\space #\newline #\return and #\tab also denote character constants.

; sharp backslash space "#\ " parses as a token and yields a char atom.
; See the code, there is a space here: " tfodxb\\"
; See the test below.

; #U+<x> notation for unichar character constants is not in ScriptFu

; Any sharp character followed by characters not described above
; MAY optionally be a sharp expression when a program
; uses the "sharp hook" by defining symbol *sharp-hook* .




;          sharp constants for whitespace

; codepoints tab 9, newline 10, return 13, space 32 (aka whitespace)
; TinyScheme and ScriptFu prints these solitary unichars by a string representation,
; but only when they are not in a string!
; This subset of codepoints are ignored by the parser as whitespace.
; It is common for older scripts to use sharp expression constants for these codepoints.
(assert '(equal? (integer->char 9) #\tab))
(assert '(equal? (integer->char 10) #\newline))
(assert '(equal? (integer->char 13) #\return))
(assert '(equal? (integer->char 32) #\space))

; Misspelled sharp constants
; Any sequence of chars starting with #\n, up to a delimiter,
; that does not match "newline"
; is parsed as the sharp constant for the lower case ASCII n char.\
; Similarly for tab, return, space
(test! "misspelled sharp char constant for newline")
; 110 is the codepoint for lower case n
(assert '(equal? (integer->char 110) #\n))
(assert '(equal? (integer->char 110) #\newlin))
(assert '(equal? (integer->char 110) #\newlines))





;          sharp constant character

; Unicode codepoints in range [33, 126]
; e.g. the letter A, ASCII 65
(assert '(equal? (integer->char 65) #\A))
(assert '(char? #\A))
(assert '(atom? #\A))

; Tests of functions using a non-printing, control character ASCII
; Codepoint BEL \x7
(assert '(equal? (integer->char 7) #\))
(assert '(char? #\))
(assert '(atom? #\))
; string function takes sequence of chars
(assert (equal? (string #\) ""))

; Unicode codepoints [0-8][11-12][14-31]
; (less than 32 excepting tab 9, newline 10, return 13)
; The "non-printing" characters
; e.g. 7, the character that in ancient times rang a bell sound

; Upstream TinyScheme prints these differently from ScriptFu, as a string repr of the char.
; since TinyScheme default compiles with option "USE_ASCII_NAMES"
;>(integer->char 7)
;#\bel
;>(integer->char 127)
;#\del

; ScriptFu prints solitary Unichars
; for codepoints below 32 and also 127 differently than upstream TinyScheme.
; Except ScriptFu is same as TinyScheme for tab, space, newline, return codepoints.
; ScriptFu shows a glyph that is a box with a hex number.
; Formerly (before the fixes for this test plan) Scriptfu printed these like TinyScheme,
; by a sharp constant hex e.g. #\x1f for 31


(test! "Edge codepoint tests")
; Tests of edge cases, near a code slightly different

; Codepoint US Unit Separator, edge case to 32, space
(assert '(equal? (integer->char 31) #\))
(assert '(equal? #\ #\x1f))

; codepoint 127 x7f (DEL), edge case to 128
(assert '(equal? (integer->char 127) #\x7f))




;          sharp constant hex character

; Sharp char expr hex denotes char atom
; But not the REPL printed representation of characters.

; is-a char
(assert '(char? #\x65))
; equals a sharp character: lower case e
(assert '(equal? #\x65 #\e))

; sharp char hex notation accepts a single hex digit
(assert '(char? #\x3))
; sharp char hex notation accepts two hex digits
(assert '(char? #\x33))

; edge case, max hex that fits in 8-bits
(assert '(char? #\xff))

; sharp car expr hex accepts three digits
; when they are leading zeroes
(assert '(char? #\x033))

; Otherwise, three digits not leading zeros
; are unicode.


; codepoint x3bb is a valid character (greek lambda)
; but is outside ASCII range.
; See sharp-expr-unichar.scm






;                sharp constant hex character: invalid unichar

; Unicode has a range, but sparsely populated with valid codes.
; Unicode is unsigned, range is [0,x10FFF]
; Greatest valid codepoint is x10FFFF (to match UTF-16)
; Sparsely populated: some codepoints in range are not valid
; because they are incorrectly encoded using UTF-8 algorithm.
; (This is a paraphrase: please consult the standard.)

; These tests are not a complete test of UTF-8 compliance !!!

; Edge case: max valid codepoint
(assert (equal? #\x10FFFF #\􏿿))

; Edge case: zero is considered a valid codepoint
; !!! Although also a string terminator.
(assert '(equal?
            (integer->char 0)
            #\x0))


(test! "sharp constants for delimiter characters")

; These test the sharp constant notation for characters space and parens
; These are in the ASCII range


; !!! A space char in a sharp constant expr
(assert (char? #\ ))
; Whose representation is a space character.
(assert (string=? (atom->string #\ )
                  " "))

; !!! A right paren char in a sharp constant expr
; Note that backslash captures the first right paren:
; the parens do not appear to match.
(assert (char? #\)))
; Ditto for left paren
(assert (char? #\())
; !!! But easy for author to confuse the parser
; assert-error can't catch syntax errors.
; So can only test in the REPL.
; > (char? #\)
; Error: syntax error: expected right paren, found EOF"

; #\# is the sharp or pound sign char
(assert (char? #\#))
(assert (string=? (atom->string #\# )
                  "#"))
; #\x is lower case x
(assert (char? #\x))
(assert (string=? (atom->string #\x )
                  "x"))



; see also integer2char.scm



;                       Common misunderstandings or typos

; #\t is a character, lower case t

; It is not the denotation for truth.
(assert `(not (equal? #\t #t)))

; It is not the denotation for #\tab.
(assert `(not (equal? #\t #\tab)))

; It is a char
(assert `(char? #\t))

; Its string representation is lower case t character
(assert `(string=? (atom->string #\t)
                    "t"))



; a number converted to string that is representation in base 16
; !!! This is not creating a Unichar.
; It is printing the hex representation of decimal 955, without a leading "\x"
(assert `(string=? (number->string 955 16)
                  "3bb"))


;               Untestable sharp constant hex character

; Test framework can't test, these cases are syntax errors.
; These cases yield "syntax: illegal sharp constant expression" in REPL

; sharp constant hex having non-hex digit is an error
; z is not in [a-f0-9]
; > #\xz
; Error: syntax: illegal sharp constant expression
; Also prints warning "Hex literal has invalid digits" in stderr



