;  Shared memory interface

(define script-fu-shmseg-new shmseg-new)
(script-fu-register "script-fu-shmseg-new"
		    "<Toolbox>/Xtns/Script-Fu/Shm/Create"
		    "Create shared memory segment"
		    "Ray Lehtiniemi"
		    "Ray Lehtiniemi"
		    "1998"
		    ""
		    SF-VALUE "Shm key" "100"
		    SF-VALUE "Shm size" "1000000"
		    SF-VALUE "Permissions" "511")


(define script-fu-shmseg-delete shmseg-delete)
(script-fu-register "script-fu-shmseg-delete"
		    "<Toolbox>/Xtns/Script-Fu/Shm/Delete"
		    "Delete shared memory segment"
		    "Ray Lehtiniemi"
		    "Ray Lehtiniemi"
		    "1998"
		    ""
		    SF-VALUE "Shmid" "100")

(define script-fu-shmseg-attach shmseg-attach)
(script-fu-register "script-fu-shmseg-attach"
		    "<Toolbox>/Xtns/Script-Fu/Shm/Attach"
		    "Attach to a shared memory segment"
		    "Ray Lehtiniemi"
		    "Ray Lehtiniemi"
		    "1998"
		    ""
		    SF-VALUE "Shmid" "100"
		    SF-VALUE "Offset" "0")

(define script-fu-shmseg-detach shmseg-detach)
(script-fu-register "script-fu-shmseg-detach"
		    "<Toolbox>/Xtns/Script-Fu/Shm/Detach"
		    "Detach from a shared memory segment"
		    "Ray Lehtiniemi"
		    "Ray Lehtiniemi"
		    "1998"
		    ""
		    SF-VALUE "Shmid" "100")

(define (script-fu-shmseg-image height width)
  (let* ((img (car (gimp-image-new width height U16_RGB)))
	 (drawable (car (gimp-layer-new img width height
                                        (+ U16_RGB_IMAGE 256)
                                        "Shared Memory Layer" 100 NORMAL))))
    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))


(script-fu-register "script-fu-shmseg-image"
		    "<Toolbox>/Xtns/Script-Fu/Shm/Image"
		    "Create an image from the shared memory segment"
		    "Ray Lehtiniemi"
		    "Ray Lehtiniemi"
		    "1998"
		    ""
                    SF-VALUE "Height" "256"
                    SF-VALUE "Width"  "280")
