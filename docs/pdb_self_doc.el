;; Be it known that Josh wrote most of this file.

(defmacro register-procedure(name blurb descr author copy date type inputs outputs)
  `(setq list-of-pdb-entries (cons (list ,name ,blurb ,descr ,author ,copy ,date ,type ',inputs ',outputs)
				   list-of-pdb-entries)))

(defvar list-of-pdb-entries nil)
(setq list-of-pdb-entries nil)

(defun make-new-file(file-name)
  (let ((buffer (get-buffer-create (generate-new-buffer-name file-name))))
    (set-buffer buffer)
    (insert
     "\\input texinfo   @c -*-texinfo-*-
@setfilename pdb.info
@settitle GIMP Procedural Database Documentation
@setchapternewpage on

@ifinfo
This file describes the GIMP procedural database.

Copyright (C) 1995, 1996, 1997 by Spencer Kimball and Peter Mattis.  All rights reserved.

We distribute @sc{gimp} under the terms of the GNU General Public
License, Version 2, which we have included with this release
in the file named @file{COPYING}, and in the ``Copying'' section of
this manual.
As indicated in the License,
we provide the program
``as is'' without warranty
of any kind, either expressed or implied, without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
@end ifinfo

@c @iftex
@c @hyphenation{Project-Header Project-Author Project-Date Project-Version}
@c @end iftex

@titlepage
@center @titlefont{GIMP: Procedural Database Documentation}
@sp 2
@center Spencer Kimball and Peter Mattis

@center eXperimental Computing Facility
@center The University of California at Berkeley
@page
@vskip 0pt plus 1filll
Copyright @copyright{} 1995, 1996, 1997 Spencer Kimball and Peter Mattis.

We distribute @sc{gimp} under the terms of the GNU General Public
License, Version 2, which we have included with this release
in the file named @file{COPYING}, and in the appendix to this manual.  As indicated in the License,
we provide the program
``as is'' without warranty
of any kind, either expressed or implied, without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
@end titlepage

@dircategory Elite Image Manipulation
@direntry
* GIMP!: (pdb).		The GIMP procedural database.
@end direntry

@node    Top, , (dir), (dir)

@ifinfo
This document is the procedural database documentation for @sc{The
GIMP}, the GNU Image Manipulation Program. The documentation is
automatically generated from help strings embedded in the code. It is
intended to provide information about the functionality, arguments and
return values for procedures in the procedural database.
@end ifinfo

@menu
* Commands::     All commands.
@end menu

@node    Commands, , Top, Top
@c 1
@chapter Commands

")
    buffer))

(defun finish-file()
  (insert "@bye\n"))

(defun pdb-sort(a b)
  (string< (car a) (car b)))

(defun make-docs(file output)
  (interactive "fInput: \nFOutput: ")
  (let ((source-buffer (find-file-noselect file))
	(doc-buffer (make-new-file output)))
    (if (not source-buffer)
	(error "Can't open file %s" file)
      (setq list-of-pdb-entries nil)
      (eval-buffer source-buffer)
      (setq list-of-pdb-entries (sort list-of-pdb-entries 'pdb-sort))
      (set-buffer doc-buffer)
      (mapcar 'output-each-pdb list-of-pdb-entries)
      (finish-file)
      (write-file output)
      (kill-buffer doc-buffer)
      (kill-buffer source-buffer))))

(defun output-each-pdb (entry)
  (let* ((name   (car entry))
	 (blurb  (car (cdr entry)))
	 (descr  (car (cdr (cdr entry))))
	 (author (car (cdr (cdr (cdr entry)))))
	 (copy   (car (cdr (cdr (cdr (cdr entry))))))
	 (date   (car (cdr (cdr (cdr (cdr (cdr entry)))))))
	 (type   (car (cdr (cdr (cdr (cdr (cdr (cdr entry))))))))
	 (inputs (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr entry)))))))))
	 (inputs2 inputs)
	 (outputs (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr entry)))))))))))

    (insert "@defun " name " ")
    (while inputs
      (insert (format "%s" (car (car inputs))))
      (if (cdr inputs)
	  (insert ", "))
      (setq inputs (cdr inputs)))
    (insert "\n")
    (insert descr)
    (insert "--@strong{")
    (insert type)
    (insert "}")
    (if inputs2
	(progn
	  (insert "\n\n@strong{Inputs}\n")
	  (insert "@itemize @bullet\n")
	  (while inputs2
	    (let ((js (car inputs2)))
	      (insert "@item ")
	      (insert (format "@emph{%s} " (car js)))
	      (insert (format "(%s)--" (car (cdr js))))
	      (insert (concat (upcase (substring (car (cdr (cdr js))) 0 1)) (substring (car (cdr (cdr js))) 1 nil)))
	      (insert "\n")
	      (setq inputs2 (cdr inputs2))))
	  (insert "@end itemize\n")))
    (if outputs
	(progn
	  (insert "\n\n@strong{Outputs}\n")
	  (insert "@itemize @bullet\n")
	  (while outputs
	    (let ((js (car outputs)))
	      (insert "@item ")
	      (insert (format "@emph{%s} " (car js)))
	      (insert (format "(%s)--" (car (cdr js))))
	      (insert (concat (upcase (substring (car (cdr (cdr js))) 0 1)) (substring (car (cdr (cdr js))) 1 nil)))
	      (insert "\n")
	      (setq outputs (cdr outputs))))
	  (insert "@end itemize\n")))
    (insert "@end defun\n")
    (insert "@emph{")
    (insert author)
    (insert "}\n\n")
    ))

(defun make-docs-noargs ()
  (make-docs "pdb_dump" "pdb.texi"))
