;; For the benefit of emacs users: -*- scheme -*-

;; remap2.scm

;; Remap with conditions
;; Every other control-a shows Hello from Scheme!
;; Turn every other "a" into a "b"


(set-pass-on-fail #t)

(use-modules (ice-9 format)) ;; for format

;; This test will probably also need this (or invoke with -y option)
(set-delay 150)

(define toggle 0)

;; Turn every other "a" into a "b"
(xbindkey-condition-function
 'a
 (lambda ()
   (let ((old-toggle toggle)
	 (do-it (> toggle 0)))
      (set! toggle (- 1 toggle))
      (if do-it
	  (run-command "xdotool key b"))
      do-it)))




