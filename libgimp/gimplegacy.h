/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplegacy.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_LEGACY_H__
#define __GIMP_LEGACY_H__

G_BEGIN_DECLS

#ifndef GIMP_DISABLE_COMPAT_CRUFT


/**
 * GimpInitProc:
 *
 * The init procedure is run at every GIMP startup.
 */
typedef void (* GimpInitProc)  (void);

/**
 * GimpQuitProc:
 *
 * The quit procedure is run each time the plug-in ends.
 */
typedef void (* GimpQuitProc)  (void);

/**
 * GimpQueryProc:
 *
 * The initialization procedure is run at GIMP startup, only the first
 * time after a plug-in is installed, or if it has been updated.
 */
typedef void (* GimpQueryProc) (void);

/**
 * GimpRunProc:
 * @name: the name of the procedure which has been called.
 * @n_params: the number of parameters passed to the procedure.
 * @param: (array length=n_params): the parameters passed to @name.
 * @n_return_vals: (out caller-allocates): the number of values returned by @name.
 * @return_vals: (array length=n_return_vals) (out callee-allocates): the returned values.
 *
 * The run procedure is run during the lifetime of the GIMP session,
 * each time a plug-in procedure is called.
 */
typedef void (* GimpRunProc)   (const gchar      *name,
                                gint              n_params,
                                const GimpParam  *param,
                                gint             *n_return_vals,
                                GimpParam       **return_vals);


/**
 * GimpPlugInInfo:
 * @init_proc:  called when the gimp application initially starts up
 * @quit_proc:  called when the gimp application exits
 * @query_proc: called by gimp so that the plug-in can inform the
 *              gimp of what it does. (ie. installing a procedure database
 *              procedure).
 * @run_proc:   called to run a procedure the plug-in installed in the
 *              procedure database.
 **/
struct _GimpPlugInInfo
{
  GimpInitProc  init_proc;
  GimpQuitProc  quit_proc;
  GimpQueryProc query_proc;
  GimpRunProc   run_proc;
};

/**
 * GimpParamDef:
 * @type:        the parameter's type.
 * @name:        the parameter's name.
 * @description: the parameter's desctiption.
 **/
struct _GimpParamDef
{
  GimpPDBArgType  type;
  gchar          *name;
  gchar          *description;
};

/**
 * GimpParamData:
 * @d_int32:       a 32-bit integer.
 * @d_int16:       a 16-bit integer.
 * @d_int8:        an 8-bit unsigned integer.
 * @d_float:       a double.
 * @d_string:      a string.
 * @d_color:       a #GimpRGB.
 * @d_int32array:  an array of int32.
 * @d_int16array:  an array of int16.
 * @d_int8array:   an array of int8.
 * @d_floatarray:  an array of floats.
 * @d_stringarray: an array of strings.
 * @d_colorarray:  an array of colors.
 * @d_display:     a display id.
 * @d_image:       an image id.
 * @d_item:        an item id.
 * @d_drawable:    a drawable id.
 * @d_layer:       a layer id.
 * @d_channel:     a channel id.
 * @d_layer_mask:  a layer mask id.
 * @d_selection:   a selection id.
 * @d_vectors:     a vectors id.
 * @d_unit:        a GimpUnit.
 * @d_parasite:    a GimpParasite.
 * @d_tattoo:      a tattoo.
 * @d_status:      a return status.
 **/
union _GimpParamData
{
  gint32            d_int32;
  gint16            d_int16;
  guint8            d_int8;
  gdouble           d_float;
  gchar            *d_string;
  GimpRGB           d_color;
  gint32           *d_int32array;
  gint16           *d_int16array;
  guint8           *d_int8array;
  gdouble          *d_floatarray;
  gchar           **d_stringarray;
  GimpRGB          *d_colorarray;
  gint32            d_display;
  gint32            d_image;
  gint32            d_item;
  gint32            d_drawable;
  gint32            d_layer;
  gint32            d_channel;
  gint32            d_layer_mask;
  gint32            d_selection;
  gint32            d_vectors;
  gint32            d_unit;
  GimpParasite      d_parasite;
  gint32            d_tattoo;
  GimpPDBStatusType d_status;
};

/**
 * GimpParam:
 * @type: the parameter's type.
 * @data: the parameter's data.
 **/
struct _GimpParam
{
  GimpPDBArgType type;
  GimpParamData  data;
};

/**
 * MAIN:
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code
 * MAIN() at the toplevel of your file.  No semicolon should be used.
 **/

#ifdef G_OS_WIN32

/* Define WinMain() because plug-ins are built as GUI applications. Also
 * define a main() in case some plug-in still is built as a console
 * application.
 */
#  ifdef __GNUC__
#    ifndef _stdcall
#      define _stdcall __attribute__((stdcall))
#    endif
#  endif

#  define MAIN()                                        \
   struct HINSTANCE__;                                  \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow);                            \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow)                             \
   {                                                    \
     return gimp_main_legacy (&PLUG_IN_INFO,            \
                              __argc, __argv);          \
   }                                                    \
                                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     /* Use __argc and __argv here, too, as they work   \
      * better with mingw-w64.                          \
      */                                                \
     return gimp_main_legacy (&PLUG_IN_INFO,            \
                              __argc, __argv);          \
   }
#else
#  define MAIN()                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     return gimp_main_legacy (&PLUG_IN_INFO,            \
                              argc, argv);              \
   }
#endif


/* The main procedure that must be called with the PLUG_IN_INFO
 * structure and the 'argc' and 'argv' that are passed to "main".
 */
gint           gimp_main_legacy         (const GimpPlugInInfo *info,
                                         gint                  argc,
                                         gchar                *argv[]);

/* Install a procedure in the procedure database.
 */
void           gimp_install_procedure   (const gchar        *name,
                                         const gchar        *blurb,
                                         const gchar        *help,
                                         const gchar        *author,
                                         const gchar        *copyright,
                                         const gchar        *date,
                                         const gchar        *menu_label,
                                         const gchar        *image_types,
                                         GimpPDBProcType     type,
                                         gint                n_params,
                                         gint                n_return_vals,
                                         const GimpParamDef *params,
                                         const GimpParamDef *return_vals);

/* Install a temporary procedure in the procedure database.
 */
void           gimp_install_temp_proc   (const gchar        *name,
                                         const gchar        *blurb,
                                         const gchar        *help,
                                         const gchar        *author,
                                         const gchar        *copyright,
                                         const gchar        *date,
                                         const gchar        *menu_label,
                                         const gchar        *image_types,
                                         GimpPDBProcType     type,
                                         gint                n_params,
                                         gint                n_return_vals,
                                         const GimpParamDef *params,
                                         const GimpParamDef *return_vals,
                                         GimpRunProc         run_proc);

/* Uninstall a temporary procedure
 */
void           gimp_uninstall_temp_proc (const gchar        *name);

/* Notify the main GIMP application that the extension is ready to run
 */
void           gimp_extension_ack       (void);

/* Enable asynchronous processing of temp_procs
 */
void           gimp_extension_enable    (void);

/* Process one temp_proc and return
 */
void           gimp_extension_process   (guint            timeout);

/* Run a procedure in the procedure database. The parameters are
 *  specified via the variable length argument list. The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam    * gimp_run_procedure       (const gchar     *name,
                                         gint            *n_return_vals,
                                         ...);

/* Run a procedure in the procedure database. The parameters are
 *  specified as an array of GimpParam.  The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam    * gimp_run_procedure2      (const gchar     *name,
                                         gint            *n_return_vals,
                                         gint             n_params,
                                         const GimpParam *params);

/* Run a procedure in the procedure database. The parameters are
 *  specified as a GimpValueArray, so are the return values.
 */
GimpValueArray * gimp_run_procedure_array (const gchar          *name,
                                           const GimpValueArray *arguments);

/* Destroy the an array of parameters. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_run_procedure'.
 */
void           gimp_destroy_params      (GimpParam       *params,
                                         gint             n_params);

/* Destroy the an array of GimpParamDef's. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_procedural_db_proc_info'.
 */
void           gimp_destroy_paramdefs   (GimpParamDef    *paramdefs,
                                         gint             n_params);

/* Retrieve the error message and return status for the last procedure
 * call.
 */
const gchar       * gimp_get_pdb_error  (void);
GimpPDBStatusType   gimp_get_pdb_status (void);

/* gimp_plugin API that should now be done by using GimpPlugIn
 */

gboolean   gimp_plugin_domain_register       (const gchar         *domain_name,
                                              const gchar         *domain_path);
gboolean   gimp_plugin_help_register         (const gchar         *domain_name,
                                              const gchar         *domain_uri);
gboolean   gimp_plugin_menu_branch_register  (const gchar         *menu_path,
                                              const gchar         *menu_name);
gboolean   gimp_plugin_set_pdb_error_handler (GimpPDBErrorHandler  handler);
GimpPDBErrorHandler
           gimp_plugin_get_pdb_error_handler (void);


/* gimp_plugin API that should now be done by using GimpProcedure
 */

gboolean   gimp_plugin_menu_register        (const gchar   *procedure_name,
                                             const gchar   *menu_path);
gboolean   gimp_plugin_icon_register        (const gchar   *procedure_name,
                                             GimpIconType   icon_type,
                                             gconstpointer  icon_data);

/* gimp_fileops API that should now be done by using GimpFileProcedure
 */

gboolean   gimp_register_magic_load_handler    (const gchar *procedure_name,
                                                const gchar *extensions,
                                                const gchar *prefixes,
                                                const gchar *magics);
gboolean   gimp_register_load_handler          (const gchar *procedure_name,
                                                const gchar *extensions,
                                                const gchar *prefixes);
gboolean   gimp_register_save_handler          (const gchar *procedure_name,
                                                const gchar *extensions,
                                                const gchar *prefixes);
gboolean   gimp_register_file_handler_priority (const gchar *procedure_name,
                                                gint         priority);
gboolean   gimp_register_file_handler_mime     (const gchar *procedure_name,
                                                const gchar *mime_types);
gboolean   gimp_register_file_handler_uri      (const gchar *procedure_name);
gboolean   gimp_register_file_handler_raw      (const gchar *procedure_name);
gboolean   gimp_register_thumbnail_loader      (const gchar *load_proc,
                                                const gchar *thumb_proc);

/* pdb stuff that should now be done using GimpPDB
 */

gchar    * gimp_pdb_temp_name     (void);
gboolean   gimp_pdb_dump          (const gchar      *filename);
gboolean   gimp_pdb_query         (const gchar      *name,
                                   const gchar      *blurb,
                                   const gchar      *help,
                                   const gchar      *author,
                                   const gchar      *copyright,
                                   const gchar      *date,
                                   const gchar      *proc_type,
                                   gint             *num_matches,
                                   gchar          ***procedure_names);
gboolean   gimp_pdb_proc_exists   (const gchar      *procedure_name);
gboolean   gimp_pdb_proc_info     (const gchar      *procedure_name,
                                   gchar           **blurb,
                                   gchar           **help,
                                   gchar           **author,
                                   gchar           **copyright,
                                   gchar           **date,
                                   GimpPDBProcType  *proc_type,
                                   gint             *num_args,
                                   gint             *num_values,
                                   GimpParamDef    **args,
                                   GimpParamDef    **return_vals);
gboolean   gimp_pdb_proc_arg      (const gchar      *procedure_name,
                                   gint              arg_num,
                                   GimpPDBArgType   *arg_type,
                                   gchar           **arg_name,
                                   gchar           **arg_desc);
gboolean   gimp_pdb_proc_val      (const gchar      *procedure_name,
                                   gint              val_num,
                                   GimpPDBArgType   *val_type,
                                   gchar           **val_name,
                                   gchar           **val_desc);


#endif /* GIMP_DISABLE_COMPAT_CRUFT */

G_END_DECLS

#endif /* __GIMP_LEGACY_H__ */
