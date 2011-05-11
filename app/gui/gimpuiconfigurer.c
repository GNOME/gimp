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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimptoolbox.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpimagewindow.h"

#include "menus/menus.h"

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
                                                                         GimpDockColumns   *dock_columns,
                                                                         gboolean           only_toolbox);
static void              gimp_ui_configurer_move_shells                 (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window,
                                                                         GimpImageWindow   *target_image_window);
static void              gimp_ui_configurer_separate_docks              (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window);
static void              gimp_ui_configurer_move_docks_to_window        (GimpUIConfigurer  *ui_configurer,
                                                                         GimpDockColumns   *dock_columns);
static void              gimp_ui_configurer_separate_shells             (GimpUIConfigurer  *ui_configurer,
                                                                         GimpImageWindow   *source_image_window);
static void              gimp_ui_configurer_configure_for_single_window (GimpUIConfigurer  *ui_configurer);
static void              gimp_ui_configurer_configure_for_multi_window  (GimpUIConfigurer  *ui_configurer);
static GimpImageWindow * gimp_ui_configurer_get_uber_window             (GimpUIConfigurer  *ui_configurer);


G_DEFINE_TYPE (GimpUIConfigurer, gimp_ui_configurer, GIMP_TYPE_OBJECT)

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
  g_type_class_add_private (klass,
                            sizeof (GimpUIConfigurerPrivate));
}

static void
gimp_ui_configurer_init (GimpUIConfigurer *ui_configurer)
{
  ui_configurer->p = G_TYPE_INSTANCE_GET_PRIVATE (ui_configurer,
                                                  GIMP_TYPE_UI_CONFIGURER,
                                                  GimpUIConfigurerPrivate);
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
gimp_ui_configurer_move_docks_to_columns (GimpUIConfigurer  *ui_configurer,
                                          GimpDockColumns   *dock_columns,
                                          gboolean           only_toolbox)
{
  GList *dialogs     = NULL;
  GList *dialog_iter = NULL;

  dialogs =
    g_list_copy (gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ()));

  for (dialog_iter = dialogs; dialog_iter; dialog_iter = dialog_iter->next)
    {
      GimpDockWindow    *dock_window    = NULL;
      GimpDockContainer *dock_container = NULL;
      GList             *docks          = NULL;
      GList             *dock_iter      = NULL;

      if (!GIMP_IS_DOCK_WINDOW (dialog_iter->data))
        continue;

      dock_window    = GIMP_DOCK_WINDOW (dialog_iter->data);
      dock_container = GIMP_DOCK_CONTAINER (dock_window);

      docks = gimp_dock_container_get_docks (dock_container);
      for (dock_iter = docks; dock_iter; dock_iter = dock_iter->next)
        {
          GimpDock *dock = GIMP_DOCK (dock_iter->data);

          if (only_toolbox &&
              ! GIMP_IS_TOOLBOX (dock))
            continue;

          /* Move the dock from the image window to the dock columns
           * widget. Note that we need a ref while the dock is parentless
           */
          g_object_ref (dock);
          gimp_dock_window_remove_dock (dock_window, dock);
          gimp_dock_columns_add_dock (dock_columns, dock, -1);
          g_object_unref (dock);
        }
      g_list_free (docks);

      /* Kill the window if removing the dock didn't destroy it
       * already. This will be the case for the toolbox dock window
       */
      /* FIXME: We should solve this in a more elegant way, valgrind
       * complains about invalid reads when the dock window already is
       * destroyed
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

  gimp_ui_configurer_move_docks_to_window (ui_configurer, left_docks);
  gimp_ui_configurer_move_docks_to_window (ui_configurer, right_docks);
}

/**
 * gimp_ui_configurer_move_docks_to_window:
 * @dock_columns:
 *
 * Moves docks in @dock_columns into a new #GimpDockWindow.
 * FIXME: Properly session manage
 */
static void
gimp_ui_configurer_move_docks_to_window (GimpUIConfigurer *ui_configurer,
                                         GimpDockColumns  *dock_columns)
{
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (dock_columns));
  GList     *docks  = g_list_copy (gimp_dock_columns_get_docks (dock_columns));
  GList     *iter   = NULL;

  for (iter = docks; iter; iter = iter->next)
    {
      GimpDock  *dock        = GIMP_DOCK (iter->data);
      GtkWidget *dock_window = NULL;

      /* Create a dock window to put the dock in. Checking for
       * GIMP_IS_TOOLBOX() is kind of ugly but not a disaster. We need
       * the dock window correctly configured if we create it for the
       * toolbox
       */
      dock_window =
        gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                        screen,
                                        NULL /*ui_manager*/,
                                        (GIMP_IS_TOOLBOX (dock) ?
                                         "gimp-toolbox-window" :
                                         "gimp-dock-window"),
                                        -1 /*view_size*/,
                                        FALSE /*present*/);

      /* Move the dock to the window */
      g_object_ref (dock);
      gimp_dock_columns_remove_dock (dock_columns, dock);
      gimp_dock_window_add_dock (GIMP_DOCK_WINDOW (dock_window), dock, -1);
      g_object_unref (dock);

      /* Don't forget to show the window */
      gtk_widget_show (dock_window);
    }

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
  /* The last display shell remains in its window */
  while (gimp_image_window_get_n_shells (source_image_window) > 1)
    {
      GimpImageWindow  *new_image_window;
      GimpDisplayShell *shell;

      /* Create a new image window */
      new_image_window = gimp_image_window_new (ui_configurer->p->gimp,
                                                NULL,
                                                global_menu_factory,
                                                gimp_dialog_factory_get_singleton ());
      /* Move the shell there */
      shell = gimp_image_window_get_shell (source_image_window, 1);

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
  Gimp            *gimp              = ui_configurer->p->gimp;
  GList           *windows           = gimp_get_image_windows (gimp);
  GList           *iter              = NULL;
  GimpImageWindow *uber_image_window = NULL;
  GimpDockColumns *left_docks        = NULL;
  GimpDockColumns *right_docks       = NULL;

  /* Get and setup the window to put everything in */
  uber_image_window = gimp_ui_configurer_get_uber_window (ui_configurer);

  /* Get dock areas */
  left_docks  = gimp_image_window_get_left_docks (uber_image_window);
  right_docks = gimp_image_window_get_right_docks (uber_image_window);

  /* First move the toolbox to the left side of the image
   * window
   */
  gimp_ui_configurer_move_docks_to_columns (ui_configurer,
                                            left_docks,
                                            TRUE /*only_toolbox*/);

  /* Then move the other docks to the right side of the image
   * window
   */
  gimp_ui_configurer_move_docks_to_columns (ui_configurer,
                                            right_docks,
                                            FALSE /*only_toolbox*/);

  /* Move stuff from other windows to the uber image window */
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
 * do whatever they deem neccessary to fit the new UI mode mode.
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
