; test methods of PDB as a database



(assert `(gimp-pdb-dump "/tmp/pdb.dump"))



; store named array of bytes into db
(assert `(gimp-pdb-set-data "myData" #(1 2 3)))

; effective: the named data can be retrieved
(assert `(equal? (car (gimp-pdb-get-data "myData"))
              #(1 2 3)))



; test existence of procedure
(assert `(gimp-pdb-proc-exists "gimp-pdb-proc-exists"))



; query the pdb by a set of regex strings
; returns list of names

; empty regex matches anything, returns all procedure names in pdb
; Test exists more than 1000 procedures
(assert `(> (length (car (gimp-pdb-query "" ; name
                         "" "" ; blurb help
                         "" "" ; authors copyright
                         "" "" ; date type
                    )))
            1000))

; a query on a specific name returns the same name
(assert `(string=? ( caar (gimp-pdb-query "gimp-pdb-proc-exists" ; name
                         "" "" ; blurb help
                         "" "" ; authors copyright
                         "" ""))
                    "gimp-pdb-proc-exists"))



; generate a unique name.
; name guaranteed to be unique for life of GIMP session.
; I.E. the PDB has a generator.
(assert `(string? (car (gimp-pdb-temp-name))))