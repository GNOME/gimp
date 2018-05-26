/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockbook.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdocked.h"
#include "gimpdockcontainer.h"
#include "gimpdockwindow.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimppanedbox.h"
#include "gimpstringaction.h"
#include "gimpuimanager.h"
#include "gimpview.h"
#include "gimpwidgets-utils.h"

#include "gimp-log.h"
#include "gimp-intl.h"


#define DEFAULT_TAB_ICON_SIZE        GTK_ICON_SIZE_BUTTON
#define DND_WIDGET_ICON_SIZE         GTK_ICON_SIZE_BUTTON
#define MENU_WIDGET_SPACING          4
#define TAB_HOVER_TIMEOUT            500
#define GIMP_DOCKABLE_DETACH_REF_KEY "gimp-dockable-detach-ref"


enum
{
  DOCKABLE_ADDED,
  DOCKABLE_REMOVED,
  DOCKABLE_REORDERED,
  LAST_SIGNAL
};


struct _GimpDockbookPrivate
{
  GimpDock       *dock;
  GimpUIManager  *ui_manager;

  guint           tab_hover_timeout;
  GimpDockable   *tab_hover_dockable;

  GimpPanedBox   *drag_handler;

  GtkWidget      *menu_button;
};


static void         gimp_dockbook_dispose               (GObject        *object);
static void         gimp_dockbook_finalize              (GObject        *object);

static void         gimp_dockbook_drag_leave            (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         guint           time);
static gboolean     gimp_dockbook_drag_motion           (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time);
static gboolean     gimp_dockbook_drag_drop             (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time);
static gboolean     gimp_dockbook_popup_menu            (GtkWidget      *widget);

static gboolean     gimp_dockbook_menu_button_press     (GimpDockbook   *dockbook,
                                                         GdkEventButton *bevent,
                                                         GtkWidget      *button);
static gboolean     gimp_dockbook_show_menu             (GimpDockbook   *dockbook);
static void         gimp_dockbook_menu_end              (GimpDockable   *dockable);
static void         gimp_dockbook_dockable_added        (GimpDockbook   *dockbook,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_dockable_removed      (GimpDockbook   *dockbook,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_source_setup (GtkWidget      *widget,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_begin        (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_end          (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_leave        (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         guint           time,
                                                         GimpDockable   *dockable);
static gboolean     gimp_dockbook_tab_drag_motion       (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time,
                                                         GimpDockable   *dockable);
static gboolean     gimp_dockbook_tab_drag_drop         (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time);

static GtkIconSize  gimp_dockbook_get_tab_icon_size     (GimpDockbook   *dockbook);
static void         gimp_dockbook_add_tab_timeout       (GimpDockbook   *dockbook,
                                                         GimpDockable   *dockable);
static void         gimp_dockbook_remove_tab_timeout    (GimpDockbook   *dockbook);
static gboolean     gimp_dockbook_tab_timeout           (GimpDockbook   *dockbook);
static void         gimp_dockbook_tab_locked_notify     (GimpDockable   *dockable,
                                                         GParamSpec     *pspec,
                                                         GimpDockbook   *dockbook);
static void         gimp_dockbook_help_func             (const gchar    *help_id,
                                                         gpointer        help_data);


G_DEFINE_TYPE (GimpDockbook, gimp_dockbook, GTK_TYPE_NOTEBOOK)

#define parent_class gimp_dockbook_parent_class

static guint dockbook_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };


static void
gimp_dockbook_class_init (GimpDockbookClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  dockbook_signals[DOCKABLE_ADDED] =
    g_signal_new ("dockable-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_added),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REMOVED] =
    g_signal_new ("dockable-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_removed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REORDERED] =
    g_signal_new ("dockable-reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_reordered),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  object_class->dispose     = gimp_dockbook_dispose;
  object_class->finalize    = gimp_dockbook_finalize;

  widget_class->drag_leave    = gimp_dockbook_drag_leave;
  widget_class->drag_motion   = gimp_dockbook_drag_motion;
  widget_class->drag_drop     = gimp_dockbook_drag_drop;
  widget_class->popup_menu    = gimp_dockbook_popup_menu;

  klass->dockable_added     = gimp_dockbook_dockable_added;
  klass->dockable_removed   = gimp_dockbook_dockable_removed;
  klass->dockable_reordered = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockbookPrivate));
}

static void
gimp_dockbook_init (GimpDockbook *dockbook)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (dockbook);
  GtkWidget   *image;

  dockbook->p = G_TYPE_INSTANCE_GET_PRIVATE (dockbook,
                                             GIMP_TYPE_DOCKBOOK,
                                             GimpDockbookPrivate);

  /* Various init */
  gtk_notebook_popup_enable (notebook);
  gtk_notebook_set_scrollable (notebook, TRUE);
  gtk_notebook_set_show_border (notebook, FALSE);
  gtk_notebook_set_show_tabs (notebook, TRUE);

  gtk_drag_dest_set (GTK_WIDGET (dockbook),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);

  /* Menu button */
  dockbook->p->menu_button = gtk_button_new ();
  gtk_widget_set_can_focus (dockbook->p->menu_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (dockbook->p->menu_button),
                         GTK_RELIEF_NONE);
  gtk_notebook_set_action_widget (notebook,
                                  dockbook->p->menu_button,
                                  GTK_PACK_END);
  gtk_widget_show (dockbook->p->menu_button);

  image = gtk_image_new_from_icon_name (GIMP_ICON_MENU_LEFT,
                                        GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
  gtk_image_set_from_icon_name (GTK_IMAGE (image), GIMP_ICON_MENU_LEFT,
                                GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockbook->p->menu_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (dockbook->p->menu_button, _("Configure this tab"),
                           GIMP_HELP_DOCK_TAB_MENU);

  g_signal_connect_swapped (dockbook->p->menu_button, "button-press-event",
                            G_CALLBACK (gimp_dockbook_menu_button_press),
                            dockbook);
}

static void
gimp_dockbook_dispose (GObject *object)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (object);
  GList        *children;
  GList        *list;

  gimp_dockbook_remove_tab_timeout (dockbook);

  children = gtk_container_get_children (GTK_CONTAINER (object));

  for (list = children; list; list = g_list_next (list))
    {
      GimpDockable *dockable = list->data;

      g_object_ref (dockable);
      gimp_dockbook_remove (dockbook, dockable);
      gtk_widget_destroy (GTK_WIDGET (dockable));
      g_object_unref (dockable);
    }

  g_list_free (children);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dockbook_finalize (GObject *object)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (object);

  g_clear_object (&dockbook->p->ui_manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dockbook_drag_leave (GtkWidget      *widget,
                          GdkDragContext *context,
                          guint           time)
{
  gimp_highlight_widget (widget, FALSE);
}

static gboolean
gimp_dockbook_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (widget);

  if (gimp_paned_box_will_handle_drag (dockbook->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      gdk_drag_status (context, 0, time);
      gimp_highlight_widget (widget, FALSE);

      return FALSE;
    }

  gdk_drag_status (context, GDK_ACTION_MOVE, time);
  gimp_highlight_widget (widget, TRUE);

  /* Return TRUE so drag_leave() is called */
  return TRUE;
}

static gboolean
gimp_dockbook_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (widget);
  gboolean      dropped;

  if (gimp_paned_box_will_handle_drag (dockbook->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  dropped = gimp_dockbook_drop_dockable (dockbook,
                                         gtk_drag_get_source_widget (context));

  gtk_drag_finish (context, dropped, TRUE, time);

  return TRUE;
}

static gboolean
gimp_dockbook_popup_menu (GtkWidget *widget)
{
  return gimp_dockbook_show_menu (GIMP_DOCKBOOK (widget));
}

static gboolean
gimp_dockbook_menu_button_press (GimpDockbook   *dockbook,
                                 GdkEventButton *bevent,
                                 GtkWidget      *button)
{
  gboolean handled = FALSE;

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    handled = gimp_dockbook_show_menu (dockbook);

  return handled;
}

static gboolean
gimp_dockbook_show_menu (GimpDockbook *dockbook)
{
  GimpUIManager *dockbook_ui_manager;
  GimpUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;
  GtkWidget     *parent_menu_widget;
  GtkAction     *parent_menu_action;
  GimpDockable  *dockable;
  gint           page_num;

  dockbook_ui_manager = gimp_dockbook_get_ui_manager (dockbook);

  if (! dockbook_ui_manager)
    return FALSE;

  parent_menu_widget =
    gtk_ui_manager_get_widget (GTK_UI_MANAGER (dockbook_ui_manager),
                               "/dockable-popup/dockable-menu");
  parent_menu_action =
    gtk_ui_manager_get_action (GTK_UI_MANAGER (dockbook_ui_manager),
                               "/dockable-popup/dockable-menu");

  if (! parent_menu_widget || ! parent_menu_action)
    return FALSE;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
  dockable = GIMP_DOCKABLE (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                       page_num));

  if (! dockable)
    return FALSE;

  dialog_ui_manager = gimp_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget *child_menu_widget;
      GtkAction *child_menu_action;
      gchar     *label;

      child_menu_widget =
        gtk_ui_manager_get_widget (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (! child_menu_widget)
        {
          g_warning ("%s: UI manager '%s' has no widget at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      child_menu_action =
        gtk_ui_manager_get_action (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (! child_menu_action)
        {
          g_warning ("%s: UI manager '%s' has no action at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      g_object_get (child_menu_action,
                    "label", &label,
                    NULL);

      g_object_set (parent_menu_action,
                    "label",     label,
                    "icon-name", gimp_dockable_get_icon_name (dockable),
                    "visible",   TRUE,
                    NULL);

      g_free (label);

      if (! GTK_IS_MENU (child_menu_widget))
        {
          g_warning ("%s: child_menu_widget (%p) is not a GtkMenu",
                     G_STRFUNC, child_menu_widget);
          return FALSE;
        }

      {
        GtkWidget *image = gimp_dockable_get_icon (dockable,
                                                   GTK_ICON_SIZE_MENU);
        gimp_menu_item_set_image (GTK_MENU_ITEM (parent_menu_widget), image);
        gtk_widget_show (image);
      }

      gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent_menu_widget),
                                 child_menu_widget);

      gimp_ui_manager_update (dialog_ui_manager, dialog_popup_data);
    }
  else
    {
      g_object_set (parent_menu_action, "visible", FALSE, NULL);
    }

  /*  an action callback may destroy both dockable and dockbook, so
   *  reference them for gimp_dockbook_menu_end()
   */
  g_object_ref (dockable);
  g_object_set_data_full (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY,
                          g_object_ref (dockbook),
                          g_object_unref);

  gimp_ui_manager_update (dockbook_ui_manager, dockable);

  gimp_ui_manager_ui_popup_at_widget (dockbook_ui_manager,
                                      "/dockable-popup",
                                      dockbook->p->menu_button,
                                      GDK_GRAVITY_WEST,
                                      GDK_GRAVITY_NORTH_EAST,
                                      NULL,
                                      (GDestroyNotify) gimp_dockbook_menu_end,
                                      dockable);

  return TRUE;
}

static void
gimp_dockbook_menu_end (GimpDockable *dockable)
{
  GimpUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;

  dialog_ui_manager = gimp_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget *child_menu_widget =
        gtk_ui_manager_get_widget (GTK_UI_MANAGER (dialog_ui_manager),
                                   dialog_ui_path);

      if (child_menu_widget)
        gtk_menu_detach (GTK_MENU (child_menu_widget));
    }

  /*  release gimp_dockbook_show_menu()'s references  */
  g_object_set_data (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY, NULL);
  g_object_unref (dockable);
}

static void
gimp_dockbook_dockable_added (GimpDockbook *dockbook,
                              GimpDockable *dockable)
{
  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                 gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
                                                        GTK_WIDGET (dockable)));
}

static void
gimp_dockbook_dockable_removed (GimpDockbook *dockbook,
                                GimpDockable *dockable)
{
}

GtkWidget *
gimp_dockbook_new (GimpMenuFactory *menu_factory)
{
  GimpDockbook *dockbook;

  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (GIMP_TYPE_DOCKBOOK, NULL);

  dockbook->p->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                           "<Dockable>",
                                                           dockbook);

  gimp_help_connect (GTK_WIDGET (dockbook), gimp_dockbook_help_func,
                     GIMP_HELP_DOCK, dockbook);

  return GTK_WIDGET (dockbook);
}

GimpDock *
gimp_dockbook_get_dock (GimpDockbook *dockbook)
{
  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), NULL);

  return dockbook->p->dock;
}

void
gimp_dockbook_set_dock (GimpDockbook *dockbook,
                        GimpDock     *dock)
{
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dock == NULL || GIMP_IS_DOCK (dock));

  dockbook->p->dock = dock;
}

GimpUIManager *
gimp_dockbook_get_ui_manager (GimpDockbook *dockbook)
{
  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), NULL);

  return dockbook->p->ui_manager;
}

void
gimp_dockbook_add (GimpDockbook *dockbook,
                   GimpDockable *dockable,
                   gint          position)
{
  GtkWidget *tab_widget;
  GtkWidget *menu_widget;

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dockbook->p->dock != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (gimp_dockable_get_dockbook (dockable) == NULL);

  GIMP_LOG (DND, "Adding GimpDockable %p to GimpDockbook %p", dockable, dockbook);

  tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);

  g_return_if_fail (GTK_IS_WIDGET (tab_widget));

  /* For the notebook right-click menu, always use the icon style */
  menu_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     gimp_dock_get_context (dockbook->p->dock),
                                     GIMP_TAB_STYLE_ICON_BLURB,
                                     GTK_ICON_SIZE_MENU);

  g_return_if_fail (GTK_IS_WIDGET (menu_widget));

  gimp_dockable_set_drag_handler (dockable, dockbook->p->drag_handler);

  if (position == -1)
    {
      gtk_notebook_append_page_menu (GTK_NOTEBOOK (dockbook),
                                     GTK_WIDGET (dockable),
                                     tab_widget,
                                     menu_widget);
    }
  else
    {
      gtk_notebook_insert_page_menu (GTK_NOTEBOOK (dockbook),
                                     GTK_WIDGET (dockable),
                                     tab_widget,
                                     menu_widget,
                                     position);
    }

  gtk_widget_show (GTK_WIDGET (dockable));

  gimp_dockable_set_dockbook (dockable, dockbook);

  gimp_dockable_set_context (dockable, gimp_dock_get_context (dockbook->p->dock));

  g_signal_connect (dockable, "notify::locked",
                    G_CALLBACK (gimp_dockbook_tab_locked_notify),
                    dockbook);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_ADDED], 0, dockable);
}

/**
 * gimp_dockbook_add_from_dialog_factory:
 * @dockbook:    The #DockBook
 * @identifiers: The dockable identifier(s)
 * @position:    The insert position
 *
 * Add a dockable from the dialog factory associated with the dockbook.
 **/
GtkWidget *
gimp_dockbook_add_from_dialog_factory (GimpDockbook *dockbook,
                                       const gchar  *identifiers,
                                       gint          position)
{
  GtkWidget *dockable;
  GimpDock  *dock;
  gchar     *identifier;
  gchar     *p;

  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  identifier = g_strdup (identifiers);

  p = strchr (identifier, '|');

  if (p)
    *p = '\0';

  dock     = gimp_dockbook_get_dock (dockbook);
  dockable = gimp_dialog_factory_dockable_new (gimp_dock_get_dialog_factory (dock),
                                               dock,
                                               identifier, -1);

  g_free (identifier);

  /*  Maybe gimp_dialog_factory_dockable_new() returned an already
   *  existing singleton dockable, so check if it already is
   *  attached to a dockbook.
   */
  if (dockable && ! gimp_dockable_get_dockbook (GIMP_DOCKABLE (dockable)))
    gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), position);

  return dockable;
}

void
gimp_dockbook_remove (GimpDockbook *dockbook,
                      GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (gimp_dockable_get_dockbook (dockable) == dockbook);

  GIMP_LOG (DND, "Removing GimpDockable %p from GimpDockbook %p", dockable, dockbook);

  gimp_dockable_set_drag_handler (dockable, NULL);

  g_object_ref (dockable);

  g_signal_handlers_disconnect_by_func (dockable,
                                        G_CALLBACK (gimp_dockbook_tab_locked_notify),
                                        dockbook);

  if (dockbook->p->tab_hover_dockable == dockable)
    gimp_dockbook_remove_tab_timeout (dockbook);

  gimp_dockable_set_dockbook (dockable, NULL);

  gimp_dockable_set_context (dockable, NULL);

  gtk_container_remove (GTK_CONTAINER (dockbook), GTK_WIDGET (dockable));

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  g_object_unref (dockable);

  if (dockbook->p->dock)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      if (! children)
        gimp_dock_remove_book (dockbook->p->dock, dockbook);

      g_list_free (children);
    }
}

/**
 * gimp_dockbook_update_with_context:
 * @dockbook:
 * @context:
 *
 * Set @context on all dockables in @dockbook.
 **/
void
gimp_dockbook_update_with_context (GimpDockbook *dockbook,
                                   GimpContext  *context)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  GList *iter;

  for (iter = children; iter; iter = g_list_next (iter))
    {
      GimpDockable *dockable = GIMP_DOCKABLE (iter->data);

      gimp_dockable_set_context (dockable, context);
    }

  g_list_free (children);
}

GtkWidget *
gimp_dockbook_create_tab_widget (GimpDockbook *dockbook,
                                 GimpDockable *dockable)
{
  GtkWidget      *tab_widget;
  GimpDockWindow *dock_window;
  GtkAction      *action = NULL;

  tab_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     gimp_dock_get_context (dockbook->p->dock),
                                     gimp_dockable_get_tab_style (dockable),
                                     gimp_dockbook_get_tab_icon_size (dockbook));

  if (! GIMP_IS_VIEW (tab_widget))
    {
      GtkWidget *event_box;

      event_box = gtk_event_box_new ();
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), TRUE);
      gtk_container_add (GTK_CONTAINER (event_box), tab_widget);
      gtk_widget_show (tab_widget);

      tab_widget = event_box;
    }

  /* EEK */
  dock_window = gimp_dock_window_from_dock (dockbook->p->dock);
  if (dock_window &&
      gimp_dock_container_get_ui_manager (GIMP_DOCK_CONTAINER (dock_window)))
    {
      const gchar *dialog_id;

      dialog_id = g_object_get_data (G_OBJECT (dockable),
                                     "gimp-dialog-identifier");

      if (dialog_id)
        {
          GimpDockContainer *dock_container;
          GimpActionGroup   *group;

          dock_container = GIMP_DOCK_CONTAINER (dock_window);

          group = gimp_ui_manager_get_action_group
            (gimp_dock_container_get_ui_manager (dock_container), "dialogs");

          if (group)
            {
              GList *actions;
              GList *list;

              actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));

              for (list = actions; list; list = g_list_next (list))
                {
                  if (GIMP_IS_STRING_ACTION (list->data) &&
                      strstr (GIMP_STRING_ACTION (list->data)->value,
                              dialog_id))
                    {
                      action = list->data;
                      break;
                    }
                }

              g_list_free (actions);
            }
        }
    }

  if (action)
    gimp_widget_set_accel_help (tab_widget, action);
  else
    gimp_help_set_help_data (tab_widget,
                             gimp_dockable_get_blurb (dockable),
                             gimp_dockable_get_help_id (dockable));

  g_object_set_data (G_OBJECT (tab_widget), "gimp-dockable", dockable);

  gimp_dockbook_tab_drag_source_setup (tab_widget, dockable);

  g_signal_connect_object (tab_widget, "drag-begin",
                           G_CALLBACK (gimp_dockbook_tab_drag_begin),
                           dockable, 0);
  g_signal_connect_object (tab_widget, "drag-end",
                           G_CALLBACK (gimp_dockbook_tab_drag_end),
                           dockable, 0);

  g_signal_connect_object (dockable, "drag-begin",
                           G_CALLBACK (gimp_dockbook_tab_drag_begin),
                           dockable, 0);
  g_signal_connect_object (dockable, "drag-end",
                           G_CALLBACK (gimp_dockbook_tab_drag_end),
                           dockable, 0);

  gtk_drag_dest_set (tab_widget,
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
  g_signal_connect_object (tab_widget, "drag-leave",
                           G_CALLBACK (gimp_dockbook_tab_drag_leave),
                           dockable, 0);
  g_signal_connect_object (tab_widget, "drag-motion",
                           G_CALLBACK (gimp_dockbook_tab_drag_motion),
                           dockable, 0);
  g_signal_connect_object (tab_widget, "drag-drop",
                           G_CALLBACK (gimp_dockbook_tab_drag_drop),
                           dockbook, 0);

  return tab_widget;
}

gboolean
gimp_dockbook_drop_dockable (GimpDockbook *dockbook,
                             GtkWidget    *drag_source)
{
  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), FALSE);

  if (drag_source)
    {
      GimpDockable *dockable =
        gimp_dockbook_drag_source_to_dockable (drag_source);

      if (dockable)
        {
          if (gimp_dockable_get_dockbook (dockable) == dockbook)
            {
              gtk_notebook_reorder_child (GTK_NOTEBOOK (dockbook),
                                          GTK_WIDGET (dockable), -1);
            }
          else
            {
              g_object_ref (dockable);

              gimp_dockbook_remove (gimp_dockable_get_dockbook (dockable), dockable);
              gimp_dockbook_add (dockbook, dockable, -1);

              g_object_unref (dockable);
            }

          return TRUE;
        }
    }

  return FALSE;
}

/**
 * gimp_dockable_set_drag_handler:
 * @dockable:
 * @handler:
 *
 * Set a drag handler that will be asked if it will handle drag events
 * before the dockbook handles the event itself.
 **/
void
gimp_dockbook_set_drag_handler (GimpDockbook *dockbook,
                                GimpPanedBox *drag_handler)
{
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  dockbook->p->drag_handler = drag_handler;
}

/**
 * gimp_dockbook_drag_source_to_dockable:
 * @drag_source: A drag-and-drop source widget
 *
 * Gets the dockable associated with a drag-and-drop source. If
 * successful, the function will also cleanup the dockable.
 *
 * Returns: The dockable
 **/
GimpDockable *
gimp_dockbook_drag_source_to_dockable (GtkWidget *drag_source)
{
  GimpDockable *dockable = NULL;

  if (GIMP_IS_DOCKABLE (drag_source))
    dockable = GIMP_DOCKABLE (drag_source);
  else
    dockable = g_object_get_data (G_OBJECT (drag_source),
                                  "gimp-dockable");

  if (dockable)
    g_object_set_data (G_OBJECT (dockable),
                       "gimp-dock-drag-widget", NULL);

  return dockable;
}

/*  tab DND source side  */

static void
gimp_dockbook_tab_drag_source_setup (GtkWidget    *widget,
                                     GimpDockable *dockable)
{
  if (gimp_dockable_is_locked (dockable))
    {
      if (widget)
        gtk_drag_source_unset (widget);

      gtk_drag_source_unset (GTK_WIDGET (dockable));
    }
  else
    {
      if (widget)
        gtk_drag_source_set (widget,
                             GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                             dialog_target_table,
                             G_N_ELEMENTS (dialog_target_table),
                             GDK_ACTION_MOVE);

      gtk_drag_source_set (GTK_WIDGET (dockable),
                           GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                           dialog_target_table,
                           G_N_ELEMENTS (dialog_target_table),
                           GDK_ACTION_MOVE);
    }
}

static void
gimp_dockbook_tab_drag_begin (GtkWidget      *widget,
                              GdkDragContext *context,
                              GimpDockable   *dockable)
{
  GtkAllocation   allocation;
  GtkWidget      *window;
  GtkWidget      *view;
  GtkRequisition  requisition;
  gint            drag_x;
  gint            drag_y;

  gtk_widget_get_allocation (widget, &allocation);

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (widget));

  view = gimp_dockable_create_drag_widget (dockable);
  gtk_container_add (GTK_CONTAINER (window), view);
  gtk_widget_show (view);

  gtk_widget_get_preferred_size (view, &requisition, NULL);

  if (requisition.width < allocation.width)
    gtk_widget_set_size_request (view, allocation.width, -1);

  gtk_widget_show (window);

  g_object_set_data_full (G_OBJECT (dockable), "gimp-dock-drag-widget",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

  gimp_dockable_get_drag_pos (dockable, &drag_x, &drag_y);
  gtk_drag_set_icon_widget (context, window, drag_x, drag_y);

  /*
   * Set the source dockable insensitive to give a visual clue that
   * it's the dockable that's being dragged around
   */
  gtk_widget_set_sensitive (GTK_WIDGET (dockable), FALSE);
}

static void
gimp_dockbook_tab_drag_end (GtkWidget      *widget,
                            GdkDragContext *context,
                            GimpDockable   *dockable)
{
  GtkWidget *drag_widget;

  drag_widget = g_object_get_data (G_OBJECT (dockable),
                                   "gimp-dock-drag-widget");

  /*  finding the drag_widget means the drop was not successful, so
   *  pop up a new dock and move the dockable there
   */
  if (drag_widget)
    {
      g_object_set_data (G_OBJECT (dockable), "gimp-dock-drag-widget", NULL);
      gimp_dockable_detach (dockable);
    }

  gimp_dockable_set_drag_pos (dockable,
                              GIMP_DOCKABLE_DRAG_OFFSET,
                              GIMP_DOCKABLE_DRAG_OFFSET);
  gtk_widget_set_sensitive (GTK_WIDGET (dockable), TRUE);
}


/*  tab DND target side  */

static void
gimp_dockbook_tab_drag_leave (GtkWidget      *widget,
                              GdkDragContext *context,
                              guint           time,
                              GimpDockable   *dockable)
{
  GimpDockbook *dockbook = gimp_dockable_get_dockbook (dockable);

  gimp_dockbook_remove_tab_timeout (dockbook);

  gimp_highlight_widget (widget, FALSE);
}

static gboolean
gimp_dockbook_tab_drag_motion (GtkWidget      *widget,
                               GdkDragContext *context,
                               gint            x,
                               gint            y,
                               guint           time,
                               GimpDockable   *dockable)
{
  GimpDockbook  *dockbook = gimp_dockable_get_dockbook (dockable);
  GtkTargetList *target_list;
  GdkAtom        target_atom;
  gboolean       handle;

  if (gimp_paned_box_will_handle_drag (dockbook->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      gdk_drag_status (context, 0, time);
      gimp_highlight_widget (widget, FALSE);

      return FALSE;
    }

  if (! dockbook->p->tab_hover_timeout ||
      dockbook->p->tab_hover_dockable != dockable)
    {
      gint page_num;

      gimp_dockbook_remove_tab_timeout (dockbook);

      page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
                                        GTK_WIDGET (dockable));

      if (page_num != gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook)))
        gimp_dockbook_add_tab_timeout (dockbook, dockable);
    }

  target_list = gtk_drag_dest_get_target_list (widget);
  target_atom = gtk_drag_dest_find_target (widget, context, target_list);

  handle = gtk_target_list_find (target_list, target_atom, NULL);

  gdk_drag_status (context, handle ? GDK_ACTION_MOVE : 0, time);
  gimp_highlight_widget (widget, handle);

  /* Return TRUE so drag_leave() is called */
  return TRUE;
}

static gboolean
gimp_dockbook_tab_drag_drop (GtkWidget      *widget,
                             GdkDragContext *context,
                             gint            x,
                             gint            y,
                             guint           time)
{
  GimpDockable *dest_dockable;
  GtkWidget    *source;
  gboolean      dropped = FALSE;

  dest_dockable = g_object_get_data (G_OBJECT (widget), "gimp-dockable");

  source = gtk_drag_get_source_widget (context);

  if (gimp_paned_box_will_handle_drag (gimp_dockable_get_drag_handler (dest_dockable),
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  if (dest_dockable && source)
    {
      GimpDockable *src_dockable =
        gimp_dockbook_drag_source_to_dockable (source);

      if (src_dockable)
        {
          gint dest_index;

          dest_index =
            gtk_notebook_page_num (GTK_NOTEBOOK (gimp_dockable_get_dockbook (dest_dockable)),
                                   GTK_WIDGET (dest_dockable));

          if (gimp_dockable_get_dockbook (src_dockable) !=
              gimp_dockable_get_dockbook (dest_dockable))
            {
              g_object_ref (src_dockable);

              gimp_dockbook_remove (gimp_dockable_get_dockbook (src_dockable), src_dockable);
              gimp_dockbook_add (gimp_dockable_get_dockbook (dest_dockable), src_dockable,
                                 dest_index);

              g_object_unref (src_dockable);

              dropped = TRUE;
            }
          else if (src_dockable != dest_dockable)
            {
              gtk_notebook_reorder_child (GTK_NOTEBOOK (gimp_dockable_get_dockbook (src_dockable)),
                                          GTK_WIDGET (src_dockable),
                                          dest_index);

              g_signal_emit (gimp_dockable_get_dockbook (src_dockable),
                             dockbook_signals[DOCKABLE_REORDERED], 0,
                             src_dockable);

              dropped = TRUE;
            }
        }
    }

  gtk_drag_finish (context, dropped, TRUE, time);

  return TRUE;
}

static GtkIconSize
gimp_dockbook_get_tab_icon_size (GimpDockbook *dockbook)
{
  Gimp        *gimp;
  GimpIconSize size;
  GtkIconSize  tab_size = DEFAULT_TAB_ICON_SIZE;

  gimp = gimp_dock_get_context (dockbook->p->dock)->gimp;

  size = gimp_gui_config_detect_icon_size (GIMP_GUI_CONFIG (gimp->config));
  /* Match GimpIconSize with GtkIconSize. */
  switch (size)
    {
    case GIMP_ICON_SIZE_SMALL:
    case GIMP_ICON_SIZE_MEDIUM:
      tab_size = GTK_ICON_SIZE_MENU;
      break;
    case GIMP_ICON_SIZE_LARGE:
      tab_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
      break;
    case GIMP_ICON_SIZE_HUGE:
      tab_size = GTK_ICON_SIZE_DND;
      break;
    default:
      /* GIMP_ICON_SIZE_DEFAULT:
       * let's use the size set by the theme. */
      gtk_widget_style_get (GTK_WIDGET (dockbook),
                            "tab-icon-size", &tab_size,
                            NULL);
      break;
    }

  return tab_size;
}

static void
gimp_dockbook_add_tab_timeout (GimpDockbook *dockbook,
                               GimpDockable *dockable)
{
  dockbook->p->tab_hover_timeout =
    g_timeout_add (TAB_HOVER_TIMEOUT,
                   (GSourceFunc) gimp_dockbook_tab_timeout,
                   dockbook);

  dockbook->p->tab_hover_dockable = dockable;
}

static void
gimp_dockbook_remove_tab_timeout (GimpDockbook *dockbook)
{
  if (dockbook->p->tab_hover_timeout)
    {
      g_source_remove (dockbook->p->tab_hover_timeout);
      dockbook->p->tab_hover_timeout  = 0;
      dockbook->p->tab_hover_dockable = NULL;
    }
}

static gboolean
gimp_dockbook_tab_timeout (GimpDockbook *dockbook)
{
  gint page_num;

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockbook->p->tab_hover_dockable));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), page_num);

  dockbook->p->tab_hover_timeout  = 0;
  dockbook->p->tab_hover_dockable = NULL;

  return FALSE;
}

static void
gimp_dockbook_tab_locked_notify (GimpDockable *dockable,
                                 GParamSpec   *pspec,
                                 GimpDockbook *dockbook)
{
  GtkWidget *tab_widget;

  tab_widget = gtk_notebook_get_tab_label (GTK_NOTEBOOK (dockbook),
                                           GTK_WIDGET (dockable));

  gimp_dockbook_tab_drag_source_setup (tab_widget, dockable);
}

static void
gimp_dockbook_help_func (const gchar *help_id,
                         gpointer     help_data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (help_data);
  GtkWidget    *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (GIMP_IS_DOCKABLE (dockable))
    gimp_standard_help_func (gimp_dockable_get_help_id (GIMP_DOCKABLE (dockable)),
                             NULL);
  else
    gimp_standard_help_func (GIMP_HELP_DOCK, NULL);
}
