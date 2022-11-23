/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadialogfactory.c
 * Copyright (C) 2001-2008 Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmacursor.h"
#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockbook.h"
#include "ligmadockable.h"
#include "ligmadockcontainer.h"
#include "ligmadockwindow.h"
#include "ligmamenufactory.h"
#include "ligmasessioninfo.h"
#include "ligmawidgets-utils.h"

#include "ligma-log.h"


enum
{
  DOCK_WINDOW_ADDED,
  DOCK_WINDOW_REMOVED,
  LAST_SIGNAL
};


struct _LigmaDialogFactoryPrivate
{
  LigmaContext      *context;
  LigmaMenuFactory  *menu_factory;

  GList            *open_dialogs;
  GList            *session_infos;

  GList            *registered_dialogs;

  LigmaDialogsState  dialog_state;
};


static void        ligma_dialog_factory_dispose              (GObject                *object);
static void        ligma_dialog_factory_finalize             (GObject                *object);
static GtkWidget * ligma_dialog_factory_constructor          (LigmaDialogFactory      *factory,
                                                             LigmaDialogFactoryEntry *entry,
                                                             LigmaContext            *context,
                                                             LigmaUIManager          *ui_manager,
                                                             gint                    view_size);
static void        ligma_dialog_factory_config_notify        (LigmaDialogFactory      *factory,
                                                             GParamSpec             *pspec,
                                                             LigmaGuiConfig          *config);
static void        ligma_dialog_factory_set_widget_data      (GtkWidget              *dialog,
                                                             LigmaDialogFactory      *factory,
                                                             LigmaDialogFactoryEntry *entry);
static void        ligma_dialog_factory_unset_widget_data    (GtkWidget              *dialog);
static gboolean    ligma_dialog_factory_set_user_pos         (GtkWidget              *dialog,
                                                             GdkEventConfigure      *cevent,
                                                             gpointer                data);
static gboolean    ligma_dialog_factory_dialog_configure     (GtkWidget              *dialog,
                                                             GdkEventConfigure      *cevent,
                                                             LigmaDialogFactory      *factory);
static void        ligma_dialog_factory_hide                 (LigmaDialogFactory      *factory);
static void        ligma_dialog_factory_show                 (LigmaDialogFactory      *factory);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDialogFactory, ligma_dialog_factory,
                            LIGMA_TYPE_OBJECT)

#define parent_class ligma_dialog_factory_parent_class

static guint factory_signals[LAST_SIGNAL] = { 0 };


/* Is set by dialogs.c to a dialog factory initialized there.
 *
 * FIXME: The layer above should not do this kind of initialization of
 * layers below.
 */
static LigmaDialogFactory *ligma_toplevel_factory = NULL;


static void
ligma_dialog_factory_class_init (LigmaDialogFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = ligma_dialog_factory_dispose;
  object_class->finalize = ligma_dialog_factory_finalize;

  factory_signals[DOCK_WINDOW_ADDED] =
    g_signal_new ("dock-window-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaDialogFactoryClass, dock_window_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCK_WINDOW);

  factory_signals[DOCK_WINDOW_REMOVED] =
    g_signal_new ("dock-window-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaDialogFactoryClass, dock_window_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCK_WINDOW);
}

static void
ligma_dialog_factory_init (LigmaDialogFactory *factory)
{
  factory->p = ligma_dialog_factory_get_instance_private (factory);
  factory->p->dialog_state = LIGMA_DIALOGS_SHOWN;
}

static void
ligma_dialog_factory_dispose (GObject *object)
{
  LigmaDialogFactory *factory = LIGMA_DIALOG_FACTORY (object);
  GList             *list;

  /*  start iterating from the beginning each time we destroyed a
   *  toplevel because destroying a dock may cause lots of items
   *  to be removed from factory->p->open_dialogs
   */
  while (factory->p->open_dialogs)
    {
      for (list = factory->p->open_dialogs; list; list = g_list_next (list))
        {
          if (gtk_widget_is_toplevel (list->data))
            {
              gtk_widget_destroy (GTK_WIDGET (list->data));
              break;
            }
        }

      /*  the list being non-empty without any toplevel is an error,
       *  so eek and chain up
       */
      if (! list)
        {
          g_warning ("%s: %d stale non-toplevel entries in factory->p->open_dialogs",
                     G_STRFUNC, g_list_length (factory->p->open_dialogs));
          break;
        }
    }

  if (factory->p->open_dialogs)
    {
      g_list_free (factory->p->open_dialogs);
      factory->p->open_dialogs = NULL;
    }

  if (factory->p->session_infos)
    {
      g_list_free_full (factory->p->session_infos,
                        (GDestroyNotify) g_object_unref);
      factory->p->session_infos = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_dialog_factory_finalize (GObject *object)
{
  LigmaDialogFactory *factory = LIGMA_DIALOG_FACTORY (object);
  GList             *list;

  for (list = factory->p->registered_dialogs; list; list = g_list_next (list))
    {
      LigmaDialogFactoryEntry *entry = list->data;

      g_free (entry->identifier);
      g_free (entry->name);
      g_free (entry->blurb);
      g_free (entry->icon_name);
      g_free (entry->help_id);

      g_slice_free (LigmaDialogFactoryEntry, entry);
    }

  if (factory->p->registered_dialogs)
    {
      g_list_free (factory->p->registered_dialogs);
      factory->p->registered_dialogs = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

LigmaDialogFactory *
ligma_dialog_factory_new (const gchar           *name,
                         LigmaContext           *context,
                         LigmaMenuFactory       *menu_factory)
{
  LigmaDialogFactory *factory;
  LigmaGuiConfig     *config;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (! menu_factory || LIGMA_IS_MENU_FACTORY (menu_factory),
                        NULL);

  factory = g_object_new (LIGMA_TYPE_DIALOG_FACTORY, NULL);

  ligma_object_set_name (LIGMA_OBJECT (factory), name);

  config = LIGMA_GUI_CONFIG (context->ligma->config);

  factory->p->context      = context;
  factory->p->menu_factory = menu_factory;
  factory->p->dialog_state = (config->hide_docks ?
                              LIGMA_DIALOGS_HIDDEN_EXPLICITLY :
                              LIGMA_DIALOGS_SHOWN);

  g_signal_connect_object (config, "notify::hide-docks",
                           G_CALLBACK (ligma_dialog_factory_config_notify),
                           factory, G_CONNECT_SWAPPED);

  return factory;
}

void
ligma_dialog_factory_register_entry (LigmaDialogFactory    *factory,
                                    const gchar          *identifier,
                                    const gchar          *name,
                                    const gchar          *blurb,
                                    const gchar          *icon_name,
                                    const gchar          *help_id,
                                    LigmaDialogNewFunc     new_func,
                                    LigmaDialogRestoreFunc restore_func,
                                    gint                  view_size,
                                    gboolean              singleton,
                                    gboolean              session_managed,
                                    gboolean              remember_size,
                                    gboolean              remember_if_open,
                                    gboolean              hideable,
                                    gboolean              image_window,
                                    gboolean              dockable)
{
  LigmaDialogFactoryEntry *entry;

  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);

  entry = g_slice_new0 (LigmaDialogFactoryEntry);

  entry->identifier       = g_strdup (identifier);
  entry->name             = g_strdup (name);
  entry->blurb            = g_strdup (blurb);
  entry->icon_name        = g_strdup (icon_name);
  entry->help_id          = g_strdup (help_id);
  entry->new_func         = new_func;
  entry->restore_func     = restore_func;
  entry->view_size        = view_size;
  entry->singleton        = singleton ? TRUE : FALSE;
  entry->session_managed  = session_managed ? TRUE : FALSE;
  entry->remember_size    = remember_size ? TRUE : FALSE;
  entry->remember_if_open = remember_if_open ? TRUE : FALSE;
  entry->hideable         = hideable ? TRUE : FALSE;
  entry->image_window     = image_window ? TRUE : FALSE;
  entry->dockable         = dockable ? TRUE : FALSE;

  factory->p->registered_dialogs = g_list_prepend (factory->p->registered_dialogs,
                                                   entry);
}

LigmaDialogFactoryEntry *
ligma_dialog_factory_find_entry (LigmaDialogFactory *factory,
                                const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->p->registered_dialogs; list; list = g_list_next (list))
    {
      LigmaDialogFactoryEntry *entry = list->data;

      if (! strcmp (identifier, entry->identifier))
        return entry;
    }

  return NULL;
}

LigmaSessionInfo *
ligma_dialog_factory_find_session_info (LigmaDialogFactory *factory,
                                       const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      LigmaSessionInfo *info = list->data;

      if (ligma_session_info_get_factory_entry (info) &&
          g_str_equal (identifier,
                       ligma_session_info_get_factory_entry (info)->identifier))
        {
          return info;
        }
    }

  return NULL;
}

GtkWidget *
ligma_dialog_factory_find_widget (LigmaDialogFactory *factory,
                                 const gchar       *identifiers)
{
  GtkWidget  *widget = NULL;
  gchar     **ids;
  gint        i;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  ids = g_strsplit (identifiers, "|", 0);

  for (i = 0; ids[i]; i++)
    {
      LigmaSessionInfo *info;

      info = ligma_dialog_factory_find_session_info (factory, ids[i]);

      if (info)
        {
          widget =  ligma_session_info_get_widget (info);

          if (widget)
            break;
        }
    }

  g_strfreev (ids);

  return widget;
}

/**
 * ligma_dialog_factory_dialog_sane:
 * @factory:
 * @widget_factory:
 * @widget_entry:
 * @widget:
 *
 * Makes sure that the @widget with the given @widget_entry that was
 * created by the given @widget_factory belongs to @efactory.
 *
 * Returns: %TRUE if that is the case, %FALSE otherwise.
 **/
static gboolean
ligma_dialog_factory_dialog_sane (LigmaDialogFactory      *factory,
                                 LigmaDialogFactory      *widget_factory,
                                 LigmaDialogFactoryEntry *widget_entry,
                                 GtkWidget              *widget)
{
  if (! widget_factory || ! widget_entry)
    {
      g_warning ("%s: dialog was not created by a LigmaDialogFactory",
                 G_STRFUNC);
      return FALSE;
    }

  if (widget_factory != factory)
    {
      g_warning ("%s: dialog was created by a different LigmaDialogFactory",
                 G_STRFUNC);
      return FALSE;
    }

  return TRUE;
}

/**
 * ligma_dialog_factory_dialog_new_internal:
 * @factory:
 * @monitor:
 * @context:
 * @ui_manager:
 * @parent:
 * @identifier:
 * @view_size:
 * @return_existing:   If %TRUE, (or if the dialog is a singleton),
 *                     don't create a new dialog if it exists, instead
 *                     return the existing one
 * @present:           If %TRUE, the toplevel that contains the dialog (if any)
 *                     will be gtk_window_present():ed
 * @create_containers: If %TRUE, then containers for the
 *                     dialog/dockable will be created as well. If you
 *                     want to manage your own containers, pass %FALSE
 *
 * This is the lowest level dialog factory creation function.
 *
 * Returns: A created or existing #GtkWidget.
 **/
static GtkWidget *
ligma_dialog_factory_dialog_new_internal (LigmaDialogFactory *factory,
                                         GdkMonitor        *monitor,
                                         LigmaContext       *context,
                                         LigmaUIManager     *ui_manager,
                                         GtkWidget         *parent,
                                         const gchar       *identifier,
                                         gint               view_size,
                                         gboolean           return_existing,
                                         gboolean           present,
                                         gboolean           create_containers)
{
  LigmaDialogFactoryEntry *entry    = NULL;
  GtkWidget              *dialog   = NULL;
  GtkWidget              *toplevel = NULL;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  entry = ligma_dialog_factory_find_entry (factory, identifier);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"",
                 G_STRFUNC, identifier);
      return NULL;
    }

  if (! entry->new_func)
    {
      g_warning ("%s: entry for \"%s\" has no constructor",
                 G_STRFUNC, identifier);
      return NULL;
    }

  /*  a singleton dialog is always returned if it already exists  */
  if (return_existing || entry->singleton)
    {
      dialog = ligma_dialog_factory_find_widget (factory, identifier);
    }

  /*  create the dialog if it was not found  */
  if (! dialog)
    {
      GtkWidget *dock        = NULL;
      GtkWidget *dockbook    = NULL;
      GtkWidget *dock_window = NULL;

      /* What follows is special-case code for some entries. At some
       * point we might want to abstract this block of code away.
       */
      if (create_containers)
        {
          if (entry->dockable)
            {
              /*  It doesn't make sense to have a dockable without a dock
               *  so create one. Create a new dock _before_ creating the
               *  dialog. We do this because the new dockable needs to be
               *  created in its dock's context.
               */
              dock = ligma_dock_with_window_new (factory, monitor, FALSE);
              dockbook = ligma_dockbook_new (factory->p->menu_factory);

              ligma_dock_add_book (LIGMA_DOCK (dock),
                                  LIGMA_DOCKBOOK (dockbook),
                                  0);
            }
          else if (strcmp ("ligma-toolbox", entry->identifier) == 0)
            {
              LigmaDockContainer *dock_container;

              dock_window = ligma_dialog_factory_dialog_new (factory,
                                                            monitor,
                                                            NULL /*ui_manager*/,
                                                            parent,
                                                            "ligma-toolbox-window",
                                                            -1 /*view_size*/,
                                                            FALSE /*present*/);

              /* When we get a dock window, we also get a UI
               * manager
               */
              dock_container = LIGMA_DOCK_CONTAINER (dock_window);
              ui_manager     = ligma_dock_container_get_ui_manager (dock_container);
            }
        }

      /*  Create the new dialog in the appropriate context which is
       *  - the passed context if not NULL
       *  - the newly created dock's context if we just created it
       *  - the factory's context, which happens when raising a toplevel
       *    dialog was the original request.
       */
      if (view_size < LIGMA_VIEW_SIZE_TINY)
        view_size = entry->view_size;

      if (context)
        dialog = ligma_dialog_factory_constructor (factory, entry,
                                                  context,
                                                  ui_manager,
                                                  view_size);
      else if (dock)
        dialog = ligma_dialog_factory_constructor (factory, entry,
                                                  ligma_dock_get_context (LIGMA_DOCK (dock)),
                                                  ligma_dock_get_ui_manager (LIGMA_DOCK (dock)),
                                                  view_size);
      else
        dialog = ligma_dialog_factory_constructor (factory, entry,
                                                  factory->p->context,
                                                  ui_manager,
                                                  view_size);

      if (dialog)
        {
          ligma_dialog_factory_set_widget_data (dialog, factory, entry);

          /*  If we created a dock before, the newly created dialog is
           *  supposed to be a LigmaDockable.
           */
          if (dock)
            {
              if (LIGMA_IS_DOCKABLE (dialog))
                {
                  gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                                            dialog, NULL);
                  gtk_widget_show (dock);
                }
              else
                {
                  g_warning ("%s: LigmaDialogFactory is a dockable factory "
                             "but constructor for \"%s\" did not return a "
                             "LigmaDockable",
                             G_STRFUNC, identifier);

                  gtk_widget_destroy (dialog);
                  gtk_widget_destroy (dock);

                  dialog = NULL;
                  dock   = NULL;
                }
            }
          else if (dock_window)
            {
              if (LIGMA_IS_DOCK (dialog))
                {
                  ligma_dock_window_add_dock (LIGMA_DOCK_WINDOW (dock_window),
                                             LIGMA_DOCK (dialog),
                                             -1 /*index*/);

                  gtk_widget_set_visible (dialog, present);
                  gtk_widget_set_visible (dock_window, present);
                }
              else
                {
                  g_warning ("%s: LigmaDialogFactory is a dock factory entry "
                             "but constructor for \"%s\" did not return a "
                             "LigmaDock",
                             G_STRFUNC, identifier);

                  gtk_widget_destroy (dialog);
                  gtk_widget_destroy (dock_window);

                  dialog      = NULL;
                  dock_window = NULL;
                }
            }
        }
      else if (dock)
        {
          g_warning ("%s: constructor for \"%s\" returned NULL",
                     G_STRFUNC, identifier);

          gtk_widget_destroy (dock);

          dock = NULL;
        }

      if (dialog)
        ligma_dialog_factory_add_dialog (factory, dialog, monitor);
    }

  /*  Finally, if we found an existing dialog or created a new one, raise it.
   */
  if (! dialog)
    return NULL;

  if (gtk_widget_is_toplevel (dialog))
    {
      gtk_window_set_screen (GTK_WINDOW (dialog),
                             gdk_display_get_default_screen (gdk_monitor_get_display (monitor)));

      if (parent)
        {
          GtkWidget *parent_toplevel = gtk_widget_get_toplevel (parent);

          if (GTK_IS_WINDOW (parent_toplevel))
            {
              gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                            GTK_WINDOW (parent_toplevel));
            }
        }

      toplevel = dialog;
    }
  else if (LIGMA_IS_DOCK (dialog))
    {
      toplevel = gtk_widget_get_toplevel (dialog);
    }
  else if (LIGMA_IS_DOCKABLE (dialog))
    {
      LigmaDockable *dockable = LIGMA_DOCKABLE (dialog);

      if (ligma_dockable_get_dockbook (dockable) &&
          ligma_dockbook_get_dock (ligma_dockable_get_dockbook (dockable)))
        {
          GtkNotebook *notebook = GTK_NOTEBOOK (ligma_dockable_get_dockbook (dockable));
          gint         num      = gtk_notebook_page_num (notebook, dialog);

          if (num != -1)
            {
              gtk_notebook_set_current_page (notebook, num);

              ligma_widget_blink (dialog);
            }
        }

      toplevel = gtk_widget_get_toplevel (dialog);
    }

  if (present && GTK_IS_WINDOW (toplevel))
    {
      /*  Work around focus-stealing protection, or whatever makes the
       *  dock appear below the one where we clicked a button to open
       *  it. See bug #630173.
       */
      gtk_widget_show_now (toplevel);
      gdk_window_raise (gtk_widget_get_window (toplevel));
    }

  return dialog;
}

/**
 * ligma_dialog_factory_dialog_new:
 * @factory:      a #LigmaDialogFactory
 * @monitor       the #GdkMonitor the dialog should appear on
 * @ui_manager:   A #LigmaUIManager, if applicable.
 * @parent:       The parent widget, from which the call originated, if
 *                applicable, NULL otherwise.
 * @identifier:   the identifier of the dialog as registered with
 *                ligma_dialog_factory_register_entry()
 * @view_size:    the initial preview size
 * @present:      whether gtk_window_present() should be called
 *
 * Creates a new toplevel dialog or a #LigmaDockable, depending on whether
 * %factory is a toplevel of dockable factory.
 *
 * Returns: the newly created dialog or an already existing singleton
 *               dialog.
 **/
GtkWidget *
ligma_dialog_factory_dialog_new (LigmaDialogFactory *factory,
                                GdkMonitor        *monitor,
                                LigmaUIManager     *ui_manager,
                                GtkWidget         *parent,
                                const gchar       *identifier,
                                gint               view_size,
                                gboolean           present)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  return ligma_dialog_factory_dialog_new_internal (factory,
                                                  monitor,
                                                  factory->p->context,
                                                  ui_manager,
                                                  parent,
                                                  identifier,
                                                  view_size,
                                                  FALSE /*return_existing*/,
                                                  present,
                                                  FALSE /*create_containers*/);
}

LigmaContext *
ligma_dialog_factory_get_context (LigmaDialogFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->context;
}

LigmaMenuFactory *
ligma_dialog_factory_get_menu_factory (LigmaDialogFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->menu_factory;
}

GList *
ligma_dialog_factory_get_open_dialogs (LigmaDialogFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->open_dialogs;
}

GList *
ligma_dialog_factory_get_session_infos (LigmaDialogFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->session_infos;
}

void
ligma_dialog_factory_add_session_info (LigmaDialogFactory *factory,
                                      LigmaSessionInfo   *info)
{
  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (LIGMA_IS_SESSION_INFO (info));

  /* We want to append rather than prepend so that the serialized
   * order in sessionrc remains the same
   */
  factory->p->session_infos = g_list_append (factory->p->session_infos,
                                             g_object_ref (info));
}

/**
 * ligma_dialog_factory_dialog_raise:
 * @factory:      a #LigmaDialogFactory
 * @monitor:      the #GdkMonitor the dialog should appear on
 * @parent:       the #GtkWidget from which the raised dialog
 *                originated, if applicable, %NULL otherwise.
 * @identifiers:  a '|' separated list of identifiers of dialogs as
 *                registered with ligma_dialog_factory_register_entry()
 * @view_size:    the initial preview size if a dialog needs to be created
 *
 * Raises any of a list of already existing toplevel dialog or
 * #LigmaDockable if it was already created by this %facory.
 *
 * Implicitly creates the first dialog in the list if none of the dialogs
 * were found.
 *
 * Returns: the raised or newly created dialog.
 **/
GtkWidget *
ligma_dialog_factory_dialog_raise (LigmaDialogFactory *factory,
                                  GdkMonitor        *monitor,
                                  GtkWidget         *parent,
                                  const gchar       *identifiers,
                                  gint               view_size)
{
  GtkWidget *dialog;
  gchar    **ids;
  gint       i;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  /*  If the identifier is a list, try to find a matching dialog and
   *  raise it. If there's no match, use the first list item.
   *
   *  (we split the identifier list manually here because we must pass
   *  a single identifier, not a list, to new_internal() below)
   */
  ids = g_strsplit (identifiers, "|", 0);
  for (i = 0; ids[i]; i++)
    {
      if (ligma_dialog_factory_find_widget (factory, ids[i]))
        break;
    }

  dialog = ligma_dialog_factory_dialog_new_internal (factory,
                                                    monitor,
                                                    NULL,
                                                    NULL,
                                                    parent,
                                                    ids[i] ? ids[i] : ids[0],
                                                    view_size,
                                                    TRUE /*return_existing*/,
                                                    TRUE /*present*/,
                                                    TRUE /*create_containers*/);
  g_strfreev (ids);

  return dialog;
}

/**
 * ligma_dialog_factory_dockable_new:
 * @factory:      a #LigmaDialogFactory
 * @dock:         a #LigmaDock created by this %factory.
 * @identifier:   the identifier of the dialog as registered with
 *                ligma_dialog_factory_register_entry()
 * @view_size:
 *
 * Creates a new #LigmaDockable in the context of the #LigmaDock it will be
 * added to.
 *
 * Implicitly raises & returns an already existing singleton dockable,
 * so callers should check that ligma_dockable_get_dockbook (dockable)
 * is NULL before trying to add it to it's #LigmaDockbook.
 *
 * Returns: the newly created #LigmaDockable or an already existing
 *               singleton dockable.
 **/
GtkWidget *
ligma_dialog_factory_dockable_new (LigmaDialogFactory *factory,
                                  LigmaDock          *dock,
                                  const gchar       *identifier,
                                  gint               view_size)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  return ligma_dialog_factory_dialog_new_internal (factory,
                                                  ligma_widget_get_monitor (GTK_WIDGET (dock)),
                                                  ligma_dock_get_context (dock),
                                                  ligma_dock_get_ui_manager (dock),
                                                  NULL,
                                                  identifier,
                                                  view_size,
                                                  FALSE /*return_existing*/,
                                                  FALSE /*present*/,
                                                  FALSE /*create_containers*/);
}

void
ligma_dialog_factory_add_dialog (LigmaDialogFactory *factory,
                                GtkWidget         *dialog,
                                GdkMonitor        *monitor)
{
  LigmaDialogFactory      *dialog_factory = NULL;
  LigmaDialogFactoryEntry *entry          = NULL;
  LigmaSessionInfo        *info           = NULL;
  GList                  *list           = NULL;
  gboolean                toplevel       = FALSE;

  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  if (g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog already registered", G_STRFUNC);
      return;
    }

  dialog_factory = ligma_dialog_factory_from_widget (dialog, &entry);

  if (! ligma_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return;

  toplevel = gtk_widget_is_toplevel (dialog);

  if (entry)
    {
      /* dialog is a toplevel (but not a LigmaDockWindow) or a LigmaDockable */

      LIGMA_LOG (DIALOG_FACTORY, "adding %s \"%s\"",
                toplevel ? "toplevel" : "dockable",
                entry->identifier);

      for (list = factory->p->session_infos; list; list = g_list_next (list))
        {
          LigmaSessionInfo *current_info = list->data;

          if (ligma_session_info_get_factory_entry (current_info) == entry)
            {
              if (ligma_session_info_get_widget (current_info))
                {
                  if (ligma_session_info_is_singleton (current_info))
                    {
                      g_warning ("%s: singleton dialog \"%s\" created twice",
                                 G_STRFUNC, entry->identifier);

                      LIGMA_LOG (DIALOG_FACTORY,
                                "corrupt session info: %p (widget %p)",
                                current_info,
                                ligma_session_info_get_widget (current_info));

                      return;
                    }

                  continue;
                }

              ligma_session_info_set_widget (current_info, dialog);

              LIGMA_LOG (DIALOG_FACTORY,
                        "updating session info %p (widget %p) for %s \"%s\"",
                        current_info,
                        ligma_session_info_get_widget (current_info),
                        toplevel ? "toplevel" : "dockable",
                        entry->identifier);

              if (toplevel &&
                  ligma_session_info_is_session_managed (current_info) &&
                  ! gtk_widget_get_visible (dialog))
                {
                  LigmaGuiConfig *gui_config;

                  gui_config = LIGMA_GUI_CONFIG (factory->p->context->ligma->config);

                  ligma_session_info_apply_geometry (current_info,
                                                    monitor,
                                                    gui_config->restore_monitor);
                }

              info = current_info;
              break;
            }
        }

      if (! info)
        {
          info = ligma_session_info_new ();

          ligma_session_info_set_widget (info, dialog);

          LIGMA_LOG (DIALOG_FACTORY,
                    "creating session info %p (widget %p) for %s \"%s\"",
                    info,
                    ligma_session_info_get_widget (info),
                    toplevel ? "toplevel" : "dockable",
                    entry->identifier);

          ligma_session_info_set_factory_entry (info, entry);

          if (ligma_session_info_is_session_managed (info))
            {
              /* Make the dialog show up at the user position the
               * first time it is shown. After it has been shown the
               * first time we don't want it to show at the mouse the
               * next time. Think of the use cases "hide and show with
               * tab" and "change virtual desktops"
               */
              LIGMA_LOG (WM, "setting GTK_WIN_POS_MOUSE for %p (\"%s\")\n",
                        dialog, entry->identifier);

              gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
              g_signal_connect (dialog, "configure-event",
                                G_CALLBACK (ligma_dialog_factory_set_user_pos),
                                NULL);
            }

          ligma_dialog_factory_add_session_info (factory, info);
          g_object_unref (info);
        }
    }

  /* Some special logic for dock windows */
  if (LIGMA_IS_DOCK_WINDOW (dialog))
    {
      g_signal_emit (factory, factory_signals[DOCK_WINDOW_ADDED], 0, dialog);
    }

  factory->p->open_dialogs = g_list_prepend (factory->p->open_dialogs, dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (ligma_dialog_factory_remove_dialog),
                           factory,
                           G_CONNECT_SWAPPED);

  if (ligma_session_info_is_session_managed (info))
    g_signal_connect_object (dialog, "configure-event",
                             G_CALLBACK (ligma_dialog_factory_dialog_configure),
                             factory,
                             0);
}

void
ligma_dialog_factory_add_foreign (LigmaDialogFactory *factory,
                                 const gchar       *identifier,
                                 GtkWidget         *dialog,
                                 GdkMonitor        *monitor)
{
  LigmaDialogFactory      *dialog_factory;
  LigmaDialogFactoryEntry *entry;

  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  dialog_factory = ligma_dialog_factory_from_widget (dialog, &entry);

  if (dialog_factory || entry)
    {
      g_warning ("%s: dialog was created by a LigmaDialogFactory",
                 G_STRFUNC);
      return;
    }

  entry = ligma_dialog_factory_find_entry (factory, identifier);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"",
                 G_STRFUNC, identifier);
      return;
    }

  if (entry->new_func)
    {
      g_warning ("%s: entry for \"%s\" has a constructor (is not foreign)",
                 G_STRFUNC, identifier);
      return;
    }

  ligma_dialog_factory_set_widget_data (dialog, factory, entry);

  ligma_dialog_factory_add_dialog (factory, dialog, monitor);
}

/**
 * ligma_dialog_factory_position_dialog:
 * @factory:
 * @identifier:
 * @dialog:
 * @monitor:
 *
 * We correctly position all newly created dialog via
 * ligma_dialog_factory_add_dialog(), but some dialogs (like various
 * color dialogs) are never destroyed but created only once per
 * session. On re-showing, whatever window managing magic kicks in and
 * the dialog sometimes goes where it shouldn't.
 *
 * This function correctly positions a dialog on re-showing so it
 * appears where it was before it was hidden.
 *
 * See https://gitlab.gnome.org/GNOME/ligma/issues/1093
 **/
void
ligma_dialog_factory_position_dialog (LigmaDialogFactory *factory,
                                     const gchar       *identifier,
                                     GtkWidget         *dialog,
                                     GdkMonitor        *monitor)
{
  LigmaSessionInfo *info;
  LigmaGuiConfig   *gui_config;

  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  info = ligma_dialog_factory_find_session_info (factory, identifier);

  if (! info)
    {
      g_warning ("%s: no session info found for \"%s\"",
                 G_STRFUNC, identifier);
      return;
    }

  if (ligma_session_info_get_widget (info) != dialog)
    {
      g_warning ("%s: session info for \"%s\" is for a different widget",
                 G_STRFUNC, identifier);
      return;
    }

  gui_config = LIGMA_GUI_CONFIG (factory->p->context->ligma->config);

  ligma_session_info_apply_geometry (info,
                                    monitor,
                                    gui_config->restore_monitor);
}

void
ligma_dialog_factory_remove_dialog (LigmaDialogFactory *factory,
                                   GtkWidget         *dialog)
{
  LigmaDialogFactory      *dialog_factory;
  LigmaDialogFactoryEntry *entry;
  GList                  *list;

  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  if (! g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog not registered", G_STRFUNC);
      return;
    }

  factory->p->open_dialogs = g_list_remove (factory->p->open_dialogs, dialog);

  dialog_factory = ligma_dialog_factory_from_widget (dialog, &entry);

  if (! ligma_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return;

  LIGMA_LOG (DIALOG_FACTORY, "removing \"%s\" (dialog = %p)",
            entry->identifier,
            dialog);

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      LigmaSessionInfo *session_info = list->data;

      if (ligma_session_info_get_widget (session_info) == dialog)
        {
          LIGMA_LOG (DIALOG_FACTORY,
                    "clearing session info %p (widget %p) for \"%s\"",
                    session_info, ligma_session_info_get_widget (session_info),
                    entry->identifier);

          ligma_session_info_set_widget (session_info, NULL);

          ligma_dialog_factory_unset_widget_data (dialog);

          g_signal_handlers_disconnect_by_func (dialog,
                                                ligma_dialog_factory_set_user_pos,
                                                NULL);
          g_signal_handlers_disconnect_by_func (dialog,
                                                ligma_dialog_factory_remove_dialog,
                                                factory);

          if (ligma_session_info_is_session_managed (session_info))
            g_signal_handlers_disconnect_by_func (dialog,
                                                  ligma_dialog_factory_dialog_configure,
                                                  factory);

          if (LIGMA_IS_DOCK_WINDOW (dialog))
            {
              /*  don't save session info for empty docks  */
              factory->p->session_infos = g_list_remove (factory->p->session_infos,
                                                         session_info);
              g_object_unref (session_info);

              g_signal_emit (factory, factory_signals[DOCK_WINDOW_REMOVED], 0,
                             dialog);
            }

          break;
        }
    }
}

void
ligma_dialog_factory_hide_dialog (GtkWidget *dialog)
{
  LigmaDialogFactory *factory = NULL;

  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));

  if (! (factory = ligma_dialog_factory_from_widget (dialog, NULL)))
    {
      g_warning ("%s: dialog was not created by a LigmaDialogFactory",
                 G_STRFUNC);
      return;
    }

  gtk_widget_hide (dialog);

  if (factory->p->dialog_state != LIGMA_DIALOGS_SHOWN)
    g_object_set_data (G_OBJECT (dialog), LIGMA_DIALOG_VISIBILITY_KEY,
                       GINT_TO_POINTER (LIGMA_DIALOG_VISIBILITY_INVISIBLE));
}

void
ligma_dialog_factory_set_state (LigmaDialogFactory *factory,
                               LigmaDialogsState   state)
{
  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));

  factory->p->dialog_state = state;

  if (state == LIGMA_DIALOGS_SHOWN)
    {
      ligma_dialog_factory_show (factory);
    }
  else
    {
      ligma_dialog_factory_hide (factory);
    }
}

LigmaDialogsState
ligma_dialog_factory_get_state (LigmaDialogFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), 0);

  return factory->p->dialog_state;
}

void
ligma_dialog_factory_show_with_display (LigmaDialogFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));

  if (factory->p->dialog_state == LIGMA_DIALOGS_HIDDEN_WITH_DISPLAY)
    {
      ligma_dialog_factory_set_state (factory, LIGMA_DIALOGS_SHOWN);
    }
}

void
ligma_dialog_factory_hide_with_display (LigmaDialogFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));

  if (factory->p->dialog_state == LIGMA_DIALOGS_SHOWN)
    {
      ligma_dialog_factory_set_state (factory, LIGMA_DIALOGS_HIDDEN_WITH_DISPLAY);
    }
}

static GQuark ligma_dialog_factory_key       = 0;
static GQuark ligma_dialog_factory_entry_key = 0;

LigmaDialogFactory *
ligma_dialog_factory_from_widget (GtkWidget               *dialog,
                                 LigmaDialogFactoryEntry **entry)
{
  g_return_val_if_fail (GTK_IS_WIDGET (dialog), NULL);

  if (! ligma_dialog_factory_key)
    {
      ligma_dialog_factory_key =
        g_quark_from_static_string ("ligma-dialog-factory");

      ligma_dialog_factory_entry_key =
        g_quark_from_static_string ("ligma-dialog-factory-entry");
    }

  if (entry)
    *entry = g_object_get_qdata (G_OBJECT (dialog),
                                 ligma_dialog_factory_entry_key);

  return g_object_get_qdata (G_OBJECT (dialog), ligma_dialog_factory_key);
}

#define LIGMA_DIALOG_FACTORY_MIN_SIZE_KEY "ligma-dialog-factory-min-size"

void
ligma_dialog_factory_set_has_min_size (GtkWindow *window,
                                      gboolean   has_min_size)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  g_object_set_data (G_OBJECT (window), LIGMA_DIALOG_FACTORY_MIN_SIZE_KEY,
                     GINT_TO_POINTER (has_min_size ? TRUE : FALSE));
}

gboolean
ligma_dialog_factory_get_has_min_size (GtkWindow *window)
{
  g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window),
                                             LIGMA_DIALOG_FACTORY_MIN_SIZE_KEY));
}


/*  private functions  */

static GtkWidget *
ligma_dialog_factory_constructor (LigmaDialogFactory      *factory,
                                 LigmaDialogFactoryEntry *entry,
                                 LigmaContext            *context,
                                 LigmaUIManager          *ui_manager,
                                 gint                    view_size)
{
  GtkWidget *widget;

  widget = entry->new_func (factory, context, ui_manager, view_size);

  /* The entry is for a dockable, so we simply need to put the created
   * widget in a dockable
   */
  if (widget && entry->dockable)
    {
      GtkWidget *dockable = NULL;

      dockable = ligma_dockable_new (entry->name, entry->blurb,
                                    entry->icon_name, entry->help_id);
      gtk_container_add (GTK_CONTAINER (dockable), widget);
      gtk_widget_show (widget);

      /* EEK */
      g_object_set_data (G_OBJECT (dockable), "ligma-dialog-identifier",
                         entry->identifier);

      /* Return the dockable instead */
      widget = dockable;
    }

  return widget;
}

static void
ligma_dialog_factory_config_notify (LigmaDialogFactory *factory,
                                   GParamSpec        *pspec,
                                   LigmaGuiConfig     *config)
{
  LigmaDialogsState state     = ligma_dialog_factory_get_state (factory);
  LigmaDialogsState new_state = state;

  /* Make sure the state and config are in sync */
  if (config->hide_docks && state == LIGMA_DIALOGS_SHOWN)
    new_state = LIGMA_DIALOGS_HIDDEN_EXPLICITLY;
  else if (! config->hide_docks)
    new_state = LIGMA_DIALOGS_SHOWN;

  if (state != new_state)
    ligma_dialog_factory_set_state (factory, new_state);
}

static void
ligma_dialog_factory_set_widget_data (GtkWidget              *dialog,
                                     LigmaDialogFactory      *factory,
                                     LigmaDialogFactoryEntry *entry)
{
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (LIGMA_IS_DIALOG_FACTORY (factory));

  if (! ligma_dialog_factory_key)
    {
      ligma_dialog_factory_key =
        g_quark_from_static_string ("ligma-dialog-factory");

      ligma_dialog_factory_entry_key =
        g_quark_from_static_string ("ligma-dialog-factory-entry");
    }

  g_object_set_qdata (G_OBJECT (dialog), ligma_dialog_factory_key, factory);

  if (entry)
    g_object_set_qdata (G_OBJECT (dialog), ligma_dialog_factory_entry_key,
                        entry);
}

static void
ligma_dialog_factory_unset_widget_data (GtkWidget *dialog)
{
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  if (! ligma_dialog_factory_key)
    return;

  g_object_set_qdata (G_OBJECT (dialog), ligma_dialog_factory_key,       NULL);
  g_object_set_qdata (G_OBJECT (dialog), ligma_dialog_factory_entry_key, NULL);
}

static gboolean
ligma_dialog_factory_set_user_pos (GtkWidget         *dialog,
                                  GdkEventConfigure *cevent,
                                  gpointer           data)
{
  GdkWindowHints geometry_mask;

  /* Not only set geometry hints, also reset window position */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);
  g_signal_handlers_disconnect_by_func (dialog,
                                        ligma_dialog_factory_set_user_pos,
                                        data);

  LIGMA_LOG (WM, "setting GDK_HINT_USER_POS for %p\n", dialog);
  geometry_mask = GDK_HINT_USER_POS;

  if (ligma_dialog_factory_get_has_min_size (GTK_WINDOW (dialog)))
    geometry_mask |= GDK_HINT_MIN_SIZE;

  gtk_window_set_geometry_hints (GTK_WINDOW (dialog), NULL, NULL,
                                 geometry_mask);

  return FALSE;
}

static gboolean
ligma_dialog_factory_dialog_configure (GtkWidget         *dialog,
                                      GdkEventConfigure *cevent,
                                      LigmaDialogFactory *factory)
{
  LigmaDialogFactory      *dialog_factory;
  LigmaDialogFactoryEntry *entry;
  GList                  *list;

  if (! g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog not registered", G_STRFUNC);
      return FALSE;
    }

  dialog_factory = ligma_dialog_factory_from_widget (dialog, &entry);

  if (! ligma_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return FALSE;

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      LigmaSessionInfo *session_info = list->data;

      if (ligma_session_info_get_widget (session_info) == dialog)
        {
          ligma_session_info_read_geometry (session_info, cevent);

          LIGMA_LOG (DIALOG_FACTORY,
                    "updated session info for \"%s\" from window geometry "
                    "(x=%d y=%d  %dx%d)",
                    entry->identifier,
                    ligma_session_info_get_x (session_info),
                    ligma_session_info_get_y (session_info),
                    ligma_session_info_get_width (session_info),
                    ligma_session_info_get_height (session_info));

          break;
        }
    }

  return FALSE;
}

void
ligma_dialog_factory_save (LigmaDialogFactory *factory,
                          LigmaConfigWriter  *writer)
{
  GList *infos;

  for (infos = factory->p->session_infos; infos; infos = g_list_next (infos))
    {
      LigmaSessionInfo *info = infos->data;

      /*  we keep session info entries for all toplevel dialogs created
       *  by the factory but don't save them if they don't want to be
       *  managed
       */
      if (! ligma_session_info_is_session_managed (info) ||
          ligma_session_info_get_factory_entry (info) == NULL)
        continue;

      if (ligma_session_info_get_widget (info))
        ligma_session_info_get_info (info);

      ligma_config_writer_open (writer, "session-info");
      ligma_config_writer_string (writer,
                                 ligma_object_get_name (factory));

      LIGMA_CONFIG_GET_IFACE (info)->serialize (LIGMA_CONFIG (info), writer, NULL);

      ligma_config_writer_close (writer);

      if (ligma_session_info_get_widget (info))
        ligma_session_info_clear_info (info);
    }
}

void
ligma_dialog_factory_restore (LigmaDialogFactory *factory,
                             GdkMonitor        *monitor)
{
  GList *infos;

  for (infos = factory->p->session_infos; infos; infos = g_list_next (infos))
    {
      LigmaSessionInfo *info = infos->data;

      if (ligma_session_info_get_open (info))
        {
          ligma_session_info_restore (info, factory, monitor);
        }
      else
        {
          LIGMA_LOG (DIALOG_FACTORY,
                    "skipping to restore session info %p, not open",
                    info);
        }
    }
}

static void
ligma_dialog_factory_hide (LigmaDialogFactory *factory)
{
  GList *list;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          LigmaDialogFactoryEntry    *entry      = NULL;
          LigmaDialogVisibilityState  visibility = LIGMA_DIALOG_VISIBILITY_UNKNOWN;

          ligma_dialog_factory_from_widget (widget, &entry);
          if (! entry->hideable)
            continue;

          if (gtk_widget_get_visible (widget))
            {
              gtk_widget_hide (widget);
              visibility = LIGMA_DIALOG_VISIBILITY_HIDDEN;

              LIGMA_LOG (WM, "Hiding '%s' [%p]",
                        gtk_window_get_title (GTK_WINDOW (widget)),
                        widget);
            }
          else
            {
              visibility = LIGMA_DIALOG_VISIBILITY_INVISIBLE;
            }

          g_object_set_data (G_OBJECT (widget),
                             LIGMA_DIALOG_VISIBILITY_KEY,
                             GINT_TO_POINTER (visibility));
        }
    }
}

static void
ligma_dialog_factory_show (LigmaDialogFactory *factory)
{
  GList *list;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          LigmaDialogVisibilityState visibility;

          visibility =
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                LIGMA_DIALOG_VISIBILITY_KEY));

          if (! gtk_widget_get_visible (widget) &&
              visibility == LIGMA_DIALOG_VISIBILITY_HIDDEN)
            {
              LIGMA_LOG (WM, "Showing '%s' [%p]",
                        gtk_window_get_title (GTK_WINDOW (widget)),
                        widget);

              /* Don't use gtk_window_present() here, we don't want the
               * keyboard focus to move.
               */
              gtk_widget_show (widget);
              g_object_set_data (G_OBJECT (widget),
                                 LIGMA_DIALOG_VISIBILITY_KEY,
                                 GINT_TO_POINTER (LIGMA_DIALOG_VISIBILITY_VISIBLE));

              if (gtk_widget_get_visible (widget))
                gdk_window_raise (gtk_widget_get_window (widget));
            }
        }
    }
}

void
ligma_dialog_factory_set_busy (LigmaDialogFactory *factory)
{
  GList *list;

  if (! factory)
    return;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          GdkWindow *window = gtk_widget_get_window (widget);

          if (window)
            {
              GdkCursor *cursor = ligma_cursor_new (window,
                                                   LIGMA_HANDEDNESS_RIGHT,
                                                   (LigmaCursorType) GDK_WATCH,
                                                   LIGMA_TOOL_CURSOR_NONE,
                                                   LIGMA_CURSOR_MODIFIER_NONE);
              gdk_window_set_cursor (window, cursor);
              g_object_unref (cursor);
            }
        }
    }
}

void
ligma_dialog_factory_unset_busy (LigmaDialogFactory *factory)
{
  GList *list;

  if (! factory)
    return;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          GdkWindow *window = gtk_widget_get_window (widget);

          if (window)
            gdk_window_set_cursor (window, NULL);
        }
    }
}

/**
 * ligma_dialog_factory_get_singleton:
 *
 * Returns: The toplevel LigmaDialogFactory instance.
 **/
LigmaDialogFactory *
ligma_dialog_factory_get_singleton (void)
{
  g_return_val_if_fail (ligma_toplevel_factory != NULL, NULL);

  return ligma_toplevel_factory;
}

/**
 * ligma_dialog_factory_set_singleton:
 * @:
 *
 * Set the toplevel LigmaDialogFactory instance. Must only be called by
 * dialogs_init()!.
 **/
void
ligma_dialog_factory_set_singleton (LigmaDialogFactory *factory)
{
  g_return_if_fail (ligma_toplevel_factory == NULL ||
                    factory               == NULL);

  ligma_toplevel_factory = factory;
}
