;; return the substring of STRING matched in MATCH-VECTOR, 
;; the Nth subexpression match (default 0). 
(define (re-match-nth string match-vector . n) 
  (let ((n (if (pair? n) (car n) 0))) 
    (substring string (car (vector-ref match-vector n)) 
                    (cdr (vector-ref match-vector n))))) 

(define (re-before-nth string match-vector . n) 
  (let ((n (if (pair? n) (car n) 0))) 
    (substring string 0 (car (vector-ref match-vector n))))) 

(define (re-after-nth string match-vector . n) 
  (let ((n (if (pair? n) (car n) 0))) 
    (substring string (cdr (vector-ref match-vector n)) 
             (string-length string)))) 