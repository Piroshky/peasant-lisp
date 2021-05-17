;; this needs to be re-written when list splicing is added.
(defmacro when (test &rest body)
  (let ((act (list 'progn)))
    (for-each (form body)
	      (append form act))
    (list 'if test act)))

(defmacro cond (&rest clauses)
  (let ((body) (prev))
    (for-each
     (clause clauses)
     (let ((cur (list 'if (first clause) (last clause))))
       (if (= (length body) 0)
	     (set body cur)	     
	     (append cur prev))
       (set prev cur)))
    body))
