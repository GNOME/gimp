; Tests of gimp module of the PDB
; These are not associated with an object class

; Generally speaking, prefix is "gimp-get"


; The serialized color configuration is a string
(assert '(string? (car (gimp-get-color-configuration))))

; The default comment on images is a string
(assert '(string? (car (gimp-get-default-comment))))

; The default unit is an integer enum for mm
(assert '(= (car (gimp-get-default-unit))
            UNIT-MM))

; The list of open images is a vector of integer ID
; At test time, we don't know how many
(assert '(number? (car (gimp-get-images))))
(assert '(vector? (cadr (gimp-get-images))))

; The list of modules to not be loaded is a list of strings
; Not double wrapped in a list, usually ("")
(assert '(list? (gimp-get-module-load-inhibit)))

; the monitor resolution is two doubles e.g. (57.0 57.0)
; TODO in what units, pixels/in?
(assert '(real? (car (gimp-get-monitor-resolution))))

; the PID is an integer
; FIXME should be renamed get-pid?
(assert '(number? (car (gimp-getpid))))




; global parasites
; They persist from session to session

; the list of global parasites is a list
(assert '(list? (gimp-get-parasite-list)))

; error to get by name not of a global parasite
(assert-error '(gimp-get-parasite "never in the list")
              "Procedure execution of")

; Can create a named parasite on global namespace.
; See elsewhere, a Parasite is a list.
(assert '(gimp-attach-parasite (list "foo" 1 "zed")))
; effective
(assert `(equal? (car (gimp-get-parasite "foo"))
                 '("foo" 1 "zed")))

; Can reattach, regardless whether same name already attached.
(assert '(gimp-attach-parasite (list "foo" 2 "bar")))
; effective
(assert `(equal? (car (gimp-get-parasite "foo"))
                 '("foo" 2 "bar")))

; detaching by name destroys a parasite
(assert `(gimp-detach-parasite "foo"))
; effective, name no longer valid
(assert-error '(gimp-get-parasite "foo")
              "Procedure execution of")


