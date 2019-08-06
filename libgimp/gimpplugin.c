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
#include "gimpplugin-private.h"
#include "gimpprocedure-private.h"


/**
 * SECTION: gimpplugin
 * @title: GimpPlugIn
 * @short_description: The base class for plug-ins to derive from
 *
 * The base class for plug-ins to derive from. Manages the plug-in's
 * #GimpProcedure objects.
 *
 * Since: 3.0
 **/


enum
{
  PROP_0,
  PROP_READ_CHANNEL,
  PROP_WRITE_CHANNEL,
  N_PROPS
};


static void   gimp_plug_in_constructed   (GObject      *object);
static void   gimp_plug_in_finalize      (GObject      *object);
static void   gimp_plug_in_set_property  (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   gimp_plug_in_get_property  (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPlugIn, gimp_plug_in, G_TYPE_OBJECT)

#define parent_class gimp_plug_in_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_plug_in_class_init (GimpPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_plug_in_constructed;
  object_class->finalize     = gimp_plug_in_finalize;
  object_class->set_property = gimp_plug_in_set_property;
  object_class->get_property = gimp_plug_in_get_property;

  props[PROP_READ_CHANNEL] =
    g_param_spec_pointer ("read-channel",
                          "Read channel",
                          "The GIOChanel to read from GIMP",
                          GIMP_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY);

  props[PROP_WRITE_CHANNEL] =
    g_param_spec_pointer ("write-channel",
                          "Write channel",
                          "The GIOChanel to write to GIMP",
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
}

static void
gimp_plug_in_finalize (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);
  GList      *list;

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
      plug_in->priv->read_channel = g_value_get_pointer (value);
      break;

    case PROP_WRITE_CHANNEL:
      plug_in->priv->write_channel = g_value_get_pointer (value);
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
      g_value_set_pointer (value, plug_in->priv->read_channel);
      break;

    case PROP_WRITE_CHANNEL:
      g_value_set_pointer (value, plug_in->priv->write_channel);
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
 * @domain_path: The absolute path to the compiled message catalog
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
 * location. This function can only be called in the
 * GimpPlugIn::query() function of a plug-in and it has to be called
 * before any procedure is installed.
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
  g_return_if_fail (G_IS_FILE (domain_path));

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
 * Add a new sub-menu to thr GIMP menus.
 *
 * This function installs a sub-menu which does not belong to any
 * procedure at the location @menu_path.
 *
 * For translations of tooltips to work properly, @menu_label should
 * only be marked for translation but passed to this function
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
 * gimp_plug_in_create_procedure:
 * @plug_in: A #GimpPlugIn
 * @name:    A procedure name.
 *
 * This functiond creates a new procedure and is called when a plug-in
 * instance is started by GIMP when one of the %GIMP_PLUGIN or
 * %GIMP_EXTENSION procedures it implements is invoked.
 *
 * This function will only ever be called with names returned by
 * implementations of GimpPlugInClass::init_procedures() or
 * GimpPlugInClass::query_procedures().
 *
 * Returns: (transfer full): The newly created #GimpProcedure.
 **/
GimpProcedure *
gimp_plug_in_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    return GIMP_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                               name);

  return NULL;
}

/**
 * gimp_plug_in_add_temp_procedure:
 * @plug_in:   A #GimpPlugIn
 * @procedure: A #GimpProcedure of type %GIMP_TEMPORARY.
 *
 * This function adds a temporary procedure to @plug_in. It is usually
 * called from a %GIMP_EXTENSION procedure's GimpProcedure::run().
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * The procedure's type <emphasis>must</emphasis> be %GIMP_TEMPORARY
 * or the function will fail.
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
                    GIMP_TEMPORARY);

  plug_in->priv->temp_procedures = g_list_prepend (plug_in->priv->temp_procedures,
                                                   g_object_ref (procedure));

  _gimp_procedure_register (procedure);
}

/**
 * gimp_plug_in_remove_temp_procedure:
 * @plug_in: A #GimpPlugIn
 * @name:    The name of a #GimpProcedure added to @plug_in.
 *
 * This function removes a temporary procedure from @plug_in by the
 * procedure's @name.
 *
 * Since: 3.0
 **/
void
gimp_plug_in_remove_temp_procedure (GimpPlugIn  *plug_in,
                                    const gchar *name)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (name != NULL);

  procedure = gimp_plug_in_get_temp_procedure (plug_in, name);

  if (procedure)
    {
      _gimp_procedure_unregister (procedure);

      plug_in->priv->temp_procedures = g_list_remove (plug_in->priv->temp_procedures,
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
 * @plug_in: A #GimpPlugIn
 * @name:    The name of a #GimpProcedure added to @plug_in.
 *
 * This function retrieves a temporary procedure from @plug_in by the
 * procedure's @name.
 *
 * Returns: (nullable) (transfer none): The procedure if registered, or %NULL.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_plug_in_get_temp_procedure (GimpPlugIn  *plug_in,
                                 const gchar *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (list = plug_in->priv->temp_procedures; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      if (! strcmp (name, gimp_procedure_get_name (procedure)))
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
 * Note that the plug-in does not need to be a #GIMP_EXTENSION to
 * register temporary procedures.
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
                        _gimp_plug_in_extension_read,
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
 * gimp_plug_in_extension_ready() to process requests for running
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
          _gimp_plug_in_single_message (plug_in);
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
      _gimp_plug_in_single_message (plug_in);
    }

#endif
}
