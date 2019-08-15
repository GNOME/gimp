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

#include "errno.h"

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-shm.h"
#include "gimp-private.h"
#include "gimpgpcompat.h"
#include "gimpgpparams.h"
#include "gimppdb_pdb.h"
#include "gimpplugin_pdb.h"
#include "gimplegacy-private.h"

#include "libgimp-intl.h"


#define WRITE_BUFFER_SIZE 1024


#define ASSERT_NO_PLUG_IN_EXISTS(strfunc)                               \
  if (gimp_get_plug_in ())                                              \
    {                                                                   \
      g_printerr ("%s ERROR: %s() cannot be called when using the "     \
                  "new plug-in API\n",                                  \
                  g_get_prgname (), strfunc);                           \
      gimp_quit ();                                                     \
    }


static void       gimp_loop                    (GimpRunProc      run_proc);
static void       gimp_process_message         (GimpWireMessage *msg);
static void       gimp_proc_run                (GPProcRun       *proc_run,
                                                GimpRunProc      run_proc);
static gboolean   gimp_plugin_io_error_handler (GIOChannel      *channel,
                                                GIOCondition     cond,
                                                gpointer         data);
static gboolean   gimp_write                   (GIOChannel      *channel,
                                                const guint8    *buf,
                                                gulong           count,
                                                gpointer         user_data);
static gboolean   gimp_flush                   (GIOChannel      *channel,
                                                gpointer         user_data);
static void       gimp_set_pdb_error           (GimpValueArray  *return_vals);


GIOChannel                *_gimp_readchannel  = NULL;
GIOChannel                *_gimp_writechannel = NULL;

static gchar               write_buffer[WRITE_BUFFER_SIZE];
static gulong              write_buffer_index = 0;

static GimpPlugInInfo      PLUG_IN_INFO       = { 0, };

static GimpPDBStatusType   pdb_error_status  = GIMP_PDB_SUCCESS;
static gchar              *pdb_error_message = NULL;


/**
 * gimp_main_legacy:
 * @info: the #GimpPlugInInfo structure
 * @argc: the number of arguments
 * @argv: (array length=argc): the arguments
 *
 * The main procedure that must be called with the #GimpPlugInInfo
 * structure and the 'argc' and 'argv' that are passed to "main".
 *
 * Returns: an exit status as defined by the C library,
 *          on success EXIT_SUCCESS.
 **/
gint
gimp_main_legacy (const GimpPlugInInfo *info,
                  gint                  argc,
                  gchar                *argv[])
{
  return _gimp_main_internal (G_TYPE_NONE, info, argc, argv);
}

/**
 * gimp_install_procedure:
 * @name:                                      the procedure's name.
 * @blurb:                                     a short text describing what the procedure does.
 * @help:                                      the help text for the procedure (usually considerably
 *                                             longer than @blurb).
 * @authors:                                   the procedure's authors.
 * @copyright:                                 the procedure's copyright.
 * @date:                                      the date the procedure was added.
 * @menu_label:                                the label to use for the procedure's menu entry,
 *                                             or %NULL if the procedure has no menu entry.
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
 * The @blurb, @help, @authors, @copyright and @date parameters are
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
 * (@n_params == 0 and @params == %NULL) is an "automatic" extension
 * that will be automatically started on each GIMP startup.
 **/
void
gimp_install_procedure (const gchar        *name,
                        const gchar        *blurb,
                        const gchar        *help,
                        const gchar        *authors,
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  proc_install.name         = (gchar *) name;
  proc_install.blurb        = (gchar *) blurb;
  proc_install.help         = (gchar *) help;
  proc_install.help_id      = (gchar *) name;
  proc_install.authors      = (gchar *) authors;
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

void
_gimp_legacy_read_expect_msg (GimpWireMessage *msg,
                              gint             type)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  while (TRUE)
    {
      if (! gimp_wire_read_msg (_gimp_readchannel, msg, NULL))
        gimp_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          gimp_process_message (msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      gimp_wire_destroy (msg);
    }
}

GimpValueArray *
gimp_run_procedure_array (const gchar          *name,
                          const GimpValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  GimpWireMessage  msg;
  GimpValueArray  *return_values;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  proc_run.name    = (gchar *) name;
  proc_run.nparams = gimp_value_array_length (arguments);
  proc_run.params  = _gimp_value_array_to_gp_params (arguments, FALSE);

  if (! gp_proc_run_write (_gimp_writechannel, &proc_run, NULL))
    gimp_quit ();

  _gimp_legacy_read_expect_msg (&msg, GP_PROC_RETURN);

  proc_return = msg.data;

  return_values = _gimp_gp_params_to_value_array (NULL,
                                                  NULL, 0,
                                                  proc_return->params,
                                                  proc_return->nparams,
                                                  TRUE, FALSE);

  gimp_wire_destroy (&msg);

  gimp_set_pdb_error (return_values);

  return return_values;
}

/**
 * gimp_get_pdb_error:
 *
 * Retrieves the error message from the last procedure call.
 *
 * If a procedure call fails, then it might pass an error message with
 * the return values. Plug-ins that are using the libgimp C wrappers
 * don't access the procedure return values directly. Thus libgimp
 * stores the error message and makes it available with this
 * function. The next procedure call unsets the error message again.
 *
 * The returned string is owned by libgimp and must not be freed or
 * modified.
 *
 * Returns: the error message
 *
 * Since: 2.6
 **/
const gchar *
gimp_get_pdb_error (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (pdb_error_message && strlen (pdb_error_message))
    return pdb_error_message;

  switch (pdb_error_status)
    {
    case GIMP_PDB_SUCCESS:
      /*  procedure executed successfully  */
      return _("success");

    case GIMP_PDB_EXECUTION_ERROR:
      /*  procedure execution failed       */
      return _("execution error");

    case GIMP_PDB_CALLING_ERROR:
      /*  procedure called incorrectly     */
      return _("calling error");

    case GIMP_PDB_CANCEL:
      /*  procedure execution cancelled    */
      return _("cancelled");

    default:
      return "invalid return status";
    }
}

void
_gimp_legacy_initialize (const GimpPlugInInfo *info,
                         GIOChannel           *read_channel,
                         GIOChannel           *write_channel)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  PLUG_IN_INFO = *info;

  _gimp_readchannel  = read_channel;
  _gimp_writechannel = write_channel;

  gp_init ();

  gimp_wire_set_writer (gimp_write);
  gimp_wire_set_flusher (gimp_flush);
}

void
_gimp_legacy_query (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (PLUG_IN_INFO.init_proc)
    gp_has_init_write (_gimp_writechannel, NULL);

  if (PLUG_IN_INFO.query_proc)
    PLUG_IN_INFO.query_proc ();
}

void
_gimp_legacy_init (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (PLUG_IN_INFO.init_proc)
    PLUG_IN_INFO.init_proc ();
}

void
_gimp_legacy_run (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  g_io_add_watch (_gimp_readchannel,
                  G_IO_ERR | G_IO_HUP,
                  gimp_plugin_io_error_handler,
                  NULL);

  gimp_loop (PLUG_IN_INFO.run_proc);
}

void
_gimp_legacy_quit (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (PLUG_IN_INFO.quit_proc)
    PLUG_IN_INFO.quit_proc ();

  _gimp_shm_close ();

  gp_quit_write (_gimp_writechannel, NULL);
}


/*  cruft from other places  */

/**
 * gimp_plugin_menu_register:
 * @procedure_name: The procedure for which to install the menu path.
 * @menu_path: The procedure's additional menu path.
 *
 * Register an additional menu path for a plug-in procedure.
 *
 * This procedure installs an additional menu entry for the given
 * procedure.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
gimp_plugin_menu_register (const gchar *procedure_name,
                           const gchar *menu_path)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_menu_register (procedure_name, menu_path);
}

/**
 * gimp_register_magic_load_handler:
 * @procedure_name: The name of the procedure to be used for loading.
 * @extensions: comma separated list of extensions this handler can load (i.e. "jpg,jpeg").
 * @prefixes: comma separated list of prefixes this handler can load (i.e. "http:,ftp:").
 * @magics: comma separated list of magic file information this handler can load (i.e. "0,string,GIF").
 *
 * Registers a file load handler procedure.
 *
 * Registers a procedural database procedure to be called to load files
 * of a particular file format using magic file information.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_register_magic_load_handler (const gchar *procedure_name,
                                  const gchar *extensions,
                                  const gchar *prefixes,
                                  const gchar *magics)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_magic_load_handler (procedure_name,
                                            extensions, prefixes, magics);
}

/**
 * gimp_register_load_handler:
 * @procedure_name: The name of the procedure to be used for loading.
 * @extensions: comma separated list of extensions this handler can load (i.e. "jpg,jpeg").
 * @prefixes: comma separated list of prefixes this handler can load (i.e. "http:,ftp:").
 *
 * Registers a file load handler procedure.
 *
 * Registers a procedural database procedure to be called to load files
 * of a particular file format.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_register_load_handler (const gchar *procedure_name,
                            const gchar *extensions,
                            const gchar *prefixes)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_load_handler (procedure_name,
                                      extensions, prefixes);
}

/**
 * gimp_register_save_handler:
 * @procedure_name: The name of the procedure to be used for saving.
 * @extensions: comma separated list of extensions this handler can save (i.e. "jpg,jpeg").
 * @prefixes: comma separated list of prefixes this handler can save (i.e. "http:,ftp:").
 *
 * Registers a file save handler procedure.
 *
 * Registers a procedural database procedure to be called to save files
 * in a particular file format.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_register_save_handler (const gchar *procedure_name,
                            const gchar *extensions,
                            const gchar *prefixes)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_save_handler (procedure_name,
                                      extensions, prefixes);
}

/**
 * gimp_register_file_handler_priority:
 * @procedure_name: The name of the procedure to set the priority of.
 * @priority: The procedure priority.
 *
 * Sets the priority of a file handler procedure.
 *
 * Sets the priority of a file handler procedure. When more than one
 * procedure matches a given file, the procedure with the lowest
 * priority is used; if more than one procedure has the lowest
 * priority, it is unspecified which one of them is used. The default
 * priority for file handler procedures is 0.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10.6
 **/
gboolean
gimp_register_file_handler_priority (const gchar *procedure_name,
                                     gint         priority)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_file_handler_priority (procedure_name, priority);
}

/**
 * gimp_register_file_handler_mime:
 * @procedure_name: The name of the procedure to associate a MIME type with.
 * @mime_types: A comma-separated list of MIME types, such as \"image/jpeg\".
 *
 * Associates MIME types with a file handler procedure.
 *
 * Registers MIME types for a file handler procedure. This allows GIMP
 * to determine the MIME type of the file opened or saved using this
 * procedure. It is recommended that only one MIME type is registered
 * per file procedure; when registering more than one MIME type, GIMP
 * will associate the first one with files opened or saved with this
 * procedure.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
gimp_register_file_handler_mime (const gchar *procedure_name,
                                 const gchar *mime_types)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_file_handler_mime (procedure_name, mime_types);
}

/**
 * gimp_register_file_handler_uri:
 * @procedure_name: The name of the procedure to enable URIs for.
 *
 * Registers a file handler procedure as capable of handling URIs.
 *
 * Registers a file handler procedure as capable of handling URIs. This
 * allows GIMP to call the procedure directly for all kinds of URIs,
 * and the 'filename' traditionally passed to file procedures turns
 * into an URI.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_register_file_handler_uri (const gchar *procedure_name)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_file_handler_uri (procedure_name);
}

/**
 * gimp_register_thumbnail_loader:
 * @load_proc: The name of the procedure the thumbnail loader with.
 * @thumb_proc: The name of the thumbnail load procedure.
 *
 * Associates a thumbnail loader with a file load procedure.
 *
 * Some file formats allow for embedded thumbnails, other file formats
 * contain a scalable image or provide the image data in different
 * resolutions. A file plug-in for such a format may register a special
 * procedure that allows GIMP to load a thumbnail preview of the image.
 * This procedure is then associated with the standard load procedure
 * using this function.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
gimp_register_thumbnail_loader (const gchar *load_proc,
                                const gchar *thumb_proc)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_thumbnail_loader (load_proc, thumb_proc);
}


/*  private functions  */

static void
gimp_loop (GimpRunProc run_proc)
{
  GimpWireMessage msg;

  while (TRUE)
    {
      if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
        return;

      switch (msg.type)
        {
        case GP_QUIT:
          gimp_wire_destroy (&msg);
          return;

        case GP_CONFIG:
          _gimp_config (msg.data);
          break;

        case GP_TILE_REQ:
        case GP_TILE_ACK:
        case GP_TILE_DATA:
          g_warning ("unexpected tile message received (should not happen)");
          break;

        case GP_PROC_RUN:
          gimp_proc_run (msg.data, run_proc);
          gimp_wire_destroy (&msg);
          return;

        case GP_PROC_RETURN:
          g_warning ("unexpected proc return message received (should not happen)");
          break;

        case GP_TEMP_PROC_RUN:
          g_warning ("unexpected temp proc run message received (should not happen");
          break;

        case GP_TEMP_PROC_RETURN:
          g_warning ("unexpected temp proc return message received (should not happen");
          break;

        case GP_PROC_INSTALL:
          g_warning ("unexpected proc install message received (should not happen)");
          break;

        case GP_HAS_INIT:
          g_warning ("unexpected has init message received (should not happen)");
          break;
        }

      gimp_wire_destroy (&msg);
    }
}

static void
gimp_process_message (GimpWireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      gimp_quit ();
      break;
    case GP_CONFIG:
      _gimp_config (msg->data);
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received (should not happen)");
      break;
    case GP_PROC_RUN:
      g_warning ("unexpected proc run message received (should not happen)");
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received (should not happen)");
      break;
    case GP_TEMP_PROC_RUN:
      g_warning ("unexpected temp proc run message received (support removed)");
      break;
    case GP_TEMP_PROC_RETURN:
      g_warning ("unexpected temp proc return message received (should not happen)");
      break;
    case GP_PROC_INSTALL:
      g_warning ("unexpected proc install message received (should not happen)");
      break;
    case GP_HAS_INIT:
      g_warning ("unexpected has init message received (should not happen)");
      break;
    }
}

static void
gimp_proc_run (GPProcRun   *proc_run,
               GimpRunProc  run_proc)
{
  GPProcReturn    proc_return;
  GimpValueArray *arguments;
  GimpValueArray *return_values = NULL;
  GimpParam      *params;
  GimpParam      *return_vals;
  gint            n_params;
  gint            n_return_vals;

  arguments = _gimp_gp_params_to_value_array (NULL,
                                              NULL, 0,
                                              proc_run->params,
                                              proc_run->nparams,
                                              FALSE, FALSE);


  n_params = gimp_value_array_length (arguments);
  params   = _gimp_value_array_to_params (arguments, FALSE);

  run_proc (proc_run->name,
            n_params,       params,
            &n_return_vals, &return_vals);

  return_values = _gimp_params_to_value_array (return_vals,
                                               n_return_vals,
                                               FALSE);

  g_free (params);
  gimp_value_array_unref (arguments);

  proc_return.name    = proc_run->name;
  proc_return.nparams = gimp_value_array_length (return_values);
  proc_return.params  = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);

  if (! gp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static gboolean
gimp_plugin_io_error_handler (GIOChannel   *channel,
                              GIOCondition  cond,
                              gpointer      data)
{
  g_printerr ("%s: fatal error: GIMP crashed\n",
              gimp_get_progname ());
  gimp_quit ();

  /* never reached */
  return TRUE;
}

static gboolean
gimp_write (GIOChannel   *channel,
            const guint8 *buf,
            gulong        count,
            gpointer      user_data)
{
  gulong bytes;

  while (count > 0)
    {
      if ((write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - write_buffer_index;
          memcpy (&write_buffer[write_buffer_index], buf, bytes);
          write_buffer_index += bytes;
          if (! gimp_wire_flush (channel, NULL))
            return FALSE;
        }
      else
        {
          bytes = count;
          memcpy (&write_buffer[write_buffer_index], buf, bytes);
          write_buffer_index += bytes;
        }

      buf += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
gimp_flush (GIOChannel *channel,
            gpointer    user_data)
{
  GIOStatus  status;
  GError    *error = NULL;
  gsize      count;
  gsize      bytes;

  if (write_buffer_index > 0)
    {
      count = 0;
      while (count != write_buffer_index)
        {
          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 &write_buffer[count],
                                                 (write_buffer_index - count),
                                                 &bytes,
                                                 &error);
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (error)
                {
                  g_warning ("%s: gimp_flush(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: gimp_flush(): error", g_get_prgname ());
                }

              return FALSE;
            }

          count += bytes;
        }

      write_buffer_index = 0;
    }

  return TRUE;
}

static void
gimp_set_pdb_error (GimpValueArray *return_values)
{
  g_clear_pointer (&pdb_error_message, g_free);
  pdb_error_status = GIMP_PDB_SUCCESS;

  if (gimp_value_array_length (return_values) > 0)
    {
      pdb_error_status =
        g_value_get_enum (gimp_value_array_index (return_values, 0));

      switch (pdb_error_status)
        {
        case GIMP_PDB_SUCCESS:
        case GIMP_PDB_PASS_THROUGH:
          break;

        case GIMP_PDB_EXECUTION_ERROR:
        case GIMP_PDB_CALLING_ERROR:
        case GIMP_PDB_CANCEL:
          if (gimp_value_array_length (return_values) > 1)
            {
              GValue *value = gimp_value_array_index (return_values, 1);

              if (G_VALUE_HOLDS_STRING (value))
                pdb_error_message = g_value_dup_string (value);
            }
          break;
        }
    }
}
