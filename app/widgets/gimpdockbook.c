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
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

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

#define DEFAULT_TAB_BORDER           0
#define DEFAULT_TAB_ICON_SIZE        GTK_ICON_SIZE_BUTTON
#define DND_WIDGET_ICON_SIZE         GTK_ICON_SIZE_BUTTON
#define MENU_WIDGET_ICON_SIZE        GTK_ICON_SIZE_MENU
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

/* List of candidates for the automatic style, starting with the
 * biggest first
 */
static const GimpTabStyle gimp_tab_style_candidates[] =
{
  GIMP_TAB_STYLE_PREVIEW_BLURB,
  GIMP_TAB_STYLE_PREVIEW_NAME,
  GIMP_TAB_STYLE_PREVIEW
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

  /* Cache for "what actual tab style for automatic styles can we use
   * for a given dockbook width
   */
  gint            min_width_for_style[G_N_ELEMENTS (gimp_tab_style_candidates)];

  /* We need a list separate from the GtkContainer children list,
   * because we need to do calculations for all dockables before we
   * can add a dockable as a child, namely automatic tab style
   * calculations
   */
  GList          *dockables;

  GtkWidget      *menu_button;
};


static void         gimp_dockbook_dispose                     (GObject        *object);
static void         gimp_dockbook_finalize                    (GObject        *object);
static void         gimp_dockbook_size_allocate               (GtkWidget      *widget,
                                                               GtkAllocation  *allocation);
static void         gimp_dockbook_style_set                   (GtkWidget      *widget,
                                                               GtkStyle       *prev_style);
static void         gimp_dockbook_drag_leave                  (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               guint           time);
static gboolean     gimp_dockbook_drag_motion                 (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               gint            x,
                                                               gint            y,
                                                               guint           time);
static gboolean     gimp_dockbook_drag_drop                   (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               gint            x,
                                                               gint            y,
                                                               guint           time);
static gboolean     gimp_dockbook_popup_menu                  (GtkWidget      *widget);
static gboolean     gimp_dockbook_menu_button_press           (GimpDockbook   *dockbook,
                                                               GdkEventButton *bevent,
                                                               GtkWidget      *button);
static gboolean     gimp_dockbook_show_menu                   (GimpDockbook   *dockbook);
static void         gimp_dockbook_menu_end                    (GimpDockable   *dockable);
static void         gimp_dockbook_dockable_added              (GimpDockbook   *dockbook,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_dockable_removed            (GimpDockbook   *dockbook,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_recreate_tab_widgets        (GimpDockbook   *dockbook,
                                                               gboolean        only_auto);
static void         gimp_dockbook_tab_drag_source_setup       (GtkWidget      *widget,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_begin              (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_end                (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               GimpDockable   *dockable);
static gboolean     gimp_dockbook_tab_drag_failed             (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               GtkDragResult   result,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_tab_drag_leave              (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               guint           time,
                                                               GimpDockable   *dockable);
static gboolean     gimp_dockbook_tab_drag_motion             (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               gint            x,
                                                               gint            y,
                                                               guint           time,
                                                               GimpDockable   *dockable);
static gboolean     gimp_dockbook_tab_drag_drop               (GtkWidget      *widget,
                                                               GdkDragContext *context,
                                                               gint            x,
                                                               gint            y,
                                                               guint           time);
static GimpTabStyle gimp_dockbook_tab_style_to_preferred       (GimpTabStyle    tab_style,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_refresh_tab_layout_lut      (GimpDockbook   *dockbook);
static void         gimp_dockbook_update_automatic_tab_style  (GimpDockbook   *dockbook);
static GtkWidget *  gimp_dockable_create_event_box_tab_widget (GimpDockable   *dockable,
                                                               GimpContext    *context,
                                                               GimpTabStyle    tab_style,
                                                               GtkIconSize     size);
static GtkIconSize  gimp_dockbook_get_tab_icon_size           (GimpDockbook   *dockbook);
static gint         gimp_dockbook_get_tab_border              (GimpDockbook   *dockbook);
static void         gimp_dockbook_add_tab_timeout             (GimpDockbook   *dockbook,
                                                               GimpDockable   *dockable);
static void         gimp_dockbook_remove_tab_timeout          (GimpDockbook   *dockbook);
static gboolean     gimp_dockbook_tab_timeout                 (GimpDockbook   *dockbook);
static void         gimp_dockbook_tab_locked_notify           (GimpDockable   *dockable,
                                                               GParamSpec     *pspec,
                                                               GimpDockbook   *dockbook);
static void         gimp_dockbook_help_func                   (const gchar    *help_id,
                                                               gpointer        help_data);
static const gchar *gimp_dockbook_get_tab_style_name          (GimpTabStyle    tab_style);

static void         gimp_dockbook_config_size_changed         (GimpGuiConfig   *config,
                                                               GimpDockbook    *dockbook);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDockbook, gimp_dockbook, GTK_TYPE_NOTEBOOK)

#define parent_class gimp_dockbook_parent_class

static guint dockbook_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };

static GList *drag_callbacks = NULL;


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

  widget_class->size_allocate = gimp_dockbook_size_allocate;
  widget_class->style_set     = gimp_dockbook_style_set;
  widget_class->drag_leave    = gimp_dockbook_drag_leave;
  widget_class->drag_motion   = gimp_dockbook_drag_motion;
  widget_class->drag_drop     = gimp_dockbook_drag_drop;
  widget_class->popup_menu    = gimp_dockbook_popup_menu;

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
  GtkNotebook *notebook = GTK_NOTEBOOK (dockbook);
  GtkWidget   *image    = NULL;

  dockbook->p = gimp_dockbook_get_instance_private (dockbook);

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

  g_signal_handlers_disconnect_by_func (dockbook->p->ui_manager->gimp->config,
                                        gimp_dockbook_config_size_changed,
                                        dockbook);

  gimp_dockbook_remove_tab_timeout (dockbook);

  while (dockbook->p->dockables)
    {
      GimpDockable *dockable = dockbook->p->dockables->data;

      g_object_ref (dockable);
      gimp_dockbook_remove (dockbook, dockable);
      gtk_widget_destroy (GTK_WIDGET (dockable));
      g_object_unref (dockable);
    }

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
gimp_dockbook_size_allocate (GtkWidget      *widget,
                             GtkAllocation  *allocation)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  /* Update tab styles, also recreates if changed */
  gimp_dockbook_update_automatic_tab_style (dockbook);
}

static void
gimp_dockbook_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  /* Don't attempt to construct widgets that require a GimpContext if
   * we are detached from a top-level, we're either on our way to
   * destruction, in which case we don't care, or we will be given a
   * new parent, in which case the widget style will be reset again
   * anyway, i.e. this function will be called again
   */
  if (! gtk_widget_is_toplevel (gtk_widget_get_toplevel (widget)))
    return;

  gimp_dockbook_recreate_tab_widgets (GIMP_DOCKBOOK (widget),
                                      FALSE /*only_auto*/);
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

static void
gimp_dockbook_menu_position (GtkMenu  *menu,
                             gint     *x,
                             gint     *y,
                             gpointer  data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  gimp_button_menu_position (dockbook->p->menu_button, menu, GTK_POS_LEFT, x, y);
}

static gboolean
gimp_dockbook_show_menu (GimpDockbook *dockbook)
{
  GimpUIManager *dockbook_ui_manager;
  GimpUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;
  GtkWidget     *parent_menu_widget;
  GimpAction    *parent_menu_action;
  GimpDockable  *dockable;
  gint           page_num;

  dockbook_ui_manager = gimp_dockbook_get_ui_manager (dockbook);

  if (! dockbook_ui_manager)
    return FALSE;

  parent_menu_widget =
    gimp_ui_manager_get_widget (dockbook_ui_manager,
                                "/dockable-popup/dockable-menu");
  parent_menu_action =
    gimp_ui_manager_get_action (dockbook_ui_manager,
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
      GtkWidget  *child_menu_widget;
      GimpAction *child_menu_action;
      gchar      *label;

      child_menu_widget =
        gimp_ui_manager_get_widget (dialog_ui_manager, dialog_ui_path);

      if (! child_menu_widget)
        {
          g_warning ("%s: UI manager '%s' has no widget at path '%s'",
                     G_STRFUNC, dialog_ui_manager->name, dialog_ui_path);
          return FALSE;
        }

      child_menu_action =
        gimp_ui_manager_get_action (dialog_ui_manager,
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

        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (parent_menu_widget),
                                       image);
        gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (parent_menu_widget),
                                                   TRUE);
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
  gimp_ui_manager_ui_popup (dockbook_ui_manager, "/dockable-popup",
                            GTK_WIDGET (dockable),
                            gimp_dockbook_menu_position, dockbook,
                            (GDestroyNotify) gimp_dockbook_menu_end, dockable);

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
        gimp_ui_manager_get_widget (dialog_ui_manager, dialog_ui_path);

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

/**
 * gimp_dockbook_get_dockable_tab_width:
 * @dockable:
 * @tab_style:
 *
 * Returns: Width of tab when the dockable is using the specified tab
 *          style.
 **/
static gint
gimp_dockbook_get_dockable_tab_width (GimpDockbook *dockbook,
                                      GimpDockable *dockable,
                                      GimpTabStyle  tab_style)
{
  GtkRequisition  dockable_request;
  GtkWidget      *tab_widget;

  tab_widget =
    gimp_dockable_create_event_box_tab_widget (dockable,
                                               gimp_dock_get_context (dockbook->p->dock),
                                               tab_style,
                                               gimp_dockbook_get_tab_icon_size (dockbook));

  /* So font-scale is applied. We can't apply styles without having a
   * GdkScreen :(
   */
  gimp_dock_temp_add (dockbook->p->dock, tab_widget);

  gtk_widget_size_request (tab_widget, &dockable_request);

  /* Also destroys the widget */
  gimp_dock_temp_remove (dockbook->p->dock, tab_widget);

  return dockable_request.width;
}

/**
 * gimp_dockbook_tab_style_to_preferred:
 * @tab_style:
 * @dockable:
 *
 * The list of tab styles to try in automatic mode only consists of
 * preview styles. For some dockables, like the tool options dockable,
 * we rather want to use the icon tab styles for the automatic
 * mode. This function is used to convert tab styles for such
 * dockables.
 *
 * Returns: An icon tab style if the dockable prefers icon tab styles
 *          in automatic mode.
 **/
static GimpTabStyle
gimp_dockbook_tab_style_to_preferred (GimpTabStyle  tab_style,
                                     GimpDockable *dockable)
{
  GimpDocked *docked = GIMP_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));

  if (gimp_docked_get_prefer_icon (docked))
    tab_style = gimp_preview_tab_style_to_icon (tab_style);

  return tab_style;
}

/**
 * gimp_dockbook_refresh_tab_layout_lut:
 * @dockbook:
 *
 * For each given set of tab widgets, there is a fixed mapping between
 * the width of the dockbook and the actual tab style to use for auto
 * tab widgets. This function refreshes that look-up table.
 **/
static void
gimp_dockbook_refresh_tab_layout_lut (GimpDockbook *dockbook)
{
  GList *auto_dockables        = NULL;
  GList *iter                  = NULL;
  gint   fixed_tab_style_space = 0;
  int    i                     = 0;

  /* Calculate space taken by dockables with fixed tab styles */
  fixed_tab_style_space = 0;
  for (iter = dockbook->p->dockables; iter; iter = g_list_next (iter))
    {
      GimpDockable *dockable  = GIMP_DOCKABLE (iter->data);
      GimpTabStyle  tab_style = gimp_dockable_get_tab_style (dockable);

      if (tab_style == GIMP_TAB_STYLE_AUTOMATIC)
        auto_dockables = g_list_prepend (auto_dockables, dockable);
      else
        fixed_tab_style_space +=
          gimp_dockbook_get_dockable_tab_width (dockbook,
                                                dockable,
                                                tab_style);
    }

  /* Calculate space taken with auto tab style for all candidates */
  for (i = 0; i < G_N_ELEMENTS (gimp_tab_style_candidates); i++)
    {
      gint         size_with_candidate = 0;
      GimpTabStyle candidate           = gimp_tab_style_candidates[i];

      for (iter = auto_dockables; iter; iter = g_list_next (iter))
        {
          GimpDockable *dockable = GIMP_DOCKABLE (iter->data);
          GimpTabStyle  style_to_use;

          style_to_use = gimp_dockbook_tab_style_to_preferred (candidate,
                                                              dockable);
          size_with_candidate +=
            gimp_dockbook_get_dockable_tab_width (dockbook,
                                                  dockable,
                                                  style_to_use);
        }

      dockbook->p->min_width_for_style[i] =
        fixed_tab_style_space + size_with_candidate;

      GIMP_LOG (AUTO_TAB_STYLE, "Total tab space taken for auto tab style %s = %d",
                gimp_dockbook_get_tab_style_name (candidate),
                dockbook->p->min_width_for_style[i]);
    }

  g_list_free (auto_dockables);
}

/**
 * gimp_dockbook_update_automatic_tab_style:
 * @dockbook:
 *
 * Based on widget allocation, sets actual tab style for dockables
 * with automatic tab styles. Takes care of recreating tab widgets if
 * necessary.
 **/
static void
gimp_dockbook_update_automatic_tab_style (GimpDockbook *dockbook)
{
  GtkWidget    *widget              = GTK_WIDGET (dockbook);
  gboolean      changed             = FALSE;
  GList        *iter                = NULL;
  GtkAllocation dockbook_allocation = { 0, };
  GtkAllocation button_allocation   = { 0, };
  GimpTabStyle  tab_style           = 0;
  int           i                   = 0;
  gint          available_space     = 0;
  guint         tab_hborder         = 0;
  gint          xthickness          = 0;
  gint          tab_curvature       = 0;
  gint          focus_width         = 0;
  gint          tab_overlap         = 0;
  gint          tab_padding         = 0;
  gint          border_loss         = 0;
  gint          action_widget_size  = 0;

  xthickness = gtk_widget_get_style (widget)->xthickness;
  g_object_get (widget,
                "tab-hborder", &tab_hborder,
                NULL);
  gtk_widget_style_get (widget,
                        "tab-curvature",    &tab_curvature,
                        "focus-line-width", &focus_width,
                        "tab-overlap",      &tab_overlap,
                        NULL);
  gtk_widget_get_allocation (dockbook->p->menu_button,
                             &button_allocation);

  /* Calculate available space. Based on code in GTK+ internal
   * functions gtk_notebook_size_request() and
   * gtk_notebook_pages_allocate()
   */
  gtk_widget_get_allocation (widget, &dockbook_allocation);

  /* Border on both sides */
  border_loss = gtk_container_get_border_width (GTK_CONTAINER (dockbook)) * 2;

  /* Space taken by action widget */
  action_widget_size = button_allocation.width + xthickness;

  /* Space taken by the tabs but not the tab widgets themselves */
  tab_padding = gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook)) *
                (2 * (xthickness + tab_curvature + focus_width + tab_hborder) -
                 tab_overlap);

  available_space = dockbook_allocation.width
    - border_loss
    - action_widget_size
    - tab_padding
    - tab_overlap;

  GIMP_LOG (AUTO_TAB_STYLE, "\n"
            "  available_space             = %d where\n"
            "    dockbook_allocation.width = %d\n"
            "    border_loss               = %d\n"
            "    action_widget_size        = %d\n"
            "    tab_padding               = %d\n"
            "    tab_overlap               = %d\n",
            available_space,
            dockbook_allocation.width,
            border_loss,
            action_widget_size,
            tab_padding,
            tab_overlap);

  /* Try all candidates, if we don't get any hit we still end up on
   * the smallest style (which we always fall back to if we don't get
   * a better match)
   */
  for (i = 0; i < G_N_ELEMENTS (gimp_tab_style_candidates); i++)
    {
      tab_style = gimp_tab_style_candidates[i];
      if (available_space > dockbook->p->min_width_for_style[i])
        {
          GIMP_LOG (AUTO_TAB_STYLE, "Choosing tab style %s",
                    gimp_dockbook_get_tab_style_name (tab_style));
          break;
        }
    }

  for (iter = dockbook->p->dockables; iter; iter = g_list_next (iter))
    {
      GimpDockable *dockable         = GIMP_DOCKABLE (iter->data);
      GimpTabStyle  actual_tab_style = tab_style;

      if (gimp_dockable_get_tab_style (dockable) != GIMP_TAB_STYLE_AUTOMATIC)
        continue;

      actual_tab_style = gimp_dockbook_tab_style_to_preferred (tab_style,
                                                              dockable);

      if (gimp_dockable_set_actual_tab_style (dockable, actual_tab_style))
        changed = TRUE;
    }

  if (changed)
    gimp_dockbook_recreate_tab_widgets (dockbook,
                                        TRUE /*only_auto*/);
}

GtkWidget *
gimp_dockbook_new (GimpMenuFactory *menu_factory)
{
  GimpDockbook *dockbook;

  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (GIMP_TYPE_DOCKBOOK, NULL);

  dockbook->p->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                           "<Dockable>",
                                                           dockbook,
                                                           FALSE);

  g_signal_connect (dockbook->p->ui_manager->gimp->config,
                    "size-changed",
                    G_CALLBACK (gimp_dockbook_config_size_changed),
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

  /* Add to internal list before doing automatic tab style
   * calculations
   */
  dockbook->p->dockables = g_list_insert (dockbook->p->dockables,
                                          dockable,
                                          position);

  gimp_dockbook_update_auto_tab_style (dockbook);

  /* Create the new tab widget, it will get the correct tab style now */
  tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);

  g_return_if_fail (GTK_IS_WIDGET (tab_widget));

  gimp_dockable_set_drag_handler (dockable, dockbook->p->drag_handler);

  /* For the notebook right-click menu, always use the icon style */
  menu_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     gimp_dock_get_context (dockbook->p->dock),
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

  if (dockable)
    gimp_dockable_set_drag_pos (GIMP_DOCKABLE (dockable),
                                GIMP_DOCKABLE_DRAG_OFFSET,
                                GIMP_DOCKABLE_DRAG_OFFSET);
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
  dockbook->p->dockables = g_list_remove (dockbook->p->dockables,
                                          dockable);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  g_object_unref (dockable);

  if (dockbook->p->dock)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      if (children)
        gimp_dockbook_update_auto_tab_style (dockbook);
      else
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
  GList *iter     = NULL;

  for (iter = children;
       iter;
       iter = g_list_next (iter))
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

  tab_widget =
    gimp_dockable_create_event_box_tab_widget (dockable,
                                               gimp_dock_get_context (dockbook->p->dock),
                                               gimp_dockable_get_actual_tab_style (dockable),
                                               gimp_dockbook_get_tab_icon_size (dockbook));

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

  g_object_set_data (G_OBJECT (tab_widget), "gimp-dockable", dockable);

  gimp_dockbook_tab_drag_source_setup (tab_widget, dockable);

  g_signal_connect_object (tab_widget, "drag-begin",
                           G_CALLBACK (gimp_dockbook_tab_drag_begin),
                           dockable, 0);
  g_signal_connect_object (tab_widget, "drag-end",
                           G_CALLBACK (gimp_dockbook_tab_drag_end),
                           dockable, 0);
  g_signal_connect_object (tab_widget, "drag-failed",
                           G_CALLBACK (gimp_dockbook_tab_drag_failed),
                           dockable, 0);

  g_signal_connect_object (dockable, "drag-begin",
                           G_CALLBACK (gimp_dockbook_tab_drag_begin),
                           dockable, 0);
  g_signal_connect_object (dockable, "drag-end",
                           G_CALLBACK (gimp_dockbook_tab_drag_end),
                           dockable, 0);
  g_signal_connect_object (dockable, "drag-failed",
                           G_CALLBACK (gimp_dockbook_tab_drag_failed),
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

/**
 * gimp_dockbook_update_auto_tab_style:
 * @dockbook:
 *
 * Refresh the table that we use to map dockbook width to actual auto
 * tab style, then update auto tabs (also recreate tab widgets if
 * necessary).
 **/
void
gimp_dockbook_update_auto_tab_style (GimpDockbook *dockbook)
{
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  gimp_dockbook_refresh_tab_layout_lut (dockbook);
  gimp_dockbook_update_automatic_tab_style (dockbook);
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

/*  tab DND source side  */

static void
gimp_dockbook_recreate_tab_widgets (GimpDockbook *dockbook,
                                    gboolean      only_auto)
{
  GList *dockables = gtk_container_get_children (GTK_CONTAINER (dockbook));
  GList *iter      = NULL;

  g_object_set (dockbook,
                "tab-border", gimp_dockbook_get_tab_border (dockbook),
                NULL);

  for (iter = dockables; iter; iter = g_list_next (iter))
    {
      GimpDockable *dockable = GIMP_DOCKABLE (iter->data);
      GtkWidget *tab_widget;

      if (only_auto &&
          ! (gimp_dockable_get_tab_style (dockable) == GIMP_TAB_STYLE_AUTOMATIC))
        continue;

      tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }

  g_list_free (dockables);
}

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
  GList          *iter;
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

  gtk_widget_size_request (view, &requisition);

  if (requisition.width < allocation.width)
    gtk_widget_set_size_request (view, allocation.width, -1);

  gtk_widget_show (window);

  g_object_set_data_full (G_OBJECT (dockable), "gimp-dock-drag-widget",
                          window,
                          (GDestroyNotify) gtk_widget_destroy);

  gimp_dockable_get_drag_pos (dockable, &drag_x, &drag_y);
  gtk_drag_set_icon_widget (context, window, drag_x, drag_y);

  iter = drag_callbacks;

  while (iter)
    {
      GimpDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, TRUE, callback_data->data);
    }
}

static void
gimp_dockbook_tab_drag_end (GtkWidget      *widget,
                            GdkDragContext *context,
                            GimpDockable   *dockable)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      GimpDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, FALSE, callback_data->data);
    }
}

static gboolean
gimp_dockbook_tab_drag_failed (GtkWidget      *widget,
                               GdkDragContext *context,
                               GtkDragResult   result,
                               GimpDockable   *dockable)
{
  /* XXX The proper way is to handle "drag-end" signal instead.
   * Unfortunately this signal seems to be broken in various cases on
   * macOS/GTK+2 (see #1924). As a consequence, we made sure we don't
   * have anything to clean unconditionally (for instance we used to set
   * the dockable unsensitive, which anyway was only a visual clue and
   * is not really useful). Only thing left to handle is the dockable
   * detachment when dropping in a non-droppable area.
   */
  GtkWidget *drag_widget;
  GtkWidget *window;

  if (result == GTK_DRAG_RESULT_SUCCESS)
    {
      /* I don't think this should happen, considering we are in the
       * "drag-failed" handler, but let's be complete as it is a
       * possible GtkDragResult value. Just in case!
       */
      return FALSE;
    }

  drag_widget = g_object_get_data (G_OBJECT (dockable),
                                   "gimp-dock-drag-widget");

  /* The drag_widget should be present if the drop was not successful,
   * in which case, we pop up a new dock and move the dockable there.
   */
  g_return_val_if_fail (drag_widget, FALSE);

  g_object_set_data (G_OBJECT (dockable), "gimp-dock-drag-widget", NULL);
  gimp_dockable_detach (dockable);

  window = gtk_widget_get_toplevel (GTK_WIDGET (dockable));
  gtk_window_present (GTK_WINDOW (window));

  return TRUE;
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

static GtkWidget *
gimp_dockable_create_event_box_tab_widget (GimpDockable *dockable,
                                           GimpContext  *context,
                                           GimpTabStyle  tab_style,
                                           GtkIconSize   size)
{
  GtkWidget *tab_widget;

  tab_widget =
    gimp_dockable_create_tab_widget (dockable,
                                     context,
                                     tab_style,
                                     size);

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

  return tab_widget;
}

static GtkIconSize
gimp_dockbook_get_tab_icon_size (GimpDockbook *dockbook)
{
  Gimp        *gimp     = dockbook->p->ui_manager->gimp;
  GtkIconSize  tab_size = DEFAULT_TAB_ICON_SIZE;
  GimpIconSize size;

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

static gint
gimp_dockbook_get_tab_border (GimpDockbook *dockbook)
{
  Gimp         *gimp       = dockbook->p->ui_manager->gimp;
  gint          tab_border = DEFAULT_TAB_BORDER;
  GimpIconSize  size;

  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-border", &tab_border,
                        NULL);

  size = gimp_gui_config_detect_icon_size (GIMP_GUI_CONFIG (gimp->config));
  /* Match GimpIconSize with GtkIconSize. */
  switch (size)
    {
    case GIMP_ICON_SIZE_SMALL:
      tab_border /= 2;
      break;
    case GIMP_ICON_SIZE_LARGE:
      tab_border *= 2;
      break;
    case GIMP_ICON_SIZE_HUGE:
      tab_border *= 3;
      break;
    default:
      /* GIMP_ICON_SIZE_MEDIUM and GIMP_ICON_SIZE_DEFAULT:
       * let's use the size set by the theme. */
      break;
    }

  return tab_border;
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

  GDK_THREADS_ENTER ();

  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockbook->p->tab_hover_dockable));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), page_num);

  dockbook->p->tab_hover_timeout  = 0;
  dockbook->p->tab_hover_dockable = NULL;

  GDK_THREADS_LEAVE ();

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

static const gchar *
gimp_dockbook_get_tab_style_name (GimpTabStyle tab_style)
{
  return g_enum_get_value (g_type_class_peek (GIMP_TYPE_TAB_STYLE),
                           tab_style)->value_name;
}

static void
gimp_dockbook_config_size_changed (GimpGuiConfig *config,
                                   GimpDockbook  *dockbook)
{
  gimp_dockbook_recreate_tab_widgets (dockbook, TRUE);
}
