;; Be it known that Josh wrote most of this file.

(defmacro register-procedure(name blurb descr author copy date type inputs outputs)
  `(setq list-of-pdb-entries (cons (list ,name ,blurb ,descr ,author ,copy ,date ,type ',inputs ',outputs)
				   list-of-pdb-entries)))

(defvar list-of-pdb-entries nil)
(setq list-of-pdb-entries nil)

(defun make-new-file (file-name)
  (let ((buffer (get-buffer-create (generate-new-buffer-name file-name))))
    (set-buffer buffer)))

(defun finish-file()
  ;; do nothing 
  )

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
  (make-docs "pdb_dump" "pdb_dump.texi"))
