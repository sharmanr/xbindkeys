;;	remap4.scm

;; This tests not only that when the condition is true the key is remapped,
;; but if the condition is false the original key is sent.

;; For this to work, set-pass-on-fail must be true:
(set-pass-on-fail #t)

(use-modules (ice-9 regex))  ;; for string-match


;; A delay is typically required when using xdotool from xbindkeys.
;; You can comment it and use the -y option instead
(set-delay 150)

(define (class-is-xterm)
  (let ((class (window-class)))
    (string-match "[Xx][Tt]erm" class)))

;; Like run-command but return #t
(define (run-command-t command)
  (run-command command)
  #t)

;; Test that a always sends b
(xbindkey-function
 '( a )
 (lambda ()
   (run-command "xdotool key b")
   ))

;; Test that c sends a d if in xterm (c otherwise)
(xbindkey-condition-function
 '( c )
 (lambda ()
   (and (class-is-xterm)
	(run-command-t "xdotool key d"))
   ;; if class is an xterm we run the command and return t
   ;; indicating the key has been changed (the event should be discarded);
   ;; otherwise we return false indicating that the event should be
   ;; handled normally (as if xbindkeys wasn't involved)
   ))


;; Test that e sends an f if not in xterm (an e in xterm)
(xbindkey-condition-function
 '( e )
 (lambda ()
   (if (class-is-xterm)
       #f
       (run-command-t "xdotool key f"))
   ))
   

;; Similar to above, but with functions keys.
;; So to test, they should be run in a window where F1 etc has some effect
;; (even if it just shows strange characters from an escape sequence).

;; Test that F1 key sends a g (unconditionally)
(xbindkey-function
 '( F1 )
 (lambda ()
   (run-command "xdotool key g")
   ))

;; Test that F2 key sends an h if in xterm
(xbindkey-condition-function
 '( F2 )
 (lambda ()
   (and (class-is-xterm)
	(run-command-t "xdotool key h"))
   ))

;; Test that F3 key sends an i if NOT in xterm
(xbindkey-condition-function
 '( F3 )
 (lambda ()
   (if (class-is-xterm)
       #f
       (run-command-t "xdotool key i"))
   ))

;; Finally, change j but to different things depending on the window class.
;; j sends a X in xterm otherwise a Y
(xbindkey-condition-function
 '( j )
 (lambda ()
   (if (class-is-xterm)
       (run-command-t "xdotool key X")
       (run-command-t "xdotool key Y"))
   ))
