;;	remap1.scm

;; A simple use of xdotool to [unconditionally] remap keys.

;; Because running xdotool from xbindkeys only seems to work if a
;; delay is introduced,  this file can be used to fine tune what
;; delay is needed.  This can then be added to other .scm files
;; with a line such as
;;	 (set-delay 150)
;; or xbindkeys invoked with the -y option,  e.g. -y 150


;; To change the delay, use one of these keys:
;;  the , key decrements the value by 50
;;  the . key increments it by 50
;;  the / key shows the value

;; When the delay is correct, the q key is remppped by xdotool into an a.
;; Also, the release of a z key is mapped into an x key.
;; (Mapping a release doen't require a delay.)

;; NOTE: Quickly releasing the key works best!  
;; If the key is held down for long it doesn't work.

(use-modules (ice-9 format)) ;; for format

; Test that q key sends an a (unconditionally)
;;   keycode 20          keycode 8
(xbindkey-function
 '( q )
 (lambda ()
   (run-command "xdotool key a" )
   ))

;;     z        ->        x
;; keycode 14       keycode 15
(xbindkey-function
 '( release z )
 (lambda ()
   (run-command "xdotool key x" )
   ))

(define (show-current-delay-value)
  (define delay (get-delay))
  (display (format #f "delay is ~d msec\n" delay)))

(define (change-delay value)
  (display (format #f "delay set to ~d msec\n" value))
  (set-delay value)
  (show-current-delay-value)
)

;; Show the current delay value
;;  slash or questionmark key
(xbindkey-function
 ' ( slash )
   (lambda ()
     (show-current-delay-value)
     ))

;; comma or < key
(xbindkey-function
 ' ( period )
   (lambda ()
     (change-delay (+ (get-delay) 50))
     ))

;; period or > key
(xbindkey-function
 ' ( comma )
   (lambda ()
     (change-delay (- (get-delay) 50))
     ))
