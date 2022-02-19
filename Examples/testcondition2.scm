;; testcondition2.scm

;; Some condition functions;  pointless, but just to give simple examples.

(use-modules (ice-9 format))	;; for format
(use-modules (ice-9 regex))	;; for string-match

;; Counter is incremented when F1 pressed and shown and cleared with F2.
(define counter 0)

;; F1	Show the window position if window is an xterm
;;	and increment counter;
;;	dislay a message if it isn't

(xbindkey-condition-function
 '( F1 )
 (lambda ()
   (if (string-match "xterm" (window-class))
       (begin
	 (run-command (format #f "xwininfo -id ~d | grep '[XY]'" (current-windowid)))
	 (set! counter (1+ counter)))
       (display "This is not an xterm"))
   (newline)
   #t) ;; retun t to absorb the event regardless of pass-on-fail status
 )

;; F2	Show the number of xterms counted above

(xbindkey-function
 '(F2)
 (lambda ()
   (display (format #f "# of xterms shown: ~d\n" counter))
   (set! counter 0)))


;; F3	Invoke xclock unless current window is a clock.

(xbindkey-condition-function
 '( F3 )
 (lambda ()
   (or (string-match "clock" (window-class))
       (run-command "xclock"))
   #t) ;; retun t to absorb the event regardless of pass-on-fail status
 )


;; F4	show a message indicating if this is an xterm or not
(xbindkey-condition-function
 '( F4 )
 (lambda ()
   (if (string-match "xterm" (window-class))
       (run-command "xmessage this is an xterm")
       (run-command "xmessage this is not an xterm"))
   #t))

;; F5	Show a message only if this is a clock window.
(xbindkey-condition-function
 '( F5 )
 (lambda ()
   (if (string-match "clock" (window-class))
       (run-command "xmessage this is a clock"))
   #t))



      
