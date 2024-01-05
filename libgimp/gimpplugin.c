/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-private.h"
#include "gimp-shm.h"
#include "gimpgpparams.h"
#include "gimpplugin-private.h"
#include "gimpplugin_pdb.h"
#include "gimpprocedure-private.h"


/**
 * GimpPlugIn:
 *
 * The base class for plug-ins to derive from.
 *
 * GimpPlugIn manages the plug-in's [class@Procedure] objects. The procedures a
 * plug-in implements are registered with GIMP by returning a list of their
 * names from either [vfunc@GimpPlugIn.query_procedures] or
 * [vfunc@GimpPlugIn.init_procedures].
 *
 * Every GIMP plug-in has to be implemented as a subclass and make it known to
 * the libgimp infrastructure and the main GIMP application by passing its
 * `GType` to [func@MAIN].
 *
 * [func@MAIN] passes the 'argc' and 'argv' of the platform's main() function,
 * along with the `GType`, to [func@main], which creates an instance of the
 * plug-in's `GimpPlugIn` subclass and calls its virtual functions, depending
 * on how the plug-in was called by GIMP.
 *
 * There are 3 different ways GIMP calls a plug-in: "query", "init" and "run".
 *
 * The plug-in is called in "query" mode once after it was installed, or when
 * the cached plug-in information in the config file "pluginrc" needs to be
 * recreated. In "query" mode, [vfunc@GimpPlugIn.query_procedures] is called
 * and returns a list of procedure names the plug-in implements. This is the
 * "normal" place to register procedures, because the existence of most
 * procedures doesn't depend on things that change between GIMP sessions.
 *
 * The plug-in is called in "init" mode at each GIMP startup, and
 * [vfunc@PlugIn.init_procedures] is called and returns a list of procedure
 * names this plug-in implements. This only happens if the plug-in actually
 * implements [vfunc@GimpPlugIn.init_procedures]. A plug-in only needs to
 * implement init_procedures if the existence of its procedures can change
 * between GIMP sessions, for example if they depend on the presence of
 * external tools, or hardware like scanners, or online services, or whatever
 * variable circumstances.
 *
 * In order to register the plug-in's procedures with the main GIMP application
 * in the plug-in's "query" and "init" modes, [class@PlugIn] calls
 * [vfunc@PlugIn.create_procedure] on all procedure names in the exact order of
 * the list returned by [vfunc@PlugIn.query_procedures] or
 * [vfunc@PlugIn.init_procedures] and then registers the returned
 * [class@Procedure].
 *
 * The plug-in is called in "run" mode whenever one of the procedures it
 * implements is called by either the main GIMP application or any other
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
 * gimp_plug_in_error_quark:
 *
 * Generic #GQuark error domain for plug-ins. Plug-ins are welcome to
 * create their own domain when they want to handle advanced error
 * handling. Often, you just want to pass an error message to the core.
 * This domain can be used for such simple usage.
 *
 * See #GError for information on error domains.
 */
G_DEFINE_QUARK (gimp-plug-in-error-quark, gimp_plug_in_error)

enum
{
  PROP_0,
  PROP_PROGRAM_NAME,
  PROP_READ_CHANNEL,
  PROP_WRITE_CHANNEL,
  N_PROPS
};


typedef struct _GimpPlugInMenuBranch GimpPlugInMenuBranch;

struct _GimpPlugInMenuBranch
{
  gchar *menu_path;
  gchar *menu_label;
};

typedef struct _GimpPlugInPrivate
{
  gchar      *program_name;

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
  GHashTable *resources;
} GimpPlugInPrivate;


static void       gimp_plug_in_constructed       (GObject         *object);
static void       gimp_plug_in_dispose           (GObject         *object);
static void       gimp_plug_in_finalize          (GObject         *object);
static void       gimp_plug_in_set_property      (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void       gimp_plug_in_get_property      (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);

static gboolean   gimp_plug_in_real_set_i18n     (GimpPlugIn      *plug_in,
                                                  const gchar     *procedure_name,
                                                  gchar          **gettext_domain,
                                                  gchar          **catalog_dir);

static void       gimp_plug_in_register          (GimpPlugIn      *plug_in,
                                                  GList           *procedures);

static gboolean   gimp_plug_in_write             (GIOChannel      *channel,
                                                  const guint8    *buf,
                                                  gulong           count,
                                                  gpointer         user_data);
static gboolean   gimp_plug_in_flush             (GIOChannel      *channel,
                                                  gpointer         user_data);
static gboolean   gimp_plug_in_io_error_handler  (GIOChannel      *channel,
                                                  GIOCondition     cond,
                                                  gpointer         data);

static void       gimp_plug_in_loop              (GimpPlugIn      *plug_in);
static void       gimp_plug_in_single_message    (GimpPlugIn      *plug_in);
static void       gimp_plug_in_process_message   (GimpPlugIn      *plug_in,
                                                  GimpWireMessage *msg);
static void       gimp_plug_in_main_proc_run     (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       gimp_plug_in_temp_proc_run     (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       gimp_plug_in_proc_run          (GPProcRun       *proc_run,
                                                  GimpProcedure   *procedure,
                                                  GPProcReturn    *proc_return);
static void       gimp_plug_in_proc_run_internal (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run,
                                                  GimpProcedure   *procedure,
                                                  GPProcReturn    *proc_return);
static gboolean   gimp_plug_in_extension_read    (GIOChannel      *channel,
                                                  GIOCondition     condition,
                                                  gpointer         data);

static void       gimp_plug_in_push_procedure    (GimpPlugIn      *plug_in,
                                                  GimpProcedure   *procedure);
static void       gimp_plug_in_pop_procedure     (GimpPlugIn      *plug_in,
                                                  GimpProcedure   *procedure);
static gboolean   gimp_plug_in_is_procedure_stack_empty (
                                                  GimpPlugIn      *plug_in);

static void       gimp_plug_in_main_run_cleanup  (GimpPlugIn      *plug_in);
static void       gimp_plug_in_temp_run_cleanup  (GimpPlugIn      *plug_in);
static void       gimp_plug_in_destroy_hashes    (GimpPlugIn      *plug_in);
static void       gimp_plug_in_destroy_all_proxies
                                                 (GimpPlugIn      *plug_in);
static void       gimp_plug_in_destroy_proxies   (GimpPlugIn      *plug_in,
                                                  GHashTable      *hash_table,
                                                  const gchar     *type,
                                                  gboolean         destroy_all);

static void       gimp_plug_in_init_i18n         (GimpPlugIn      *plug_in);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPlugIn, gimp_plug_in, G_TYPE_OBJECT)

#define parent_class gimp_plug_in_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_plug_in_class_init (GimpPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_plug_in_constructed;
  object_class->dispose      = gimp_plug_in_dispose;
  object_class->finalize     = gimp_plug_in_finalize;
  object_class->set_property = gimp_plug_in_set_property;
  object_class->get_property = gimp_plug_in_get_property;

  klass->set_i18n            = gimp_plug_in_real_set_i18n;

  /**
   * GimpPlugIn:program-name:
   *
   * The program name as usually found on argv[0]
   */
  props[PROP_PROGRAM_NAME] =
    g_param_spec_string ("program-name",
                         "The plug-in executable",
                         "The executable name as usually found on argv[0]",
                         NULL,
                         GIMP_PARAM_WRITABLE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpPlugIn:read-channel:
   *
   * The [struct@GLib.IOChannel] to read from GIMP
   */
  props[PROP_READ_CHANNEL] =
    g_param_spec_boxed ("read-channel",
                        "Read channel",
                        "The GIOChanel to read from GIMP",
                        G_TYPE_IO_CHANNEL,
                        GIMP_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpPlugIn:write-channel:
   *
   * The [struct@GLib.IOChannel] to write to GIMP
   */
  props[PROP_WRITE_CHANNEL] =
    g_param_spec_boxed ("write-channel",
                        "Write channel",
                        "The GIOChanel to write to GIMP",
                        G_TYPE_IO_CHANNEL,
                        GIMP_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_plug_in_init (GimpPlugIn *plug_in)
{
}

static void
gimp_plug_in_constructed (GObject *object)
{
  GimpPlugIn        *plug_in = GIMP_PLUG_IN (object);
  GimpPlugInPrivate *priv;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv = gimp_plug_in_get_instance_private (plug_in);

  g_assert (priv->read_channel != NULL);
  g_assert (priv->write_channel != NULL);

  gp_init ();

  gimp_wire_set_writer (gimp_plug_in_write);
  gimp_wire_set_flusher (gimp_plug_in_flush);
}

static void
gimp_plug_in_dispose (GObject *object)
{
  GimpPlugIn        *plug_in = GIMP_PLUG_IN (object);
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (priv->extension_source_id)
    {
      g_source_remove (priv->extension_source_id);
      priv->extension_source_id = 0;
    }

  if (priv->temp_procedures)
    {
      g_list_free_full (priv->temp_procedures, g_object_unref);
      priv->temp_procedures = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_plug_in_finalize (GObject *object)
{
  GimpPlugIn        *plug_in = GIMP_PLUG_IN (object);
  GimpPlugInPrivate *priv;
  GList             *list;

  priv = gimp_plug_in_get_instance_private (plug_in);

  g_clear_pointer (&priv->program_name, g_free);
  g_clear_pointer (&priv->translation_domain_name, g_free);
  g_clear_object  (&priv->translation_domain_path);

  g_clear_pointer (&priv->help_domain_name, g_free);
  g_clear_object  (&priv->help_domain_uri);

  for (list = priv->menu_branches; list; list = g_list_next (list))
    {
      GimpPlugInMenuBranch *branch = list->data;

      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_slice_free (GimpPlugInMenuBranch, branch);
    }

  g_clear_pointer (&priv->menu_branches, g_list_free);

  gimp_plug_in_destroy_proxies (plug_in, priv->displays,  "display",  TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->images,    "image",    TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->items,     "item",     TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->resources, "resource", TRUE);

  gimp_plug_in_destroy_hashes (plug_in);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_plug_in_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpPlugIn        *plug_in = GIMP_PLUG_IN (object);
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  switch (property_id)
    {
    case PROP_PROGRAM_NAME:
      priv->program_name = g_value_dup_string (value);
      break;

    case PROP_READ_CHANNEL:
      priv->read_channel = g_value_get_boxed (value);
      break;

    case PROP_WRITE_CHANNEL:
      priv->write_channel = g_value_get_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_plug_in_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpPlugIn        *plug_in = GIMP_PLUG_IN (object);
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  switch (property_id)
    {
    case PROP_READ_CHANNEL:
      g_value_set_boxed (value, priv->read_channel);
      break;

    case PROP_WRITE_CHANNEL:
      g_value_set_boxed (value, priv->write_channel);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_plug_in_real_set_i18n (GimpPlugIn   *plug_in,
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
 * gimp_plug_in_set_help_domain:
 * @plug_in:     A #GimpPlugIn.
 * @domain_name: The XML namespace of the plug-in's help pages.
 * @domain_uri:  The root URI of the plug-in's help pages.
 *
 * Set a help domain and path for the @plug_in.
 *
 * This function registers user documentation for the calling plug-in
 * with the GIMP help system. The @domain_uri parameter points to the
 * root directory where the plug-in help is installed. For each
 * supported language there should be a file called 'gimp-help.xml'
 * that maps the help IDs to the actual help files.
 *
 * This function can only be called in the
 * [vfunc@PlugIn.query_procedures] function of a plug-in.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_set_help_domain (GimpPlugIn  *plug_in,
                              const gchar *domain_name,
                              GFile       *domain_uri)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (G_IS_FILE (domain_uri));

  priv = gimp_plug_in_get_instance_private (plug_in);

  g_free (priv->help_domain_name);
  priv->help_domain_name = g_strdup (domain_name);

  g_set_object (&priv->help_domain_uri, domain_uri);
}

/**
 * gimp_plug_in_add_menu_branch:
 * @plug_in:    A #GimpPlugIn
 * @menu_path:  The sub-menu's menu path.
 * @menu_label: The menu label of the sub-menu.
 *
 * Add a new sub-menu to the GIMP menus.
 *
 * This function installs a sub-menu which does not belong to any
 * procedure at the location @menu_path.
 *
 * For translations of @menu_label to work properly, @menu_label
 * should only be marked for translation but passed to this function
 * untranslated, for example using N_("Submenu"). GIMP will look up
 * the translation in the textdomain registered for the plug-in.
 *
 * See also: gimp_procedure_add_menu_path().
 *
 * Since: 3.0
 **/
void
gimp_plug_in_add_menu_branch (GimpPlugIn  *plug_in,
                              const gchar *menu_path,
                              const gchar *menu_label)
{
  GimpPlugInPrivate    *priv;
  GimpPlugInMenuBranch *branch;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  branch = g_slice_new (GimpPlugInMenuBranch);

  branch->menu_path  = g_strdup (menu_path);
  branch->menu_label = g_strdup (menu_label);

  priv->menu_branches = g_list_append (priv->menu_branches, branch);
}

/**
 * gimp_plug_in_add_temp_procedure:
 * @plug_in:   A #GimpPlugIn
 * @procedure: A #GimpProcedure of type %GIMP_PDB_PROC_TYPE_TEMPORARY.
 *
 * This function adds a temporary procedure to @plug_in. It is usually
 * called from a %GIMP_PDB_PROC_TYPE_EXTENSION procedure's
 * [vfunc@Procedure.run].
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * The procedure's type _must_ be
 * %GIMP_PDB_PROC_TYPE_TEMPORARY or the function will fail.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the GIMP core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either [method@PlugIn.extension_enable] or
 * [method@PlugIn.extension_process]. See their respective documentation
 * for details.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_add_temp_procedure (GimpPlugIn    *plug_in,
                                 GimpProcedure *procedure)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (gimp_procedure_get_proc_type (procedure) ==
                    GIMP_PDB_PROC_TYPE_TEMPORARY);

  priv = gimp_plug_in_get_instance_private (plug_in);

  priv->temp_procedures =
    g_list_prepend (priv->temp_procedures,
                    g_object_ref (procedure));

  GIMP_PROCEDURE_GET_CLASS (procedure)->install (procedure);
}

/**
 * gimp_plug_in_remove_temp_procedure:
 * @plug_in:        A #GimpPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function removes a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_remove_temp_procedure (GimpPlugIn  *plug_in,
                                    const gchar *procedure_name)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (gimp_is_canonical_identifier (procedure_name));

  procedure = gimp_plug_in_get_temp_procedure (plug_in, procedure_name);

  if (procedure)
    {
      GimpPlugInPrivate *priv;

      GIMP_PROCEDURE_GET_CLASS (procedure)->uninstall (procedure);

      priv = gimp_plug_in_get_instance_private (plug_in);

      priv->temp_procedures =
        g_list_remove (priv->temp_procedures,
                       procedure);
      g_object_unref (procedure);
    }
}

/**
 * gimp_plug_in_get_temp_procedures:
 * @plug_in: A plug-in
 *
 * This function retrieves the list of temporary procedure of @plug_in as
 * added with [method@PlugIn.add_temp_procedure].
 *
 * Returns: (transfer none) (element-type GimpProcedure): The list of
 *          procedures.
 *
 * Since: 3.0
 **/
GList *
gimp_plug_in_get_temp_procedures (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  return priv->temp_procedures;
}

/**
 * gimp_plug_in_get_temp_procedure:
 * @plug_in:        A #GimpPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function retrieves a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Returns: (nullable) (transfer none): The procedure if registered, or %NULL.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_plug_in_get_temp_procedure (GimpPlugIn  *plug_in,
                                 const gchar *procedure_name)
{
  GimpPlugInPrivate *priv;
  GList             *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  for (list = priv->temp_procedures; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      if (! strcmp (procedure_name, gimp_procedure_get_name (procedure)))
        return procedure;
    }

  return NULL;
}

/**
 * gimp_plug_in_extension_enable:
 * @plug_in: A plug-in
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
 * [method@PlugIn.add_temp_procedure], it needs to be able to receive
 * requests to execute them. Usually this will be done by running
 * [method@PlugIn.extension_process] in an endless loop.
 *
 * If the plug-in cannot use [method@PlugIn.extension_process], i.e. if
 * it has a GUI and is hanging around in a [struct@GLib.MainLoop], it
 * must call [method@PlugIn.extension_enable].
 *
 * Note that the plug-in does not need to be a
 * [enum@Gimp.PDBProcType.EXTENSION] to register temporary procedures.
 *
 * See also: [method@PlugIn.add_temp_procedure].
 *
 * Since: 3.0
 **/
void
gimp_plug_in_extension_enable (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (! priv->extension_source_id)
    {
      priv->extension_source_id =
        g_io_add_watch (priv->read_channel, G_IO_IN | G_IO_PRI,
                        gimp_plug_in_extension_read,
                        plug_in);
    }
}

/**
 * gimp_plug_in_extension_process:
 * @plug_in: A plug-in.
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by GIMP and returns.
 *
 * Call this function in an endless loop after calling
 * gimp_procedure_extension_ready() to process requests for running
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
gimp_plug_in_extension_process (GimpPlugIn *plug_in,
                                guint       timeout)
{
  GimpPlugInPrivate *priv;
#ifndef G_OS_WIN32
  gint               select_val;
#else
  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD            pollfd;
#endif

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

#ifndef G_OS_WIN32
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
      FD_SET (g_io_channel_unix_get_fd (priv->read_channel),
              &readfds);

      if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
        {
          gimp_plug_in_single_message (plug_in);
        }
      else if (select_val == -1 && errno != EINTR)
        {
          perror ("gimp_plug_in_extension_process");
          gimp_quit ();
        }
    }
  while (select_val == -1 && errno == EINTR);

#else
  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (priv->read_channel, G_IO_IN,
                                  &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    {
      gimp_plug_in_single_message (plug_in);
    }

#endif
}

/**
 * gimp_plug_in_set_pdb_error_handler:
 * @plug_in: A plug-in
 * @handler: Who is responsible for handling procedure call errors.
 *
 * Sets an error handler for procedure calls.
 *
 * This procedure changes the way that errors in procedure calls are
 * handled. By default GIMP will raise an error dialog if a procedure
 * call made by a plug-in fails. Using this procedure the plug-in can
 * change this behavior. If the error handler is set to
 * %GIMP_PDB_ERROR_HANDLER_PLUGIN, then the plug-in is responsible for
 * calling gimp_pdb_get_last_error() and handling the error whenever
 * one if its procedure calls fails. It can do this by displaying the
 * error message or by forwarding it in its own return values.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_set_pdb_error_handler (GimpPlugIn          *plug_in,
                                    GimpPDBErrorHandler  handler)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  _gimp_plug_in_set_pdb_error_handler (handler);
}

/**
 * gimp_plug_in_get_pdb_error_handler:
 * @plug_in: A plug-in
 *
 * Retrieves the active error handler for procedure calls.
 *
 * This procedure retrieves the currently active error handler for
 * procedure calls made by the calling plug-in. See
 * gimp_plugin_set_pdb_error_handler() for details.
 *
 * Returns: Who is responsible for handling procedure call errors.
 *
 * Since: 3.0
 **/
GimpPDBErrorHandler
gimp_plug_in_get_pdb_error_handler (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in),
                        GIMP_PDB_ERROR_HANDLER_INTERNAL);

  return _gimp_plug_in_get_pdb_error_handler ();
}


/*  internal functions  */

void
_gimp_plug_in_query (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    gp_has_init_write (priv->write_channel, plug_in);

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures)
    {
      GList *procedures =
        GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures (plug_in);

      gimp_plug_in_register (plug_in, procedures);
    }
}

void
_gimp_plug_in_init (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    {
      GList *procedures =
        GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures (plug_in);

      gimp_plug_in_register (plug_in, procedures);
    }
}

void
_gimp_plug_in_run (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  g_io_add_watch (priv->read_channel,
                  G_IO_ERR | G_IO_HUP,
                  gimp_plug_in_io_error_handler,
                  NULL);

  gimp_plug_in_loop (plug_in);
}

void
_gimp_plug_in_quit (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->quit)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);

  _gimp_shm_close ();

  gp_quit_write (priv->write_channel, plug_in);
}

GIOChannel *
_gimp_plug_in_get_read_channel (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  return priv->read_channel;
}

GIOChannel *
_gimp_plug_in_get_write_channel (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  return priv->write_channel;
}

void
_gimp_plug_in_read_expect_msg (GimpPlugIn      *plug_in,
                               GimpWireMessage *msg,
                               gint             type)
{
  GimpPlugInPrivate *priv;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  while (TRUE)
    {
      if (! gimp_wire_read_msg (priv->read_channel, msg, NULL))
        gimp_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          gimp_plug_in_process_message (plug_in, msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      gimp_wire_destroy (msg);
    }
}

gboolean
_gimp_plug_in_set_i18n (GimpPlugIn   *plug_in,
                        const gchar  *procedure_name,
                        gchar       **gettext_domain,
                        gchar       **catalog_dir)
{
  GimpPlugInPrivate *priv;
  gboolean           use_gettext;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (gettext_domain && *gettext_domain == NULL, FALSE);
  g_return_val_if_fail (catalog_dir && *catalog_dir == NULL, FALSE);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (! priv->translation_domain_path ||
      ! priv->translation_domain_name)
    gimp_plug_in_init_i18n (plug_in);

  if (! GIMP_PLUG_IN_GET_CLASS (plug_in)->set_i18n)
    {
      use_gettext = FALSE;
    }
  else
    {
      gchar *utf8_catalog_dir = NULL;

      use_gettext = GIMP_PLUG_IN_GET_CLASS (plug_in)->set_i18n (plug_in,
                                                                procedure_name,
                                                                gettext_domain,
                                                                &utf8_catalog_dir);
      if (use_gettext)
        {
          gboolean reserved = FALSE;

          if (*gettext_domain == NULL)
            {
              *gettext_domain = g_strdup (priv->translation_domain_name);
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
              if (utf8_catalog_dir != NULL)
                g_printerr ("[%s] Do not set a catalog directory with set_i18n() with reserved domain: %s\n",
                            procedure_name, *gettext_domain);

              *catalog_dir = g_strdup (gimp_locale_directory ());
              reserved = TRUE;
            }

          if (utf8_catalog_dir != NULL && *catalog_dir == NULL)
            {
              GError *error = NULL;

              /* The passed-on catalog directory is in UTF-8 because this is
               * usually hardcoded (in which case it's easier to request a
               * specific encoding, chosen at development time, rather than the
               * "OS encoding", depending on runtime).
               * But now we want to transform it to the encoding used for
               * filenames by GLib.
               */
              *catalog_dir = g_filename_from_utf8 (utf8_catalog_dir, -1, NULL, NULL, &error);

              if (*catalog_dir == NULL)
                g_printerr ("[%s] provided catalog directory is not proper UTF-8: %s\n",
                            procedure_name, error ? error->message : "(N/A)");

              g_clear_error (&error);
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
                  gchar *rootdir   = g_path_get_dirname (gimp_get_progname ());
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
              *catalog_dir = g_file_get_path (priv->translation_domain_path);
            }
        }

      g_clear_pointer (&utf8_catalog_dir, g_free);
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

GimpProcedure *
_gimp_plug_in_create_procedure (GimpPlugIn  *plug_in,
                                const gchar *procedure_name)
{
  gchar *gettext_domain = NULL;
  gchar *catalog_dir    = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  if (_gimp_plug_in_set_i18n (plug_in, procedure_name, &gettext_domain, &catalog_dir))
    {
      gimp_bind_text_domain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    return GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                               procedure_name);

  return NULL;
}


/*  private functions  */

static void
gimp_plug_in_register (GimpPlugIn *plug_in,
                       GList      *procedures)
{
  GimpPlugInPrivate *priv;
  GList             *list;

  for (list = procedures; list; list = g_list_next (list))
    {
      const gchar   *name = list->data;
      GimpProcedure *procedure;

      procedure = _gimp_plug_in_create_procedure (plug_in, name);
      if (procedure)
        {
          GIMP_PROCEDURE_GET_CLASS (procedure)->install (procedure);
          g_object_unref (procedure);
        }
      else
        {
          g_warning ("Plug-in failed to create procedure '%s'\n", name);
        }
    }

  g_list_free_full (procedures, g_free);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (priv->help_domain_name)
    {
      _gimp_plug_in_help_register (priv->help_domain_name,
                                   priv->help_domain_uri);
    }

  for (list = priv->menu_branches; list; list = g_list_next (list))
    {
      GimpPlugInMenuBranch *branch = list->data;

      _gimp_plug_in_menu_branch_register (branch->menu_path,
                                          branch->menu_label);
    }
}

static gboolean
gimp_plug_in_write (GIOChannel   *channel,
                    const guint8 *buf,
                    gulong        count,
                    gpointer      user_data)
{
  GimpPlugIn        *plug_in = user_data;
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  while (count > 0)
    {
      gulong bytes;

      if ((priv->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - priv->write_buffer_index;
          memcpy (&priv->write_buffer[priv->write_buffer_index],
                  buf, bytes);
          priv->write_buffer_index += bytes;

          if (! gimp_wire_flush (channel, plug_in))
            return FALSE;
        }
      else
        {
          bytes = count;
          memcpy (&priv->write_buffer[priv->write_buffer_index],
                  buf, bytes);
          priv->write_buffer_index += bytes;
        }

      buf   += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
gimp_plug_in_flush (GIOChannel *channel,
                    gpointer    user_data)
{
  GimpPlugIn        *plug_in = user_data;
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (priv->write_buffer_index > 0)
    {
      gsize count = 0;

      while (count != priv->write_buffer_index)
        {
          GIOStatus status;
          gsize     bytes;
          GError   *error = NULL;

          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 &priv->write_buffer[count],
                                                 (priv->write_buffer_index - count),
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

      priv->write_buffer_index = 0;
    }

  return TRUE;
}

static gboolean
gimp_plug_in_io_error_handler (GIOChannel   *channel,
                               GIOCondition  cond,
                               gpointer      data)
{
  g_printerr ("%s: fatal error: GIMP crashed\n", gimp_get_progname ());
  gimp_quit ();

  /* never reached */
  return TRUE;
}

static void
gimp_plug_in_loop (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  priv = gimp_plug_in_get_instance_private (plug_in);

  while (TRUE)
    {
      GimpWireMessage msg;

      if (! gimp_wire_read_msg (priv->read_channel, &msg, NULL))
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
          gimp_plug_in_main_proc_run (plug_in, msg.data);
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
gimp_plug_in_single_message (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;
  GimpWireMessage    msg;

  priv = gimp_plug_in_get_instance_private (plug_in);

  /* Run a temp function */
  if (! gimp_wire_read_msg (priv->read_channel, &msg, NULL))
    gimp_quit ();

  gimp_plug_in_process_message (plug_in, &msg);

  gimp_wire_destroy (&msg);
}

static void
gimp_plug_in_process_message (GimpPlugIn      *plug_in,
                              GimpWireMessage *msg)
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
      gimp_plug_in_temp_proc_run (plug_in, msg->data);
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

/* Run a proc that is main, i.e. root of a plugin call stack. */
static void
gimp_plug_in_main_proc_run (GimpPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  GimpPlugInPrivate *priv;
  GPProcReturn       proc_return;
  GimpProcedure     *procedure;

  procedure = _gimp_plug_in_create_procedure (plug_in, proc_run->name);
  priv      = gimp_plug_in_get_instance_private (plug_in);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
      g_object_unref (procedure);
    }

  gimp_plug_in_main_run_cleanup (plug_in);

  if (! gp_proc_return_write (priv->write_channel,
                              &proc_return, plug_in))
    gimp_quit ();

  _gimp_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
gimp_plug_in_temp_proc_run (GimpPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  GimpPlugInPrivate *priv;
  GPProcReturn       proc_return;
  GimpProcedure     *procedure;

  procedure = gimp_plug_in_get_temp_procedure (plug_in, proc_run->name);
  priv      = gimp_plug_in_get_instance_private (plug_in);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
    }

  gimp_plug_in_temp_run_cleanup (plug_in);

  if (! gp_temp_proc_return_write (priv->write_channel,
                                   &proc_return, plug_in))
    gimp_quit ();

  _gimp_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

/* Run the proc, passing args from proc_run
 * and returning values in proc_return.
 * This does not alter the state of the GimpPlugin.
 */
static void
gimp_plug_in_proc_run (GPProcRun     *proc_run,
                       GimpProcedure *procedure,
                       GPProcReturn  *proc_return)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values  = NULL;

  arguments = _gimp_gp_params_to_value_array (NULL,
                                              NULL, 0,
                                              proc_run->params,
                                              proc_run->n_params,
                                              FALSE);

  return_values = _gimp_procedure_run_array (procedure, arguments);

  gimp_value_array_unref (arguments);

  proc_return->name     = proc_run->name;
  proc_return->n_params = gimp_value_array_length (return_values);
  proc_return->params   = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);
}


/* Setup translation, maintain proc stack, and run proc.
 * Proc is a main or temp proc.
 * "internal" means private, not that the proc is type INTERNAL.
 */
static void
gimp_plug_in_proc_run_internal (GimpPlugIn    *plug_in,
                                GPProcRun     *proc_run,
                                GimpProcedure *procedure,
                                GPProcReturn  *proc_return)
{
  gchar *gettext_domain = NULL;
  gchar *catalog_dir    = NULL;

  if (_gimp_plug_in_set_i18n (plug_in, gimp_procedure_get_name (procedure),
                              &gettext_domain, &catalog_dir))
    {
      gimp_bind_text_domain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  gimp_plug_in_push_procedure (plug_in, procedure);

  gimp_plug_in_proc_run (proc_run, procedure, proc_return);

  gimp_plug_in_pop_procedure (plug_in, procedure);
}

static gboolean
gimp_plug_in_extension_read (GIOChannel  *channel,
                             GIOCondition condition,
                             gpointer     data)
{
  GimpPlugIn *plug_in = data;

  gimp_plug_in_single_message (plug_in);

  return G_SOURCE_CONTINUE;
}


/*  procedure stack / display-, image-, item-cache  */


GimpProcedure *
_gimp_plug_in_get_procedure (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  g_return_val_if_fail (priv->procedure_stack != NULL, NULL);

  return priv->procedure_stack->data;
}

static gboolean
gimp_plug_in_is_procedure_stack_empty (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  return (priv->procedure_stack == NULL);
}

static void
gimp_plug_in_push_procedure (GimpPlugIn    *plug_in,
                             GimpProcedure *procedure)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  priv->procedure_stack =
    g_list_prepend (priv->procedure_stack, procedure);
}

/* After a run of a main proc, cleanup.
 * We are about to return to GIMP or another calling plugin process.
 * This plugin process will soon terminate.
 *
 * Expect the proc stack is empty: don't destroy proxies
 * when there are still calling procs that might have a reference.
 */
static void
gimp_plug_in_main_run_cleanup (GimpPlugIn *plug_in)
{
  if (gimp_plug_in_is_procedure_stack_empty (plug_in))
    {
      g_debug ("%s proc stack empty, destroy proxies.", G_STRFUNC);
      gimp_plug_in_destroy_all_proxies (plug_in);
      gimp_plug_in_destroy_hashes (plug_in);
    }
  else
    {
      /* Unexpected. */
      g_warning ("%s proc stack not empty when main proc returns.", G_STRFUNC);
    }
}

/* After a run of a temp proc, cleanup.
 * We are about to return to another calling proc.
 *
 * When the just-run temp proc is returning to top proc on stack and it is an extension,
 * cleanup is destroy the plugin's proxies.
 * The proc stack is never empty for an extension: the top is e.g. extension-script-fu,
 * which must not reference proxies.
 */
static void
gimp_plug_in_temp_run_cleanup (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  /* When at top of proc stack and top is an extension, destroy proxies. */
  if ((g_list_length (priv->procedure_stack) == 1) &&
      (gimp_procedure_get_proc_type (priv->procedure_stack->data) == GIMP_PDB_PROC_TYPE_EXTENSION))
    {
      g_debug ("%s top of proc stack is extension, destroy proxies.", G_STRFUNC);
      gimp_plug_in_destroy_all_proxies (plug_in);
      gimp_plug_in_destroy_hashes (plug_in);
    }
  else
    {
      /* Normal.  The temp proc just run was called by a calling proc
       * which is not a main proc of an extension plugin.
       * We can't destroy proxies because the calling proc may retain a reference.
       */
      g_debug ("%s Not destroy proxies for temp proc.", G_STRFUNC);
    }
}

static void
gimp_plug_in_pop_procedure (GimpPlugIn    *plug_in,
                            GimpProcedure *procedure)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  priv->procedure_stack = g_list_remove (priv->procedure_stack, procedure);

  /* Make procedure object unref it's proxy references.
   * Our hashes still have references to proxies.
   * Calling procs may also have retained references to proxies.
   */
  _gimp_procedure_destroy_proxies (procedure);

  /* Don't destroy proxies now because any proc, especially temporary procs,
   * may have passed a reference to a proc higher in the stack e.g. the main procedure.
   * We don't have separate proxy hashes for each pushed procedure,
   * only a hash table for the run.
   */
}

GimpDisplay *
_gimp_plug_in_get_display (GimpPlugIn *plug_in,
                           gint32      display_id)
{
  GimpPlugInPrivate *priv;

  GimpDisplay *display = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->displays))
    priv->displays =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  display = g_hash_table_lookup (priv->displays,
                                 GINT_TO_POINTER (display_id));

  if (! display)
    {
      display = g_object_new (GIMP_TYPE_DISPLAY,
                              "id", display_id,
                              NULL);

      g_hash_table_insert (priv->displays,
                           GINT_TO_POINTER (display_id),
                           display);
    }

  return display;
}

GimpImage *
_gimp_plug_in_get_image (GimpPlugIn *plug_in,
                         gint32      image_id)
{
  GimpPlugInPrivate *priv;
  GimpImage         *image = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->images))
    priv->images =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  image = g_hash_table_lookup (priv->images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    {
      image = g_object_new (GIMP_TYPE_IMAGE,
                            "id", image_id,
                            NULL);

      g_hash_table_insert (priv->images,
                           GINT_TO_POINTER (image_id),
                           image);
    }

  return image;
}

GimpItem *
_gimp_plug_in_get_item (GimpPlugIn *plug_in,
                        gint32      item_id)
{
  GimpPlugInPrivate *priv;
  GimpItem          *item = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->items))
    priv->items =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  item = g_hash_table_lookup (priv->items,
                              GINT_TO_POINTER (item_id));

  if (! item)
    {
      if (gimp_item_id_is_text_layer (item_id))
        {
          item = g_object_new (GIMP_TYPE_TEXT_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_group_layer (item_id))
        {
          item = g_object_new (GIMP_TYPE_GROUP_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_layer (item_id))
        {
          item = g_object_new (GIMP_TYPE_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_layer_mask (item_id))
        {
          item = g_object_new (GIMP_TYPE_LAYER_MASK,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_selection (item_id))
        {
          item = g_object_new (GIMP_TYPE_SELECTION,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_channel (item_id))
        {
          item = g_object_new (GIMP_TYPE_CHANNEL,
                               "id", item_id,
                               NULL);
        }
      else if (gimp_item_id_is_path (item_id))
        {
          item = g_object_new (GIMP_TYPE_PATH,
                               "id", item_id,
                               NULL);
        }

      if (item)
        g_hash_table_insert (priv->items,
                             GINT_TO_POINTER (item_id),
                             item);
    }

  return item;
}

GimpResource *
_gimp_plug_in_get_resource (GimpPlugIn *plug_in,
                            gint32      resource_id)
{
  GimpPlugInPrivate *priv;
  GimpResource      *resource = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->resources))
    priv->resources =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  resource = g_hash_table_lookup (priv->resources,
                                  GINT_TO_POINTER (resource_id));

  if (! resource)
    {
      if (gimp_resource_id_is_brush (resource_id))
        {
          resource = g_object_new (GIMP_TYPE_BRUSH,
                                   "id", resource_id,
                                   NULL);
        }
      else if (gimp_resource_id_is_pattern (resource_id))
        {
          resource = g_object_new (GIMP_TYPE_PATTERN,
                                   "id", resource_id,
                                   NULL);
        }
      else if (gimp_resource_id_is_gradient (resource_id))
        {
          resource = g_object_new (GIMP_TYPE_GRADIENT,
                                   "id", resource_id,
                                   NULL);
        }
      else if (gimp_resource_id_is_palette (resource_id))
        {
          resource = g_object_new (GIMP_TYPE_PALETTE,
                                   "id", resource_id,
                                   NULL);
        }
      else if (gimp_resource_id_is_font (resource_id))
        {
          resource = g_object_new (GIMP_TYPE_FONT,
                                   "id", resource_id,
                                   NULL);
        }

      if (resource)
        g_hash_table_insert (priv->resources,
                             GINT_TO_POINTER (resource_id),
                             resource);
    }

  return resource;
}

gboolean
_gimp_plug_in_manage_memory_manually (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;
  gboolean           manual_management = TRUE;

  priv = gimp_plug_in_get_instance_private (plug_in);

  if (priv->program_name)
    {
      GFile *file = g_file_new_for_path (priv->program_name);

      /* Limitations:
       * 1. Checking a file extension (and trusting argv[0] in general) is not
       *    the best idea. I thought about the possibility of passing through
       *    the name of the used interpreter from core to plug-in, but even this
       *    should not be trusted because it is configurable.
       * 2. An alternative could be to pass the information if a plug-in is
       *    interpreted (i.e. it has an interp file). Though I'm unsure if there
       *    might be some interpreted language out there with manual memory
       *    management.
       * 3. This list only contains the bindings we actually tested with (and
       *    have demo plug-ins for). Much more bindings are possible.
       *
       * So in the end, this test is just worth so far, but it's
       * acceptable for the officially supported bindings at least.
       */
      if (gimp_file_has_extension (file, ".lua") ||
          gimp_file_has_extension (file, ".py")  ||
          gimp_file_has_extension (file, ".js")  ||
          gimp_file_has_extension (file, ".gjs"))
        manual_management = FALSE;

      g_object_unref (file);
    }

  return manual_management;
}

static void
gimp_plug_in_destroy_hashes (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  g_clear_pointer (&priv->displays,  g_hash_table_unref);
  g_clear_pointer (&priv->images,    g_hash_table_unref);
  g_clear_pointer (&priv->items,     g_hash_table_unref);
  g_clear_pointer (&priv->resources, g_hash_table_unref);
}

/* Destroy proxies of all kinds.
 * This destroys with prejudice, i.e. destroy_all==TRUE.
 * All procedures, main and temporary, of the plugin must not be run subsequently,
 * especially to reference a proxy we are destroying.
 */
static void
gimp_plug_in_destroy_all_proxies (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv = gimp_plug_in_get_instance_private (plug_in);

  gimp_plug_in_destroy_proxies (plug_in, priv->displays,  "display",  TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->images,    "image",    TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->items,     "item",     TRUE);
  gimp_plug_in_destroy_proxies (plug_in, priv->resources, "resource", TRUE);
}

static void
gimp_plug_in_destroy_proxies (GimpPlugIn  *plug_in,
                              GHashTable  *hash_table,
                              const gchar *type,
                              gboolean     destroy_all)
{
  GHashTableIter iter;
  gpointer       key, value;

  if (! hash_table)
    return;

  g_hash_table_iter_init (&iter, hash_table);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GObject *object = value;

      if (object->ref_count == 1)
        {
          /* this is the normal case for an unused proxy, since we already
           * destroyed the only other reference in procedure with
           * _gimp_procedure_destroy_proxies().
           */
          g_hash_table_iter_remove (&iter);
        }
      else if (! G_IS_OBJECT (object))
        {
          /* this is debug code, a plug-in MUST NOT unref a proxy. To be nice,
           * we steal the object from the table, as removing it normally would
           * crash, since the object is not valid anymore.
           */
          g_printerr ("%s: ERROR: %s proxy was unrefed "
                      "by plug-in, it MUST NOT do that!\n",
                      G_STRFUNC, type);

          g_hash_table_iter_steal (&iter);
        }
      else if (destroy_all)
        {
          /* this is debug code, a plug-in MUST NOT ref a proxy */
          gint id;

          g_object_get (object, "id", &id, NULL);

          /* Some bindings are always keeping a reference to objects, in
           * particular this is the behavior of lua and gjs bindings where
           * object->ref_count == 2 at this point.
           * As for the Python binding, it is only true for global
           * variables (scoped variables are properly freed before the
           * plug-in ends), which anyway is not the best practice.
           * In any case, it's not something which the plug-in developer can do
           * anything about so this error is only confusing.
           * See #3912.
           */
          if (_gimp_plug_in_manage_memory_manually (plug_in))
            g_printerr ("%s: ERROR: %s proxy with ID %d was refed "
                        "by plug-in, it MUST NOT do that!\n",
                        G_STRFUNC, G_OBJECT_TYPE_NAME (object), id);

#if 0
          /* The code used to do this, which is only meaningful when the bug is
           * really in the plug-in code. If it's one of the cases where the
           * reference is actually held by a bindings, it's unwise and will
           * likely end up in double-free crashes.
           */
          while (object->ref_count > 1)
            g_object_unref (object);
#endif
          g_hash_table_iter_remove (&iter);
        }
    }
}

static void
gimp_plug_in_init_i18n (GimpPlugIn *plug_in)
{
  GimpPlugInPrivate *priv;
  gchar             *rootdir      = g_path_get_dirname (gimp_get_progname ());
  GFile             *root_file    = g_file_new_for_path (rootdir);
  GFile             *catalog_file = NULL;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  priv = gimp_plug_in_get_instance_private (plug_in);

  /* Default domain name is the program directory name. */
  g_free (priv->translation_domain_name);
  priv->translation_domain_name = g_path_get_basename (rootdir);

  /* Default catalog path is the locale/ directory under the root
   * directory.
   */
  catalog_file = g_file_resolve_relative_path (root_file, "locale");
  g_set_object (&priv->translation_domain_path, catalog_file);

  g_free (rootdir);
  g_object_unref (root_file);
  g_object_unref (catalog_file);
}
