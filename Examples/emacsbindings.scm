;; Example of some Emacs bindings.
;; Used when the event window is neither emacs nor xterm.
;; Add more elements to variable bindings as desired
;; The second element of each binding is the replacement key
;; name passed to xdotool.

;; The following is necessary to pass on the key when the condition fails:
(set-pass-on-fail #t)

;; Delay required for xdotool,  you may need to adjust this to 200
;; how many milliseconds to sleep before the xdotool command
;; (If we are not running the command, do not delay at all.)
(define ms-sleep 150)

(define bindings '(
		   ( (control a) "Home" )
		   ( (control e) "End" )
		   ( (control b) "Left" )
		   ( (control f) "Right" )
		   ( (control p) "Up" )
		   ( (control n) "Down" )
		   ( (control d) "Delete" )
		   ))

(use-modules (ice-9 format)) ;; for format

;; Don't rebind for some classes, such as
;; emacs, Xterm, xfce-terminal, xedit
(define re-exclude (make-regexp "Emacs|XTerm|terminal|Xedit"))
(define (exclude? str)
  (regexp-exec re-exclude str))


;; like run-command, but sleep and always return true.
(define (delay-run-command-t cmd)
  (usleep (* 1000 ms-sleep))
  (run-command cmd)
  #t)

      
(define (emacs-binding key)
  (let ((for-us (not (exclude? (window-class)))))
    (if for-us
	(delay-run-command-t (format #f "xdotool key --clearmodifiers ~s" key)))
    for-us))

(define (generate p)
  (while (not (null? p))
  	 (let ((x (car p)))
	   (display (format #f  "bind ~s to ~s" (car x)(cadr x)))
	   (newline)
	   (xbindkey-condition-function
	    (car x)
	    (lambda ()
	      (emacs-binding (cadr x)))))
	 (set! p (cdr p))
	 ))

;; Call xbindkey-condition-function on all elements of bindings:
(generate bindings)

