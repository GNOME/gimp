; listhome.scm
; Sample usage of TinyScheme Extension
; This simple program lists the directory entries on the
; user's home directory.

; It uses the following TinyScheme Extension functions:
;    getenv
;      Used to get HOME environment variable.
;    open-dir-stream
;      Used to open directory stream.
;    read-dir-entry
;      Used to read directory entries.
;    close-dir-entry
;      Used at the end, to close directory stream when done.

; check that extensions are enabled
(if (not (defined? 'load-extension))
    (begin
      (display "TinyScheme has extensions disabled. Enable them!!")
      (newline)
      (quit)))

; load TinyScheme extension
(load-extension "tsx-1.1/tsx")

; check that the necessary functions are available (the user
; might have removed some functionality...)
(if (or
      (not (defined? 'getenv))
      (not (defined? 'dir-open-stream))
      (not (defined? 'dir-read-entry))
      (not (defined? 'dir-close-stream)))
    (begin
      (display "Some necessary functions are not available. Exiting!")
      (newline)
      (quit)))

; get user's home dir from HOME environment var
(define homedir (getenv "HOME"))
(display "Listing contents of ") (display homedir) (newline)

; create directory stream to read dir entries
(define dirstream (dir-open-stream homedir))
(if (not dirstream)
  (begin
    (display "Unable to open home directory!! Check value of HOME environment var.")
    (quit)))

(let listentry ((entry (dir-read-entry dirstream)))
  (if (eof-object? entry)
    #t
    (begin
      (display entry)
      (newline)
      (listentry (dir-read-entry dirstream)))))

(dir-close-stream dirstream)

