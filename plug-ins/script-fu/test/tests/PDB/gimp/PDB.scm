; test methods of PDB as a database


(script-fu-use-v3)


(test! "dump the PDB to a file")
(assert `(gimp-pdb-dump "/tmp/pdb.dump"))


(test! "store named array of bytes into db")
(assert `(gimp-pdb-set-data "myData" #(1 2 3)))

; effective: the named data can be retrieved
(assert `(equal? (gimp-pdb-get-data "myData")
                 #(1 2 3)))


(test! "procedure existence predicate")
(assert `(gimp-pdb-proc-exists "gimp-pdb-proc-exists"))


(test! "query the pdb by a set of regex strings")
; returns list of names

; empty regex matches anything, returns all procedure names in pdb
; Test exists more than 900 procedures
(assert `(> (length (gimp-pdb-query
                         "" ; name
                         "" "" ; blurb help
                         "" "" ; authors copyright
                         "" "" ; date type
                    ))
            900))

; a query on a specific name returns the same name
(assert `(string=? ( car (gimp-pdb-query
                            "gimp-pdb-proc-exists" ; name
                            "" "" ; blurb help
                            "" "" ; authors copyright
                            "" ""))
                    "gimp-pdb-proc-exists"))


(test! "PDB generate a unique name.")
; name guaranteed to be unique for life of GIMP session.
; I.E. the PDB has a generator.
; Only testing type is string, not that it is unique.
(assert `(string? (gimp-pdb-temp-name)))




(script-fu-use-v2)