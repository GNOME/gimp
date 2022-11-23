/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaplugin.c
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#include "config.h"

#include <errno.h>
#include <libintl.h>
#include <string.h>

#include "ligma.h"

#include "libligmabase/ligmaprotocol.h"
#include "libligmabase/ligmawire.h"

#include "ligma-private.h"
#include "ligma-shm.h"
#include "ligmagpparams.h"
#include "ligmaplugin-private.h"
#include "ligmaplugin_pdb.h"
#include "ligmaprocedure-private.h"


/**
 * LigmaPlugIn:
 *
 * The base class for plug-ins to derive from.
 *
 * LigmaPlugIn manages the plug-in's [class@Procedure] objects. The procedures a
 * plug-in implements are registered with LIGMA by returning a list of their
 * names from either [vfunc@LigmaPlugIn.query_procedures] or
 * [vfunc@LigmaPlugIn.init_procedures].
 *
 * Every LIGMA plug-in has to be implemented as a subclass and make it known to
 * the libligma infrastructure and the main LIGMA application by passing its
 * `GType` to [func@MAIN].
 *
 * [func@MAIN] passes the 'argc' and 'argv' of the platform's main() function,
 * along with the `GType`, to [func@main], which creates an instance of the
 * plug-in's `LigmaPlugIn` subclass and calls its virtual functions, depending
 * on how the plug-in was called by LIGMA.
 *
 * There are 3 different ways LIGMA calls a plug-in: "query", "init" and "run".
 *
 * The plug-in is called in "query" mode once after it was installed, or when
 * the cached plug-in information in the config file "pluginrc" needs to be
 * recreated. In "query" mode, [vfunc@LigmaPlugIn.query_procedures] is called
 * and returns a list of procedure names the plug-in implements. This is the
 * "normal" place to register procedures, because the existence of most
 * procedures doesn't depend on things that change between LIGMA sessions.
 *
 * The plug-in is called in "init" mode at each LIGMA startup, and
 * [vfunc@PlugIn.init_procedures] is called and returns a list of procedure
 * names this plug-in implements. This only happens if the plug-in actually
 * implements [vfunc@LigmaPlugIn.init_procedures]. A plug-in only needs to
 * implement init_procedures if the existence of its procedures can change
 * between LIGMA sessions, for example if they depend on the presence of
 * external tools, or hardware like scanners, or online services, or whatever
 * variable circumstances.
 *
 * In order to register the plug-in's procedures with the main LIGMA application
 * in the plug-in's "query" and "init" modes, [class@PlugIn] calls
 * [vfunc@PlugIn.create_procedure] on all procedure names in the exact order of
 * the list returned by [vfunc@PlugIn.query_procedures] or
 * [vfunc@PlugIn.init_procedures] and then registers the returned
 * [class@Procedure].
 *
 * The plug-in is called in "run" mode whenever one of the procedures it
 * implements is called by either the main LIGMA application or any other
 * plug-in. In "run" mode, one of the procedure names returned by
 * [vfunc@PlugIn.query_procedures] or [vfunc@PlugIn.init_procedures] is passed
 * to [vfunc@PlugIn.create_procedure] which must return a [class@Procedure] for
 * the passed name. The procedure is then executed by calling
 * [method@Procedure.run].
 *
 * In any of the three modes, [vfunc@PlugIn.quit] is called before the plug-in
 * process exits, so the plug-in can perform whatever cleanup necessary.
 *
 * Since: 3.0
 */


#define WRITE_BUFFER_SIZE 1024

/**
 * ligma_plug_in_error_quark:
 *
 * Generic #GQuark error domain for plug-ins. Plug-ins are welcome to
 * create their own domain when they want to handle advanced error
 * handling. Often, you just want to pass an error message to the core.
 * This domain can be used for such simple usage.
 *
 * See #GError for information on error domains.
 */
G_DEFINE_QUARK (ligma-plug-in-error-quark, ligma_plug_in_error)

enum
{
  PROP_0,
  PROP_READ_CHANNEL,
  PROP_WRITE_CHANNEL,
  N_PROPS
};


typedef struct _LigmaPlugInMenuBranch LigmaPlugInMenuBranch;

struct _LigmaPlugInMenuBranch
{
  gchar *menu_path;
  gchar *menu_label;
};

struct _LigmaPlugInPrivate
{
  GIOChannel *read_channel;
  GIOChannel *write_channel;

  gchar       write_buffer[WRITE_BUFFER_SIZE];
  gulong      write_buffer_index;

  guint       extension_source_id;

  gchar      *translation_domain_name;
  GFile      *translation_domain_path;

  gchar      *help_domain_name;
  GFile      *help_domain_uri;

  GList      *menu_branches;

  GList      *temp_procedures;

  GList      *procedure_stack;
  GHashTable *displays;
  GHashTable *images;
  GHashTable *items;
};


static void       ligma_plug_in_constructed       (GObject         *object);
static void       ligma_plug_in_dispose           (GObject         *object);
static void       ligma_plug_in_finalize          (GObject         *object);
static void       ligma_plug_in_set_property      (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void       ligma_plug_in_get_property      (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);

static gboolean   ligma_plug_in_real_set_i18n     (LigmaPlugIn      *plug_in,
                                                  const gchar     *procedure_name,
                                                  gchar          **gettext_domain,
                                                  gchar          **catalog_dir);

static void       ligma_plug_in_register          (LigmaPlugIn      *plug_in,
                                                  GList           *procedures);

static gboolean   ligma_plug_in_write             (GIOChannel      *channel,
                                                  const guint8    *buf,
                                                  gulong           count,
                                                  gpointer         user_data);
static gboolean   ligma_plug_in_flush             (GIOChannel      *channel,
                                                  gpointer         user_data);
static gboolean   ligma_plug_in_io_error_handler  (GIOChannel      *channel,
                                                  GIOCondition     cond,
                                                  gpointer         data);

static void       ligma_plug_in_loop              (LigmaPlugIn      *plug_in);
static void       ligma_plug_in_single_message    (LigmaPlugIn      *plug_in);
static void       ligma_plug_in_process_message   (LigmaPlugIn      *plug_in,
                                                  LigmaWireMessage *msg);
static void       ligma_plug_in_proc_run          (LigmaPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       ligma_plug_in_temp_proc_run     (LigmaPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       ligma_plug_in_proc_run_internal (LigmaPlugIn      *plug_in,
                                                  GPProcRun       *proc_run,
                                                  LigmaProcedure   *procedure,
                                                  GPProcReturn    *proc_return);
static gboolean   ligma_plug_in_extension_read    (GIOChannel      *channel,
                                                  GIOCondition     condition,
                                                  gpointer         data);

static void       ligma_plug_in_push_procedure    (LigmaPlugIn      *plug_in,
                                                  LigmaProcedure   *procedure);
static void       ligma_plug_in_pop_procedure     (LigmaPlugIn      *plug_in,
                                                  LigmaProcedure   *procedure);
static void       ligma_plug_in_destroy_hashes    (LigmaPlugIn      *plug_in);
static void       ligma_plug_in_destroy_proxies   (GHashTable      *hash_table,
                                                  gboolean         destroy_all);

static void       ligma_plug_in_init_i18n         (LigmaPlugIn      *plug_in);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPlugIn, ligma_plug_in, G_TYPE_OBJECT)

#define parent_class ligma_plug_in_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
ligma_plug_in_class_init (LigmaPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_plug_in_constructed;
  object_class->dispose      = ligma_plug_in_dispose;
  object_class->finalize     = ligma_plug_in_finalize;
  object_class->set_property = ligma_plug_in_set_property;
  object_class->get_property = ligma_plug_in_get_property;

  klass->set_i18n            = ligma_plug_in_real_set_i18n;

  /**
   * LigmaPlugIn:read-channel:
   *
   * The [struct@GLib.IOChannel] to read from LIGMA
   */
  props[PROP_READ_CHANNEL] =
    g_param_spec_boxed ("read-channel",
                        "Read channel",
                        "The GIOChanel to read from LIGMA",
                        G_TYPE_IO_CHANNEL,
                        LIGMA_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaPlugIn:write-channel:
   *
   * The [struct@GLib.IOChannel] to write to LIGMA
   */
  props[PROP_WRITE_CHANNEL] =
    g_param_spec_boxed ("write-channel",
                        "Write channel",
                        "The GIOChanel to write to LIGMA",
                        G_TYPE_IO_CHANNEL,
                        LIGMA_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
ligma_plug_in_init (LigmaPlugIn *plug_in)
{
  plug_in->priv = ligma_plug_in_get_instance_private (plug_in);
}

static void
ligma_plug_in_constructed (GObject *object)
{
  LigmaPlugIn *plug_in = LIGMA_PLUG_IN (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (plug_in->priv->read_channel != NULL);
  g_assert (plug_in->priv->write_channel != NULL);

  gp_init ();

  ligma_wire_set_writer (ligma_plug_in_write);
  ligma_wire_set_flusher (ligma_plug_in_flush);
}

static void
ligma_plug_in_dispose (GObject *object)
{
  LigmaPlugIn *plug_in = LIGMA_PLUG_IN (object);

  if (plug_in->priv->extension_source_id)
    {
      g_source_remove (plug_in->priv->extension_source_id);
      plug_in->priv->extension_source_id = 0;
    }

  if (plug_in->priv->temp_procedures)
    {
      g_list_free_full (plug_in->priv->temp_procedures, g_object_unref);
      plug_in->priv->temp_procedures = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_plug_in_finalize (GObject *object)
{
  LigmaPlugIn *plug_in = LIGMA_PLUG_IN (object);
  GList      *list;

  g_clear_pointer (&plug_in->priv->translation_domain_name, g_free);
  g_clear_object  (&plug_in->priv->translation_domain_path);

  g_clear_pointer (&plug_in->priv->help_domain_name, g_free);
  g_clear_object  (&plug_in->priv->help_domain_uri);

  for (list = plug_in->priv->menu_branches; list; list = g_list_next (list))
    {
      LigmaPlugInMenuBranch *branch = list->data;

      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_slice_free (LigmaPlugInMenuBranch, branch);
    }

  g_clear_pointer (&plug_in->priv->menu_branches, g_list_free);

  ligma_plug_in_destroy_proxies (plug_in->priv->displays, TRUE);
  ligma_plug_in_destroy_proxies (plug_in->priv->images,   TRUE);
  ligma_plug_in_destroy_proxies (plug_in->priv->items,    TRUE);

  ligma_plug_in_destroy_hashes (plug_in);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_plug_in_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  LigmaPlugIn *plug_in = LIGMA_PLUG_IN (object);

  switch (property_id)
    {
    case PROP_READ_CHANNEL:
      plug_in->priv->read_channel = g_value_get_boxed (value);
      break;

    case PROP_WRITE_CHANNEL:
      plug_in->priv->write_channel = g_value_get_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_plug_in_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LigmaPlugIn *plug_in = LIGMA_PLUG_IN (object);

  switch (property_id)
    {
    case PROP_READ_CHANNEL:
      g_value_set_boxed (value, plug_in->priv->read_channel);
      break;

    case PROP_WRITE_CHANNEL:
      g_value_set_boxed (value, plug_in->priv->write_channel);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_plug_in_real_set_i18n (LigmaPlugIn   *plug_in,
                            const gchar  *procedure_name,
                            gchar       **gettext_domain,
                            gchar       **catalog_dir)
{
  /* Default to enabling localization by gettext. It will have the good
   * side-effect of warning plug-in developers of the existence of the
   * ability through stderr if the catalog directory is missing.
   */
  return TRUE;
}


/*  public functions  */

/**
 * ligma_plug_in_set_help_domain:
 * @plug_in:     A #LigmaPlugIn.
 * @domain_name: The XML namespace of the plug-in's help pages.
 * @domain_uri:  The root URI of the plug-in's help pages.
 *
 * Set a help domain and path for the @plug_in.
 *
 * This function registers user documentation for the calling plug-in
 * with the LIGMA help system. The @domain_uri parameter points to the
 * root directory where the plug-in help is installed. For each
 * supported language there should be a file called 'ligma-help.xml'
 * that maps the help IDs to the actual help files.
 *
 * This function can only be called in the
 * [vfunc@PlugIn.query_procedures] function of a plug-in.
 *
 * Since: 3.0
 **/
void
ligma_plug_in_set_help_domain (LigmaPlugIn  *plug_in,
                              const gchar *domain_name,
                              GFile       *domain_uri)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (G_IS_FILE (domain_uri));

  g_free (plug_in->priv->help_domain_name);
  plug_in->priv->help_domain_name = g_strdup (domain_name);

  g_set_object (&plug_in->priv->help_domain_uri, domain_uri);
}

/**
 * ligma_plug_in_add_menu_branch:
 * @plug_in:    A #LigmaPlugIn
 * @menu_path:  The sub-menu's menu path.
 * @menu_label: The menu label of the sub-menu.
 *
 * Add a new sub-menu to the LIGMA menus.
 *
 * This function installs a sub-menu which does not belong to any
 * procedure at the location @menu_path.
 *
 * For translations of @menu_label to work properly, @menu_label
 * should only be marked for translation but passed to this function
 * untranslated, for example using N_("Submenu"). LIGMA will look up
 * the translation in the textdomain registered for the plug-in.
 *
 * See also: ligma_procedure_add_menu_path().
 *
 * Since: 3.0
 **/
void
ligma_plug_in_add_menu_branch (LigmaPlugIn  *plug_in,
                              const gchar *menu_path,
                              const gchar *menu_label)
{
  LigmaPlugInMenuBranch *branch;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  branch = g_slice_new (LigmaPlugInMenuBranch);

  branch->menu_path  = g_strdup (menu_path);
  branch->menu_label = g_strdup (menu_label);

  plug_in->priv->menu_branches = g_list_append (plug_in->priv->menu_branches,
                                                branch);
}

/**
 * ligma_plug_in_add_temp_procedure:
 * @plug_in:   A #LigmaPlugIn
 * @procedure: A #LigmaProcedure of type %LIGMA_PDB_PROC_TYPE_TEMPORARY.
 *
 * This function adds a temporary procedure to @plug_in. It is usually
 * called from a %LIGMA_PDB_PROC_TYPE_EXTENSION procedure's
 * [vfunc@Procedure.run].
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * The procedure's type _must_ be
 * %LIGMA_PDB_PROC_TYPE_TEMPORARY or the function will fail.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the LIGMA core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either [method@PlugIn.extension_enable] or
 * [method@PlugIn.extension_process]. See their respective documentation
 * for details.
 *
 * Since: 3.0
 **/
void
ligma_plug_in_add_temp_procedure (LigmaPlugIn    *plug_in,
                                 LigmaProcedure *procedure)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));
  g_return_if_fail (ligma_procedure_get_proc_type (procedure) ==
                    LIGMA_PDB_PROC_TYPE_TEMPORARY);

  plug_in->priv->temp_procedures =
    g_list_prepend (plug_in->priv->temp_procedures,
                    g_object_ref (procedure));

  LIGMA_PROCEDURE_GET_CLASS (procedure)->install (procedure);
}

/**
 * ligma_plug_in_remove_temp_procedure:
 * @plug_in:        A #LigmaPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function removes a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Since: 3.0
 **/
void
ligma_plug_in_remove_temp_procedure (LigmaPlugIn  *plug_in,
                                    const gchar *procedure_name)
{
  LigmaProcedure *procedure;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (ligma_is_canonical_identifier (procedure_name));

  procedure = ligma_plug_in_get_temp_procedure (plug_in, procedure_name);

  if (procedure)
    {
      LIGMA_PROCEDURE_GET_CLASS (procedure)->uninstall (procedure);

      plug_in->priv->temp_procedures =
        g_list_remove (plug_in->priv->temp_procedures,
                       procedure);
      g_object_unref (procedure);
    }
}

/**
 * ligma_plug_in_get_temp_procedures:
 * @plug_in: A plug-in
 *
 * This function retrieves the list of temporary procedure of @plug_in as
 * added with [method@PlugIn.add_temp_procedure].
 *
 * Returns: (transfer none) (element-type LigmaProcedure): The list of
 *          procedures.
 *
 * Since: 3.0
 **/
GList *
ligma_plug_in_get_temp_procedures (LigmaPlugIn *plug_in)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->temp_procedures;
}

/**
 * ligma_plug_in_get_temp_procedure:
 * @plug_in:        A #LigmaPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function retrieves a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Returns: (nullable) (transfer none): The procedure if registered, or %NULL.
 *
 * Since: 3.0
 **/
LigmaProcedure *
ligma_plug_in_get_temp_procedure (LigmaPlugIn  *plug_in,
                                 const gchar *procedure_name)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);

  for (list = plug_in->priv->temp_procedures; list; list = g_list_next (list))
    {
      LigmaProcedure *procedure = list->data;

      if (! strcmp (procedure_name, ligma_procedure_get_name (procedure)))
        return procedure;
    }

  return NULL;
}

/**
 * ligma_plug_in_extension_enable:
 * @plug_in: A plug-in
 *
 * Enables asynchronous processing of messages from the main LIGMA
 * application.
 *
 * Normally, a plug-in is not called by LIGMA except for the call to
 * the procedure it implements. All subsequent communication is
 * triggered by the plug-in and all messages sent from LIGMA to the
 * plug-in are just answers to requests the plug-in made.
 *
 * If the plug-in however registered temporary procedures using
 * [method@PlugIn.add_temp_procedure], it needs to be able to receive
 * requests to execute them. Usually this will be done by running
 * [method@PlugIn.extension_process] in an endless loop.
 *
 * If the plug-in cannot use [method@PlugIn.extension_process], i.e. if
 * it has a GUI and is hanging around in a [struct@GLib.MainLoop], it
 * must call [method@PlugIn.extension_enable].
 *
 * Note that the plug-in does not need to be a
 * [const@PDBProcType.EXTENSION] to register temporary procedures.
 *
 * See also: [method@PlugIn.add_temp_procedure].
 *
 * Since: 3.0
 **/
void
ligma_plug_in_extension_enable (LigmaPlugIn *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  if (! plug_in->priv->extension_source_id)
    {
      plug_in->priv->extension_source_id =
        g_io_add_watch (plug_in->priv->read_channel, G_IO_IN | G_IO_PRI,
                        ligma_plug_in_extension_read,
                        plug_in);
    }
}

/**
 * ligma_plug_in_extension_process:
 * @plug_in: A plug-in.
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by LIGMA and returns.
 *
 * Call this function in an endless loop after calling
 * ligma_procedure_extension_ready() to process requests for running
 * temporary procedures.
 *
 * See [method@PlugIn.extension_enable] for an asynchronous way of
 * doing the same if running an endless loop is not an option.
 *
 * See also: [method@PlugIn.add_temp_procedure].
 *
 * Since: 3.0
 **/
void
ligma_plug_in_extension_process (LigmaPlugIn *plug_in,
                                guint       timeout)
{
#ifndef G_OS_WIN32

  gint select_val;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

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
      FD_SET (g_io_channel_unix_get_fd (plug_in->priv->read_channel),
              &readfds);

      if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
        {
          ligma_plug_in_single_message (plug_in);
        }
      else if (select_val == -1 && errno != EINTR)
        {
          perror ("ligma_plug_in_extension_process");
          ligma_quit ();
        }
    }
  while (select_val == -1 && errno == EINTR);

#else

  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD pollfd;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (plug_in->priv->read_channel, G_IO_IN,
                                  &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    {
      ligma_plug_in_single_message (plug_in);
    }

#endif
}

/**
 * ligma_plug_in_set_pdb_error_handler:
 * @plug_in: A plug-in
 * @handler: Who is responsible for handling procedure call errors.
 *
 * Sets an error handler for procedure calls.
 *
 * This procedure changes the way that errors in procedure calls are
 * handled. By default LIGMA will raise an error dialog if a procedure
 * call made by a plug-in fails. Using this procedure the plug-in can
 * change this behavior. If the error handler is set to
 * %LIGMA_PDB_ERROR_HANDLER_PLUGIN, then the plug-in is responsible for
 * calling ligma_pdb_get_last_error() and handling the error whenever
 * one if its procedure calls fails. It can do this by displaying the
 * error message or by forwarding it in its own return values.
 *
 * Since: 3.0
 **/
void
ligma_plug_in_set_pdb_error_handler (LigmaPlugIn          *plug_in,
                                    LigmaPDBErrorHandler  handler)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  _ligma_plug_in_set_pdb_error_handler (handler);
}

/**
 * ligma_plug_in_get_pdb_error_handler:
 * @plug_in: A plug-in
 *
 * Retrieves the active error handler for procedure calls.
 *
 * This procedure retrieves the currently active error handler for
 * procedure calls made by the calling plug-in. See
 * ligma_plugin_set_pdb_error_handler() for details.
 *
 * Returns: Who is responsible for handling procedure call errors.
 *
 * Since: 3.0
 **/
LigmaPDBErrorHandler
ligma_plug_in_get_pdb_error_handler (LigmaPlugIn *plug_in)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in),
                        LIGMA_PDB_ERROR_HANDLER_INTERNAL);

  return _ligma_plug_in_get_pdb_error_handler ();
}


/*  internal functions  */

void
_ligma_plug_in_query (LigmaPlugIn *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  if (LIGMA_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    gp_has_init_write (plug_in->priv->write_channel, plug_in);

  if (LIGMA_PLUG_IN_GET_CLASS (plug_in)->query_procedures)
    {
      GList *procedures =
        LIGMA_PLUG_IN_GET_CLASS (plug_in)->query_procedures (plug_in);

      ligma_plug_in_register (plug_in, procedures);
    }
}

void
_ligma_plug_in_init (LigmaPlugIn *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  if (LIGMA_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    {
      GList *procedures =
        LIGMA_PLUG_IN_GET_CLASS (plug_in)->init_procedures (plug_in);

      ligma_plug_in_register (plug_in, procedures);
    }
}

void
_ligma_plug_in_run (LigmaPlugIn *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  g_io_add_watch (plug_in->priv->read_channel,
                  G_IO_ERR | G_IO_HUP,
                  ligma_plug_in_io_error_handler,
                  NULL);

  ligma_plug_in_loop (plug_in);
}

void
_ligma_plug_in_quit (LigmaPlugIn *plug_in)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  if (LIGMA_PLUG_IN_GET_CLASS (plug_in)->quit)
    LIGMA_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);

  _ligma_shm_close ();

  gp_quit_write (plug_in->priv->write_channel, plug_in);
}

GIOChannel *
_ligma_plug_in_get_read_channel (LigmaPlugIn *plug_in)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->read_channel;
}

GIOChannel *
_ligma_plug_in_get_write_channel (LigmaPlugIn *plug_in)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->write_channel;
}

void
_ligma_plug_in_read_expect_msg (LigmaPlugIn      *plug_in,
                               LigmaWireMessage *msg,
                               gint             type)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  while (TRUE)
    {
      if (! ligma_wire_read_msg (plug_in->priv->read_channel, msg, NULL))
        ligma_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          ligma_plug_in_process_message (plug_in, msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      ligma_wire_destroy (msg);
    }
}

gboolean
_ligma_plug_in_set_i18n (LigmaPlugIn   *plug_in,
                        const gchar  *procedure_name,
                        gchar       **gettext_domain,
                        gchar       **catalog_dir)
{
  gboolean use_gettext;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (gettext_domain && *gettext_domain == NULL, FALSE);
  g_return_val_if_fail (catalog_dir && *catalog_dir == NULL, FALSE);

  if (! plug_in->priv->translation_domain_path ||
      ! plug_in->priv->translation_domain_name)
    ligma_plug_in_init_i18n (plug_in);

  if (! LIGMA_PLUG_IN_GET_CLASS (plug_in)->set_i18n)
    {
      use_gettext = FALSE;
    }
  else
    {
      use_gettext = LIGMA_PLUG_IN_GET_CLASS (plug_in)->set_i18n (plug_in,
                                                                procedure_name,
                                                                gettext_domain,
                                                                catalog_dir);
      if (use_gettext)
        {
          gboolean reserved = FALSE;

          if (! (*gettext_domain))
            {
              *gettext_domain = g_strdup (plug_in->priv->translation_domain_name);
            }
          else if (g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-std-plug-ins") == 0 ||
                   g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-script-fu") == 0 ||
                   g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-python") == 0)
            {
              /* We special-case these 3 domains as the only ones where
               * it is allowed to set an absolute system dir (actually
               * set by the lib itself; devs must set NULL). See docs of
               * set_i18n() method.
               */
              if (*catalog_dir)
                {
                  g_printerr ("[%s] Do not set a catalog directory with set_i18n() with reserved domain: %s\n",
                              procedure_name, *gettext_domain);
                  g_clear_pointer (catalog_dir, g_free);
                }

              *catalog_dir = g_strdup (ligma_locale_directory ());
              reserved = TRUE;
            }

          if (*catalog_dir && ! reserved)
            {
              if (g_path_is_absolute (*catalog_dir))
                {
                  g_printerr ("[%s] The catalog directory set by set_i18n() is not relative: %s\n",
                              procedure_name, *catalog_dir);
                  g_printerr ("[%s] Localization disabled\n", procedure_name);

                  use_gettext = FALSE;
                }
              else
                {
                  gchar *rootdir   = g_path_get_dirname (ligma_get_progname ());
                  GFile *root_file = g_file_new_for_path (rootdir);
                  GFile *catalog_file;
                  GFile *parent;

                  catalog_file = g_file_resolve_relative_path (root_file, *catalog_dir);

                  /* Verify that the catalog is a subdir of the plug-in folder.
                   * We do not want to allow plug-ins to look outside their own
                   * realm.
                   */
                  parent = g_file_dup (catalog_file);
                  do
                    {
                      if (g_file_equal (parent, root_file))
                        break;
                      g_clear_object (&parent);
                    }
                  while ((parent = g_file_get_parent (parent)));

                  if (parent == NULL)
                    {
                      g_printerr ("[%s] The catalog directory set by set_i18n() is not a subdirectory: %s\n",
                                  procedure_name, *catalog_dir);
                      g_printerr ("[%s] Localization disabled\n", procedure_name);

                      use_gettext = FALSE;
                    }

                  g_free (rootdir);
                  g_object_unref (root_file);
                  g_clear_object (&parent);
                  g_object_unref (catalog_file);
                }
            }
          else if (! *catalog_dir)
            {
              *catalog_dir = g_file_get_path (plug_in->priv->translation_domain_path);
            }
        }
    }

  if (use_gettext && ! g_file_test (*catalog_dir, G_FILE_TEST_IS_DIR))
    {
      g_printerr ("[%s] The catalog directory does not exist: %s\n",
                  procedure_name, *catalog_dir);
      g_printerr ("[%s] Override method set_i18n() for the plug-in to customize or disable localization.\n",
                  procedure_name);
      g_printerr ("[%s] Localization disabled\n", procedure_name);

      use_gettext = FALSE;
    }

  if (! use_gettext)
    {
      g_clear_pointer (gettext_domain, g_free);
      g_clear_pointer (catalog_dir, g_free);
    }

  return use_gettext;
}

LigmaProcedure *
_ligma_plug_in_create_procedure (LigmaPlugIn  *plug_in,
                                const gchar *procedure_name)
{
  gchar *gettext_domain = NULL;
  gchar *catalog_dir    = NULL;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);

  if (_ligma_plug_in_set_i18n (plug_in, procedure_name, &gettext_domain, &catalog_dir))
    {
      bindtextdomain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  if (LIGMA_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    return LIGMA_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                               procedure_name);

  return NULL;
}


/*  private functions  */

static void
ligma_plug_in_register (LigmaPlugIn *plug_in,
                       GList      *procedures)
{
  GList *list;

  for (list = procedures; list; list = g_list_next (list))
    {
      const gchar   *name = list->data;
      LigmaProcedure *procedure;

      procedure = _ligma_plug_in_create_procedure (plug_in, name);
      if (procedure)
        {
          LIGMA_PROCEDURE_GET_CLASS (procedure)->install (procedure);
          g_object_unref (procedure);
        }
      else
        {
          g_warning ("Plug-in failed to create procedure '%s'\n", name);
        }
    }

  g_list_free_full (procedures, g_free);

  if (plug_in->priv->help_domain_name)
    {
      _ligma_plug_in_help_register (plug_in->priv->help_domain_name,
                                   plug_in->priv->help_domain_uri);
    }

  for (list = plug_in->priv->menu_branches; list; list = g_list_next (list))
    {
      LigmaPlugInMenuBranch *branch = list->data;

      _ligma_plug_in_menu_branch_register (branch->menu_path,
                                          branch->menu_label);
    }
}

static gboolean
ligma_plug_in_write (GIOChannel   *channel,
                    const guint8 *buf,
                    gulong        count,
                    gpointer      user_data)
{
  LigmaPlugIn *plug_in = user_data;

  while (count > 0)
    {
      gulong bytes;

      if ((plug_in->priv->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - plug_in->priv->write_buffer_index;
          memcpy (&plug_in->priv->write_buffer[plug_in->priv->write_buffer_index],
                  buf, bytes);
          plug_in->priv->write_buffer_index += bytes;

          if (! ligma_wire_flush (channel, plug_in))
            return FALSE;
        }
      else
        {
          bytes = count;
          memcpy (&plug_in->priv->write_buffer[plug_in->priv->write_buffer_index],
                  buf, bytes);
          plug_in->priv->write_buffer_index += bytes;
        }

      buf   += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
ligma_plug_in_flush (GIOChannel *channel,
                    gpointer    user_data)
{
  LigmaPlugIn *plug_in = user_data;

  if (plug_in->priv->write_buffer_index > 0)
    {
      gsize count = 0;

      while (count != plug_in->priv->write_buffer_index)
        {
          GIOStatus status;
          gsize     bytes;
          GError   *error = NULL;

          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 &plug_in->priv->write_buffer[count],
                                                 (plug_in->priv->write_buffer_index - count),
                                                 &bytes,
                                                 &error);
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (error)
                {
                  g_warning ("%s: ligma_flush(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: ligma_flush(): error", g_get_prgname ());
                }

              return FALSE;
            }

          count += bytes;
        }

      plug_in->priv->write_buffer_index = 0;
    }

  return TRUE;
}

static gboolean
ligma_plug_in_io_error_handler (GIOChannel   *channel,
                               GIOCondition  cond,
                               gpointer      data)
{
  g_printerr ("%s: fatal error: LIGMA crashed\n", ligma_get_progname ());
  ligma_quit ();

  /* never reached */
  return TRUE;
}

static void
ligma_plug_in_loop (LigmaPlugIn *plug_in)
{
  while (TRUE)
    {
      LigmaWireMessage msg;

      if (! ligma_wire_read_msg (plug_in->priv->read_channel, &msg, NULL))
        return;

      switch (msg.type)
        {
        case GP_QUIT:
          ligma_wire_destroy (&msg);
          return;

        case GP_CONFIG:
          _ligma_config (msg.data);
          break;

        case GP_TILE_REQ:
        case GP_TILE_ACK:
        case GP_TILE_DATA:
          g_warning ("unexpected tile message received (should not happen)");
          break;

        case GP_PROC_RUN:
          ligma_plug_in_proc_run (plug_in, msg.data);
          ligma_wire_destroy (&msg);
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

      ligma_wire_destroy (&msg);
    }
}

static void
ligma_plug_in_single_message (LigmaPlugIn *plug_in)
{
  LigmaWireMessage msg;

  /* Run a temp function */
  if (! ligma_wire_read_msg (plug_in->priv->read_channel, &msg, NULL))
    ligma_quit ();

  ligma_plug_in_process_message (plug_in, &msg);

  ligma_wire_destroy (&msg);
}

static void
ligma_plug_in_process_message (LigmaPlugIn      *plug_in,
                              LigmaWireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      ligma_quit ();
      break;
    case GP_CONFIG:
      _ligma_config (msg->data);
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
      ligma_plug_in_temp_proc_run (plug_in, msg->data);
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
ligma_plug_in_proc_run (LigmaPlugIn *plug_in,
                       GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  LigmaProcedure *procedure;

  procedure = _ligma_plug_in_create_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      ligma_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
      g_object_unref (procedure);
    }

  if (! gp_proc_return_write (plug_in->priv->write_channel,
                              &proc_return, plug_in))
    ligma_quit ();

  _ligma_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
ligma_plug_in_temp_proc_run (LigmaPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  LigmaProcedure *procedure;

  procedure = ligma_plug_in_get_temp_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      ligma_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
    }

  if (! gp_temp_proc_return_write (plug_in->priv->write_channel,
                                   &proc_return, plug_in))
    ligma_quit ();

  _ligma_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
ligma_plug_in_proc_run_internal (LigmaPlugIn    *plug_in,
                                GPProcRun     *proc_run,
                                LigmaProcedure *procedure,
                                GPProcReturn  *proc_return)
{
  LigmaValueArray *arguments;
  LigmaValueArray *return_values  = NULL;
  gchar          *gettext_domain = NULL;
  gchar          *catalog_dir    = NULL;

  if (_ligma_plug_in_set_i18n (plug_in, ligma_procedure_get_name (procedure),
                              &gettext_domain, &catalog_dir))
    {
      bindtextdomain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  ligma_plug_in_push_procedure (plug_in, procedure);

  arguments = _ligma_gp_params_to_value_array (NULL,
                                              NULL, 0,
                                              proc_run->params,
                                              proc_run->n_params,
                                              FALSE);

  return_values = ligma_procedure_run (procedure, arguments);

  ligma_value_array_unref (arguments);

  proc_return->name     = proc_run->name;
  proc_return->n_params = ligma_value_array_length (return_values);
  proc_return->params   = _ligma_value_array_to_gp_params (return_values, TRUE);

  ligma_value_array_unref (return_values);

  ligma_plug_in_pop_procedure (plug_in, procedure);
}

static gboolean
ligma_plug_in_extension_read (GIOChannel  *channel,
                             GIOCondition condition,
                             gpointer     data)
{
  LigmaPlugIn *plug_in = data;

  ligma_plug_in_single_message (plug_in);

  return G_SOURCE_CONTINUE;
}


/*  procedure stack / display-, image-, item-cache  */

LigmaProcedure *
_ligma_plug_in_get_procedure (LigmaPlugIn *plug_in)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (plug_in->priv->procedure_stack != NULL, NULL);

  return plug_in->priv->procedure_stack->data;
}

static void
ligma_plug_in_push_procedure (LigmaPlugIn    *plug_in,
                             LigmaProcedure *procedure)
{
  plug_in->priv->procedure_stack =
    g_list_prepend (plug_in->priv->procedure_stack, procedure);
}

static void
ligma_plug_in_pop_procedure (LigmaPlugIn    *plug_in,
                            LigmaProcedure *procedure)
{
  plug_in->priv->procedure_stack =
    g_list_remove (plug_in->priv->procedure_stack, procedure);

  _ligma_procedure_destroy_proxies (procedure);

  ligma_plug_in_destroy_proxies (plug_in->priv->displays, FALSE);
  ligma_plug_in_destroy_proxies (plug_in->priv->images,   FALSE);
  ligma_plug_in_destroy_proxies (plug_in->priv->items,    FALSE);

  if (! plug_in->priv->procedure_stack)
    {
      ligma_plug_in_destroy_proxies (plug_in->priv->displays, TRUE);
      ligma_plug_in_destroy_proxies (plug_in->priv->images,   TRUE);
      ligma_plug_in_destroy_proxies (plug_in->priv->items,    TRUE);

      ligma_plug_in_destroy_hashes (plug_in);
    }
}

LigmaDisplay *
_ligma_plug_in_get_display (LigmaPlugIn *plug_in,
                           gint32      display_id)
{
  LigmaDisplay *display = NULL;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  if (G_UNLIKELY (! plug_in->priv->displays))
    plug_in->priv->displays =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  display = g_hash_table_lookup (plug_in->priv->displays,
                                 GINT_TO_POINTER (display_id));

  if (! display)
    {
      display = g_object_new (LIGMA_TYPE_DISPLAY,
                              "id", display_id,
                              NULL);

      g_hash_table_insert (plug_in->priv->displays,
                           GINT_TO_POINTER (display_id),
                           g_object_ref (display) /* add debug ref */);
    }

  return display;
}

LigmaImage *
_ligma_plug_in_get_image (LigmaPlugIn *plug_in,
                         gint32      image_id)
{
  LigmaImage *image = NULL;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  if (G_UNLIKELY (! plug_in->priv->images))
    plug_in->priv->images =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  image = g_hash_table_lookup (plug_in->priv->images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    {
      image = g_object_new (LIGMA_TYPE_IMAGE,
                            "id", image_id,
                            NULL);

      g_hash_table_insert (plug_in->priv->images,
                           GINT_TO_POINTER (image_id),
                           g_object_ref (image) /* add debug ref */);
    }

  return image;
}

LigmaItem *
_ligma_plug_in_get_item (LigmaPlugIn *plug_in,
                        gint32      item_id)
{
  LigmaItem *item = NULL;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  if (G_UNLIKELY (! plug_in->priv->items))
    plug_in->priv->items =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  item = g_hash_table_lookup (plug_in->priv->items,
                              GINT_TO_POINTER (item_id));

  if (! item)
    {
      if (ligma_item_id_is_text_layer (item_id))
        {
          item = g_object_new (LIGMA_TYPE_TEXT_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (ligma_item_id_is_layer (item_id))
        {
          item = g_object_new (LIGMA_TYPE_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (ligma_item_id_is_layer_mask (item_id))
        {
          item = g_object_new (LIGMA_TYPE_LAYER_MASK,
                               "id", item_id,
                               NULL);
        }
      else if (ligma_item_id_is_selection (item_id))
        {
          item = g_object_new (LIGMA_TYPE_SELECTION,
                               "id", item_id,
                               NULL);
        }
      else if (ligma_item_id_is_channel (item_id))
        {
          item = g_object_new (LIGMA_TYPE_CHANNEL,
                               "id", item_id,
                               NULL);
        }
      else if (ligma_item_id_is_vectors (item_id))
        {
          item = g_object_new (LIGMA_TYPE_VECTORS,
                               "id", item_id,
                               NULL);
        }

      if (item)
        g_hash_table_insert (plug_in->priv->items,
                             GINT_TO_POINTER (item_id),
                             g_object_ref (item) /* add debug ref */);
    }

  return item;
}

static void
ligma_plug_in_destroy_hashes (LigmaPlugIn *plug_in)
{
  g_clear_pointer (&plug_in->priv->displays, g_hash_table_unref);
  g_clear_pointer (&plug_in->priv->images,   g_hash_table_unref);
  g_clear_pointer (&plug_in->priv->items,    g_hash_table_unref);
}

static void
ligma_plug_in_destroy_proxies (GHashTable *hash_table,
                              gboolean    destroy_all)
{
  GHashTableIter iter;
  gpointer       key, value;

  if (! hash_table)
    return;

  g_hash_table_iter_init (&iter, hash_table);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GObject *object = value;

      if (object->ref_count == 2)
        {
          /* this is the normal case for an unused proxy */

          g_object_unref (object);
          g_hash_table_iter_remove (&iter);
        }
      else if (object->ref_count == 1)
        {
          /* this is debug code, a plug-in MUST NOT unref a proxy, we
           * only catch one unref here, more then one will crash right
           * in this function
           */

          gint id;

          g_object_get (object, "id", &id, NULL);

          g_printerr ("%s: ERROR: %s proxy with ID %d was unrefed "
                      "by plug-in, it MUST NOT do that!\n",
                      G_STRFUNC, G_OBJECT_TYPE_NAME (object), id);

          g_hash_table_iter_remove (&iter);
        }
      else if (destroy_all)
        {
          /* this is debug code, a plug-in MUST NOT ref a proxy */

          gint id;

          g_object_get (object, "id", &id, NULL);

          g_printerr ("%s: ERROR: %s proxy with ID %d was refed "
                      "by plug-in, it MUST NOT do that!\n",
                      G_STRFUNC, G_OBJECT_TYPE_NAME (object), id);

#if 0
          /* the code used to do this, but it appears that some bindings
           * keep references around until later, let's keep this for
           * reference and as a reminder to figure if we can do anything
           * generic about it that works for all bindings.
           */
          while (object->ref_count > 1)
            g_object_unref (object);
#else
          g_object_unref (object);
#endif
          g_hash_table_iter_remove (&iter);
        }
    }
}

static void
ligma_plug_in_init_i18n (LigmaPlugIn *plug_in)
{
  gchar *rootdir      = g_path_get_dirname (ligma_get_progname ());
  GFile *root_file    = g_file_new_for_path (rootdir);
  GFile *catalog_file = NULL;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  /* Default domain name is the program directory name. */
  g_free (plug_in->priv->translation_domain_name);
  plug_in->priv->translation_domain_name = g_path_get_basename (rootdir);

  /* Default catalog path is the locale/ directory under the root
   * directory.
   */
  catalog_file = g_file_resolve_relative_path (root_file, "locale");
  g_set_object (&plug_in->priv->translation_domain_path, catalog_file);

  g_free (rootdir);
  g_object_unref (root_file);
  g_object_unref (catalog_file);
}
