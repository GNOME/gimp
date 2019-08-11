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


/**
 * SECTION: gimplegacy
 * @title: GimpLegacy
 * @short_description: Main functions needed for building a GIMP plug-in.
 *                     This is the old legacy API, please use GimpPlugIn
 *                     and GimpProcedure for all new plug-ins.
 *
 * Main functions needed for building a GIMP plug-in. Compat cruft.
 **/


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
static void       gimp_single_message          (void);
static gboolean   gimp_extension_read          (GIOChannel      *channel,
                                                GIOCondition     condition,
                                                gpointer         data);
static void       gimp_proc_run                (GPProcRun       *proc_run,
                                                GimpRunProc      run_proc);
static void       gimp_temp_proc_run           (GPProcRun       *proc_run);
static void       gimp_proc_run_internal       (GPProcRun       *proc_run,
                                                GimpRunProc      run_proc,
                                                GPProcReturn    *proc_return);
static gboolean   gimp_plugin_io_error_handler (GIOChannel      *channel,
                                                GIOCondition     cond,
                                                gpointer         data);
static gboolean   gimp_write                   (GIOChannel      *channel,
                                                const guint8    *buf,
                                                gulong           count,
                                                gpointer         user_data);
static gboolean   gimp_flush                   (GIOChannel      *channel,
                                                gpointer         user_data);


GIOChannel            *_gimp_readchannel  = NULL;
GIOChannel            *_gimp_writechannel = NULL;

static gchar           write_buffer[WRITE_BUFFER_SIZE];
static gulong          write_buffer_index = 0;

static GimpPlugInInfo  PLUG_IN_INFO       = { 0, };

static GHashTable     *gimp_temp_proc_ht  = NULL;


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
 * @author:                                    the procedure's author(s).
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
 * (@n_params == 0 and @params == %NULL) is an "automatic" extension
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  proc_install.name         = (gchar *) name;
  proc_install.blurb        = (gchar *) blurb;
  proc_install.help         = (gchar *) help;
  proc_install.help_id      = (gchar *) name;
  proc_install.authors      = (gchar *) author;
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
 * @menu_label:    the procedure's menu label, or %NULL if the procedure has
 *                 no menu entry.
 * @image_types:   the drawable types the procedure can handle.
 * @type:          the type of the procedure.
 * @n_params:      the number of parameters the procedure takes.
 * @n_return_vals: the number of return values the procedure returns.
 * @params: (array length=n_params):
 *                 the procedure's parameters.
 * @return_vals: (array length=n_return_vals):
 *                 the procedure's return values.
 * @run_proc: (closure) (scope async):
 *                 the function to call for executing the procedure.
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  gimp_install_procedure (name,
                          blurb, help,
                          author, copyright, date,
                          menu_label,
                          image_types,
                          type,
                          n_params, n_return_vals,
                          params, return_vals);

  /*  Insert the temp proc run function into the hash table  */
  g_hash_table_insert (gimp_temp_proc_ht, g_strdup (name),
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  proc_uninstall.name = (gchar *) name;

  if (! gp_proc_uninstall_write (_gimp_writechannel, &proc_uninstall, NULL))
    gimp_quit ();

  found = g_hash_table_lookup_extended (gimp_temp_proc_ht, name, &hash_name,
                                        NULL);
  if (found)
    {
      g_hash_table_remove (gimp_temp_proc_ht, (gpointer) name);
      g_free (hash_name);
    }
}

/**
 * gimp_extension_ack:
 *
 * Notify the main GIMP application that the extension has been properly
 * initialized and is ready to run.
 *
 * This function <emphasis>must</emphasis> be called from every
 * procedure that was registered as #GIMP_EXTENSION.
 *
 * Subsequently, extensions can process temporary procedure run
 * requests using either gimp_extension_enable() or
 * gimp_extension_process().
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_ack (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (! gp_extension_ack_write (_gimp_writechannel, NULL))
    gimp_quit ();
}

/**
 * gimp_extension_enable:
 *
 * Enables asynchronous processing of messages from the main GIMP
 * application.
 *
 * Normally, a plug-in is not called by GIMP except for the call to
 * the procedure it implements. All subsequent communication is
 * triggered by the plug-in and all messages sent from GIMP to the
 * plug-in are just answers to requests the plug-in made.
 *
 * If the plug-in however registered temporary procedures using
 * gimp_install_temp_proc(), it needs to be able to receive requests
 * to execute them. Usually this will be done by running
 * gimp_extension_process() in an endless loop.
 *
 * If the plug-in cannot use gimp_extension_process(), i.e. if it has
 * a GUI and is hanging around in a #GMainLoop, it must call
 * gimp_extension_enable().
 *
 * Note that the plug-in does not need to be a #GIMP_EXTENSION to
 * register temporary procedures.
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_enable (void)
{
  static gboolean callback_added = FALSE;

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (! callback_added)
    {
      g_io_add_watch (_gimp_readchannel, G_IO_IN | G_IO_PRI,
                      gimp_extension_read,
                      NULL);

      callback_added = TRUE;
    }
}

/**
 * gimp_extension_process:
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by GIMP and returns.
 *
 * Call this function in an endless loop after calling
 * gimp_extension_ack() to process requests for running temporary
 * procedures.
 *
 * See gimp_extension_enable() for an asynchronous way of doing the
 * same if running an endless loop is not an option.
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_process (guint timeout)
{
#ifndef G_OS_WIN32
  gint select_val;

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  do
    {
      fd_set readfds;
      struct timeval  tv;
      struct timeval *tvp;

      if (timeout)
        {
          tv.tv_sec  = timeout / 1000;
          tv.tv_usec = (timeout % 1000) * 1000;
          tvp = &tv;
        }
      else
        tvp = NULL;

      FD_ZERO (&readfds);
      FD_SET (g_io_channel_unix_get_fd (_gimp_readchannel), &readfds);

      if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
        {
          gimp_single_message ();
        }
      else if (select_val == -1 && errno != EINTR)
        {
          perror ("gimp_extension_process");
          gimp_quit ();
        }
    }
  while (select_val == -1 && errno == EINTR);
#else
  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD pollfd;

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (_gimp_readchannel, G_IO_IN, &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    gimp_single_message ();
#endif
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

/**
 * gimp_run_procedure: (skip)
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
 * Returns: the procedure's return values unless there was an error,
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

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
 * gimp_run_procedure2: (rename-to gimp_run_procedure)
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
 * Returns: the procedure's return values unless there was an error,
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  arguments = _gimp_params_to_value_array (params, n_params, FALSE);

  return_values = gimp_run_procedure_array (name, arguments);

  gimp_value_array_unref (arguments);

  *n_return_vals = gimp_value_array_length (return_values);
  return_vals    = _gimp_value_array_to_params (return_values, TRUE);

  gimp_value_array_unref (return_values);

  return return_vals;
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

  _gimp_set_pdb_error (return_values);

  return return_values;
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

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

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
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  while (n_params--)
    {
      g_free (paramdefs[n_params].name);
      g_free (paramdefs[n_params].description);
    }

  g_free (paramdefs);
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

  gimp_temp_proc_ht = g_hash_table_new (g_str_hash, g_str_equal);

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
 * gimp_plugin_domain_register:
 * @domain_name: The name of the textdomain (must be unique).
 * @domain_path: The absolute path to the compiled message catalog (may be NULL).
 *
 * Registers a textdomain for localisation.
 *
 * This procedure adds a textdomain to the list of domains Gimp
 * searches for strings when translating its menu entries. There is no
 * need to call this function for plug-ins that have their strings
 * included in the 'gimp-std-plugins' domain as that is used by
 * default. If the compiled message catalog is not in the standard
 * location, you may specify an absolute path to another location. This
 * procedure can only be called in the query function of a plug-in and
 * it has to be called before any procedure is installed.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_plugin_domain_register (const gchar *domain_name,
                             const gchar *domain_path)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_domain_register (domain_name, domain_path);
}

/**
 * gimp_plugin_help_register:
 * @domain_name: The XML namespace of the plug-in's help pages.
 * @domain_uri: The root URI of the plug-in's help pages.
 *
 * Register a help path for a plug-in.
 *
 * This procedure registers user documentation for the calling plug-in
 * with the GIMP help system. The domain_uri parameter points to the
 * root directory where the plug-in help is installed. For each
 * supported language there should be a file called 'gimp-help.xml'
 * that maps the help IDs to the actual help files.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_plugin_help_register (const gchar *domain_name,
                           const gchar *domain_uri)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_help_register (domain_name, domain_uri);
}

/**
 * gimp_plugin_menu_branch_register:
 * @menu_path: The sub-menu's menu path.
 * @menu_name: The name of the sub-menu.
 *
 * Register a sub-menu.
 *
 * This procedure installs a sub-menu which does not belong to any
 * procedure. The menu-name should be the untranslated menu label. GIMP
 * will look up the translation in the textdomain registered for the
 * plug-in.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_plugin_menu_branch_register (const gchar *menu_path,
                                  const gchar *menu_name)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_menu_branch_register (menu_path, menu_name);
}

/**
 * gimp_plugin_set_pdb_error_handler:
 * @handler: Who is responsible for handling procedure call errors.
 *
 * Sets an error handler for procedure calls.
 *
 * This procedure changes the way that errors in procedure calls are
 * handled. By default GIMP will raise an error dialog if a procedure
 * call made by a plug-in fails. Using this procedure the plug-in can
 * change this behavior. If the error handler is set to
 * %GIMP_PDB_ERROR_HANDLER_PLUGIN, then the plug-in is responsible for
 * calling gimp_get_pdb_error() and handling the error whenever one if
 * its procedure calls fails. It can do this by displaying the error
 * message or by forwarding it in its own return values.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.6
 **/
gboolean
gimp_plugin_set_pdb_error_handler (GimpPDBErrorHandler handler)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_set_pdb_error_handler (handler);
}

/**
 * gimp_plugin_get_pdb_error_handler:
 *
 * Retrieves the active error handler for procedure calls.
 *
 * This procedure retrieves the currently active error handler for
 * procedure calls made by the calling plug-in. See
 * gimp_plugin_set_pdb_error_handler() for details.
 *
 * Returns: Who is responsible for handling procedure call errors.
 *
 * Since: 2.6
 **/
GimpPDBErrorHandler
gimp_plugin_get_pdb_error_handler (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_plugin_get_pdb_error_handler ();
}

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
 * gimp_plugin_icon_register:
 * @procedure_name: The procedure for which to install the icon.
 * @icon_type: The type of the icon.
 * @icon_data: The procedure's icon. The format depends on @icon_type.
 *
 * Register an icon for a plug-in procedure.
 *
 * This procedure installs an icon for the given procedure.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
gimp_plugin_icon_register (const gchar   *procedure_name,
                           GimpIconType   icon_type,
                           gconstpointer  icon_data)
{
  guint8   *data;
  gsize     data_length;
  gboolean  success;

  g_return_val_if_fail (procedure_name != NULL, FALSE);
  g_return_val_if_fail (icon_data != NULL, FALSE);

  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      data        = (guint8 *) icon_data;
      data_length = strlen (icon_data) + 1;
      break;

    case GIMP_ICON_TYPE_PIXBUF:
      if (! gdk_pixbuf_save_to_buffer ((GdkPixbuf *) icon_data,
                                       (gchar **) &data, &data_length,
                                       "png", NULL, NULL))
        return FALSE;
      break;

    case GIMP_ICON_TYPE_IMAGE_FILE:
      data        = (guint8 *) g_file_get_uri ((GFile *) icon_data);
      data_length = strlen (icon_data) + 1;
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  success = _gimp_plugin_icon_register (procedure_name,
                                        icon_type, data_length, data);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_free (data);
      break;
    }

  return success;
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
 * gimp_register_file_handler_raw:
 * @procedure_name: The name of the procedure to enable raw handling for.
 *
 * Registers a file handler procedure as capable of handling raw camera
 * files.
 *
 * Registers a file handler procedure as capable of handling raw
 * digital camera files. Use this procedure only to register raw load
 * handlers, calling it on a save handler will generate an error.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_register_file_handler_raw (const gchar *procedure_name)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_register_file_handler_raw (procedure_name);
}

/**
 * gimp_pdb_temp_name:
 *
 * Generates a unique temporary PDB name.
 *
 * This procedure generates a temporary PDB entry name that is
 * guaranteed to be unique.
 *
 * Returns: (transfer full): A unique temporary name for a temporary PDB entry.
 *          The returned value must be freed with g_free().
 **/
gchar *
gimp_pdb_temp_name (void)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_pdb_temp_name ();
}

/**
 * gimp_pdb_dump:
 * @filename: The dump filename.
 *
 * Dumps the current contents of the procedural database
 *
 * This procedure dumps the contents of the procedural database to the
 * specified file. The file will contain all of the information
 * provided for each registered procedure.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_dump (const gchar *filename)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_pdb_dump (filename);
}

/**
 * gimp_pdb_query:
 * @name: The regex for procedure name.
 * @blurb: The regex for procedure blurb.
 * @help: The regex for procedure help.
 * @author: The regex for procedure author.
 * @copyright: The regex for procedure copyright.
 * @date: The regex for procedure date.
 * @proc_type: The regex for procedure type: { 'Internal GIMP procedure', 'GIMP Plug-in', 'GIMP Extension', 'Temporary Procedure' }.
 * @num_matches: (out): The number of matching procedures.
 * @procedure_names: (out) (array length=num_matches) (element-type gchar*) (transfer full): The list of procedure names.
 *
 * Queries the procedural database for its contents using regular
 * expression matching.
 *
 * This procedure queries the contents of the procedural database. It
 * is supplied with seven arguments matching procedures on { name,
 * blurb, help, author, copyright, date, procedure type}. This is
 * accomplished using regular expression matching. For instance, to
 * find all procedures with \"jpeg\" listed in the blurb, all seven
 * arguments can be supplied as \".*\", except for the second, which
 * can be supplied as \".*jpeg.*\". There are two return arguments for
 * this procedure. The first is the number of procedures matching the
 * query. The second is a concatenated list of procedure names
 * corresponding to those matching the query. If no matching entries
 * are found, then the returned string is NULL and the number of
 * entries is 0.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_query (const gchar   *name,
                const gchar   *blurb,
                const gchar   *help,
                const gchar   *author,
                const gchar   *copyright,
                const gchar   *date,
                const gchar   *proc_type,
                gint          *num_matches,
                gchar       ***procedure_names)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_pdb_query (name,
                          blurb, help,
                          author, copyright, date,
                          proc_type,
                          num_matches, procedure_names);
}

/**
 * gimp_pdb_proc_exists:
 * @procedure_name: The procedure name.
 *
 * Checks if the specified procedure exists in the procedural database
 *
 * This procedure checks if the specified procedure is registered in
 * the procedural database.
 *
 * Returns: Whether a procedure of that name is registered.
 *
 * Since: 2.6
 **/
gboolean
gimp_pdb_proc_exists (const gchar *procedure_name)
{
  ASSERT_NO_PLUG_IN_EXISTS (G_STRFUNC);

  return _gimp_pdb_proc_exists (procedure_name);
}

/**
 * gimp_pdb_proc_info:
 * @procedure_name: The procedure name.
 * @blurb: A short blurb.
 * @help: Detailed procedure help.
 * @author: Author(s) of the procedure.
 * @copyright: The copyright.
 * @date: Copyright date.
 * @proc_type: The procedure type.
 * @num_args: The number of input arguments.
 * @num_values: The number of return values.
 * @args: The input arguments.
 * @return_vals: The return values.
 *
 * Queries the procedural database for information on the specified
 * procedure.
 *
 * This procedure returns information on the specified procedure. A
 * short blurb, detailed help, author(s), copyright information,
 * procedure type, number of input, and number of return values are
 * returned. Additionally this function returns specific information
 * about each input argument and return value.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_pdb_proc_info (const gchar      *procedure_name,
                    gchar           **blurb,
                    gchar           **help,
                    gchar           **author,
                    gchar           **copyright,
                    gchar           **date,
                    GimpPDBProcType  *proc_type,
                    gint             *num_args,
                    gint             *num_values,
                    GimpParamDef    **args,
                    GimpParamDef    **return_vals)
{
  gint i;
  gboolean success = TRUE;

  success = _gimp_pdb_proc_info (procedure_name,
                                 blurb,
                                 help,
                                 author,
                                 copyright,
                                 date,
                                 proc_type,
                                 num_args,
                                 num_values);

  if (success)
    {
      *args        = g_new (GimpParamDef, *num_args);
      *return_vals = g_new (GimpParamDef, *num_values);

      for (i = 0; i < *num_args; i++)
        {
          if (! gimp_pdb_proc_arg (procedure_name, i,
                                   &(*args)[i].type,
                                   &(*args)[i].name,
                                   &(*args)[i].description))
            {
              g_free (*args);
              g_free (*return_vals);

              return FALSE;
            }
        }

      for (i = 0; i < *num_values; i++)
        {
          if (! gimp_pdb_proc_val (procedure_name, i,
                                   &(*return_vals)[i].type,
                                   &(*return_vals)[i].name,
                                   &(*return_vals)[i].description))
            {
              g_free (*args);
              g_free (*return_vals);

              return FALSE;
            }
        }
    }

  return success;
}

/**
 * gimp_pdb_proc_arg:
 * @procedure_name: The procedure name.
 * @arg_num: The argument number.
 * @arg_type: (out): The type of argument.
 * @arg_name: (out) (transfer full): The name of the argument.
 * @arg_desc: (out) (transfer full): A description of the argument.
 *
 * Queries the procedural database for information on the specified
 * procedure's argument.
 *
 * This procedure returns information on the specified procedure's
 * argument. The argument type, name, and a description are retrieved.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_proc_arg (const gchar     *procedure_name,
                   gint             arg_num,
                   GimpPDBArgType  *arg_type,
                   gchar          **arg_name,
                   gchar          **arg_desc)
{
  return _gimp_pdb_proc_arg (procedure_name, arg_num,
                             arg_type, arg_name, arg_desc);
}

/**
 * gimp_pdb_proc_val:
 * @procedure_name: The procedure name.
 * @val_num: The return value number.
 * @val_type: (out): The type of return value.
 * @val_name: (out) (transfer full): The name of the return value.
 * @val_desc: (out) (transfer full): A description of the return value.
 *
 * Queries the procedural database for information on the specified
 * procedure's return value.
 *
 * This procedure returns information on the specified procedure's
 * return value. The return value type, name, and a description are
 * retrieved.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_proc_val (const gchar     *procedure_name,
                   gint             val_num,
                   GimpPDBArgType  *val_type,
                   gchar          **val_name,
                   gchar          **val_desc)
{
  return _gimp_pdb_proc_val (procedure_name, val_num,
                             val_type, val_name, val_desc);
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
      gimp_temp_proc_run (msg->data);
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
gimp_single_message (void)
{
  GimpWireMessage msg;

  /* Run a temp function */
  if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
    gimp_quit ();

  gimp_process_message (&msg);

  gimp_wire_destroy (&msg);
}

static gboolean
gimp_extension_read (GIOChannel  *channel,
                     GIOCondition condition,
                     gpointer     data)
{
  gimp_single_message ();

  return G_SOURCE_CONTINUE;
}

static void
gimp_proc_run (GPProcRun   *proc_run,
               GimpRunProc  run_proc)
{
  GPProcReturn proc_return;

  gimp_proc_run_internal (proc_run, run_proc, &proc_return);

  if (! gp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static void
gimp_temp_proc_run (GPProcRun *proc_run)
{
  GPProcReturn proc_return;
  GimpRunProc  run_proc = g_hash_table_lookup (gimp_temp_proc_ht,
                                               proc_run->name);

  if (run_proc)
    {
#ifdef GDK_WINDOWING_QUARTZ
      if (proc_run->params &&
          proc_run->params[0].data.d_int == GIMP_RUN_INTERACTIVE)
        {
          [NSApp activateIgnoringOtherApps: YES];
        }
#endif

      gimp_proc_run_internal (proc_run, run_proc, &proc_return);

      if (! gp_temp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
        gimp_quit ();
    }
}

static void
gimp_proc_run_internal (GPProcRun     *proc_run,
                        GimpRunProc   run_proc,
                        GPProcReturn *proc_return)
{
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

  proc_return->name    = proc_run->name;
  proc_return->nparams = gimp_value_array_length (return_values);
  proc_return->params  = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);
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
