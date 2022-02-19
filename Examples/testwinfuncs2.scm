;; testwinfuncs2.scm

;; Test of window-xxx functions
;; Like testwinfunc1.scm, but output is in an xmessage and the window functions
;; are called without the optional argument.

(use-modules (ice-9 format)) ;; for format

(define (show msg)
  (run-command (string-append "xmessage " msg)))

(xbindkey-function
 '(F1)
 (lambda ()
   (show (format #f "window id is ~d" (current-windowid)))
   ))

(xbindkey-function
 '(F2)
 (lambda ()
   (show (string-append "window name is " (window-name)))
   ))


(xbindkey-function
 '(F3)
 (lambda ()
   (show (string-append "window class is " (window-class)))
   ))

;; For many windows there is no window command.
(xbindkey-function
 '(F4)
 (lambda ()
   (define cmd (window-command))
   (if (string= cmd "")
       (show "there is no window command")
       (show (string-append "window command is " (window-command))))
   ))



