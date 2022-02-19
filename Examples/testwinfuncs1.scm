;; testwinfuncs1.scm

;; Test of window-xxx functions, with optional argument.
;; The output of F1 should be the same as the output of F2,  similary F3 with F4.

(xbindkey-function
 '(F1)
 (lambda ()
   (display (string-append "window name is "
			   (window-name (current-windowid))))
   (newline)
   ))

(xbindkey-function
 '(F2)
 (lambda ()
   (display (string-append "window name is "
			   (window-name)))
   (newline)
   ))


(xbindkey-function
 '(F3)
 (lambda ()
   (display (string-append "window class is "
			   (window-class (current-windowid))))
   (newline)
   ))

(xbindkey-function
 '(F4)
 (lambda ()
   (display (string-append "window class is "
			   (window-class)))
   (newline)
   ))


(xbindkey-function
 '(F5)
 (lambda ()
   (display (string-append "window command is "
			   (window-command (current-windowid))))
   (newline)
   ))

(xbindkey-function
 '(F6)
 (lambda ()
   (display (string-append "window command is "
			   (window-command)))
   (newline)

   ))

