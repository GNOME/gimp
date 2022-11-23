/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockbook.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmaactiongroup.h"
#include "ligmadialogfactory.h"
#include "ligmadnd.h"
#include "ligmadock.h"
#include "ligmadockable.h"
#include "ligmadockbook.h"
#include "ligmadocked.h"
#include "ligmadockcontainer.h"
#include "ligmadockwindow.h"
#include "ligmahelp-ids.h"
#include "ligmamenufactory.h"
#include "ligmapanedbox.h"
#include "ligmastringaction.h"
#include "ligmauimanager.h"
#include "ligmaview.h"
#include "ligmawidgets-utils.h"

#include "ligma-log.h"
#include "ligma-intl.h"


#define DEFAULT_TAB_ICON_SIZE        GTK_ICON_SIZE_BUTTON
#define DND_WIDGET_ICON_SIZE         GTK_ICON_SIZE_BUTTON
#define MENU_WIDGET_SPACING          4
#define TAB_HOVER_TIMEOUT            500
#define LIGMA_DOCKABLE_DETACH_REF_KEY "ligma-dockable-detach-ref"


enum
{
  DOCKABLE_ADDED,
  DOCKABLE_REMOVED,
  DOCKABLE_REORDERED,
  LAST_SIGNAL
};


typedef struct
{
  LigmaDockbookDragCallback callback;
  gpointer                 data;
} LigmaDockbookDragCallbackData;

struct _LigmaDockbookPrivate
{
  LigmaDock       *dock;
  LigmaUIManager  *ui_manager;

  guint           tab_hover_timeout;
  LigmaDockable   *tab_hover_dockable;

  LigmaPanedBox   *drag_handler;

  GtkWidget      *menu_button;
};


static void         ligma_dockbook_finalize          (GObject        *object);

static void         ligma_dockbook_style_updated     (GtkWidget       *widget);
static void         ligma_dockbook_drag_begin        (GtkWidget      *widget,
                                                     GdkDragContext *context);
static void         ligma_dockbook_drag_end          (GtkWidget      *widget,
                                                     GdkDragContext *context);
static gboolean     ligma_dockbook_drag_motion       (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     ligma_dockbook_drag_drop         (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     ligma_dockbook_popup_menu        (GtkWidget      *widget);

static GtkNotebook *ligma_dockbook_create_window     (GtkNotebook    *notebook,
                                                     GtkWidget      *page,
                                                     gint            x,
                                                     gint            y);
static void         ligma_dockbook_page_added        (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         ligma_dockbook_page_removed      (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         ligma_dockbook_page_reordered    (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);

static gboolean     ligma_dockbook_menu_button_press (LigmaDockbook   *dockbook,
                                                     GdkEventButton *bevent,
                                                     GtkWidget      *button);
static gboolean     ligma_dockbook_show_menu         (LigmaDockbook   *dockbook);
static void         ligma_dockbook_menu_end          (LigmaDockable   *dockable);
static void         ligma_dockbook_tab_locked_notify (LigmaDockable   *dockable,
                                                     GParamSpec     *pspec,
                                                     LigmaDockbook   *dockbook);

static void         ligma_dockbook_help_func         (const gchar    *help_id,
                                                     gpointer        help_data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDockbook, ligma_dockbook, GTK_TYPE_NOTEBOOK)

#define parent_class ligma_dockbook_parent_class

static guint dockbook_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry dialog_target_table[] = { LIGMA_TARGET_NOTEBOOK_TAB };

static GList *drag_callbacks = NULL;


static void
ligma_dockbook_class_init (LigmaDockbookClass *klass)
{
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class   = GTK_WIDGET_CLASS (klass);
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

  dockbook_signals[DOCKABLE_ADDED] =
    g_signal_new ("dockable-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockbookClass, dockable_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REMOVED] =
    g_signal_new ("dockable-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockbookClass, dockable_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REORDERED] =
    g_signal_new ("dockable-reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDockbookClass, dockable_reordered),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DOCKABLE);

  object_class->finalize         = ligma_dockbook_finalize;

  widget_class->style_updated    = ligma_dockbook_style_updated;
  widget_class->drag_begin       = ligma_dockbook_drag_begin;
  widget_class->drag_end         = ligma_dockbook_drag_end;
  widget_class->drag_motion      = ligma_dockbook_drag_motion;
  widget_class->drag_drop        = ligma_dockbook_drag_drop;
  widget_class->popup_menu       = ligma_dockbook_popup_menu;

  notebook_class->create_window  = ligma_dockbook_create_window;
  notebook_class->page_added     = ligma_dockbook_page_added;
  notebook_class->page_removed   = ligma_dockbook_page_removed;
  notebook_class->page_reordered = ligma_dockbook_page_reordered;

  klass->dockable_added          = NULL;
  klass->dockable_removed        = NULL;
  klass->dockable_reordered      = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              LIGMA_PARAM_READABLE));
}

static void
ligma_dockbook_init (LigmaDockbook *dockbook)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (dockbook);
  GtkWidget   *image;

  dockbook->p = ligma_dockbook_get_instance_private (dockbook);

  /* Various init */
  gtk_notebook_popup_enable (notebook);
  gtk_notebook_set_scrollable (notebook, TRUE);
  gtk_notebook_set_show_border (notebook, FALSE);
  gtk_notebook_set_show_tabs (notebook, TRUE);
  gtk_notebook_set_group_name (notebook, "ligma-dockbook");

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

  image = gtk_image_new_from_icon_name (LIGMA_ICON_MENU_LEFT,
                                        GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
  gtk_image_set_from_icon_name (GTK_IMAGE (image), LIGMA_ICON_MENU_LEFT,
                                GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockbook->p->menu_button), image);
  gtk_widget_show (image);

  ligma_help_set_help_data (dockbook->p->menu_button, _("Configure this tab"),
                           LIGMA_HELP_DOCK_TAB_MENU);

  g_signal_connect_swapped (dockbook->p->menu_button, "button-press-event",
                            G_CALLBACK (ligma_dockbook_menu_button_press),
                            dockbook);
}

static void
ligma_dockbook_finalize (GObject *object)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (object);

  g_clear_object (&dockbook->p->ui_manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_dockbook_style_updated (GtkWidget *widget)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (widget);
  LigmaContext  *context;
  GtkWidget    *tab_widget;
  GList        *children;
  GList        *iter;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! dockbook->p->dock ||
      ! (context = ligma_dock_get_context (dockbook->p->dock)))
    return;

  children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  for (iter = children; iter; iter = g_list_next (iter))
    {
      LigmaDockable *dockable = LIGMA_DOCKABLE (iter->data);

      tab_widget = ligma_dockbook_create_tab_widget (dockbook, dockable);
      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
  g_list_free (children);

  ligma_dock_invalidate_geometry (LIGMA_DOCK (dockbook->p->dock));
}

static void
ligma_dockbook_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context)
{
  GList *iter;

  if (GTK_WIDGET_CLASS (parent_class)->drag_begin)
    GTK_WIDGET_CLASS (parent_class)->drag_begin (widget, context);

  iter = drag_callbacks;

  while (iter)
    {
      LigmaDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, TRUE, callback_data->data);
    }
}

static void
ligma_dockbook_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      LigmaDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, FALSE, callback_data->data);
    }

  if (GTK_WIDGET_CLASS (parent_class)->drag_end)
    GTK_WIDGET_CLASS (parent_class)->drag_end (widget, context);
}

static gboolean
ligma_dockbook_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (widget);

  if (ligma_paned_box_will_handle_drag (dockbook->p->drag_handler,
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
ligma_dockbook_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (widget);

  if (ligma_paned_box_will_handle_drag (dockbook->p->drag_handler,
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
ligma_dockbook_popup_menu (GtkWidget *widget)
{
  return ligma_dockbook_show_menu (LIGMA_DOCKBOOK (widget));
}

static GtkNotebook *
ligma_dockbook_create_window (GtkNotebook *notebook,
                             GtkWidget   *page,
                             gint         x,
                             gint         y)
{
  LigmaDockbook      *dockbook = LIGMA_DOCKBOOK (notebook);
  LigmaDialogFactory *dialog_factory;
  LigmaMenuFactory   *menu_factory;
  LigmaDockWindow    *src_dock_window;
  LigmaDock          *src_dock;
  GtkWidget         *new_dock;
  LigmaDockWindow    *new_dock_window;
  GtkWidget         *new_dockbook;

  src_dock        = ligma_dockbook_get_dock (dockbook);
  src_dock_window = ligma_dock_window_from_dock (src_dock);

  dialog_factory = ligma_dock_get_dialog_factory (src_dock);
  menu_factory   = ligma_dialog_factory_get_menu_factory (dialog_factory);

  new_dock = ligma_dock_with_window_new (dialog_factory,
                                        ligma_widget_get_monitor (page),
                                        FALSE);

  new_dock_window = ligma_dock_window_from_dock (LIGMA_DOCK (new_dock));
  gtk_window_set_position (GTK_WINDOW (new_dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    ligma_dock_window_setup (new_dock_window, src_dock_window);

  new_dockbook = ligma_dockbook_new (menu_factory);

  ligma_dock_add_book (LIGMA_DOCK (new_dock), LIGMA_DOCKBOOK (new_dockbook), 0);

  gtk_widget_show (GTK_WIDGET (new_dock_window));
  gtk_widget_show (new_dock);

  return GTK_NOTEBOOK (new_dockbook);
}

static void
ligma_dockbook_page_added (GtkNotebook *notebook,
                          GtkWidget   *child,
                          guint        page_num)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (notebook);
  LigmaDockable *dockable = LIGMA_DOCKABLE (child);
  GtkWidget    *tab_widget;
  GtkWidget    *menu_widget;

  LIGMA_LOG (DND, "LigmaDockable %p added to LigmaDockbook %p",
            dockable, dockbook);

  tab_widget = ligma_dockbook_create_tab_widget (dockbook, dockable);

  /* For the notebook right-click menu, always use the icon style */
  menu_widget =
    ligma_dockable_create_tab_widget (dockable,
                                     ligma_dock_get_context (dockbook->p->dock),
                                     LIGMA_TAB_STYLE_ICON_BLURB,
                                     GTK_ICON_SIZE_MENU);

  gtk_notebook_set_tab_label (notebook, child, tab_widget);
  gtk_notebook_set_menu_label (notebook, child, menu_widget);

  if (! ligma_dockable_get_locked (dockable))
    {
      gtk_notebook_set_tab_reorderable (notebook, child, TRUE);
      gtk_notebook_set_tab_detachable (notebook, child, TRUE);
    }

  ligma_dockable_set_dockbook (dockable, dockbook);

  ligma_dockable_set_context (dockable,
                             ligma_dock_get_context (dockbook->p->dock));

  g_signal_connect (dockable, "notify::locked",
                    G_CALLBACK (ligma_dockbook_tab_locked_notify),
                    dockbook);

  gtk_widget_show (child);
  gtk_notebook_set_current_page (notebook, page_num);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_ADDED], 0, dockable);
}

static void
ligma_dockbook_page_removed (GtkNotebook *notebook,
                            GtkWidget   *child,
                            guint        page_num)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (notebook);
  LigmaDockable *dockable = LIGMA_DOCKABLE (child);

  LIGMA_LOG (DND, "LigmaDockable removed %p from LigmaDockbook %p",
            dockable, dockbook);

  g_signal_handlers_disconnect_by_func (dockable,
                                        G_CALLBACK (ligma_dockbook_tab_locked_notify),
                                        dockbook);

  ligma_dockable_set_dockbook (dockable, NULL);

  ligma_dockable_set_context (dockable, NULL);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  if (dockbook->p->dock)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      if (! children)
        ligma_dock_remove_book (dockbook->p->dock, dockbook);

      g_list_free (children);
    }
}

static void
ligma_dockbook_page_reordered (GtkNotebook *notebook,
                              GtkWidget   *child,
                              guint        page_num)
{
}

static gboolean
ligma_dockbook_menu_button_press (LigmaDockbook   *dockbook,
                                 GdkEventButton *bevent,
                                 GtkWidget      *button)
{
  gboolean handled = FALSE;

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    handled = ligma_dockbook_show_menu (dockbook);

  return handled;
}

static gboolean
ligma_dockbook_show_menu (LigmaDockbook *dockbook)
{
  LigmaUIManager *dockbook_ui_manager;
  LigmaUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;
  GtkWidget     *parent_menu_widget;
  LigmaAction    *parent_menu_action;
  LigmaDockable  *dockable;
  gint           page_num;

  dockbook_ui_manager = ligma_dockbook_get_ui_manager (dockbook);

  if (! dockbook_ui_manager)
    return FALSE;

  parent_menu_widget =
    ligma_ui_manager_get_widget (dockbook_ui_manager,
                                "/dockable-popup/dockable-menu");
  parent_menu_action =
    ligma_ui_manager_get_action (dockbook_ui_manager,
                                "/dockable-popup/dockable-menu");

  if (! parent_menu_widget || ! parent_menu_action)
    return FALSE;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
  dockable = LIGMA_DOCKABLE (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                       page_num));

  if (! dockable)
    return FALSE;

  dialog_ui_manager = ligma_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget  *child_menu_widget;
      LigmaAction *child_menu_action;
      gchar      *label;

      child_menu_widget =
        ligma_ui_manager_get_widget (dialog_ui_manager, dialog_ui_path);

      if (! child_menu_widget)
        {
          g_warning ("%s: UI manager '%s' has no widget at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      child_menu_action =
        ligma_ui_manager_get_action (dialog_ui_manager,
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
                    "icon-name", ligma_dockable_get_icon_name (dockable),
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
        GtkWidget *image = ligma_dockable_get_icon (dockable,
                                                   GTK_ICON_SIZE_MENU);
        ligma_menu_item_set_image (GTK_MENU_ITEM (parent_menu_widget), image);
        gtk_widget_show (image);
      }

      gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent_menu_widget),
                                 child_menu_widget);

      ligma_ui_manager_update (dialog_ui_manager, dialog_popup_data);
    }
  else
    {
      g_object_set (parent_menu_action, "visible", FALSE, NULL);
    }

  /*  an action callback may destroy both dockable and dockbook, so
   *  reference them for ligma_dockbook_menu_end()
   */
  g_object_ref (dockable);
  g_object_set_data_full (G_OBJECT (dockable), LIGMA_DOCKABLE_DETACH_REF_KEY,
                          g_object_ref (dockbook),
                          g_object_unref);

  ligma_ui_manager_update (dockbook_ui_manager, dockable);

  ligma_ui_manager_ui_popup_at_widget (dockbook_ui_manager,
                                      "/dockable-popup",
                                      dockbook->p->menu_button,
                                      GDK_GRAVITY_WEST,
                                      GDK_GRAVITY_NORTH_EAST,
                                      NULL,
                                      (GDestroyNotify) ligma_dockbook_menu_end,
                                      dockable);

  return TRUE;
}

static void
ligma_dockbook_menu_end (LigmaDockable *dockable)
{
  LigmaUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;

  dialog_ui_manager = ligma_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  if (dialog_ui_manager && dialog_ui_path)
    {
      GtkWidget *child_menu_widget =
        ligma_ui_manager_get_widget (dialog_ui_manager, dialog_ui_path);

      if (child_menu_widget)
        gtk_menu_detach (GTK_MENU (child_menu_widget));
    }

  /*  release ligma_dockbook_show_menu()'s references  */
  g_object_set_data (G_OBJECT (dockable), LIGMA_DOCKABLE_DETACH_REF_KEY, NULL);
  g_object_unref (dockable);
}


/*  public functions  */

GtkWidget *
ligma_dockbook_new (LigmaMenuFactory *menu_factory)
{
  LigmaDockbook *dockbook;

  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (LIGMA_TYPE_DOCKBOOK, NULL);

  dockbook->p->ui_manager = ligma_menu_factory_manager_new (menu_factory,
                                                           "<Dockable>",
                                                           dockbook);

  ligma_help_connect (GTK_WIDGET (dockbook), ligma_dockbook_help_func,
                     LIGMA_HELP_DOCK, dockbook, NULL);

  return GTK_WIDGET (dockbook);
}

LigmaDock *
ligma_dockbook_get_dock (LigmaDockbook *dockbook)
{
  g_return_val_if_fail (LIGMA_IS_DOCKBOOK (dockbook), NULL);

  return dockbook->p->dock;
}

void
ligma_dockbook_set_dock (LigmaDockbook *dockbook,
                        LigmaDock     *dock)
{
  LigmaContext *context;

  g_return_if_fail (LIGMA_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dock == NULL || LIGMA_IS_DOCK (dock));

  if (dockbook->p->dock &&
      (context = ligma_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_handlers_disconnect_by_func (LIGMA_GUI_CONFIG (context->ligma->config),
                                            G_CALLBACK (ligma_dockbook_style_updated),
                                            dockbook);
    }

  dockbook->p->dock = dock;

  if (dockbook->p->dock &&
      (context = ligma_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                               "notify::theme",
                               G_CALLBACK (ligma_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                               "notify::override-theme-icon-size",
                               G_CALLBACK (ligma_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (LIGMA_GUI_CONFIG (context->ligma->config),
                               "notify::custom-icon-size",
                               G_CALLBACK (ligma_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }
}

LigmaUIManager *
ligma_dockbook_get_ui_manager (LigmaDockbook *dockbook)
{
  g_return_val_if_fail (LIGMA_IS_DOCKBOOK (dockbook), NULL);

  return dockbook->p->ui_manager;
}

/**
 * ligma_dockbook_add_from_dialog_factory:
 * @dockbook:    The #DockBook
 * @identifiers: The dockable identifier(s)
 *
 * Add a dockable from the dialog factory associated with the dockbook.
 **/
GtkWidget *
ligma_dockbook_add_from_dialog_factory (LigmaDockbook *dockbook,
                                       const gchar  *identifiers)
{
  GtkWidget *dockable;
  LigmaDock  *dock;
  gchar     *identifier;
  gchar     *p;

  g_return_val_if_fail (LIGMA_IS_DOCKBOOK (dockbook), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  identifier = g_strdup (identifiers);

  p = strchr (identifier, '|');

  if (p)
    *p = '\0';

  dock     = ligma_dockbook_get_dock (dockbook);
  dockable = ligma_dialog_factory_dockable_new (ligma_dock_get_dialog_factory (dock),
                                               dock,
                                               identifier, -1);

  g_free (identifier);

  /*  Maybe ligma_dialog_factory_dockable_new() returned an already
   *  existing singleton dockable, so check if it already is
   *  attached to a dockbook.
   */
  if (dockable && ! ligma_dockable_get_dockbook (LIGMA_DOCKABLE (dockable)))
    gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                              dockable, NULL);

  return dockable;
}

/**
 * ligma_dockbook_update_with_context:
 * @dockbook:
 * @context:
 *
 * Set @context on all dockables in @dockbook.
 **/
void
ligma_dockbook_update_with_context (LigmaDockbook *dockbook,
                                   LigmaContext  *context)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  GList *iter;

  for (iter = children; iter; iter = g_list_next (iter))
    {
      LigmaDockable *dockable = LIGMA_DOCKABLE (iter->data);

      ligma_dockable_set_context (dockable, context);
    }

  g_list_free (children);
}

GtkWidget *
ligma_dockbook_create_tab_widget (LigmaDockbook *dockbook,
                                 LigmaDockable *dockable)
{
  GtkWidget      *tab_widget;
  LigmaDockWindow *dock_window;
  LigmaAction     *action = NULL;
  GtkIconSize     tab_size = DEFAULT_TAB_ICON_SIZE;

  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);
  tab_widget =
    ligma_dockable_create_tab_widget (dockable,
                                     ligma_dock_get_context (dockbook->p->dock),
                                     ligma_dockable_get_tab_style (dockable),
                                     tab_size);

  if (LIGMA_IS_VIEW (tab_widget))
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
  dock_window = ligma_dock_window_from_dock (dockbook->p->dock);
  if (dock_window &&
      ligma_dock_container_get_ui_manager (LIGMA_DOCK_CONTAINER (dock_window)))
    {
      const gchar *dialog_id;

      dialog_id = g_object_get_data (G_OBJECT (dockable),
                                     "ligma-dialog-identifier");

      if (dialog_id)
        {
          LigmaDockContainer *dock_container;
          LigmaActionGroup   *group;

          dock_container = LIGMA_DOCK_CONTAINER (dock_window);

          group = ligma_ui_manager_get_action_group
            (ligma_dock_container_get_ui_manager (dock_container), "dialogs");

          if (group)
            {
              GList *actions;
              GList *list;

              actions = ligma_action_group_list_actions (group);

              for (list = actions; list; list = g_list_next (list))
                {
                  if (LIGMA_IS_STRING_ACTION (list->data) &&
                      strstr (LIGMA_STRING_ACTION (list->data)->value,
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
    ligma_widget_set_accel_help (tab_widget, action);
  else
    ligma_help_set_help_data (tab_widget,
                             ligma_dockable_get_blurb (dockable),
                             ligma_dockable_get_help_id (dockable));

  return tab_widget;
}

/**
 * ligma_dockable_set_drag_handler:
 * @dockable:
 * @handler:
 *
 * Set a drag handler that will be asked if it will handle drag events
 * before the dockbook handles the event itself.
 **/
void
ligma_dockbook_set_drag_handler (LigmaDockbook *dockbook,
                                LigmaPanedBox *drag_handler)
{
  g_return_if_fail (LIGMA_IS_DOCKBOOK (dockbook));

  dockbook->p->drag_handler = drag_handler;
}

void
ligma_dockbook_add_drag_callback (LigmaDockbookDragCallback callback,
                                 gpointer                 data)
{
  LigmaDockbookDragCallbackData *callback_data;

  callback_data = g_slice_new (LigmaDockbookDragCallbackData);

  callback_data->callback = callback;
  callback_data->data     = data;

  drag_callbacks = g_list_prepend (drag_callbacks, callback_data);
}

void
ligma_dockbook_remove_drag_callback (LigmaDockbookDragCallback callback,
                                    gpointer                 data)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      LigmaDockbookDragCallbackData *callback_data = iter->data;
      GList                        *next          = g_list_next (iter);

      if (callback_data->callback == callback &&
          callback_data->data     == data)
        {
          g_slice_free (LigmaDockbookDragCallbackData, callback_data);

          drag_callbacks = g_list_delete_link (drag_callbacks, iter);
        }

      iter = next;
    }
}


/*  private functions  */

static void
ligma_dockbook_tab_locked_notify (LigmaDockable *dockable,
                                 GParamSpec   *pspec,
                                 LigmaDockbook *dockbook)
{
  gboolean locked = ligma_dockable_get_locked (dockable);

  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockable), ! locked);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (dockbook),
                                   GTK_WIDGET (dockable), ! locked);
}

static void
ligma_dockbook_help_func (const gchar *help_id,
                         gpointer     help_data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (help_data);
  GtkWidget    *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (LIGMA_IS_DOCKABLE (dockable))
    ligma_standard_help_func (ligma_dockable_get_help_id (LIGMA_DOCKABLE (dockable)),
                             NULL);
  else
    ligma_standard_help_func (LIGMA_HELP_DOCK, NULL);
}
