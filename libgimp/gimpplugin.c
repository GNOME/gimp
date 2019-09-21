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
 * SECTION: gimpplugin
 * @title: GimpPlugIn
 * @short_description: The base class for plug-ins to derive from
 *
 * The base class for plug-ins to derive from. #GimpPlugIn manages the
 * plug-in's #GimpProcedure objects. The procedures a plug-in
 * implements are registered with GIMP by returning a #GList of their
 * names from either #GimpPlugInClass.query_procedures() or
 * #GimpPlugInClass.init_procedures().
 *
 * Every GIMP plug-in has to implement a #GimpPlugIn subclass and make
 * it known to the libgimp infrastructure and the main GIMP
 * application by passing its #GType to GIMP_MAIN().
 *
 * GIMP_MAIN() passes the 'argc' and 'argv' of the platform's main()
 * function, along with the #GType, to gimp_main(), which creates an
 * instance of the plug-in's #GimpPlugIn subclass and calls its
 * virtual functions, depending on how the plug-in was called by GIMP.
 *
 * There are three different ways GIMP calls a plug-in, "query",
 * "init" and "run".
 *
 * The plug-in is called in "query" mode once after it was installed,
 * or when the cached plug-in information in the config file
 * "pluginrc" needs to be recreated. In "query" mode,
 * #GimpPlugInClass.query_procedures() is called and returns a #GList
 * of procedure names the plug-in implements. This is the "normal"
 * place to register procedures, because the existence of most
 * procedures doesn't depend on things that change between GIMP
 * sessions.
 *
 * The plug-in is called in "init" mode at each GIMP startup, and
 * #GimpPlugInClass.init_procedures() is called and returns a #GList
 * of procedure names this plug-in implements. This only happens if
 * the plug-in actually implements
 * #GimpPlugInClass.init_procedures(). A plug-in only needs to
 * implement #GimpPlugInClass.init_procedures() if the existence of
 * its procedures can change between GIMP sessions, for example if
 * they depend on the presence of external tools, or hardware like
 * scanners, or online services, or whatever variable circumstances.
 *
 * In order to register the plug-in's procedures with the main GIMP
 * application in the plug-in's "query" and "init" modes, #GimpPlugIn
 * calls #GimpPlugInClass.create_procedure() on all procedure names in
 * the exact order of the #GList returned by
 * #GimpPlugInClass.query_procedures() or
 * #GimpPlugInClass.init_procedures() and then registers the returned
 * #GimpProcedure using #GimpProcedureClass.register().
 *
 * The plug-in is called in "run" mode whenever one of the procedures
 * it implements is called by either the main GIMP application or any
 * other plug-in. In "run" mode, one of the procedure names returned
 * by #GimpPlugInClass.query_procedures() or
 * #GimpPlugInClass.init_procedures() is passed to
 * #GimpPlugInClass.create_procedure() which must return a
 * #GimpProcedure for the passed name. The procedure is then executed
 * by calling gimp_procedure_run().
 *
 * In any of the three modes, #GimpPlugInClass.quit() is called before
 * the plug-in process exits, so the plug-in can perform whatever
 * cleanup necessary.
 *
 * Since: 3.0
 **/


#define WRITE_BUFFER_SIZE 1024


enum
{
  PROP_0,
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

struct _GimpPlugInPrivate
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
static void       gimp_plug_in_proc_run          (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       gimp_plug_in_temp_proc_run     (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
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
static void       gimp_plug_in_destroy_hashes    (GimpPlugIn      *plug_in);
static void       gimp_plug_in_destroy_proxies   (GHashTable      *hash_table,
                                                  gboolean         destroy_all);


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

  props[PROP_READ_CHANNEL] =
    g_param_spec_boxed ("read-channel",
                        "Read channel",
                        "The GIOChanel to read from GIMP",
                        G_TYPE_IO_CHANNEL,
                        GIMP_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

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
  plug_in->priv = gimp_plug_in_get_instance_private (plug_in);
}

static void
gimp_plug_in_constructed (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (plug_in->priv->read_channel != NULL);
  g_assert (plug_in->priv->write_channel != NULL);

  gp_init ();

  gimp_wire_set_writer (gimp_plug_in_write);
  gimp_wire_set_flusher (gimp_plug_in_flush);
}

static void
gimp_plug_in_dispose (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

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
gimp_plug_in_finalize (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);
  GList      *list;

  g_clear_pointer (&plug_in->priv->translation_domain_name, g_free);
  g_clear_object  (&plug_in->priv->translation_domain_path);

  g_clear_pointer (&plug_in->priv->help_domain_name, g_free);
  g_clear_object  (&plug_in->priv->help_domain_uri);

  for (list = plug_in->priv->menu_branches; list; list = g_list_next (list))
    {
      GimpPlugInMenuBranch *branch = list->data;

      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_slice_free (GimpPlugInMenuBranch, branch);
    }

  g_clear_pointer (&plug_in->priv->menu_branches, g_list_free);

  gimp_plug_in_destroy_proxies (plug_in->priv->displays, TRUE);
  gimp_plug_in_destroy_proxies (plug_in->priv->images,   TRUE);
  gimp_plug_in_destroy_proxies (plug_in->priv->items,    TRUE);

  gimp_plug_in_destroy_hashes (plug_in);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_plug_in_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

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
gimp_plug_in_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

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


/*  public functions  */

/**
 * gimp_plug_in_set_translation_domain:
 * @plug_in:     A #GimpPlugIn.
 * @domain_name: The name of the textdomain (must be unique).
 * @domain_path: (nullable): A #GFile pointing to the compiled message catalog
 *               (may be %NULL).
 *
 * Sets a textdomain for localisation for the @plug_in.
 *
 * This function adds a textdomain to the list of domains Gimp
 * searches for strings when translating its menu entries. There is no
 * need to call this function for plug-ins that have their strings
 * included in the 'gimp-std-plugins' domain as that is used by
 * default. If the compiled message catalog is not in the standard
 * location, you may specify an absolute path to another
 * location.
 *
 * This function can only be called in the
 * #GimpPlugInClass.query_procedures() function of a plug-in.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_set_translation_domain (GimpPlugIn  *plug_in,
                                     const gchar *domain_name,
                                     GFile       *domain_path)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (domain_path == NULL || G_IS_FILE (domain_path));

  g_free (plug_in->priv->translation_domain_name);
  plug_in->priv->translation_domain_name = g_strdup (domain_name);

  g_set_object (&plug_in->priv->translation_domain_path, domain_path);
}

/**
 * gimp_plug_in_set_help_domain:
 * @plug_in:     A #GimpPlugIn.
 * @domain_name: The XML namespace of the plug-in's help pages.
 * @domain_uri:  The root URI of the plug-in's help pages.
 *
 * Set a help domain and path for the @plug_in.
 *
 * This function registers user documentation for the calling plug-in
 * with the GIMP help system. The domain_uri parameter points to the
 * root directory where the plug-in help is installed. For each
 * supported language there should be a file called 'gimp-help.xml'
 * that maps the help IDs to the actual help files.
 *
 * This function can only be called in the
 * #GimpPlugInClass.query_procedures() function of a plug-in.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_set_help_domain (GimpPlugIn  *plug_in,
                              const gchar *domain_name,
                              GFile       *domain_uri)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (G_IS_FILE (domain_uri));

  g_free (plug_in->priv->help_domain_name);
  plug_in->priv->help_domain_name = g_strdup (domain_name);

  g_set_object (&plug_in->priv->help_domain_uri, domain_uri);
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
  GimpPlugInMenuBranch *branch;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  branch = g_slice_new (GimpPlugInMenuBranch);

  branch->menu_path  = g_strdup (menu_path);
  branch->menu_label = g_strdup (menu_label);

  plug_in->priv->menu_branches = g_list_append (plug_in->priv->menu_branches,
                                                branch);
}

/**
 * gimp_plug_in_add_temp_procedure:
 * @plug_in:   A #GimpPlugIn
 * @procedure: A #GimpProcedure of type %GIMP_PDB_PROC_TYPE_TEMPORARY.
 *
 * This function adds a temporary procedure to @plug_in. It is usually
 * called from a %GIMP_PDB_PROC_TYPE_EXTENSION procedure's
 * #GimpProcedureClass.run().
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * The procedure's type <emphasis>must</emphasis> be
 * %GIMP_PDB_PROC_TYPE_TEMPORARY or the function will fail.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the GIMP core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either gimp_plug_in_extension_enable() or
 * gimp_plug_in_extension_process(). See this functions' documentation
 * for details.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_add_temp_procedure (GimpPlugIn    *plug_in,
                                 GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (gimp_procedure_get_proc_type (procedure) ==
                    GIMP_PDB_PROC_TYPE_TEMPORARY);

  plug_in->priv->temp_procedures =
    g_list_prepend (plug_in->priv->temp_procedures,
                    g_object_ref (procedure));

  GIMP_PROCEDURE_GET_CLASS (procedure)->install (procedure);
}

/**
 * gimp_plug_in_remove_temp_procedure:
 * @plug_in:        A #GimpPlugIn
 * @procedure_name: The name of a #GimpProcedure added to @plug_in.
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
      GIMP_PROCEDURE_GET_CLASS (procedure)->uninstall (procedure);

      plug_in->priv->temp_procedures =
        g_list_remove (plug_in->priv->temp_procedures,
                       procedure);
      g_object_unref (procedure);
    }
}

/**
 * gimp_plug_in_get_temp_procedures:
 * @plug_in: A #GimpPlugIn
 *
 * This function retrieves the list of temporary procedure of @plug_in as
 * added with gimp_plug_in_add_temp_procedure().
 *
 * Returns: (transfer none) (element-type GimpProcedure): The list of
 *          procedures.
 *
 * Since: 3.0
 **/
GList *
gimp_plug_in_get_temp_procedures (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->temp_procedures;
}

/**
 * gimp_plug_in_get_temp_procedure:
 * @plug_in:        A #GimpPlugIn
 * @procedure_name: The name of a #GimpProcedure added to @plug_in.
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
  GList *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  for (list = plug_in->priv->temp_procedures; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      if (! strcmp (procedure_name, gimp_procedure_get_name (procedure)))
        return procedure;
    }

  return NULL;
}

/**
 * gimp_plug_in_extension_enable:
 * @plug_in: A #GimpPlugIn
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
 * gimp_plug_in_add_temp_procedure(), it needs to be able to receive
 * requests to execute them. Usually this will be done by running
 * gimp_plug_in_extension_process() in an endless loop.
 *
 * If the plug-in cannot use gimp_plug_in_extension_process(), i.e. if
 * it has a GUI and is hanging around in a #GMainLoop, it must call
 * gimp_plug_in_extension_enable().
 *
 * Note that the plug-in does not need to be a
 * #GIMP_PDB_PROC_TYPE_EXTENSION to register temporary procedures.
 *
 * See also: gimp_plug_in_add_temp_procedure().
 *
 * Since: 3.0
 **/
void
gimp_plug_in_extension_enable (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (! plug_in->priv->extension_source_id)
    {
      plug_in->priv->extension_source_id =
        g_io_add_watch (plug_in->priv->read_channel, G_IO_IN | G_IO_PRI,
                        gimp_plug_in_extension_read,
                        plug_in);
    }
}

/**
 * gimp_plug_in_extension_process:
 * @plug_in: A #GimpPlugIn.
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by GIMP and returns.
 *
 * Call this function in an endless loop after calling
 * gimp_procedure_extension_ready() to process requests for running
 * temporary procedures.
 *
 * See gimp_plug_in_extension_enable() for an asynchronous way of
 * doing the same if running an endless loop is not an option.
 *
 * See also: gimp_plug_in_add_temp_procedure().
 *
 * Since: 3.0
 **/
void
gimp_plug_in_extension_process (GimpPlugIn *plug_in,
                                guint       timeout)
{
#ifndef G_OS_WIN32

  gint select_val;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

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

  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD pollfd;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (plug_in->priv->read_channel, G_IO_IN,
                                  &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    {
      gimp_plug_in_single_message (plug_in);
    }

#endif
}

/**
 * gimp_plug_in_set_pdb_error_handler:
 * @plug_in: A #GimpPlugIn
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
 * @plug_in: A #GimpPlugIn
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
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    gp_has_init_write (plug_in->priv->write_channel, plug_in);

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
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  g_io_add_watch (plug_in->priv->read_channel,
                  G_IO_ERR | G_IO_HUP,
                  gimp_plug_in_io_error_handler,
                  NULL);

  gimp_plug_in_loop (plug_in);
}

void
_gimp_plug_in_quit (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->quit)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);

  _gimp_shm_close ();

  gp_quit_write (plug_in->priv->write_channel, plug_in);
}

GIOChannel *
_gimp_plug_in_get_read_channel (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->read_channel;
}

GIOChannel *
_gimp_plug_in_get_write_channel (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  return plug_in->priv->write_channel;
}

void
_gimp_plug_in_read_expect_msg (GimpPlugIn      *plug_in,
                               GimpWireMessage *msg,
                               gint             type)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  while (TRUE)
    {
      if (! gimp_wire_read_msg (plug_in->priv->read_channel, msg, NULL))
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

GimpProcedure *
_gimp_plug_in_create_procedure (GimpPlugIn  *plug_in,
                                const gchar *procedure_name)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

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
  GList *list;

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

  if (plug_in->priv->translation_domain_name)
    {
      _gimp_plug_in_domain_register (plug_in->priv->translation_domain_name,
                                     plug_in->priv->translation_domain_path);
    }

  if (plug_in->priv->help_domain_name)
    {
      _gimp_plug_in_help_register (plug_in->priv->help_domain_name,
                                   plug_in->priv->help_domain_uri);
    }

  for (list = plug_in->priv->menu_branches; list; list = g_list_next (list))
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
  GimpPlugIn *plug_in = user_data;

  while (count > 0)
    {
      gulong bytes;

      if ((plug_in->priv->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - plug_in->priv->write_buffer_index;
          memcpy (&plug_in->priv->write_buffer[plug_in->priv->write_buffer_index],
                  buf, bytes);
          plug_in->priv->write_buffer_index += bytes;

          if (! gimp_wire_flush (channel, plug_in))
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
gimp_plug_in_flush (GIOChannel *channel,
                    gpointer    user_data)
{
  GimpPlugIn *plug_in = user_data;

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

      plug_in->priv->write_buffer_index = 0;
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
  while (TRUE)
    {
      GimpWireMessage msg;

      if (! gimp_wire_read_msg (plug_in->priv->read_channel, &msg, NULL))
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
          gimp_plug_in_proc_run (plug_in, msg.data);
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
  GimpWireMessage msg;

  /* Run a temp function */
  if (! gimp_wire_read_msg (plug_in->priv->read_channel, &msg, NULL))
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

static void
gimp_plug_in_proc_run (GimpPlugIn *plug_in,
                       GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  GimpProcedure *procedure;

  procedure = _gimp_plug_in_create_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
      g_object_unref (procedure);
    }

  if (! gp_proc_return_write (plug_in->priv->write_channel,
                              &proc_return, plug_in))
    gimp_quit ();

  _gimp_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
gimp_plug_in_temp_proc_run (GimpPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  GimpProcedure *procedure;

  procedure = gimp_plug_in_get_temp_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (plug_in,
                                      proc_run, procedure,
                                      &proc_return);
    }

  if (! gp_temp_proc_return_write (plug_in->priv->write_channel,
                                   &proc_return, plug_in))
    gimp_quit ();

  _gimp_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
gimp_plug_in_proc_run_internal (GimpPlugIn    *plug_in,
                                GPProcRun     *proc_run,
                                GimpProcedure *procedure,
                                GPProcReturn  *proc_return)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values = NULL;

  gimp_plug_in_push_procedure (plug_in, procedure);

  arguments = _gimp_gp_params_to_value_array (NULL,
                                              NULL, 0,
                                              proc_run->params,
                                              proc_run->n_params,
                                              FALSE);

  return_values = gimp_procedure_run (procedure, arguments);

  gimp_value_array_unref (arguments);

  proc_return->name     = proc_run->name;
  proc_return->n_params = gimp_value_array_length (return_values);
  proc_return->params   = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);

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
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (plug_in->priv->procedure_stack != NULL, NULL);

  return plug_in->priv->procedure_stack->data;
}

static void
gimp_plug_in_push_procedure (GimpPlugIn    *plug_in,
                             GimpProcedure *procedure)
{
  plug_in->priv->procedure_stack =
    g_list_prepend (plug_in->priv->procedure_stack, procedure);
}

static void
gimp_plug_in_pop_procedure (GimpPlugIn    *plug_in,
                            GimpProcedure *procedure)
{
  plug_in->priv->procedure_stack =
    g_list_remove (plug_in->priv->procedure_stack, procedure);

  _gimp_procedure_destroy_proxies (procedure);

  gimp_plug_in_destroy_proxies (plug_in->priv->displays, FALSE);
  gimp_plug_in_destroy_proxies (plug_in->priv->images,   FALSE);
  gimp_plug_in_destroy_proxies (plug_in->priv->items,    FALSE);

  if (! plug_in->priv->procedure_stack)
    {
      gimp_plug_in_destroy_proxies (plug_in->priv->displays, TRUE);
      gimp_plug_in_destroy_proxies (plug_in->priv->images,   TRUE);
      gimp_plug_in_destroy_proxies (plug_in->priv->items,    TRUE);

      gimp_plug_in_destroy_hashes (plug_in);
    }
}

GimpDisplay *
_gimp_plug_in_get_display (GimpPlugIn *plug_in,
                           gint32      display_id)
{
  GimpDisplay *display = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

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
      display = g_object_new (GIMP_TYPE_DISPLAY,
                              "id", display_id,
                              NULL);

      g_hash_table_insert (plug_in->priv->displays,
                           GINT_TO_POINTER (display_id),
                           g_object_ref (display) /* add debug ref */);
    }

  return display;
}

GimpImage *
_gimp_plug_in_get_image (GimpPlugIn *plug_in,
                         gint32      image_id)
{
  GimpImage *image = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

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
      image = g_object_new (GIMP_TYPE_IMAGE,
                            "id", image_id,
                            NULL);

      g_hash_table_insert (plug_in->priv->images,
                           GINT_TO_POINTER (image_id),
                           g_object_ref (image) /* add debug ref */);
    }

  return image;
}

GimpItem *
_gimp_plug_in_get_item (GimpPlugIn *plug_in,
                        gint32      item_id)
{
  GimpItem *item = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

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
      if (gimp_item_id_is_layer (item_id))
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
      else if (gimp_item_id_is_vectors (item_id))
        {
          item = g_object_new (GIMP_TYPE_VECTORS,
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
gimp_plug_in_destroy_hashes (GimpPlugIn *plug_in)
{
  g_clear_pointer (&plug_in->priv->displays, g_hash_table_unref);
  g_clear_pointer (&plug_in->priv->images,   g_hash_table_unref);
  g_clear_pointer (&plug_in->priv->items,    g_hash_table_unref);
}

static void
gimp_plug_in_destroy_proxies (GHashTable *hash_table,
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
