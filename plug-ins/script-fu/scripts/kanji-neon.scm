;;  KANJI-NEON (´Á»ú¤¤¤ê) -*-scheme-*-
;;  Create a text effect that simulates neon lighting
;;  Shuji Narazaki (narazaki@InetQ.or.jp)
;;  Time-stamp: <1997/06/13 23:16:39 narazaki@InetQ.or.jp>
;;  Version 0.7

(define (script-fu-kanji-neon glow-color tube-hue text size font)
  (define (set-pt a index x y)
    (prog1
     (aset a (* index 2) x)
     (aset a (+ (* index 2) 1) y)))

  (define (neon-spline1)
    (let* ((a (cons-array 6 'byte)))
      (set-pt a 0 0 0)
      (set-pt a 1 127 145)
      (set-pt a 2 255 255)
      a))

  (define (neon-spline2)
    (let* ((a (cons-array 6 'byte)))
      (set-pt a 0 0 0)
      (set-pt a 1 110 150)
      (set-pt a 2 255 255)
      a))

  (define (neon-spline3)
    (let* ((a (cons-array 6 'byte)))
      (set-pt a 0 0 0)
      (set-pt a 1 100 185)
      (set-pt a 2 255 255)
      a))

  (let* ((img (car (gimp-image-new 256 256 RGB)))
	 (border (/ size 4))
	 (shrink (/ size 14))
	 (grow (/ size 40))
	 (feather (/ size 5))
	 (feather1 (/ size 25))
	 (feather2 (/ size 12))
	 (inc-shrink (/ size 100))
	 (glow-layer (car (plug-in-vftext 1 img -1 0 0 text border 1 size 1 "" 
					  font "" "" "" 0)))
	 (width (car (gimp-drawable-width glow-layer)))
	 (height (car (gimp-drawable-height glow-layer)))
	 (bg-layer (car (gimp-layer-new img width height RGB_IMAGE "Neon Glow" 100 NORMAL)))
	 (selection 0)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background))))
    (gimp-image-disable-undo img)
    ;(gimp-display-new img)		; for debug
    (gimp-image-resize img width height 0 0)
    (gimp-image-add-layer img bg-layer 1)

    (gimp-palette-set-background '(0 0 0))
    (gimp-selection-layer-alpha img glow-layer)
    ;; make lines thick to avoid an unknown error of the following process
    (gimp-selection-grow img 1)
    (gimp-edit-fill img glow-layer)
    (set! selection (car (gimp-selection-save img)))
    (gimp-selection-none img)

    (gimp-edit-fill img bg-layer)
    (gimp-edit-fill img glow-layer)

    (gimp-selection-load img selection)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img glow-layer)
    (gimp-selection-shrink img shrink)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill img selection)
    (gimp-edit-fill img glow-layer)

    (gimp-selection-none img)
    (plug-in-gauss-rle 1 img glow-layer feather1 TRUE TRUE)
    (gimp-selection-load img selection)
    (plug-in-gauss-rle 1 img glow-layer feather2 TRUE TRUE)

    (gimp-brightness-contrast img glow-layer -10 15)
    (gimp-selection-none img)
    (gimp-hue-saturation img glow-layer 0 tube-hue -15 70)

    (gimp-selection-load img selection)
    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img inc-shrink)
    (gimp-curves-spline img glow-layer 0 6 (neon-spline1))

    (gimp-selection-load img selection)
    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img (* inc-shrink 2))
    (gimp-curves-spline img glow-layer 0 6 (neon-spline2))

    (gimp-selection-load img selection)
    (gimp-selection-feather img inc-shrink)
    (gimp-selection-shrink img (* inc-shrink 3))
    (gimp-curves-spline img glow-layer 0 6 (neon-spline3))

    (gimp-selection-load img selection)
    (gimp-selection-grow img grow)
    (gimp-selection-invert img)
    (gimp-edit-clear img glow-layer)
    (gimp-selection-invert img)

    (gimp-selection-feather img feather)
    (gimp-palette-set-background glow-color)
    (gimp-edit-fill img bg-layer)
    (gimp-selection-none img)

    (gimp-layer-set-name glow-layer "Neon Tubes")
    (gimp-palette-set-background old-bg)
    (gimp-palette-set-foreground old-fg)
    (gimp-image-enable-undo img)
    (gimp-display-new img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-kanji-neon"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Kanji Neon"
		    "Kanji Neon Text Cyan (0.3)"
		    "Shuji Narazaki <narazaki@InetQ.or.jp>"
		    "Shuji Narazaki"
		    "1997"
		    ""
		    SF-COLOR "Glow Color" '(38 211 255)
		    SF-VALUE "Tube Hue" "-170"
		    SF-VALUE "Text (ill-displayed)" "\"´Á»ú\""
		    SF-VALUE "Font Size (in pixels)" "100"
		    SF-VALUE "Font (string)" "\"min\"")

;; Local Variables:
;; buffer-file-coding-system: euc-japan
;; End:
