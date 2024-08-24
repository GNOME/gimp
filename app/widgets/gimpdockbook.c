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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "menus/menus.h"

#include "gimpactiongroup.h"
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


typedef struct
{
  GimpDockbookDragCallback callback;
  gpointer                 data;
} GimpDockbookDragCallbackData;

struct _GimpDockbookPrivate
{
  GimpDock       *dock;
  GimpUIManager  *ui_manager;

  guint           tab_hover_timeout;
  GimpDockable   *tab_hover_dockable;

  GimpPanedBox   *drag_handler;

  GtkWidget      *menu_button;
};


static void         gimp_dockbook_style_updated     (GtkWidget       *widget);
static void         gimp_dockbook_drag_begin        (GtkWidget      *widget,
                                                     GdkDragContext *context);
static void         gimp_dockbook_drag_end          (GtkWidget      *widget,
                                                     GdkDragContext *context);
static gboolean     gimp_dockbook_drag_motion       (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     gimp_dockbook_drag_drop         (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     gimp_dockbook_popup_menu        (GtkWidget      *widget);

static GtkNotebook *gimp_dockbook_create_window     (GtkNotebook    *notebook,
                                                     GtkWidget      *page,
                                                     gint            x,
                                                     gint            y);
static void         gimp_dockbook_page_added        (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         gimp_dockbook_page_removed      (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         gimp_dockbook_page_reordered    (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);

static gboolean     gimp_dockbook_tab_scroll_cb     (GtkWidget      *widget,
                                                     GdkEventScroll *event);

static gboolean     gimp_dockbook_menu_button_press (GimpDockbook   *dockbook,
                                                     GdkEventButton *bevent,
                                                     GtkWidget      *button);
static gboolean     gimp_dockbook_show_menu         (GimpDockbook   *dockbook);
static void         gimp_dockbook_menu_end          (GimpDockable   *dockable);
static void         gimp_dockbook_tab_locked_notify (GimpDockable   *dockable,
                                                     GParamSpec     *pspec,
                                                     GimpDockbook   *dockbook);

static void         gimp_dockbook_help_func         (const gchar    *help_id,
                                                     gpointer        help_data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDockbook, gimp_dockbook, GTK_TYPE_NOTEBOOK)

#define parent_class gimp_dockbook_parent_class

static guint dockbook_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_NOTEBOOK_TAB };

static GList *drag_callbacks = NULL;


static void
gimp_dockbook_class_init (GimpDockbookClass *klass)
{
  GtkWidgetClass   *widget_class   = GTK_WIDGET_CLASS (klass);
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

  dockbook_signals[DOCKABLE_ADDED] =
    g_signal_new ("dockable-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REMOVED] =
    g_signal_new ("dockable-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REORDERED] =
    g_signal_new ("dockable-reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockbookClass, dockable_reordered),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKABLE);

  widget_class->style_updated    = gimp_dockbook_style_updated;
  widget_class->drag_begin       = gimp_dockbook_drag_begin;
  widget_class->drag_end         = gimp_dockbook_drag_end;
  widget_class->drag_motion      = gimp_dockbook_drag_motion;
  widget_class->drag_drop        = gimp_dockbook_drag_drop;
  widget_class->popup_menu       = gimp_dockbook_popup_menu;

  notebook_class->create_window  = gimp_dockbook_create_window;
  notebook_class->page_added     = gimp_dockbook_page_added;
  notebook_class->page_removed   = gimp_dockbook_page_removed;
  notebook_class->page_reordered = gimp_dockbook_page_reordered;

  klass->dockable_added          = NULL;
  klass->dockable_removed        = NULL;
  klass->dockable_reordered      = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              GIMP_PARAM_READABLE));
}

static void
gimp_dockbook_init (GimpDockbook *dockbook)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (dockbook);
  GtkWidget   *image;

  dockbook->p = gimp_dockbook_get_instance_private (dockbook);

  /* Various init */
  gtk_notebook_popup_enable (notebook);
  gtk_notebook_set_scrollable (notebook, TRUE);
  gtk_notebook_set_show_border (notebook, FALSE);
  gtk_notebook_set_show_tabs (notebook, TRUE);
  gtk_notebook_set_group_name (notebook, "gimp-dockbook");

  gtk_widget_add_events (GTK_WIDGET (notebook),
                         GDK_SCROLL_MASK);
  g_signal_connect (GTK_WIDGET (notebook), "scroll-event",
                    G_CALLBACK (gimp_dockbook_tab_scroll_cb),
                    NULL);

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
gimp_dockbook_style_updated (GtkWidget *widget)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (widget);
  GimpContext  *context;
  GtkWidget    *tab_widget;
  GList        *children;
  GList        *iter;
  GtkIconSize   tab_size = DEFAULT_TAB_ICON_SIZE;
  gint          icon_size = 12;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! dockbook->p->dock ||
      ! (context = gimp_dock_get_context (dockbook->p->dock)))
    return;

  /* Update size of 'Configure this tab' icon */
  children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  for (iter = children; iter; iter = g_list_next (iter))
    {
      GimpDockable *dockable = GIMP_DOCKABLE (iter->data);

      tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);
      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
  g_list_free (children);

  children = gtk_container_get_children (GTK_CONTAINER (dockbook->p->menu_button));
  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);
  gtk_icon_size_lookup (tab_size, &icon_size, NULL);
  gtk_image_set_pixel_size (GTK_IMAGE (children->data), icon_size * 0.75f);
  g_list_free (children);

  gimp_dock_invalidate_geometry (GIMP_DOCK (dockbook->p->dock));
}

static void
gimp_dockbook_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context)
{
  GList *iter;

  if (GTK_WIDGET_CLASS (parent_class)->drag_begin)
    GTK_WIDGET_CLASS (parent_class)->drag_begin (widget, context);

  iter = drag_callbacks;

  while (iter)
    {
      GimpDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, TRUE, callback_data->data);
    }
}

static void
gimp_dockbook_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      GimpDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, FALSE, callback_data->data);
    }

  if (GTK_WIDGET_CLASS (parent_class)->drag_end)
    GTK_WIDGET_CLASS (parent_class)->drag_end (widget, context);
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

      return FALSE;
    }

  return GTK_WIDGET_CLASS (parent_class)->drag_motion (widget, context,
                                                       x, y, time);
}

static gboolean
gimp_dockbook_drag_drop (GtkWidget      *widget,
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
      return FALSE;
    }

  if (widget == gtk_drag_get_source_widget (context))
    {
      GList *children   = gtk_container_get_children (GTK_CONTAINER (widget));
      gint   n_children = g_list_length (children);

      g_list_free (children);

      /* we dragged the only tab, and want to drop it back on the same
       * notebook, this would remove and add the page, causing the
       * dockbook to be destroyed in the process because it becomes
       * empty
       */
      if (n_children == 1)
        return FALSE;
    }

  return GTK_WIDGET_CLASS (parent_class)->drag_drop (widget, context,
                                                     x, y, time);
}

static gboolean
gimp_dockbook_popup_menu (GtkWidget *widget)
{
  return gimp_dockbook_show_menu (GIMP_DOCKBOOK (widget));
}

static GtkNotebook *
gimp_dockbook_create_window (GtkNotebook *notebook,
                             GtkWidget   *page,
                             gint         x,
                             gint         y)
{
  GimpDockbook      *dockbook = GIMP_DOCKBOOK (notebook);
  GimpDialogFactory *dialog_factory;
  GimpMenuFactory   *menu_factory;
  GimpDockWindow    *src_dock_window;
  GimpDock          *src_dock;
  GtkWidget         *new_dock;
  GimpDockWindow    *new_dock_window;
  GtkWidget         *new_dockbook;

  src_dock        = gimp_dockbook_get_dock (dockbook);
  src_dock_window = gimp_dock_window_from_dock (src_dock);

  dialog_factory = gimp_dock_get_dialog_factory (src_dock);
  menu_factory   = menus_get_global_menu_factory (gimp_dialog_factory_get_context (dialog_factory)->gimp);

  new_dock = gimp_dock_with_window_new (dialog_factory,
                                        gimp_widget_get_monitor (page),
                                        FALSE);

  new_dock_window = gimp_dock_window_from_dock (GIMP_DOCK (new_dock));
  gtk_window_set_position (GTK_WINDOW (new_dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    gimp_dock_window_setup (new_dock_window, src_dock_window);

  new_dockbook = gimp_dockbook_new (menu_factory);

  gimp_dock_add_book (GIMP_DOCK (new_dock), GIMP_DOCKBOOK (new_dockbook), 0);

  gtk_widget_show (GTK_WIDGET (new_dock_window));
  gtk_widget_show (new_dock);

  return GTK_NOTEBOOK (new_dockbook);
}

static void
gimp_dockbook_page_added (GtkNotebook *notebook,
                          GtkWidget   *child,
                          guint        page_num)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (notebook);
  GimpDockable *dockable = GIMP_DOCKABLE (child);
  GtkWidget    *tab_widget;
  GtkWidget    *menu_widget;

  GIMP_LOG (DND, "GimpDockable %p added to GimpDockbook %p",
            dockable, dockbook);

  tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);

  /* For the notebook right-click menu, always use the icon style */
  menu_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     gimp_dock_get_context (dockbook->p->dock),
                                     GIMP_TAB_STYLE_ICON_BLURB,
                                     GTK_ICON_SIZE_MENU);

  gtk_notebook_set_tab_label (notebook, child, tab_widget);
  gtk_notebook_set_menu_label (notebook, child, menu_widget);

  if (! gimp_dockable_get_locked (dockable))
    {
      gtk_notebook_set_tab_reorderable (notebook, child, TRUE);
      gtk_notebook_set_tab_detachable (notebook, child, TRUE);
    }

  gimp_dockable_set_dockbook (dockable, dockbook);

  gimp_dockable_set_context (dockable,
                             gimp_dock_get_context (dockbook->p->dock));

  g_signal_connect (dockable, "notify::locked",
                    G_CALLBACK (gimp_dockbook_tab_locked_notify),
                    dockbook);

  gtk_widget_show (child);
  gtk_notebook_set_current_page (notebook, page_num);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_ADDED], 0, dockable);
}

static void
gimp_dockbook_page_removed (GtkNotebook *notebook,
                            GtkWidget   *child,
                            guint        page_num)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (notebook);
  GimpDockable *dockable = GIMP_DOCKABLE (child);

  GIMP_LOG (DND, "GimpDockable removed %p from GimpDockbook %p",
            dockable, dockbook);

  g_signal_handlers_disconnect_by_func (dockable,
                                        G_CALLBACK (gimp_dockbook_tab_locked_notify),
                                        dockbook);

  gimp_dockable_set_dockbook (dockable, NULL);

  gimp_dockable_set_context (dockable, NULL);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  if (dockbook->p->dock)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      if (! children)
        gimp_dock_remove_book (dockbook->p->dock, dockbook);

      g_list_free (children);
    }
}

/* Restore GTK2 behavior of mouse-scrolling to switch between
 * notebook tabs. References Geany's notebook_tab_bar_click_cb ()
 * at https://github.com/geany/geany/blob/master/src/notebook.c
 */
static gboolean
gimp_dockbook_tab_scroll_cb (GtkWidget      *widget,
                             GdkEventScroll *event)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (widget);
  GtkWidget   *page     = NULL;

  page = gtk_notebook_get_nth_page (notebook,
                                    gtk_notebook_get_current_page (notebook));
  if (! page)
    return FALSE;

  switch (event->direction)
    {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page (notebook);
      break;

    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page (notebook);
      break;

    default:
      break;
    }

  return TRUE;
}

static void
gimp_dockbook_page_reordered (GtkNotebook *notebook,
                              GtkWidget   *child,
                              guint        page_num)
{
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
  GimpDockable  *dockable;
  gint           page_num;

  dockbook_ui_manager = dockbook->p->ui_manager;

  if (! dockbook_ui_manager)
    return FALSE;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
  dockable = GIMP_DOCKABLE (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                       page_num));

  if (! dockable)
    return FALSE;

  dialog_ui_manager = gimp_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  /*  an action callback may destroy both dockable and dockbook, so
   *  reference them for gimp_dockbook_menu_end()
   */
  g_object_ref (dockable);
  g_object_set_data_full (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY,
                          g_object_ref (dockbook),
                          g_object_unref);

  if (dialog_ui_manager)
    gimp_ui_manager_update (dialog_ui_manager, dialog_popup_data);

  gimp_ui_manager_update (dockbook_ui_manager, dockable);

  gimp_ui_manager_ui_popup_at_widget (dockbook_ui_manager,
                                      "/dockable-popup",
                                      dialog_ui_manager,
                                      dialog_ui_path,
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
  /*  release gimp_dockbook_show_menu()'s references  */
  g_object_set_data (G_OBJECT (dockable), GIMP_DOCKABLE_DETACH_REF_KEY, NULL);
  g_object_unref (dockable);
}


/*  public functions  */

GtkWidget *
gimp_dockbook_new (GimpMenuFactory *menu_factory)
{
  GimpDockbook *dockbook;

  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (GIMP_TYPE_DOCKBOOK, NULL);

  dockbook->p->ui_manager = gimp_menu_factory_get_manager (menu_factory,
                                                           "<Dockable>",
                                                           dockbook);

  gimp_help_connect (GTK_WIDGET (dockbook), NULL, gimp_dockbook_help_func,
                     GIMP_HELP_DOCK, dockbook, NULL);

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
  GimpContext *context;

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dock == NULL || GIMP_IS_DOCK (dock));

  if (dockbook->p->dock &&
      (context = gimp_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_handlers_disconnect_by_func (GIMP_GUI_CONFIG (context->gimp->config),
                                            G_CALLBACK (gimp_dockbook_style_updated),
                                            dockbook);
    }

  dockbook->p->dock = dock;

  if (dockbook->p->dock &&
      (context = gimp_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_connect_object (GIMP_GUI_CONFIG (context->gimp->config),
                               "notify::theme",
                               G_CALLBACK (gimp_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (GIMP_GUI_CONFIG (context->gimp->config),
                               "notify::override-theme-icon-size",
                               G_CALLBACK (gimp_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (GIMP_GUI_CONFIG (context->gimp->config),
                               "notify::custom-icon-size",
                               G_CALLBACK (gimp_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }
}

/**
 * gimp_dockbook_add_from_dialog_factory:
 * @dockbook:    The #DockBook
 * @identifiers: The dockable identifier(s)
 *
 * Add a dockable from the dialog factory associated with the dockbook.
 **/
GtkWidget *
gimp_dockbook_add_from_dialog_factory (GimpDockbook *dockbook,
                                       const gchar  *identifiers)
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
    gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                              dockable, NULL);

  return dockable;
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
  GimpAction     *action = NULL;
  GtkIconSize     tab_size = DEFAULT_TAB_ICON_SIZE;

  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);
  tab_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     gimp_dock_get_context (dockbook->p->dock),
                                     gimp_dockable_get_tab_style (dockable),
                                     tab_size);

  if (GIMP_IS_VIEW (tab_widget))
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

              actions = gimp_action_group_list_actions (group);

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

  return tab_widget;
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

void
gimp_dockbook_add_drag_callback (GimpDockbookDragCallback callback,
                                 gpointer                 data)
{
  GimpDockbookDragCallbackData *callback_data;

  callback_data = g_slice_new (GimpDockbookDragCallbackData);

  callback_data->callback = callback;
  callback_data->data     = data;

  drag_callbacks = g_list_prepend (drag_callbacks, callback_data);
}

void
gimp_dockbook_remove_drag_callback (GimpDockbookDragCallback callback,
                                    gpointer                 data)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      GimpDockbookDragCallbackData *callback_data = iter->data;
      GList                        *next          = g_list_next (iter);

      if (callback_data->callback == callback &&
          callback_data->data     == data)
        {
          g_slice_free (GimpDockbookDragCallbackData, callback_data);

          drag_callbacks = g_list_delete_link (drag_callbacks, iter);
        }

      iter = next;
    }
}


/*  private functions  */

static void
gimp_dockbook_tab_locked_notify (GimpDockable *dockable,
                                 GParamSpec   *pspec,
                                 GimpDockbook *dockbook)
{
  gboolean locked = gimp_dockable_get_locked (dockable);

  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockable), ! locked);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (dockbook),
                                   GTK_WIDGET (dockable), ! locked);
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
