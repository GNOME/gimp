; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.  
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;
; waves-anim.scm   version 1.00   09/04/97
;
; Copyright (C) 1997 Sven Neumann (neumanns@uni-duesseldorf.de)
; 
;  
; Makes a copy of your image and creates an animation of the active layer
; as if a stone was thrown into the image. The animation may be saved with 
; the gif-plug-in. 

(define (script-fu-waves-anim img 
			      drawable 
			      amplitude 
			      wavelength 
			      num-frames
			      invert)
  (let* ((amplitude (max 0 amplitude))
	 (wavelength (max 0 wavelength))
	 (num-frames (max 1 num-frames))
	 (remaining-frames num-frames)
	 (phase 0)
	 (phaseshift (/ 360 num-frames))
         (image (car (gimp-channel-ops-duplicate img))))
   
  (gimp-image-disable-undo image)

  (if (= invert TRUE)
      (set! phaseshift (- 0 phaseshift)))
  (set! source-layer (car (gimp-image-get-active-layer image)))
  
  (while (> remaining-frames 1)
         (set! waves-layer (car (gimp-layer-copy source-layer TRUE)))
         (gimp-layer-set-preserve-trans waves-layer FALSE)
	 (gimp-image-add-layer image waves-layer -1)
	 (set! layer-name (string-append "Frame " 
					 (number->string
					  (- (+ num-frames 2) 
					     remaining-frames) 10)))
	 (gimp-layer-set-name waves-layer layer-name)
	 
	 (plug-in-waves 1 
			image 
			waves-layer
			amplitude 
			phase 
			wavelength 
			0 
			FALSE)
	  
	 (set! remaining-frames (- remaining-frames 1))
	 (set! phase (- phase phaseshift)))

  (gimp-layer-set-name source-layer "Frame 1")
  (plug-in-waves 1 
		 image 
		 source-layer
		 amplitude 
		 phase 
		 wavelength 
		 0 
		 FALSE)

  (gimp-image-enable-undo image)
  (gimp-display-new image)))

(script-fu-register "script-fu-waves-anim" 
		    "<Image>/Script-Fu/Animators/Waves"
		    "Animate an image like a stone's been thrown into it"
		    "Sven Neumann (neumanns@uni-duesseldorf.de)"
		    "Sven Neumann"
		    "09/04/1997"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Amplitude" "10"
		    SF-VALUE "Wavelength" "10" 
		    SF-VALUE "Number of Frames" "6"
		    SF-TOGGLE "Invert direction" FALSE)

























