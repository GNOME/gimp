; Deprecated and should not be used in new scripts.
; GIMP developers strongly recommend you not use this file
; in scripts that you will distribute to other users.

; This file defines aliases.
; To help make old ScriptFu scripts more compatible with GIMP PDB version 3.
; The aliases redirect old PDB procedure names to a new name.

; This script might omit less commonly used PDB procedures.
; This might omit PDB procedures where the signature changed.

; This file is NOT automatically loaded by ScriptFu.
; A script can load this file at runtime like this:
;
; (define my-plug-in-run-func
;     (load (string-append
;              script-fu-sys-init-directory
;              DIR-SEPARATOR
;              "PDB-compat-v2.scm"))
;     ...
; )
;
; Note this puts the definitions in execution-scope, not global scope.
; They will affect all functions called.
; They can affect ScriptFu plugin scripts called.
; They will go out of scope when the run-func completes.

; See also the companion file SIOD-compat.scm
; which defines Scheme functions for compatibility with the SIOD dialect.



(define gimp-brightness-contrast               gimp-drawable-brightness-contrast       )
(define gimp-brushes-get-brush                 gimp-context-get-brush                  )

(define gimp-drawable-is-channel               gimp-item-id-is-channel                 )
(define gimp-drawable-is-layer                 gimp-item-id-is-layer                   )
(define gimp-drawable-is-layer-mask            gimp-item-id-is-layer-mask              )
(define gimp-drawable-is-text-layer            gimp-item-id-is-text-layer              )
(define gimp-drawable-is-valid                 gimp-item-id-is-valid                   )
(define gimp-drawable-transform-2d             gimp-item-transform-2d                  )
(define gimp-drawable-transform-flip           gimp-item-transform-flip                )
(define gimp-drawable-transform-flip-simple    gimp-item-transform-flip-simple         )
(define gimp-drawable-transform-matrix         gimp-item-transform-matrix              )
(define gimp-drawable-transform-perspective    gimp-item-transform-perspective         )
(define gimp-drawable-transform-rotate         gimp-item-transform-rotate              )
(define gimp-drawable-transform-rotate-simple  gimp-item-transform-rotate-simple       )
(define gimp-drawable-transform-scale          gimp-item-transform-scale               )
(define gimp-drawable-transform-shear          gimp-item-transform-shear               )

(define gimp-display-is-valid                  gimp-display-id-is-valid                )

(define gimp-image-is-valid                    gimp-image-id-is-valid                  )
(define gimp-image-freeze-vectors              gimp-image-freeze-paths                 )
(define gimp-image-get-vectors                 gimp-image-get-paths                    )
(define gimp-image-get-selected-vectors        gimp-image-get-selected-paths           )
(define gimp-image-set-selected-vectors        gimp-image-set-selected-paths           )
(define gimp-image-thaw-vectors                gimp-image-thaw-paths                   )

(define gimp-item-is-channel                   gimp-item-id-is-channel                 )
(define gimp-item-is-drawable                  gimp-item-id-is-drawable                )
(define gimp-item-is-layer                     gimp-item-id-is-layer                   )
(define gimp-item-is-layer-mask                gimp-item-id-is-layer-mask              )
(define gimp-item-is-selection                 gimp-item-id-is-selection               )
(define gimp-item-is-text-layer                gimp-item-id-is-text-layer              )
(define gimp-item-is-valid                     gimp-item-id-is-valid                   )
(define gimp-item-is-vectors                   gimp-item-id-is-path                    )
(define gimp-item-id-is-vectors                gimp-item-id-is-path                    )

(define gimp-vectors-new                       gimp-path-new                           )
; ? missing others where "vectors" renamed to "path"

(define gimp-layer-group-new                   gimp-group-layer-new                    )
; ? missing others where "layer-group" renamed to "group-layer"

(define gimp-procedural-db-dump                gimp-pdb-dump                           )
(define gimp-procedural-db-get-data            gimp-pdb-get-data                       )
(define gimp-procedural-db-set-data            gimp-pdb-set-data                       )
; Obsolete: gimp-procedural-db-get-data-size
; Just call gimp-pdb-get-data and in Scheme find its size
(define gimp-procedural-db-proc-arg            gimp-pdb-get-proc-argument              )
(define gimp-procedural-db-proc-info           gimp-pdb-get-proc-info                  )
(define gimp-procedural-db-proc-val            gimp-pdb-get-proc-return-value          )
(define gimp-procedural-db-proc-exists         gimp-pdb-proc-exists                    )
(define gimp-procedural-db-query               gimp-pdb-query                          )
(define gimp-procedural-db-temp-name           gimp-pdb-temp-name                      )

(define gimp-image-get-exported-uri            gimp-image-get-exported-file            )
(define gimp-image-get-imported-uri            gimp-image-get-imported-file            )
(define gimp-image-get-xcf-uri                 gimp-image-get-xcf-file                 )
(define gimp-image-get-filename                gimp-image-get-file                     )
(define gimp-image-set-filename                gimp-image-set-file                     )

(define gimp-plugin-menu-register              gimp-pdb-add-proc-menu-path             )
(define gimp-plugin-get-pdb-error-handler      gimp-plug-in-get-pdb-error-handler      )
(define gimp-plugin-help-register              gimp-plug-in-help-register              )
(define gimp-plugin-menu-branch-register       gimp-plug-in-menu-branch-register       )
(define gimp-plugin-set-pdb-error-handler      gimp-plug-in-set-pdb-error-handler      )

(define gimp-plugins-query                     gimp-plug-ins-query                     )
(define file-gtm-save                          file-html-table-export                  )
(define python-fu-histogram-export             histogram-export                        )
(define python-fu-gradient-save-as-css         gradient-save-as-css                    )