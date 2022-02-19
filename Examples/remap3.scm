;; remap3.scm

;; Test changing keys conditionally depending on the current window class.


;; The following is required to pass the event if the condition fails:
(set-pass-on-fail #t)

(use-modules (ice-9 regex))  ;; for string-match

;; A delay is typically required when using xdotool from xbindkeys.
;; You can comment it and use the -y option instead
(set-delay 150)

; Test that F1 key sends an a (unconditionally)
(xbindkey-function
 '( F1 )
 (lambda ()
   (run-command "xdotool key a")
   ))

;; Similary control-F1 send a b unconditionally
;; We need clearmodifiers here so we don't send a control-b
(xbindkey-function
 '( control F1 )
 (lambda ()
   (run-command "xdotool key --clearmodifiers b")
   ))


(define (class-is-xterm)
   (let ((class (window-class)))
    (string-match "[Xx][Tt]erm" class)))

;; Like run-command but return #t
(define (run-command-t command)
  (run-command command)
  #t)

	
;; Test that F2 key sends a b if in xterm
(xbindkey-condition-function
 '( F2 )
 (lambda ()
   (if (class-is-xterm)
       (run-command-t "xdotool key b")
       #f)
   ))


;; Test that F3 key sends a c if NOT in xterm
(xbindkey-condition-function
 '( F3 )
 (lambda ()
   (if (class-is-xterm)
       #f
       (run-command-t "xdotool key c"))
   ))
