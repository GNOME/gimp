/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplegacy.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"
#include "libgimpbase/gimpprotocol.h"

#include "gimp.h"
#include "gimp-private.h"
#include "gimpgpcompat.h"
#include "gimpgpparams.h"


/**
 * SECTION: gimplegacy
 * @title: GimpLegacy
 * @short_description: Main functions needed for building a GIMP plug-in.
 *                     This is the old legacy API, please use GimpPlugIn
 *                     and GimpProcedure for all new plug-ins.
 *
 * Main functions needed for building a GIMP plug-in. Compat cruft.
 **/


static gpointer   gimp_param_copy (gpointer boxed);
static void       gimp_param_free (gpointer boxed);


/**
 * gimp_plug_in_info_set_callbacks:
 * @info: the PLUG_IN_INFO structure
 * @init_proc:  (closure) (scope async) (nullable): the init procedure
 * @quit_proc:  (closure) (scope async) (nullable): the quit procedure
 * @query_proc: (closure) (scope async) (nullable): the query procedure
 * @run_proc:   (closure) (scope async) (nullable): the run procedure
 *
 * The procedure that must be called with the PLUG_IN_INFO structure to
 * set the initialization, query, run and quit callbacks.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_info_set_callbacks (GimpPlugInInfo *info,
                                 GimpInitProc    init_proc,
                                 GimpQuitProc    quit_proc,
                                 GimpQueryProc   query_proc,
                                 GimpRunProc     run_proc)
{
  info->init_proc  = init_proc;
  info->quit_proc  = quit_proc;
  info->query_proc = query_proc;
  info->run_proc   = run_proc;
}

/**
 * gimp_param_from_int32:
 * @value: the #gint32 value to set.
 *
 * The procedure creates a new #GimpParam for a int32 value.
 *
 * Returns: (transfer full): A newly allocated #GimpParam.
 *
 * Since: 3.0
 **/
GimpParam *
gimp_param_from_int32 (gint32 value)
{
  GimpParam * param = g_new0 (GimpParam, 1);

  param->type = GIMP_PDB_INT32;
  param->data.d_int32 = value;

  return param;
}

/**
 * gimp_param_get_int32:
 * @param: the #GimpParam.
 *
 * Unwrap the integer value contained in @param. Running this function
 * if @param is not an int32 #GimpParam is a programming error.
 *
 * Returns: the #gint32 value contained in @param.
 *
 * Since: 3.0
 **/
gint32
gimp_param_get_int32 (GimpParam *param)
{
  g_return_val_if_fail (param->type == GIMP_PDB_INT32, 0);

  return param->data.d_int32;
}

/**
 * gimp_param_from_status:
 * @value: the #GimpPDBStatusType value to set.
 *
 * The procedure creates a new #GimpParam for a status value.
 *
 * Returns: (transfer full): A newly allocated #GimpParam.
 *
 * Since: 3.0
 **/
GimpParam *
gimp_param_from_status (GimpPDBStatusType value)
{
  GimpParam * param = g_new0 (GimpParam, 1);

  param->type = GIMP_PDB_STATUS;
  param->data.d_status = value;

  return param;
}

/**
 * gimp_param_get_status:
 * @param: the #GimpParam.
 *
 * Unwrap the status value contained in @param. Running this function
 * if @param is not a status #GimpParam is a programming error.
 *
 * Returns: the #GimpPDBStatusType value contained in @param.
 *
 * Since: 3.0
 **/
GimpPDBStatusType
gimp_param_get_status (GimpParam *param)
{
  g_return_val_if_fail (param->type == GIMP_PDB_STATUS, 0);

  return param->data.d_status;
}

/**
 * gimp_param_from_string:
 * @value: the string value to set.
 *
 * The procedure creates a new #GimpParam for a string value.
 *
 * Returns: (transfer full): A newly allocated #GimpParam.
 *
 * Since: 3.0
 **/
GimpParam *
gimp_param_from_string (gchar *value)
{
  GimpParam * param = g_new0 (GimpParam, 1);

  param->type = GIMP_PDB_STRING;
  param->data.d_string = g_strdup (value);

  return param;
}

/**
 * gimp_param_get_string:
 * @param: the #GimpParam.
 *
 * Unwrap the string value contained in @param. Running this function
 * if @param is not a string #GimpParam is a programming error.
 *
 * Returns: the string value contained in @param.
 *
 * Since: 3.0
 **/
gchar *
gimp_param_get_string (GimpParam *param)
{
  g_return_val_if_fail (param->type == GIMP_PDB_STRING, NULL);

  return param->data.d_string;
}

/**
 * gimp_install_procedure:
 * @name:                                      the procedure's name.
 * @blurb:                                     a short text describing what the procedure does.
 * @help:                                      the help text for the procedure (usually considerably
 *                                             longer than @blurb).
 * @author:                                    the procedure's author(s).
 * @copyright:                                 the procedure's copyright.
 * @date:                                      the date the procedure was added.
 * @menu_label:                                the label to use for the procedure's menu entry,
 *                                             or #NULL if the procedure has no menu entry.
 * @image_types:                               the drawable types the procedure can handle.
 * @type:                                      the type of the procedure.
 * @n_params:                                  the number of parameters the procedure takes.
 * @n_return_vals:                             the number of return values the procedure returns.
 * @params: (array length=n_params):           the procedure's parameters.
 * @return_vals: (array length=n_return_vals): the procedure's return values.
 *
 * Installs a new procedure with the PDB (procedural database).
 *
 * Call this function from within your plug-in's query() function for
 * each procedure your plug-in implements.
 *
 * The @name parameter is mandatory and should be unique, or it will
 * overwrite an already existing procedure (overwrite procedures only
 * if you know what you're doing).
 *
 * The @blurb, @help, @author, @copyright and @date parameters are
 * optional but then you shouldn't write procedures without proper
 * documentation, should you.
 *
 * @menu_label defines the label that should be used for the
 * procedure's menu entry. The position where to register in the menu
 * hierarchy is chosen using gimp_plugin_menu_register().
 *
 * It is possible to register a procedure only for keyboard-shortcut
 * activation by passing a @menu_label to gimp_install_procedure() but
 * not registering any menu path with gimp_plugin_menu_register(). In
 * this case, the given @menu_label will only be used as the
 * procedure's user-visible name in the keyboard shortcut editor.
 *
 * @image_types is a comma separated list of image types, or actually
 * drawable types, that this procedure can deal with. Wildcards are
 * possible here, so you could say "RGB*" instead of "RGB, RGBA" or
 * "*" for all image types. If the procedure doesn't need an image to
 * run, use the empty string.
 *
 * @type must be one of %GIMP_PLUGIN or %GIMP_EXTENSION. Note that
 * temporary procedures must be installed using
 * gimp_install_temp_proc().
 *
 * NOTE: Unlike the GIMP 1.2 API, %GIMP_EXTENSION no longer means
 * that the procedure's menu prefix is &lt;Toolbox&gt;, but that
 * it will install temporary procedures. Therefore, the GIMP core
 * will wait until the %GIMP_EXTENSION procedure has called
 * gimp_extension_ack(), which means that the procedure has done
 * its initialization, installed its temporary procedures and is
 * ready to run.
 *
 * <emphasis>Not calling gimp_extension_ack() from a %GIMP_EXTENSION
 * procedure will cause the GIMP core to lock up.</emphasis>
 *
 * Additionally, a %GIMP_EXTENSION procedure with no parameters
 * (@n_params == 0 and @params == #NULL) is an "automatic" extension
 * that will be automatically started on each GIMP startup.
 **/
void
gimp_install_procedure (const gchar        *name,
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
                        const GimpParamDef *return_vals)
{
  GPProcInstall  proc_install;
  GList         *pspecs = NULL;
  gint           i;

  g_return_if_fail (name != NULL);
  g_return_if_fail (type != GIMP_INTERNAL);
  g_return_if_fail ((n_params == 0 && params == NULL) ||
                    (n_params > 0  && params != NULL));
  g_return_if_fail ((n_return_vals == 0 && return_vals == NULL) ||
                    (n_return_vals > 0  && return_vals != NULL));

  proc_install.name         = (gchar *) name;
  proc_install.blurb        = (gchar *) blurb;
  proc_install.help         = (gchar *) help;
  proc_install.author       = (gchar *) author;
  proc_install.copyright    = (gchar *) copyright;
  proc_install.date         = (gchar *) date;
  proc_install.menu_label   = (gchar *) menu_label;
  proc_install.image_types  = (gchar *) image_types;
  proc_install.type         = type;
  proc_install.nparams      = n_params;
  proc_install.nreturn_vals = n_return_vals;
  proc_install.params       = g_new0 (GPParamDef, n_params);
  proc_install.return_vals  = g_new0 (GPParamDef, n_return_vals);

  for (i = 0; i < n_params; i++)
    {
      GParamSpec *pspec = _gimp_gp_compat_param_spec (params[i].type,
                                                      params[i].name,
                                                      params[i].name,
                                                      params[i].description);

      _gimp_param_spec_to_gp_param_def (pspec, &proc_install.params[i]);

      pspecs = g_list_prepend (pspecs, pspec);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      GParamSpec *pspec = _gimp_gp_compat_param_spec (return_vals[i].type,
                                                      return_vals[i].name,
                                                      return_vals[i].name,
                                                      return_vals[i].description);

      _gimp_param_spec_to_gp_param_def (pspec, &proc_install.return_vals[i]);

      pspecs = g_list_prepend (pspecs, pspec);
    }

  if (! gp_proc_install_write (_gimp_writechannel, &proc_install, NULL))
    gimp_quit ();

  g_list_foreach (pspecs, (GFunc) g_param_spec_ref_sink, NULL);
  g_list_free_full (pspecs, (GDestroyNotify) g_param_spec_unref);

  g_free (proc_install.params);
  g_free (proc_install.return_vals);
}

/**
 * gimp_install_temp_proc:
 * @name:          the procedure's name.
 * @blurb:         a short text describing what the procedure does.
 * @help:          the help text for the procedure (usually considerably
 *                 longer than @blurb).
 * @author:        the procedure's author(s).
 * @copyright:     the procedure's copyright.
 * @date:          the date the procedure was added.
 * @menu_label:    the procedure's menu label, or #NULL if the procedure has
 *                 no menu entry.
 * @image_types:   the drawable types the procedure can handle.
 * @type:          the type of the procedure.
 * @n_params:      the number of parameters the procedure takes.
 * @n_return_vals: the number of return values the procedure returns.
 * @params:        the procedure's parameters.
 * @return_vals:   the procedure's return values.
 * @run_proc:      the function to call for executing the procedure.
 *
 * Installs a new temporary procedure with the PDB (procedural database).
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * See gimp_install_procedure() for most details.
 *
 * @type <emphasis>must</emphasis> be %GIMP_TEMPORARY or the function
 * will fail.
 *
 * @run_proc is the function which will be called to execute the
 * procedure.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the GIMP core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either gimp_extension_enable() or
 * gimp_extension_process(). See this functions' documentation for
 * details.
 **/
void
gimp_install_temp_proc (const gchar        *name,
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
                        GimpRunProc         run_proc)
{
  g_return_if_fail (name != NULL);
  g_return_if_fail ((n_params == 0 && params == NULL) ||
                    (n_params > 0  && params != NULL));
  g_return_if_fail ((n_return_vals == 0 && return_vals == NULL) ||
                    (n_return_vals > 0  && return_vals != NULL));
  g_return_if_fail (type == GIMP_TEMPORARY);
  g_return_if_fail (run_proc != NULL);

  gimp_install_procedure (name,
                          blurb, help,
                          author, copyright, date,
                          menu_label,
                          image_types,
                          type,
                          n_params, n_return_vals,
                          params, return_vals);

  /*  Insert the temp proc run function into the hash table  */
  g_hash_table_insert (_gimp_temp_proc_ht, g_strdup (name),
                       (gpointer) run_proc);
}

/**
 * gimp_uninstall_temp_proc:
 * @name: the procedure's name
 *
 * Uninstalls a temporary procedure which has previously been
 * installed using gimp_install_temp_proc().
 **/
void
gimp_uninstall_temp_proc (const gchar *name)
{
  GPProcUninstall proc_uninstall;
  gpointer        hash_name;
  gboolean        found;

  g_return_if_fail (name != NULL);

  proc_uninstall.name = (gchar *) name;

  if (! gp_proc_uninstall_write (_gimp_writechannel, &proc_uninstall, NULL))
    gimp_quit ();

  found = g_hash_table_lookup_extended (_gimp_temp_proc_ht, name, &hash_name,
                                        NULL);
  if (found)
    {
      g_hash_table_remove (_gimp_temp_proc_ht, (gpointer) name);
      g_free (hash_name);
    }
}

/**
 * gimp_run_procedure:
 * @name:          the name of the procedure to run
 * @n_return_vals: return location for the number of return values
 * @...:           list of procedure parameters
 *
 * This function calls a GIMP procedure and returns its return values.
 *
 * The procedure's parameters are given by a va_list in the format
 * (type, value, type, value) and must be terminated by %GIMP_PDB_END.
 *
 * This function converts the va_list of parameters into an array and
 * passes them to gimp_run_procedure2(). Please look there for further
 * information.
 *
 * Return value: the procedure's return values unless there was an error,
 * in which case the zero-th return value will be the error status, and
 * the first return value will be a string detailing the error.
 **/
GimpParam *
gimp_run_procedure (const gchar *name,
                    gint        *n_return_vals,
                    ...)
{
  GimpPDBArgType  param_type;
  GimpParam      *return_vals;
  GimpParam      *params   = NULL;
  gint            n_params = 0;
  va_list         args;
  gint            i;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  va_start (args, n_return_vals);
  param_type = va_arg (args, GimpPDBArgType);

  while (param_type != GIMP_PDB_END)
    {
      switch (param_type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_STATUS:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_INT16:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_INT8:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_FLOAT:
          (void) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          (void) va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          (void) va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          (void) va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          (void) va_arg (args, gint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          (void) va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          (void) va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
        case GIMP_PDB_COLORARRAY:
          (void) va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_PARASITE:
          (void) va_arg (args, GimpParasite *);
          break;
        case GIMP_PDB_END:
          break;
        }

      n_params++;

      param_type = va_arg (args, GimpPDBArgType);
    }

  va_end (args);

  params = g_new0 (GimpParam, n_params);

  va_start (args, n_return_vals);

  for (i = 0; i < n_params; i++)
    {
      params[i].type = va_arg (args, GimpPDBArgType);

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
          params[i].data.d_int32 = (gint32) va_arg (args, gint);
          break;
        case GIMP_PDB_INT16:
          params[i].data.d_int16 = (gint16) va_arg (args, gint);
          break;
        case GIMP_PDB_INT8:
          params[i].data.d_int8 = (guint8) va_arg (args, gint);
          break;
        case GIMP_PDB_FLOAT:
          params[i].data.d_float = (gdouble) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          params[i].data.d_string = va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          params[i].data.d_int32array = va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          params[i].data.d_int16array = va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          params[i].data.d_int8array = va_arg (args, guint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          params[i].data.d_floatarray = va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          params[i].data.d_stringarray = va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
          params[i].data.d_color = *va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_ITEM:
          params[i].data.d_item = va_arg (args, gint32);
          break;
        case GIMP_PDB_DISPLAY:
          params[i].data.d_display = va_arg (args, gint32);
          break;
        case GIMP_PDB_IMAGE:
          params[i].data.d_image = va_arg (args, gint32);
          break;
        case GIMP_PDB_LAYER:
          params[i].data.d_layer = va_arg (args, gint32);
          break;
        case GIMP_PDB_CHANNEL:
          params[i].data.d_channel = va_arg (args, gint32);
          break;
        case GIMP_PDB_DRAWABLE:
          params[i].data.d_drawable = va_arg (args, gint32);
          break;
        case GIMP_PDB_SELECTION:
          params[i].data.d_selection = va_arg (args, gint32);
          break;
        case GIMP_PDB_COLORARRAY:
          params[i].data.d_colorarray = va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_VECTORS:
          params[i].data.d_vectors = va_arg (args, gint32);
          break;
        case GIMP_PDB_PARASITE:
          {
            GimpParasite *parasite = va_arg (args, GimpParasite *);

            if (parasite == NULL)
              {
                params[i].data.d_parasite.name = NULL;
                params[i].data.d_parasite.data = NULL;
              }
            else
              {
                params[i].data.d_parasite.name  = parasite->name;
                params[i].data.d_parasite.flags = parasite->flags;
                params[i].data.d_parasite.size  = parasite->size;
                params[i].data.d_parasite.data  = parasite->data;
              }
          }
          break;
        case GIMP_PDB_STATUS:
          params[i].data.d_status = va_arg (args, gint32);
          break;
        case GIMP_PDB_END:
          break;
        }
    }

  va_end (args);

  return_vals = gimp_run_procedure2 (name, n_return_vals, n_params, params);

  g_free (params);

  return return_vals;
}

/**
 * gimp_run_procedure2:
 * @name:          the name of the procedure to run
 * @n_return_vals: return location for the number of return values
 * @n_params:      the number of parameters the procedure takes.
 * @params:        the procedure's parameters array.
 *
 * This function calls a GIMP procedure and returns its return values.
 * To get more information about the available procedures and the
 * parameters they expect, please have a look at the Procedure Browser
 * as found in the Xtns menu in GIMP's toolbox.
 *
 * As soon as you don't need the return values any longer, you should
 * free them using gimp_destroy_params().
 *
 * Return value: the procedure's return values unless there was an error,
 * in which case the zero-th return value will be the error status, and
 * if there are two values returned, the other return value will be a
 * string detailing the error.
 **/
GimpParam *
gimp_run_procedure2 (const gchar     *name,
                     gint            *n_return_vals,
                     gint             n_params,
                     const GimpParam *params)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values;
  GimpParam      *return_vals;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  arguments = _gimp_params_to_value_array (params, n_params, FALSE);

  return_values = gimp_run_procedure_with_array (name, arguments);

  gimp_value_array_unref (arguments);

  *n_return_vals = gimp_value_array_length (return_values);
  return_vals    = _gimp_value_array_to_params (return_values, TRUE);

  gimp_value_array_unref (return_values);

  return return_vals;
}

/**
 * gimp_destroy_params:
 * @params:   the #GimpParam array to destroy
 * @n_params: the number of elements in the array
 *
 * Destroys a #GimpParam array as returned by gimp_run_procedure() or
 * gimp_run_procedure2().
 **/
void
gimp_destroy_params (GimpParam *params,
                     gint       n_params)
{
  gint i;

  for (i = 0; i < n_params; i++)
    {
      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
        case GIMP_PDB_COLOR:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_STATUS:
          break;

        case GIMP_PDB_STRING:
          g_free (params[i].data.d_string);
          break;

        case GIMP_PDB_INT32ARRAY:
          g_free (params[i].data.d_int32array);
          break;

        case GIMP_PDB_INT16ARRAY:
          g_free (params[i].data.d_int16array);
          break;

        case GIMP_PDB_INT8ARRAY:
          g_free (params[i].data.d_int8array);
          break;

        case GIMP_PDB_FLOATARRAY:
          g_free (params[i].data.d_floatarray);
          break;

        case GIMP_PDB_STRINGARRAY:
          if ((i > 0) && (params[i-1].type == GIMP_PDB_INT32))
            {
              gint count = params[i-1].data.d_int32;
              gint j;

              for (j = 0; j < count; j++)
                g_free (params[i].data.d_stringarray[j]);

              g_free (params[i].data.d_stringarray);
            }
          break;

        case GIMP_PDB_COLORARRAY:
          g_free (params[i].data.d_colorarray);
          break;

        case GIMP_PDB_PARASITE:
          if (params[i].data.d_parasite.name)
            g_free (params[i].data.d_parasite.name);
          if (params[i].data.d_parasite.data)
            g_free (params[i].data.d_parasite.data);
          break;

        case GIMP_PDB_END:
          break;
        }
    }

  g_free (params);
}

/**
 * gimp_destroy_paramdefs:
 * @paramdefs: the #GimpParamDef array to destroy
 * @n_params:  the number of elements in the array
 *
 * Destroys a #GimpParamDef array as returned by
 * gimp_procedural_db_proc_info().
 **/
void
gimp_destroy_paramdefs (GimpParamDef *paramdefs,
                        gint          n_params)
{
  while (n_params--)
    {
      g_free (paramdefs[n_params].name);
      g_free (paramdefs[n_params].description);
    }

  g_free (paramdefs);
}

/* Define boxed type functions. */

static gpointer
gimp_param_copy (gpointer boxed)
{
  GimpParam *param = boxed;
  GimpParam *new_param;

  new_param = g_slice_new (GimpParam);
  new_param->type = param->type;
  switch (param->type)
    {
    case GIMP_PDB_STRING:
      new_param->data.d_string = g_strdup (param->data.d_string);
      break;
    case GIMP_PDB_INT32ARRAY:
    case GIMP_PDB_INT16ARRAY:
    case GIMP_PDB_INT8ARRAY:
    case GIMP_PDB_FLOATARRAY:
    case GIMP_PDB_COLORARRAY:
    case GIMP_PDB_STRINGARRAY:
      /* XXX: we can't copy these because we don't know the size, and
       * we are bounded by the GBoxed copy function signature.
       * Anyway this is only temporary until we replace GimpParam in the
       * new API.
       */
      g_return_val_if_reached (new_param);
      break;
    default:
      new_param->data = param->data;
      break;
    }

  return new_param;
}

static void
gimp_param_free (gpointer boxed)
{
  GimpParam *param = boxed;

  switch (param->type)
    {
    case GIMP_PDB_STRING:
      g_free (param->data.d_string);
      break;
    case GIMP_PDB_INT32ARRAY:
      g_free (param->data.d_int32array);
      break;
    case GIMP_PDB_INT16ARRAY:
      g_free (param->data.d_int16array);
      break;
    case GIMP_PDB_INT8ARRAY:
      g_free (param->data.d_int8array);
      break;
    case GIMP_PDB_FLOATARRAY:
      g_free (param->data.d_floatarray);
      break;
    case GIMP_PDB_COLORARRAY:
      g_free (param->data.d_colorarray);
      break;
    case GIMP_PDB_STRINGARRAY:
      /* XXX: we also want to free each element string. Unfortunately
       * this type is not zero-terminated or anything of the sort to
       * determine the number of elements.
       * It uses the previous parameter, but we cannot have such value
       * in a GBoxed's free function.
       * Since this is all most likely temporary code until we update
       * the plug-in API, let's just leak for now.
       */
      g_free (param->data.d_stringarray);
      break;
    default:
      /* Pass-through. */
      break;
    }

  g_slice_free (GimpParam, boxed);
}

G_DEFINE_BOXED_TYPE (GimpParam, gimp_param,
                     gimp_param_copy,
                     gimp_param_free)


/*  old gimp_plugin cruft  */

gboolean
gimp_plugin_icon_register (const gchar  *procedure_name,
                           GimpIconType  icon_type,
                           const guint8 *icon_data)
{
  gint icon_data_length;

  g_return_val_if_fail (procedure_name != NULL, FALSE);
  g_return_val_if_fail (icon_data != NULL, FALSE);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      icon_data_length = strlen ((const gchar *) icon_data) + 1;
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      g_return_val_if_fail (g_ntohl (*((gint32 *) icon_data)) == 0x47646b50,
                            FALSE);

      icon_data_length = g_ntohl (*((gint32 *) (icon_data + 4)));
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return _gimp_plugin_icon_register (procedure_name,
                                     icon_type, icon_data_length, icon_data);
}
