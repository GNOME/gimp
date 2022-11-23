/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockable.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"

#include "ligmadialogfactory.h"
#include "ligmadnd.h"
#include "ligmadock.h"
#include "ligmadockable.h"
#include "ligmadockbook.h"
#include "ligmadocked.h"
#include "ligmadockwindow.h"
#include "ligmahelp-ids.h"
#include "ligmapanedbox.h"
#include "ligmasessioninfo-aux.h"
#include "ligmasessionmanaged.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LOCKED
};


struct _LigmaDockablePrivate
{
  gchar        *name;
  gchar        *blurb;
  gchar        *icon_name;
  gchar        *help_id;
  LigmaTabStyle  tab_style;
  gboolean      locked;

  LigmaDockbook *dockbook;

  LigmaContext  *context;
};


static void       ligma_dockable_session_managed_iface_init (LigmaSessionManagedInterface *iface);

static void       ligma_dockable_dispose       (GObject            *object);
static void       ligma_dockable_set_property  (GObject            *object,
                                               guint               property_id,
                                               const GValue       *value,
                                               GParamSpec         *pspec);
static void       ligma_dockable_get_property  (GObject            *object,
                                               guint               property_id,
                                               GValue             *value,
                                               GParamSpec         *pspec);

static void       ligma_dockable_style_updated (GtkWidget          *widget);

static void       ligma_dockable_add           (GtkContainer       *container,
                                               GtkWidget          *widget);
static GType      ligma_dockable_child_type    (GtkContainer       *container);

static GList    * ligma_dockable_get_aux_info  (LigmaSessionManaged *managed);
static void       ligma_dockable_set_aux_info  (LigmaSessionManaged *managed,
                                               GList              *aux_info);


G_DEFINE_TYPE_WITH_CODE (LigmaDockable, ligma_dockable, GTK_TYPE_BIN,
                         G_ADD_PRIVATE (LigmaDockable)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_SESSION_MANAGED,
                                                ligma_dockable_session_managed_iface_init))

#define parent_class ligma_dockable_parent_class

static const GtkTargetEntry dialog_target_table[] = { LIGMA_TARGET_NOTEBOOK_TAB };


static void
ligma_dockable_class_init (LigmaDockableClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose       = ligma_dockable_dispose;
  object_class->set_property  = ligma_dockable_set_property;
  object_class->get_property  = ligma_dockable_get_property;

  widget_class->style_updated = ligma_dockable_style_updated;

  container_class->add        = ligma_dockable_add;
  container_class->child_type = ligma_dockable_child_type;

  gtk_container_class_handle_border_width (container_class);

  g_object_class_install_property (object_class, PROP_LOCKED,
                                   g_param_spec_boolean ("locked", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             LIGMA_PARAM_READABLE));
}

static void
ligma_dockable_init (LigmaDockable *dockable)
{
  dockable->p = ligma_dockable_get_instance_private (dockable);

  dockable->p->tab_style = LIGMA_TAB_STYLE_PREVIEW;

  gtk_drag_dest_set (GTK_WIDGET (dockable),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
ligma_dockable_session_managed_iface_init (LigmaSessionManagedInterface *iface)
{
  iface->get_aux_info = ligma_dockable_get_aux_info;
  iface->set_aux_info = ligma_dockable_set_aux_info;
}

static void
ligma_dockable_dispose (GObject *object)
{
  LigmaDockable *dockable = LIGMA_DOCKABLE (object);

  if (dockable->p->context)
    ligma_dockable_set_context (dockable, NULL);

  g_clear_pointer (&dockable->p->blurb,     g_free);
  g_clear_pointer (&dockable->p->name,      g_free);
  g_clear_pointer (&dockable->p->icon_name, g_free);
  g_clear_pointer (&dockable->p->help_id,   g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_dockable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LigmaDockable *dockable = LIGMA_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      ligma_dockable_set_locked (dockable, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dockable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaDockable *dockable = LIGMA_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      g_value_set_boolean (value, dockable->p->locked);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dockable_style_updated (GtkWidget *widget)
{
  gint content_border;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "content-border", &content_border,
                        NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);
}


static void
ligma_dockable_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  LigmaDockable *dockable;

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  /*  not all tab styles are supported by all children  */
  dockable = LIGMA_DOCKABLE (container);
  ligma_dockable_set_tab_style (dockable, dockable->p->tab_style);
}

static GType
ligma_dockable_child_type (GtkContainer *container)
{
  if (gtk_bin_get_child (GTK_BIN (container)))
    return G_TYPE_NONE;

  return LIGMA_TYPE_DOCKED;
}

static GtkWidget *
ligma_dockable_new_tab_widget_internal (LigmaDockable *dockable,
                                       LigmaContext  *context,
                                       LigmaTabStyle  tab_style,
                                       GtkIconSize   size,
                                       gboolean      dnd)
{
  GtkWidget *tab_widget = NULL;
  GtkWidget *label      = NULL;
  GtkWidget *icon       = NULL;

  switch (tab_style)
    {
    case LIGMA_TAB_STYLE_NAME:
    case LIGMA_TAB_STYLE_ICON_NAME:
    case LIGMA_TAB_STYLE_PREVIEW_NAME:
      label = gtk_label_new (dockable->p->name);
      break;

    case LIGMA_TAB_STYLE_BLURB:
    case LIGMA_TAB_STYLE_ICON_BLURB:
    case LIGMA_TAB_STYLE_PREVIEW_BLURB:
      label = gtk_label_new (dockable->p->blurb);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case LIGMA_TAB_STYLE_ICON:
    case LIGMA_TAB_STYLE_ICON_NAME:
    case LIGMA_TAB_STYLE_ICON_BLURB:
      icon = ligma_dockable_get_icon (dockable, size);
      break;

    case LIGMA_TAB_STYLE_PREVIEW:
    case LIGMA_TAB_STYLE_PREVIEW_NAME:
    case LIGMA_TAB_STYLE_PREVIEW_BLURB:
      {
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

        if (child)
          icon = ligma_docked_get_preview (LIGMA_DOCKED (child),
                                          context, size);

        if (! icon)
          icon = ligma_dockable_get_icon (dockable, size);
      }
      break;

    default:
      break;
    }

  if (label && dnd)
    ligma_label_set_attributes (GTK_LABEL (label),
                               PANGO_ATTR_WEIGHT, PANGO_WEIGHT_SEMIBOLD,
                               -1);

  switch (tab_style)
    {
    case LIGMA_TAB_STYLE_ICON:
    case LIGMA_TAB_STYLE_PREVIEW:
      tab_widget = icon;
      break;

    case LIGMA_TAB_STYLE_NAME:
    case LIGMA_TAB_STYLE_BLURB:
      tab_widget = label;
      break;

    case LIGMA_TAB_STYLE_ICON_NAME:
    case LIGMA_TAB_STYLE_ICON_BLURB:
    case LIGMA_TAB_STYLE_PREVIEW_NAME:
    case LIGMA_TAB_STYLE_PREVIEW_BLURB:
      tab_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, dnd ? 6 : 2);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      break;
    }

  return tab_widget;
}

/*  public functions  */

GtkWidget *
ligma_dockable_new (const gchar *name,
                   const gchar *blurb,
                   const gchar *icon_name,
                   const gchar *help_id)
{
  LigmaDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dockable = g_object_new (LIGMA_TYPE_DOCKABLE, NULL);

  dockable->p->name      = g_strdup (name);
  dockable->p->icon_name = g_strdup (icon_name);
  dockable->p->help_id   = g_strdup (help_id);

  if (blurb)
    dockable->p->blurb  = g_strdup (blurb);
  else
    dockable->p->blurb  = g_strdup (dockable->p->name);

  ligma_help_set_help_data (GTK_WIDGET (dockable), NULL, help_id);

  return GTK_WIDGET (dockable);
}

void
ligma_dockable_set_dockbook (LigmaDockable *dockable,
                            LigmaDockbook *dockbook)
{
  g_return_if_fail (LIGMA_IS_DOCKABLE (dockable));
  g_return_if_fail (dockbook == NULL ||
                    LIGMA_IS_DOCKBOOK (dockbook));

  dockable->p->dockbook = dockbook;
}

LigmaDockbook *
ligma_dockable_get_dockbook (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return dockable->p->dockbook;
}

void
ligma_dockable_set_tab_style (LigmaDockable *dockable,
                             LigmaTabStyle  tab_style)
{
  GtkWidget *child;

  g_return_if_fail (LIGMA_IS_DOCKABLE (dockable));

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child && ! LIGMA_DOCKED_GET_IFACE (child)->get_preview)
    tab_style = ligma_preview_tab_style_to_icon (tab_style);

  dockable->p->tab_style = tab_style;
}

LigmaTabStyle
ligma_dockable_get_tab_style (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), -1);

  return dockable->p->tab_style;
}

void
ligma_dockable_set_locked (LigmaDockable *dockable,
                          gboolean      lock)
{
  g_return_if_fail (LIGMA_IS_DOCKABLE (dockable));

  if (dockable->p->locked != lock)
    {
      dockable->p->locked = lock ? TRUE : FALSE;

      g_object_notify (G_OBJECT (dockable), "locked");
    }
}

gboolean
ligma_dockable_get_locked (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), FALSE);

  return dockable->p->locked;
}

const gchar *
ligma_dockable_get_name (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return dockable->p->name;
}

const gchar *
ligma_dockable_get_blurb (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return dockable->p->blurb;
}

const gchar *
ligma_dockable_get_help_id (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return dockable->p->help_id;
}

const gchar *
ligma_dockable_get_icon_name (LigmaDockable *dockable)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return dockable->p->icon_name;
}

GtkWidget *
ligma_dockable_get_icon (LigmaDockable *dockable,
                        GtkIconSize   size)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);

  return gtk_image_new_from_icon_name (dockable->p->icon_name, size);
}

GtkWidget *
ligma_dockable_create_tab_widget (LigmaDockable *dockable,
                                 LigmaContext  *context,
                                 LigmaTabStyle  tab_style,
                                 GtkIconSize   size)
{
  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return ligma_dockable_new_tab_widget_internal (dockable, context,
                                                tab_style, size, FALSE);
}

void
ligma_dockable_set_context (LigmaDockable *dockable,
                           LigmaContext  *context)
{
  g_return_if_fail (LIGMA_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  if (context != dockable->p->context)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

      if (child)
        ligma_docked_set_context (LIGMA_DOCKED (child), context);

      dockable->p->context = context;
    }
}

LigmaUIManager *
ligma_dockable_get_menu (LigmaDockable  *dockable,
                        const gchar  **ui_path,
                        gpointer      *popup_data)
{
  GtkWidget *child;

  g_return_val_if_fail (LIGMA_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return ligma_docked_get_menu (LIGMA_DOCKED (child), ui_path, popup_data);

  return NULL;
}

void
ligma_dockable_detach (LigmaDockable *dockable)
{
  LigmaDialogFactory *dialog_factory;
  LigmaMenuFactory   *menu_factory;
  LigmaDockWindow    *src_dock_window;
  LigmaDock          *src_dock;
  GtkWidget         *dock;
  LigmaDockWindow    *dock_window;
  GtkWidget         *dockbook;

  g_return_if_fail (LIGMA_IS_DOCKABLE (dockable));
  g_return_if_fail (LIGMA_IS_DOCKBOOK (dockable->p->dockbook));

  src_dock        = ligma_dockbook_get_dock (dockable->p->dockbook);
  src_dock_window = ligma_dock_window_from_dock (src_dock);

  dialog_factory = ligma_dock_get_dialog_factory (src_dock);
  menu_factory   = ligma_dialog_factory_get_menu_factory (dialog_factory);

  dock = ligma_dock_with_window_new (dialog_factory,
                                    ligma_widget_get_monitor (GTK_WIDGET (dockable)),
                                    FALSE /*toolbox*/);
  dock_window = ligma_dock_window_from_dock (LIGMA_DOCK (dock));
  gtk_window_set_position (GTK_WINDOW (dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    ligma_dock_window_setup (dock_window, src_dock_window);

  dockbook = ligma_dockbook_new (menu_factory);

  ligma_dock_add_book (LIGMA_DOCK (dock), LIGMA_DOCKBOOK (dockbook), 0);

  g_object_ref (dockable);
  gtk_container_remove (GTK_CONTAINER (dockable->p->dockbook),
                        GTK_WIDGET (dockable));

  gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                            GTK_WIDGET (dockable), NULL);
  g_object_unref (dockable);

  gtk_widget_show (GTK_WIDGET (dock_window));
  gtk_widget_show (dock);
}


/*  private functions  */

static GList *
ligma_dockable_get_aux_info (LigmaSessionManaged *session_managed)
{
  LigmaDockable *dockable;
  GtkWidget    *child;

  g_return_val_if_fail (LIGMA_IS_DOCKABLE (session_managed), NULL);

  dockable = LIGMA_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return ligma_docked_get_aux_info (LIGMA_DOCKED (child));

  return NULL;
}

static void
ligma_dockable_set_aux_info (LigmaSessionManaged *session_managed,
                            GList              *aux_info)
{
  LigmaDockable *dockable;
  GtkWidget    *child;

  g_return_if_fail (LIGMA_IS_DOCKABLE (session_managed));

  dockable = LIGMA_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    ligma_docked_set_aux_info (LIGMA_DOCKED (child), aux_info);
}
