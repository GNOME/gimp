/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockbook.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdnd.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimphelp-ids.h"
#include "gimpimagedock.h"
#include "gimpmenufactory.h"
#include "gimpstringaction.h"
#include "gimpuimanager.h"
#include "gimpview.h"
#include "gimpwidgets-utils.h"


#define DEFAULT_TAB_BORDER     0
#define DEFAULT_TAB_ICON_SIZE  GTK_ICON_SIZE_BUTTON
#define DND_WIDGET_ICON_SIZE   GTK_ICON_SIZE_BUTTON
#define MENU_WIDGET_ICON_SIZE  GTK_ICON_SIZE_MENU
#define MENU_WIDGET_SPACING    4


enum
{
  DOCKABLE_ADDED,
  DOCKABLE_REMOVED,
  DOCKABLE_REORDERED,
  LAST_SIGNAL
};


static void      gimp_dockbook_finalize         (GObject        *object);

static void      gimp_dockbook_style_set        (GtkWidget      *widget,
                                                 GtkStyle       *prev_style);
static gboolean  gimp_dockbook_drag_drop        (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 gint            x,
                                                 gint            y,
                                                 guint           time);

static void      gimp_dockbook_dockable_added   (GimpDockbook   *dockbook,
                                                 GimpDockable   *dockable);
static void      gimp_dockbook_dockable_removed (GimpDockbook   *dockbook,
                                                 GimpDockable   *dockable);
static void      gimp_dockbook_update_tabs      (GimpDockbook   *dockbook,
                                                 gboolean        added);

static void      gimp_dockbook_tab_drag_begin   (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 GimpDockable   *dockable);
static void      gimp_dockbook_tab_drag_end     (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 GimpDockable   *dockable);
static gboolean  gimp_dockbook_tab_drag_drop    (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 gint            x,
                                                 gint            y,
                                                 guint           time);
static gboolean  gimp_dockbook_tab_drag_expose  (GtkWidget      *widget,
                                                 GdkEventExpose *event);

static void      gimp_dockbook_help_func        (const gchar    *help_id,
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

  object_class->finalize    = gimp_dockbook_finalize;

  widget_class->style_set   = gimp_dockbook_style_set;
  widget_class->drag_drop   = gimp_dockbook_drag_drop;

  klass->dockable_added     = gimp_dockbook_dockable_added;
  klass->dockable_removed   = gimp_dockbook_dockable_removed;
  klass->dockable_reordered = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("tab-border",
                                                             NULL, NULL,
                                                             0, G_MAXINT,
                                                             DEFAULT_TAB_BORDER,
                                                             GIMP_PARAM_READABLE));
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
  dockbook->dock       = NULL;
  dockbook->ui_manager = NULL;

  gtk_notebook_popup_enable (GTK_NOTEBOOK (dockbook));
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (dockbook), TRUE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (dockbook), FALSE);

  gtk_drag_dest_set (GTK_WIDGET (dockbook),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
gimp_dockbook_finalize (GObject *object)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (object);

  if (dockbook->ui_manager)
    {
      g_object_unref (dockbook->ui_manager);
      dockbook->ui_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dockbook_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  GList *children;
  GList *list;
  gint   tab_border;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "tab-border", &tab_border,
                        NULL);

  g_object_set (widget,
                "tab-border", tab_border,
                NULL);

  children = gtk_container_get_children (GTK_CONTAINER (widget));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *tab_widget;

      tab_widget = gimp_dockbook_get_tab_widget (GIMP_DOCKBOOK (widget),
                                                 GIMP_DOCKABLE (list->data));

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (widget),
                                  GTK_WIDGET (list->data),
                                  tab_widget);
    }

  g_list_free (children);
}

static gboolean
gimp_dockbook_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  return gimp_dockbook_drop_dockable (GIMP_DOCKBOOK (widget),
                                      gtk_drag_get_source_widget (context));
}

static void
gimp_dockbook_dockable_added (GimpDockbook *dockbook,
                              GimpDockable *dockable)
{
  gimp_dockbook_update_tabs (dockbook, TRUE);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                 gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
                                                        GTK_WIDGET (dockable)));
}

static void
gimp_dockbook_dockable_removed (GimpDockbook *dockbook,
                                GimpDockable *dockable)
{
  gimp_dockbook_update_tabs (dockbook, FALSE);
}

static void
gimp_dockbook_update_tabs (GimpDockbook *dockbook,
                           gboolean      added)
{
  switch (gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook)))
    {
    case 1:
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dockbook), FALSE);
      break;

    case 2:
      if (added)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dockbook), TRUE);
      break;

    default:
      break;
    }
}

GtkWidget *
gimp_dockbook_new (GimpMenuFactory *menu_factory)
{
  GimpDockbook *dockbook;

  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (GIMP_TYPE_DOCKBOOK, NULL);

  dockbook->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                        "<Dockable>",
                                                        dockbook,
                                                        FALSE);

  gimp_help_connect (GTK_WIDGET (dockbook), gimp_dockbook_help_func,
                     GIMP_HELP_DOCK, dockbook);

  return GTK_WIDGET (dockbook);
}

void
gimp_dockbook_add (GimpDockbook *dockbook,
                   GimpDockable *dockable,
                   gint          position)
{
  GtkWidget *tab_widget;
  GtkWidget *menu_widget;

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dockbook->dock != NULL);
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook == NULL);

  tab_widget = gimp_dockbook_get_tab_widget (dockbook, dockable);

  g_return_if_fail (GTK_IS_WIDGET (tab_widget));

  menu_widget = gimp_dockable_get_tab_widget (dockable,
                                              dockbook->dock->context,
                                              GIMP_TAB_STYLE_ICON_BLURB,
                                              MENU_WIDGET_ICON_SIZE);

  g_return_if_fail (GTK_IS_WIDGET (menu_widget));

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

  dockable->dockbook = dockbook;

  gimp_dockable_set_context (dockable, dockbook->dock->context);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_ADDED], 0, dockable);
}

void
gimp_dockbook_remove (GimpDockbook *dockbook,
                      GimpDockable *dockable)
{
  GList *children;

  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook == dockbook);

  g_object_ref (dockable);

  dockable->dockbook = NULL;

  gimp_dockable_set_context (dockable, NULL);

  gtk_container_remove (GTK_CONTAINER (dockbook), GTK_WIDGET (dockable));

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  g_object_unref (dockable);

  children = gtk_container_get_children (GTK_CONTAINER (dockbook));

  if (! g_list_length (children))
    gimp_dock_remove_book (dockbook->dock, dockbook);

  g_list_free (children);
}

GtkWidget *
gimp_dockbook_get_tab_widget (GimpDockbook *dockbook,
                              GimpDockable *dockable)
{
  GtkWidget   *tab_widget;
  GtkIconSize  tab_size = DEFAULT_TAB_ICON_SIZE;
  GtkAction   *action   = NULL;

  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);

  tab_widget = gimp_dockable_get_tab_widget (dockable,
                                             dockbook->dock->context,
                                             dockable->tab_style,
                                             tab_size);

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
  if (GIMP_IS_IMAGE_DOCK (dockbook->dock) &&
      GIMP_IMAGE_DOCK (dockbook->dock)->ui_manager != NULL)
    {
      const gchar *dialog_id;

      dialog_id = g_object_get_data (G_OBJECT (dockable),
                                     "gimp-dialog-identifier");

      if (dialog_id)
        {
          GimpActionGroup *group;

          group = gimp_ui_manager_get_action_group
            (GIMP_IMAGE_DOCK (dockbook->dock)->ui_manager, "dialogs");

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
    gimp_help_set_help_data (tab_widget, dockable->blurb, dockable->help_id);

  g_object_set_data (G_OBJECT (tab_widget), "gimp-dockable", dockable);

  /*  set the drag source *before* connecting button_press because we
   *  stop button_press emission by returning TRUE from the callback
   */
  gtk_drag_source_set (GTK_WIDGET (tab_widget),
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                       GDK_ACTION_MOVE);
  g_signal_connect (tab_widget, "drag-begin",
                    G_CALLBACK (gimp_dockbook_tab_drag_begin),
                    dockable);
  g_signal_connect (tab_widget, "drag-end",
                    G_CALLBACK (gimp_dockbook_tab_drag_end),
                    dockable);

  gtk_drag_source_set (GTK_WIDGET (dockable),
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                       GDK_ACTION_MOVE);
  g_signal_connect (dockable, "drag-begin",
                    G_CALLBACK (gimp_dockbook_tab_drag_begin),
                    dockable);
  g_signal_connect (dockable, "drag-end",
                    G_CALLBACK (gimp_dockbook_tab_drag_end),
                    dockable);

  gtk_drag_dest_set (GTK_WIDGET (tab_widget),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
  g_signal_connect (tab_widget, "drag-drop",
                    G_CALLBACK (gimp_dockbook_tab_drag_drop),
                    dockbook);

  return tab_widget;
}

gboolean
gimp_dockbook_drop_dockable (GimpDockbook *dockbook,
                             GtkWidget    *drag_source)
{
  g_return_val_if_fail (GIMP_IS_DOCKBOOK (dockbook), FALSE);

  if (drag_source)
    {
      GimpDockable *dockable;

      if (GIMP_IS_DOCKABLE (drag_source))
        dockable = GIMP_DOCKABLE (drag_source);
      else
        dockable = (GimpDockable *) g_object_get_data (G_OBJECT (drag_source),
                                                       "gimp-dockable");

      if (dockable)
        {
          g_object_set_data (G_OBJECT (dockable),
                             "gimp-dock-drag-widget", NULL);

          if (dockable->dockbook == dockbook)
            {
              gtk_notebook_reorder_child (GTK_NOTEBOOK (dockbook),
                                          GTK_WIDGET (dockable), -1);
            }
          else
            {
              g_object_ref (dockable);

              gimp_dockbook_remove (dockable->dockbook, dockable);
              gimp_dockbook_add (dockbook, dockable, -1);

              g_object_unref (dockable);
            }

          return TRUE;
        }
    }

  return FALSE;
}

static void
gimp_dockbook_tab_drag_begin (GtkWidget      *widget,
                              GdkDragContext *context,
                              GimpDockable   *dockable)
{
  GtkWidget      *window = gtk_window_new (GTK_WINDOW_POPUP);
  GtkWidget      *view;
  GtkRequisition  requisition;

  view = gimp_dockable_get_tab_widget (dockable,
                                       dockable->context,
                                       GIMP_TAB_STYLE_ICON_BLURB,
                                       DND_WIDGET_ICON_SIZE);

  g_signal_connect (view, "expose-event",
                    G_CALLBACK (gimp_dockbook_tab_drag_expose),
                    NULL);

  if (GTK_IS_CONTAINER (view))
    gtk_container_set_border_width (GTK_CONTAINER (view), 6);

  if (GTK_IS_HBOX (view))
    gtk_box_set_spacing (GTK_BOX (view), 6);

  gtk_container_add (GTK_CONTAINER (window), view);
  gtk_widget_show (view);

  gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (widget));

  gtk_widget_size_request (view, &requisition);

  if (requisition.width < widget->allocation.width)
    gtk_widget_set_size_request (view, widget->allocation.width, -1);

  gtk_widget_show (window);

  g_object_set_data_full (G_OBJECT (dockable), "gimp-dock-drag-widget",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window,
                            dockable->drag_x, dockable->drag_y);

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
  GtkWidget *drag_widget = g_object_get_data (G_OBJECT (dockable),
                                              "gimp-dock-drag-widget");

  /*  finding the drag_widget means the drop was not successful, so
   *  pop up a new dock and move the dockable there
   */
  if (drag_widget)
    {
      g_object_set_data (G_OBJECT (dockable), "gimp-dock-drag-widget", NULL);
      gimp_dockable_detach (dockable);
    }

  dockable->drag_x = GIMP_DOCKABLE_DRAG_OFFSET;
  dockable->drag_y = GIMP_DOCKABLE_DRAG_OFFSET;
  gtk_widget_set_sensitive (GTK_WIDGET (dockable), TRUE);
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

  dest_dockable = g_object_get_data (G_OBJECT (widget), "gimp-dockable");

  source = gtk_drag_get_source_widget (context);

  if (dest_dockable && source)
    {
      GimpDockable *src_dockable;

      if (GIMP_IS_DOCKABLE (source))
        src_dockable = GIMP_DOCKABLE (source);
      else
        src_dockable = g_object_get_data (G_OBJECT (source), "gimp-dockable");

      if (src_dockable)
        {
          gint dest_index;

          dest_index =
            gtk_notebook_page_num (GTK_NOTEBOOK (dest_dockable->dockbook),
                                   GTK_WIDGET (dest_dockable));

          g_object_set_data (G_OBJECT (src_dockable),
                             "gimp-dock-drag-widget", NULL);

          if (src_dockable->dockbook != dest_dockable->dockbook)
            {
              g_object_ref (src_dockable);

              gimp_dockbook_remove (src_dockable->dockbook, src_dockable);
              gimp_dockbook_add (dest_dockable->dockbook, src_dockable,
                                 dest_index);

              g_object_unref (src_dockable);

              return TRUE;
            }
          else if (src_dockable != dest_dockable)
            {
              gtk_notebook_reorder_child (GTK_NOTEBOOK (src_dockable->dockbook),
                                          GTK_WIDGET (src_dockable),
                                          dest_index);

              g_signal_emit (src_dockable->dockbook,
                             dockbook_signals[DOCKABLE_REORDERED], 0,
                             src_dockable);

              return TRUE;
            }
        }
    }

  return FALSE;
}

static gboolean
gimp_dockbook_tab_drag_expose (GtkWidget      *widget,
                               GdkEventExpose *event)
{
  /*  mimic the appearance of a notebook tab  */

  gtk_paint_extension (widget->style, widget->window,
                       widget->state, GTK_SHADOW_OUT,
                       &event->area, widget, "tab",
                       widget->allocation.x,
                       widget->allocation.y,
                       widget->allocation.width,
                       widget->allocation.height,
                       GTK_POS_BOTTOM);

  return FALSE;
}

static void
gimp_dockbook_help_func (const gchar *help_id,
                         gpointer     help_data)
{
  GimpDockbook *dockbook;
  GtkWidget    *dockable;
  gint          page_num;

  dockbook = GIMP_DOCKBOOK (help_data);

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (GIMP_IS_DOCKABLE (dockable))
    gimp_standard_help_func (GIMP_DOCKABLE (dockable)->help_id, NULL);
  else
    gimp_standard_help_func (GIMP_HELP_DOCK, NULL);
}
