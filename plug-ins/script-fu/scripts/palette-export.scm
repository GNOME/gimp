; -----------------------------------------------------------------------------
; GIMP palette export toolkit -
; Written by Barak Itkin <lightningismyname@gmail.com>
;
; This script includes various exporters for GIMP palettes, and other
; utility function to help in exporting to other (text-based) formats.
; See instruction on adding new exporters at the end
;
; -----------------------------------------------------------------------------
; Numbers and Math
; -----------------------------------------------------------------------------

; For all the opertations below, this is the order of respectable digits:
(define conversion-digits (list "0" "1" "2" "3" "4" "5" "6" "7" "8" "9"
                                "a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k"
				"l" "m" "n" "o" "p" "q" "r" "s" "t" "u" "v"
				"w" "x" "y" "z"))

; Converts a decimal number to another base. The returned number is a string
(define (convert-decimal-to-base num base)
  (define (highest-order num base)
    (if (and (<= 0 num) (< num base))
        0
        (+ 1 (highest-order (quotient num base) base))
        )
    )
  (define (calc base num str)
    (let ((max-order (highest-order num base)))
      (cond ((not (= 0 max-order))
             (let ((num-of-times (quotient
				  num (inexact->exact (expt base max-order)))))
               (calc base
                     (- num (* num-of-times (expt base max-order)))
                     (string-append str
				    (list-ref conversion-digits num-of-times)))
               )
             )
            (else (string-append str (list-ref conversion-digits num)))
            ))
    )
  (calc base num "")
  )

; Convert a string representation of a number in some base, to a decimal number
(define (convert-base-to-decimal base num-str)
  (define (convert-char num-char)
    (if (char-numeric? num-char)
        (string->number (string num-char))
        (+ 10 (- (char->integer num-char) (char->integer #\a)))
        )
    )
  (define (calc base num-str num)
    (if (equal? num-str "")
        num
        (calc base
              (substring num-str 1)
              (+ (* num base) (convert-char (string-ref num-str 0)))
              )
        )
    )
  (calc base num-str 0)
  )

; If a string num-str is shorter then size, pad it with pad-str in the
; begining untill it's at least size long
(define (pre-pad-number num-str size pad-str)
  (if (< (string-length num-str) size)
      (pre-pad-number (string-append pad-str num-str) size pad-str)
      num-str
      )
  )

; -----------------------------------------------------------------------------
; Color convertors
; -----------------------------------------------------------------------------

; The standard way for representing a color would be a list of red
; green and blue (GIMP's default)
(define color-get-red car)
(define color-get-green cadr)
(define color-get-blue caddr)

; Convert a color to a hexadecimal string
; '(255 255 255) => "#ffffff"

(define (color-rgb-to-hexa-decimal color)
  (string-append "#"
                 (pre-pad-number
		  (convert-decimal-to-base (color-get-red color) 16) 2 "0")
                 (pre-pad-number
		  (convert-decimal-to-base (color-get-green color) 16) 2 "0")
                 (pre-pad-number
		  (convert-decimal-to-base (color-get-blue color) 16) 2 "0")
                 )
  )

; Convert a color to a css color
; '(255 255 255) => "rgb(255, 255, 255)"
(define (color-rgb-to-css color)
  (string-append "rgb(" (number->string (color-get-red color))
                 ", " (number->string (color-get-green color))
                 ", " (number->string (color-get-blue color)) ")")
  )

; Convert a color to a simple pair of braces with comma seperated values
; '(255 255 255) => "(255, 255, 255)"
(define (color-rgb-to-comma-seperated-list color)
  (string-append "(" (number->string (color-get-red color))
                 ", " (number->string (color-get-green color))
                 ", " (number->string (color-get-blue color)) ")")
  )


; -----------------------------------------------------------------------------
; Export utils
; -----------------------------------------------------------------------------

; List of charcters that should not appear in file names
(define illegal-file-name-chars (list #\\ #\/ #\: #\* #\? #\" #\< #\> #\|))

; A function to filter a list lst by a given predicate pred
(define (filter pred lst)
  (if (null? lst)
      '()
      (if (pred (car lst))
          (cons (car lst) (filter pred (cdr lst)))
          (filter pred (cdr lst))
          )
      )
  )

; A function to check if a certain value obj is inside a list lst
(define (contained? obj lst) (member obj lst))

; This functions filters a string to have only characters which are
; either alpha-numeric or contained in more-legal (which is a variable
; holding a list of characters)
(define (clean str more-legal)
  (list->string (filter (lambda (ch) (or (char-alphabetic? ch)
                                         (char-numeric? ch)
                                         (contained? ch more-legal)))
                        (string->list str)))
  )

; A function that recieves the a file-name, and filters out all the
; character that shouldn't appear in file names. Then, it makes sure
; the remaining name isn't only white-spaces. If it's only
; white-spaces, the function returns false. Otherwise, it returns the
; fixed file-name
(define (valid-file-name name)
  (let* ((clean (list->string (filter (lambda (ch)
					(not (contained?
					      ch illegal-file-name-chars)))
                                      (string->list name))))
         (clean-without-spaces (list->string (filter (lambda (ch)
						       (not (char-whitespace?
							     ch)))
                                                     (string->list clean))))
         )
    (if (equal? clean-without-spaces "")
        #f
        clean
        )
    )
  )

; Filters a string from all the characters which are not alpha-numeric
; (this also removes whitespaces)
(define (name-alpha-numeric str)
  (clean str '())
  )

; This function does the same as name-alpha-numeric, with an added
; operation - it removes any numbers from the begining
(define (name-standard str)
  (let ((cleaned (clean str '())))
    (while (char-numeric? (string-ref cleaned 0))
           (set! cleaned (substring cleaned 1))
           )
    cleaned
    )
  )

(define name-no-conversion (lambda (obj) obj))
(define color-none (lambda (x) ""))
(define name-none (lambda (x) ""))

(define displayln (lambda (obj) (display obj) (display "\n")))

; The loop for exporting all the colors
(define (export-palette palette-name color-convertor name-convertor
                        start name-pre name-after name-color-seperator
                        color-pre color-after entry-seperator end)

  (define (write-color-line index)
    (display name-pre)
    (display (name-convertor
	      (car (gimp-palette-entry-get-name palette-name index))))
    (display name-after)
    (display name-color-seperator)
    (display color-pre)
    (display (color-convertor
	      (car (gimp-palette-entry-get-color palette-name index))))
    (display color-after)
    )

  (let ((color-count (car (gimp-palette-get-colors palette-name)))
        (i 0)
        )

    (display start)

    (while (< i (- color-count 1))
           (begin
             (write-color-line i)
             (display entry-seperator)
             (set! i (+ 1 i))
             )
           )

    (write-color-line i)
    (display end)
    )
  )

(define (register-palette-exporter
	 export-type export-name file-type description author copyright date)
  (script-fu-register (string-append "gimp-palette-export-" export-type)
                      export-name
                      description
                      author
                      copyright
                      date
                      ""
                      SF-DIRNAME _"Folder for the output file" ""
                      SF-STRING _"The name of the file to create (if a file with this name already exist, it will be replaced)"
                      (string-append "palette." file-type)
                      )
  (script-fu-menu-register (string-append "gimp-palette-export-" export-type)
                           "<Palettes>/Export as")
  )

(define (bad-file-name)
  (gimp-message (string-append _"The filename you entered is not a suitable name for a file."
			       "\n\n"
			       _"All characters in the name are either white-spaces or characters which can not appear in filenames.")))

; -----------------------------------------------------------------------------
; Exporters
; -----------------------------------------------------------------------------

(define (gimp-palette-export-css directory-name file-name)
  (let ((valid-name (valid-file-name file-name)))
    (if valid-name
        (with-output-to-file (string-append
			      directory-name DIR-SEPARATOR file-name)
          (lambda () (export-palette (car (gimp-context-get-palette))
                                     color-rgb-to-css
                                     name-alpha-numeric        ; name-convertor
                                     "/* Generated with GIMP Palette Export */\n" ; start
                                     "."                       ; name-pre
                                     ""                        ; name-after
                                     " { "                     ; name-color-seperator
                                     "color: "                 ; color-pre
                                     " }"                      ; color-after
                                     "\n"                      ; entry-seperator
                                     ""                        ; end
                                     )))
        (bad-file-name)
        )
    )
  )
(register-palette-exporter "css" "_CSS stylesheet..." "css"
                           (string-append _"Export the active palette as a CSS stylesheet with the color entry name as their class name, and the color itself as the color attribute")
                           "Barak Itkin <lightningismyname@gmail.com>"
			   "Barak Itkin" "May 15th, 2009")

(define (gimp-palette-export-php directory-name file-name)
  (let ((valid-name (valid-file-name file-name)))
    (if valid-name
        (with-output-to-file (string-append
			      directory-name DIR-SEPARATOR file-name)
          (lambda () (export-palette (car (gimp-context-get-palette))
                                     color-rgb-to-hexa-decimal
                                     name-standard             ; name-convertor
                                     "<?php\n/* Generated with GIMP Palette Export */\n$colors={\n" ; start
                                     "'"                       ; name-pre
                                     "'"                       ; name-after
                                     " => "                    ; name-color-seperator
                                     "'"                       ; color-pre
                                     "'"                       ; color-after
                                     ",\n"                     ; entry-seperator
                                     "}\n?>"                 ; end
                                     )))
        (bad-file-name)
        )
    )
  )
(register-palette-exporter "php" "P_HP dictionary..." "php"
                           _"Export the active palette as a PHP dictionary (name => color)"
                           "Barak Itkin <lightningismyname@gmail.com>"
			   "Barak Itkin" "May 15th, 2009")

(define (gimp-palette-export-python directory-name file-name)
  (let ((valid-name (valid-file-name file-name)))
    (if valid-name
        (with-output-to-file (string-append
			      directory-name DIR-SEPARATOR file-name)
          (lambda ()
            (let ((palette-name (car (gimp-context-get-palette))))
              (begin (displayln "# Generated with GIMP Palette Export")
                     (displayln (string-append
				 "# Based on the palette " palette-name))
                     (export-palette palette-name
                                     color-rgb-to-hexa-decimal
                                     name-standard             ; name-convertor
                                     "colors={\n"              ; start
                                     "'"                       ; name-pre
                                     "'"                       ; name-after
                                     ": "                      ; name-color-seperator
                                     "'"                       ; color-pre
                                     "'"                       ; color-after
                                     ",\n"                     ; entry-seperator
                                     "}"                       ; end
                                     ))))
          )
        (bad-file-name)
        )
    )
  )
(register-palette-exporter "python" "_Python dictionary" "py"
                           _"Export the active palette as a Python dictionary (name: color)"
                           "Barak Itkin <lightningismyname@gmail.com>"
			   "Barak Itkin" "May 15th, 2009")

(define (gimp-palette-export-text directory-name file-name)
  (let ((valid-name (valid-file-name file-name)))
    (if valid-name
        (with-output-to-file (string-append
			      directory-name DIR-SEPARATOR file-name)
          (lambda ()
            (export-palette (car (gimp-context-get-palette))
                            color-rgb-to-hexa-decimal
                            name-none                 ; name-convertor
                            ""                        ; start
                            ""                        ; name-pre
                            ""                        ; name-after
                            ""                        ; name-color-seperator
                            ""                        ; color-pre
                            ""                        ; color-after
                            "\n"                      ; entry-seperator
                            ""                        ; end
                            )
            )
          )
        (bad-file-name)
        )
    )
  )
(register-palette-exporter "text" "_Text file..." "txt"
                           _"Write all the colors in a palette to a text file, one hexadecimal value per line (no names)"
                           "Barak Itkin <lightningismyname@gmail.com>"
			   "Barak Itkin" "May 15th, 2009")

(define (gimp-palette-export-java directory-name file-name)
  (let ((valid-name (valid-file-name file-name)))
    (if valid-name
        (with-output-to-file (string-append directory-name
					    DIR-SEPARATOR file-name)
          (lambda ()
            (let ((palette-name (car (gimp-context-get-palette))))
              (begin (displayln "")
                     (displayln "import java.awt.Color;")
                     (displayln "import java.util.Hashtable;")
                     (displayln "")
                     (displayln "// Generated with GIMP palette Export ")
                     (displayln (string-append
				 "// Based on the palette " palette-name))
                     (displayln (string-append
				 "public class "
				 (name-standard palette-name) " {"))
                     (displayln "")
                     (displayln "    Hashtable<String, Color> colors;")
                     (displayln "")
                     (displayln (string-append
				 "    public "
				 (name-standard palette-name) "() {"))
                     (export-palette (car (gimp-context-get-palette))
                                     color-rgb-to-comma-seperated-list
                                     name-no-conversion
                                     "        colors = new Hashtable<String,Color>();\n" ; start
                                     "        colors.put(\""                             ; name-pre
                                     "\""                                                ; name-after
                                     ", "                                                ; name-color-seperator
                                     "new Color"                                         ; color-pre
                                     ");"                                                ; color-after
                                     "\n"                                                ; entry-seperator
                                     "\n    }"                                           ; end
                                     )
                     (display "\n}"))))
          )
        (bad-file-name)
        )
    )
  )

(register-palette-exporter "java" "J_ava map..." "java"
                           _"Export the active palette as a java.util.Hashtable<String, Color>"
                           "Barak Itkin <lightningismyname@gmail.com>"
			   "Barak Itkin" "May 15th, 2009")

