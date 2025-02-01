#!/usr/bin/env gimp-script-fu-interpreter-3.0
;!# Close comment started on first line. Needed by gettext.

; An independently interpreted Scheme plugin
; to test the registration function script-fu-register-18n.

; This is marked with translateable strings,
; but we don't expect translators to translate.
; We mock up translation data.

; Has translateable dialog.
; Dialog appears in native language
; only when the script-fu-register-18n call is proper
; and mockup translation data installed corresponding to said call.
(define (plug-in-test-i18n-1 orientation)
  ; does nothing
)
(define (plug-in-test-i18n-2 elevation)
  ; does nothing
)


; Not a filter, always enabled.
(script-fu-register-procedure "plug-in-test-i18n-1"
  _"Test SF i18n One..."  ; menu item
  ""  ; tooltip
  "LKK"
  "2025"
  ; a non-filter procedure has no image types, always enabled
  ; a non-filter procedure has no drawable arity, always enabled

  ; One arg, just to test the translation of its label
  SF-OPTION     _"Orientation"        '(_"Horizontal"
                                        _"Vertical")
)
(script-fu-register-procedure "plug-in-test-i18n-2"
  _"Test SF i18n Two..."  ; menu item
  ""  ; tooltip
  "LKK"
  "2025"
  SF-OPTION     _"Elevation"        '(_"High"
                                      _"Low")
)

(script-fu-menu-register "plug-in-test-i18n-1"
                         "<Image>/Filters/Development/Test")
(script-fu-menu-register "plug-in-test-i18n-2"
                         "<Image>/Filters/Development/Test")



; This documents the cases for a script to declare translations data.
; This is only for plugins installed to /plug-ins (independently interpreted.)
;
; The script must also have GUI strings marked for translation using notation _"foo".
; A script may be marked, but not have translator produced translation files.
;
; 1. The script can simply omit a call to script-fu-register-i18n.
;    This means: there is no translation data for the procedure.
;
; 2. The script can call (script-fu-register-i18n "proc_name" "None")
;    This also means: there is no translation data for the procedure.
;
; 3. The script can call (script-fu-register-i18n "proc_name" "Standard")
;    The script will be in native language subject to the existence of translation files
;    installed the usual, standard way.
;    This means: the domain name is the script's name
;    and the translation files are in .../plug-ins/<plugin_name>/locale directory
;    for example for French language there exists a file
;    .../plug-ins/<plugin_name>/locale/fr/LC_MESSAGES/<plugin_name>.mo
;    all installed, for example,
;    in GIMP's installed data e.g. /usr/lib/share/GIMP/3.0/plug-ins
;    (when the plugin is an official plugin supported by GIMP)
;    or in the user's ~/.config/GIMP/3.0/plug-ins
;    (when the user wrote or installed a third-party plugin for their own private use.)
;
; 4. The script can call (script-fu-register-i18n "proc_name" "foo_domain" "bar_path")
;    This means a custom install location for the translation files,
;    and a custom name for the domain, i.e. name of the .mo files.
;
;    Typically, this is NOT USEFUL: why change the names?
;    Typically this is NOT USEFUL for a group of plugins sharing translation files.
;
;    It is not useful for group of plugins to share translations data because
;    "bar_path" must be a relative (not absolute) path to the plugin's directory,
;    AND a subdirectory of the plugin's directory.
;    NOT ALLOWED: "../bar" meaning parent dir of bar i.e. in /plug-ins.
;    NOT ALLOWED: "/usr/bar" meaning some absolute path starting at root.
;
;    As of this writing, the useful ways for a group of third-party plugins to share
;    common translations data are:
;       a. Put all the procedures in the same .scm file (you can do that)
;          with a call (script-fu-register-i18n "proc_nameX" "Standard")
;          for each procedure X in the .scm file,
;          and install the usual way e.g. .../plug-ins/plugin-name/plugin-name.scm
;          and install .../plug-ins/plugin-name/locale/fr/LC_MESSAGES/plugin-name.mo
;       b. Install each plugin in its own subdirectory of .../plugins
;          with a call (script-fu-register-i18n "proc_nameX" "Standard")
;          and at install time, distribute the one shared translation file foo.mo
;          to each of the many catalog directories, with renaming,
;          e.g. to     .../plug-ins/plugin-name1/locale/fr/LC_MESSAGES/plugin-name1.
;          and also to .../plug-ins/plugin-name2/locale/fr/LC_MESSAGES/plugin-name2.
;
;    In the example, the translations files will be found
;    in .../plug-ins/proc_name/bar_path
;    which will contain for example one or more files like
;    .../plug-in/proc_name/bar_path/fr/LC_MESSAGES/foo_domain.mo
;
; 5. The script can call (script-fu-register-i18n "proc_name" "gimp30-script-fu")
;    This means use the translation files
;    common to all Scheme plugins distributed with GIMP.
;    This is only useful for official plugins distributed with GIMP.
;    It is not useful for third-party plugins,
;    since they should not be installed in the sys GIMP data directory
;    (else they will be lost in an upgrade)
;    and also since the official translations data installed with GIMP
;    should not be altered by a third-party plugin.

; Note that one script file may define many PDB procedures
; and call script-fu-register-i18n for each of them.
; Some procedures may be translated, and others not.
;
; A script can (but shouldn't) call script-fu-register-i18n more than once
; for the same procedure.
; Only the last one will have effect.




; These are test cases for calls to script-fu-register-i18n.
; To test, comment out one or more trailing cases and run again.
; Only the last uncommented one has effect.


; Valid use cases
; No errors at registration time, but can error at runtime
; depending on existence of translation data.

; Case: Standard translation data
; As of this writing there exists no mockup translation data for this case.
; Expect this will throw an error at plugin run time,
; to the console where GIMP was started,
; since the standard catalog directory (owned by the plugin) does not yet exist:
; .../plug-ins/test-i18n/locale/es/LC_MESSAGES/test-i18n.mo
;(script-fu-register-i18n "plug-in-test-i18n" "Standard")

; Case: no translation data, plugin not in native language.
; Just omit the call to script-fu-register-i18n

; Case: declare domain "None", no translation data, plugin not in native language.
; Expect this does not throw an error, and never translates the plugin.
; Same as previous case, but documents that no translation is done.
;(script-fu-register-i18n "plug-in-test-i18n" "None")

; Case: Rename the domain but not the catalog.
; Expect plugin translates when /plug-ins/test-i18n/locale exists
; and contains es/LC_MESSAGES/scriptfu-test.mo.
; Expect throws an error at plugin run time when said catalog directory not exist
; in a subdirectory of the plugin root dir
; i.e. /usr/local/lib/gimp/3.0/plug-ins/test-i18n/locale/es/LC_MESSAGES/scriptfu-test.mo
(script-fu-register-i18n "plug-in-test-i18n-1" ; plugin name
                         "scriptfu-test")     ; domain name
(script-fu-register-i18n "plug-in-test-i18n-2" ; plugin name
                         "scriptfu-test")     ; domain name
; This is the same as: (script-fu-register-i18n "plug-in-test-i18n" "fooDomain" "locale" )

; Case: Rename the domain and the catalog.
; Expect plugin translates when /plug-ins/plug-in-test-i18n/barCatalog exists
; and contains fr/LC_MESSAGES/fooDomain.mo.
; Expect throws an error at plugin run time when said catalog directory not exist.
; (script-fu-register-i18n "plug-in-test-i18n" "fooDomain" "barCatalog")

; Case: Rename the domain to the one shared by official GIMP ScriptFu plugins.
; Expect plugin translates when GIMP is properly installed,
; and the shared translations data contains translation pairs
; that match translateable strings in the plugin.
; Expect throws an error at plugin run time when GIMP is not properly installed.
; Expect strings in the plugin are in the native language
; (only since by design the string "Orientation" is in gimp30-script-fu.mo)
;(script-fu-register-i18n "plug-in-test-i18n" "gimp30-script-fu")

; Error cases:

; Error case: a relative path to a catalog above the plugin's directory.
; GIMP requires the catalog dir is a subdir of, i.e. beneath, a plugins' install dir.
; Expect this throws an error: "The catalog directory set by set_i18n() is not a subdirectory: ../bar"
; at plugin run time, not registration time.
;(script-fu-register-i18n "plug-in-test-i18n" "fooDomain" "../bar" )

; Error case: an absolute path to a catalog.
; Because GIMP requires path to the catalog dir is not absolute, i.e. starting with "/"
; Expect this throws an error: The catalog directory set by set_i18n() is not relative: /bar
; at plugin run time, not registration time.
;(script-fu-register-i18n "plug-in-test-i18n" "fooDomain" "/bar" )

; Error Case: not enough arguments.
; Expect: Error: script-fu-register-i18n takes two or three args
; in the console, at registration time, but the plugin will still work, without translations.
; (script-fu-register-i18n "plug-in-test-i18n")



