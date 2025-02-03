; Test color profile conversion operations on image

; color profiles are in files
; ~/.local/share/icc on Ubuntu for local displays i.e. monitors
; Are they communicated from the monitors themselves when monitor sensed at startup?

; These tests are not now testing the algorithms themselves,
; only the crude algebra of calls to the API.
; That is, we don't render the image and have a human look at the results.
; Also, there is no API for programmatically finding what rendered colors are out of gamut.
; Also, there is no API for programmatically rendering under the simulation profile.
; i.e. like the user would do under View>Color Management>SoftProof.

; Notes about test data, all binary files
; name                        vector   file  MODE
; sRGBGimp                       672   NA    RGB  'GIMP built-in sRGB'
; grayGimp???                    544   NA    GRAY 'GIMP built-in D65 Grayscale with sRGB TRC'
; sRGB2014.icc                  3024   3kb,  RGB
; CGATS001Compat-v2-micro.icc   8464   9kb,  CMYK but minimal, can't be used for conversion.
; sGrey-v2-nano.icc              290   1kb,  GRAY

; These are too large for ScriptFu to handle
; SNAP2007.icc                        719kb, CMYK?
; Coated_Fogra39L_VIGC_300.icc        8.7Mb, CMYK?


; Since 3.0rc3, all getters of color profile are PRIVATE to libgimp !!!



; setup
(define testImage (testing:load-test-image "gimp-logo.png"))

; an image we use just to transfer color profiles
(define colorImage (car (gimp-image-new 21 22 RGB)))

; setup a testProfile
; Now PRIVATE
;(define testProfile (car (gimp-image-get-color-profile colorImage)))
; testProfile is a Scheme vector, a C GBytes
; testProfile is the default effective one, named sRGBGimp



; tests

; Initial profiles on an image

; Now PRIVATE

; the test image has no assigned color profile
;(assert `(= (vector-length (car (gimp-image-get-color-profile ,testImage)))
;            0))
; the test image has no assigned simulation color profile
;(assert `(= (vector-length (car (gimp-image-get-simulation-profile ,testImage)))
;            0))
; the test image has an effective profile of a generated profile (named sRGBGimp)
; Is different length than one named sRGB2014 promulgated by ICC organization?
;(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
;           672))


; Setting profile

; When image is mode RGB and profile not set,
; can't set profile from incompatible profile:
; SNAP2007 is CMYK?
(assert-error `(gimp-image-set-color-profile-from-file
                  ,testImage
                  (testing:path-to-color-profile "SNAP2007.icc")
                  COLOR-RENDERING-INTENT-PERCEPTUAL
                  TRUE ) ; black point compensation
    "Procedure execution of gimp-image-set-color-profile-from-file failed: ICC profile validation failed:")
;    " Color profile is not for RGB color space "

; When image is mode RGB, can set profile from compatible profile (another "RGB" profile)
(assert `(gimp-image-set-color-profile-from-file
            ,testImage
            (testing:path-to-color-profile "sRGB2014.icc")
            COLOR-RENDERING-INTENT-PERCEPTUAL
            TRUE )) ; black point compensation

; effective: profile is now a longer vector
; Now PRIVATE
;(assert `(= (vector-length (car (gimp-image-get-color-profile ,testImage)))
;            3024))


; convert-color-profile is now PRIVATE to libgimp
; convert from raw profile that is empty vector fails
;(assert-error `(gimp-image-convert-color-profile ,testImage ,emptyTestProfile)
;              "Procedure execution of gimp-image-convert-color-profile failed: Data does not appear to be an ICC color profile ")


; simulation profile
; You only get/set a simulation profile, you don't convert image to a simulation profile.
; Instead you only soft-proof the image using the simulation profile.

; You can set the simulation profile from an RGB profile, same as actual profile
(assert `(gimp-image-set-simulation-profile-from-file
            ,testImage
            (testing:path-to-color-profile "sRGB2014.icc")))
; effective
;(assert `(= (vector-length (car (gimp-image-get-simulation-profile ,testImage)))
;            3024))

; You can set the simulation profile from any profile (even not compatible i.e. CMYK.)
(assert `(gimp-image-set-simulation-profile-from-file
            ,testImage
            (testing:path-to-color-profile "CGATS001Compat-v2-micro.icc")))
; effective
;(assert `(= (vector-length (car (gimp-image-get-simulation-profile ,testImage)))
;            8464))

; You can set the simulation profile from a large CMYK file
; This is a stress test: a large profile
(assert `(gimp-image-set-simulation-profile-from-file
            ,testImage
            (testing:path-to-color-profile "Coated_Fogra39L_VIGC_300.icc")))
; effective: the vector of the profile is a different length than prior
; Not tested, crashes, because TS runs out of memory??? Tried setting CELL_SEGSIZE to 100k, did not help
;(assert `(= (vector-length (car (gimp-image-get-simulation-profile ,testImage)))
;         3024))




; grayscale

; testImage is mode RGB and has a color profile set on it.

; Converting mode automatically changes the effective color profile, but not the set color profile

; Can convert mode of testImage to greyscale, when color profile is RGB
(assert `(gimp-image-convert-grayscale ,testImage))
; mode conversion was effective
(assert `(= (car (gimp-image-get-base-type ,testImage))
            GRAY))
; Not effective on setting the color profile, it was cleared.
;(assert `(= (vector-length (car (gimp-image-get-color-profile ,testImage)))
;            0))
; effective at changing the effective color profile
;(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
;            544))

; mode GRAY image with no color profile set can be set to a GRAY profile

; succeeds
(assert `(gimp-image-convert-color-profile-from-file
            ,testImage
            (testing:path-to-color-profile "sGrey-v2-nano.icc")
            COLOR-RENDERING-INTENT-PERCEPTUAL
            TRUE ))
; Effective on setting the color profile
;(assert `(= (vector-length (car (gimp-image-get-color-profile ,testImage)))
;            290))
; effective at changing the effective color profile
;(assert `(= (vector-length (car (gimp-image-get-effective-color-profile ,testImage)))
;            290))


; setting profile GRAY=>RGB fails
(assert-error `(gimp-image-set-color-profile-from-file
                  ,testImage
                  (testing:path-to-color-profile "sRGB2014.icc")
                  COLOR-RENDERING-INTENT-PERCEPTUAL
                  TRUE ); black point compensation
              (string-append
                "Procedure execution of gimp-image-set-color-profile-from-file failed: "
                "ICC profile validation failed: Color profile is not for grayscale color space"))
; color profile is same as before
;(assert `(= (vector-length (car (gimp-image-get-color-profile ,testImage)))
;            290))


; for debugging individual test file:
;(gimp-display-new testImage)
