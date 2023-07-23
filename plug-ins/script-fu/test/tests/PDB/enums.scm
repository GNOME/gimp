
; Test enums of the PDB

; These test and illustrate enums


; ImageBaseType
(assert '(= RGB     0))
(assert '(= GRAY    1))
(assert '(= INDEXED 2))

; ImageType is not same as ImageBaseType
(assert '(= RGB-IMAGE      0))
(assert '(= RGBA-IMAGE     1))
(assert '(= GRAY-IMAGE     2))
(assert '(= GRAYA-IMAGE    3))
(assert '(= INDEXED-IMAGE  4))
(assert '(= INDEXEDA-IMAGE 5))

