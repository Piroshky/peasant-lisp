(defmacro when (test &rest forms)
  (let ((body `(progn ,@forms)))
    `(if ,test ,body)))

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
