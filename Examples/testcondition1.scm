;; testcondition1.scm

;; Use conditions to make change behaviour passed on history, and
;; simple condition tests based on  window class
(use-modules (ice-9 regex))  ;; for string-match 


;; This is only necessary if you want to pass the event on if the condition fails
(set-pass-on-fail #t)

;; Every other control-a shows Hello from Scheme!
;; (other ones are handled as normal)
(define toggle 1)

(xbindkey-condition-function
 '(control a)
 (lambda ()
   (let ((old-toggle toggle)
	 (special (> toggle 0)))
     (set! toggle (- 1 toggle))
     (if special
	 (display "Hello from Scheme!\n"))
     special)))

(xbindkey-condition-function
 '( F1 )
 (lambda ()
   (if (string-match "clock" (window-class))
       (begin
	 (run-command "xmessage this is a clock")
	 #t)
       #f)))

(xbindkey-condition-function
 '( F2 )
 (lambda ()
   (if (string-match "xterm" (window-class))
       (begin
	 (run-command "xmessage this is an xterm")
	 #t)
       #f)))
