/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuiconfigurer.c
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpimagewindow.h"

#include "gimpuiconfigurer.h"


enum
{
  PROP_0,
  PROP_GIMP
};


struct _GimpUIConfigurerPrivate
{
  Gimp *gimp;
};


static void              gimp_ui_configurer_set_property                (GObject           *object,
                                                                         guint              property_id,
                                                                         const GValue      *value,
                                                                         GParamSpec        *pspec);
static void              gimp_ui_configurer_get_property                (GObject           *object,
                                                                         guint              property_id,
                                                                         GValue            *value,
                                                                         GParamSpec        *pspec);
static void              gimp_ui_configurer_move_docks_to_columns       (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *uber_image_window);
static void              gimp_ui_configurer_move_shells                 (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window,
                                                                         GimpImageWindow   *target_image_window);
static void              gimp_ui_configurer_separate_docks              (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window);
static void              gimp_ui_configurer_move_docks_to_window        (GimpUIConfigurer  *ui_configurer,
                                                                         GimpDockColumns   *dock_columns,
                                                                         GimpAlignmentType  screen_side);
static void              gimp_ui_configurer_separate_shells             (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window);
static void              gimp_ui_configurer_configure_for_single_window (GimpUIConfigurer  *ui_configurer);
static void              gimp_ui_configurer_configure_for_multi_window  (GimpUIConfigurer  *ui_configurer);
static GimpImageWindow * gimp_ui_configurer_get_uber_window             (GimpUIConfigurer  *ui_configurer);


G_DEFINE_TYPE_WITH_PRIVATE (GimpUIConfigurer, gimp_ui_configurer,
                            GIMP_TYPE_OBJECT)

#define parent_class gimp_ui_configurer_parent_class


static void
gimp_ui_configurer_class_init (GimpUIConfigurerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_ui_configurer_set_property;
  object_class->get_property = gimp_ui_configurer_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_ui_configurer_init (GimpUIConfigurer *ui_configurer)
{
  ui_configurer->p = gimp_ui_configurer_get_instance_private (ui_configurer);
}

static void
gimp_ui_configurer_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpUIConfigurer *ui_configurer = GIMP_UI_CONFIGURER (object);

  switch (property_id)
    {
    case PROP_GIMP:
      ui_configurer->p->gimp = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_ui_configurer_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpUIConfigurer *ui_configurer = GIMP_UI_CONFIGURER (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, ui_configurer->p->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
gimp_ui_configurer_get_window_center_pos (GtkWindow *window,
                                          gint      *out_x,
                                          gint      *out_y)
{
  gint x, y, w, h;
  gtk_window_get_position (window, &x, &y);
  gtk_window_get_size (window, &w, &h);

  if (out_x)
    *out_x = x + w / 2;
  if (out_y)
    *out_y = y + h / 2;
}

/**
 * gimp_ui_configurer_get_relative_window_pos:
 * @window_a:
 * @window_b:
 *
 * Returns: At what side @window_b is relative to @window_a. Either
 * GIMP_ALIGN_LEFT or GIMP_ALIGN_RIGHT.
 **/
static GimpAlignmentType
gimp_ui_configurer_get_relative_window_pos (GtkWindow *window_a,
                                            GtkWindow *window_b)
{
  gint a_x, b_x;

  gimp_ui_configurer_get_window_center_pos (window_a, &a_x, NULL);
  gimp_ui_configurer_get_window_center_pos (window_b, &b_x, NULL);

  return b_x < a_x ? GIMP_ALIGN_LEFT : GIMP_ALIGN_RIGHT;
}

static void
gimp_ui_configurer_move_docks_to_columns (GimpUIConfigurer *ui_configurer,
                                          GimpImageWindow  *uber_image_window)
{
  GList *dialogs     = NULL;
  GList *dialog_iter = NULL;

  dialogs =
    g_list_copy (gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ()));

  for (dialog_iter = dialogs; dialog_iter; dialog_iter = dialog_iter->next)
    {
      GimpDockWindow    *dock_window;
      GimpDockContainer *dock_container;
      GimpDockColumns   *dock_columns;
      GList             *docks;
      GList             *dock_iter;

      if (!GIMP_IS_DOCK_WINDOW (dialog_iter->data))
        continue;

      dock_window = GIMP_DOCK_WINDOW (dialog_iter->data);

      /* If the dock window is on the left side of the image window,
       * move the docks to the left side. If the dock window is on the
       * right side, move the docks to the right side of the image
       * window.
       */
      if (gimp_ui_configurer_get_relative_window_pos (GTK_WINDOW (uber_image_window),
                                                      GTK_WINDOW (dock_window)) == GIMP_ALIGN_LEFT)
        dock_columns = gimp_image_window_get_left_docks (uber_image_window);
      else
        dock_columns = gimp_image_window_get_right_docks (uber_image_window);

      dock_container = GIMP_DOCK_CONTAINER (dock_window);
      g_object_add_weak_pointer (G_OBJECT (dock_window),
                                 (gpointer) &dock_window);

      docks = gimp_dock_container_get_docks (dock_container);
      for (dock_iter = docks; dock_iter; dock_iter = dock_iter->next)
        {
          GimpDock *dock = GIMP_DOCK (dock_iter->data);

          /* Move the dock from the image window to the dock columns
           * widget. Note that we need a ref while the dock is parentless
           */
          g_object_ref (dock);
          gimp_dock_window_remove_dock (dock_window, dock);
          gimp_dock_columns_add_dock (dock_columns, dock, -1);
          g_object_unref (dock);
        }
      g_list_free (docks);

      if (dock_window)
        g_object_remove_weak_pointer (G_OBJECT (dock_window),
                                      (gpointer) &dock_window);

      /* Kill the window if removing the dock didn't destroy it
       * already. This will be the case for the toolbox dock window
       */
      if (GTK_IS_WIDGET (dock_window))
        {
          guint docks_len;

          docks     = gimp_dock_container_get_docks (dock_container);
          docks_len = g_list_length (docks);

          if (docks_len == 0)
            {
              gimp_dialog_factory_remove_dialog (gimp_dialog_factory_get_singleton (),
                                                 GTK_WIDGET (dock_window));
              gtk_widget_destroy (GTK_WIDGET (dock_window));
            }

          g_list_free (docks);
        }
    }

  g_list_free (dialogs);
}

/**
 * gimp_ui_configurer_move_shells:
 * @ui_configurer:
 * @source_image_window:
 * @target_image_window:
 *
 * Move all display shells from one image window to the another.
 **/
static void
gimp_ui_configurer_move_shells (GimpUIConfigurer  *ui_configurer,
                                GimpImageWindow   *source_image_window,
                                GimpImageWindow   *target_image_window)
{
  while (gimp_image_window_get_n_shells (source_image_window) > 0)
    {
      GimpDisplayShell *shell;

      shell = gimp_image_window_get_shell (source_image_window, 0);

      g_object_ref (shell);
      gimp_image_window_remove_shell (source_image_window, shell);
      gimp_image_window_add_shell (target_image_window, shell);
      g_object_unref (shell);
    }
}

/**
 * gimp_ui_configurer_separate_docks:
 * @ui_configurer:
 * @image_window:
 *
 * Move out the docks from the image window.
 **/
static void
gimp_ui_configurer_separate_docks (GimpUIConfigurer  *ui_configurer,
                                   GimpImageWindow   *image_window)
{
  GimpDockColumns *left_docks  = NULL;
  GimpDockColumns *right_docks = NULL;

  left_docks  = gimp_image_window_get_left_docks (image_window);
  right_docks = gimp_image_window_get_right_docks (image_window);

  gimp_ui_configurer_move_docks_to_window (ui_configurer, left_docks, GIMP_ALIGN_LEFT);
  gimp_ui_configurer_move_docks_to_window (ui_configurer, right_docks, GIMP_ALIGN_RIGHT);
}

/**
 * gimp_ui_configurer_move_docks_to_window:
 * @dock_columns:
 * @screen_side: At what side of the screen the dock window should be put.
 *
 * Moves docks in @dock_columns into a new #GimpDockWindow and
 * position it on the screen in a non-overlapping manner.
 */
static void
gimp_ui_configurer_move_docks_to_window (GimpUIConfigurer  *ui_configurer,
                                         GimpDockColumns   *dock_columns,
                                         GimpAlignmentType  screen_side)
{
  GdkScreen     *screen;
  gint           monitor;
  GdkRectangle   monitor_rect;
  GList         *docks;
  GList         *iter;
  gboolean       contains_toolbox = FALSE;
  GtkWidget     *dock_window;
  GtkAllocation  original_size;
  gchar          geometry[32];

  docks = g_list_copy (gimp_dock_columns_get_docks (dock_columns));
  if (! docks)
    return;

  screen  = gtk_widget_get_screen (GTK_WIDGET (dock_columns));
  monitor = gimp_widget_get_monitor (GTK_WIDGET (dock_columns));

  gdk_screen_get_monitor_workarea (screen, monitor, &monitor_rect);

  /* Remember the size so we can set the new dock window to the same
   * size
   */
  gtk_widget_get_allocation (GTK_WIDGET (dock_columns), &original_size);

  /* Do we need a toolbox window? */
  for (iter = docks; iter; iter = g_list_next (iter))
    {
      GimpDock *dock = GIMP_DOCK (iter->data);

      if (GIMP_IS_TOOLBOX (dock))
        {
          contains_toolbox = TRUE;
          break;
        }
    }

  /* Create a dock window to put the dock in. Checking for
   * GIMP_IS_TOOLBOX() is kind of ugly but not a disaster. We need
   * the dock window correctly configured if we create it for the
   * toolbox
   */
  dock_window =
    gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                    screen,
                                    monitor,
                                    NULL /*ui_manager*/,
                                    (contains_toolbox ?
                                     "gimp-toolbox-window" :
                                     "gimp-dock-window"),
                                    -1 /*view_size*/,
                                    FALSE /*present*/);

  for (iter = docks; iter; iter = g_list_next (iter))
    {
      GimpDock *dock = GIMP_DOCK (iter->data);

      /* Move the dock to the window */
      g_object_ref (dock);
      gimp_dock_columns_remove_dock (dock_columns, dock);
      gimp_dock_window_add_dock (GIMP_DOCK_WINDOW (dock_window), dock, -1);
      g_object_unref (dock);
    }

  /* Position the window */
  if (screen_side == GIMP_ALIGN_LEFT)
    {
      g_snprintf (geometry, sizeof (geometry), "%+d%+d",
                  monitor_rect.x,
                  monitor_rect.y);
    }
  else if (screen_side == GIMP_ALIGN_RIGHT)
    {
      g_snprintf (geometry, sizeof (geometry), "%+d%+d",
                  monitor_rect.x + monitor_rect.width - original_size.width,
                  monitor_rect.y);
    }
  else
    {
      gimp_assert_not_reached ();
    }

  gtk_window_parse_geometry (GTK_WINDOW (dock_window), geometry);

  /* Try to keep the same size */
  gtk_window_set_default_size (GTK_WINDOW (dock_window),
                               original_size.width,
                               original_size.height);

  /* Don't forget to show the window */
  gtk_widget_show (dock_window);

  g_list_free (docks);
}

/**
 * gimp_ui_configurer_separate_shells:
 * @ui_configurer:
 * @source_image_window:
 *
 * Create one image window per display shell and move it there.
 **/
static void
gimp_ui_configurer_separate_shells (GimpUIConfigurer *ui_configurer,
                                    GimpImageWindow  *source_image_window)
{
  GimpDisplayShell *active_shell  = gimp_image_window_get_active_shell (source_image_window);
  GimpImageWindow  *active_window = NULL;

  /* The last display shell remains in its window */
  while (gimp_image_window_get_n_shells (source_image_window) > 1)
    {
      GimpImageWindow  *new_image_window;
      GimpDisplayShell *shell;

      /* Create a new image window */
      new_image_window = gimp_image_window_new (ui_configurer->p->gimp,
                                                NULL,
                                                gimp_dialog_factory_get_singleton (),
                                                gtk_widget_get_screen (GTK_WIDGET (source_image_window)),
                                                gimp_widget_get_monitor (GTK_WIDGET (source_image_window)));
      /* Move the shell there */
      shell = gimp_image_window_get_shell (source_image_window, 1);

      if (shell == active_shell)
        active_window = new_image_window;

      g_object_ref (shell);
      gimp_image_window_remove_shell (source_image_window, shell);
      gimp_image_window_add_shell (new_image_window, shell);
      g_object_unref (shell);

      /* FIXME: If we don't set a size request here the window will be
       * too small. Get rid of this hack and fix it the proper way
       */
      gtk_widget_set_size_request (GTK_WIDGET (new_image_window), 640, 480);

      /* Show after we have added the shell */
      gtk_widget_show (GTK_WIDGET (new_image_window));
    }

  /* If none of the shells were active, I assume the first one is. */
  if (active_window == NULL)
    active_window = source_image_window;

  /* The active tab must stay at the top of the windows stack. */
  gtk_window_present (GTK_WINDOW (active_window));
}

/**
 * gimp_ui_configurer_configure_for_single_window:
 * @ui_configurer:
 *
 * Move docks and display shells into a single window.
 **/
static void
gimp_ui_configurer_configure_for_single_window (GimpUIConfigurer *ui_configurer)
{
  Gimp             *gimp              = ui_configurer->p->gimp;
  GList            *windows           = gimp_get_image_windows (gimp);
  GList            *iter              = NULL;
  GimpImageWindow  *uber_image_window = NULL;
  GimpDisplay      *active_display    = gimp_context_get_display (gimp_get_user_context (gimp));
  GimpDisplayShell *active_shell      = gimp_display_get_shell (active_display);

  /* Get and setup the window to put everything in */
  uber_image_window = gimp_ui_configurer_get_uber_window (ui_configurer);

  /* Mve docks to the left and right side of the image window */
  gimp_ui_configurer_move_docks_to_columns (ui_configurer,
                                            uber_image_window);

  /* Move image shells from other windows to the uber image window */
  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GimpImageWindow *image_window = GIMP_IMAGE_WINDOW (iter->data);

      /* Don't move stuff to itself */
      if (image_window == uber_image_window)
        continue;

      /* Put the displays in the rest of the image windows into
       * the uber image window
       */
      gimp_ui_configurer_move_shells (ui_configurer,
                                      image_window,
                                      uber_image_window);
      /* Destroy the window */
      gimp_image_window_destroy (image_window);
    }

  /* Ensure the context shell remains active after mode switch. */
  gimp_image_window_set_active_shell (uber_image_window, active_shell);

  g_list_free (windows);
}

/**
 * gimp_ui_configurer_configure_for_multi_window:
 * @ui_configurer:
 *
 * Moves all display shells into their own image window.
 **/
static void
gimp_ui_configurer_configure_for_multi_window (GimpUIConfigurer *ui_configurer)
{
  Gimp  *gimp    = ui_configurer->p->gimp;
  GList *windows = gimp_get_image_windows (gimp);
  GList *iter    = NULL;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GimpImageWindow *image_window = GIMP_IMAGE_WINDOW (iter->data);

      gimp_ui_configurer_separate_docks (ui_configurer, image_window);

      gimp_ui_configurer_separate_shells (ui_configurer, image_window);
    }

  g_list_free (windows);
}

/**
 * gimp_ui_configurer_get_uber_window:
 * @ui_configurer:
 *
 * Returns: The window to be used as the main window for single-window
 *          mode.
 **/
static GimpImageWindow *
gimp_ui_configurer_get_uber_window (GimpUIConfigurer *ui_configurer)
{
  Gimp             *gimp         = ui_configurer->p->gimp;
  GimpDisplay      *display      = gimp_get_display_iter (gimp)->data;
  GimpDisplayShell *shell        = gimp_display_get_shell (display);
  GimpImageWindow  *image_window = gimp_display_shell_get_window (shell);

  return image_window;
}

/**
 * gimp_ui_configurer_update_appearance:
 * @ui_configurer:
 *
 * Updates the appearance of all shells in all image windows, so they
 * do whatever they deem necessary to fit the new UI mode mode.
 **/
static void
gimp_ui_configurer_update_appearance (GimpUIConfigurer *ui_configurer)
{
  Gimp  *gimp    = ui_configurer->p->gimp;
  GList *windows = gimp_get_image_windows (gimp);
  GList *list;

  for (list = windows; list; list = g_list_next (list))
    {
      GimpImageWindow *image_window = GIMP_IMAGE_WINDOW (list->data);
      gint             n_shells;
      gint             i;

      n_shells = gimp_image_window_get_n_shells (image_window);

      for (i = 0; i < n_shells; i++)
        {
          GimpDisplayShell *shell;

          shell = gimp_image_window_get_shell (image_window, i);

          gimp_display_shell_appearance_update (shell);
        }
    }

  g_list_free (windows);
}

/**
 * gimp_ui_configurer_configure:
 * @ui_configurer:
 * @single_window_mode:
 *
 * Configure the UI.
 **/
void
gimp_ui_configurer_configure (GimpUIConfigurer *ui_configurer,
                              gboolean          single_window_mode)
{
  if (single_window_mode)
    gimp_ui_configurer_configure_for_single_window (ui_configurer);
  else
    gimp_ui_configurer_configure_for_multi_window (ui_configurer);

  gimp_ui_configurer_update_appearance (ui_configurer);
}
