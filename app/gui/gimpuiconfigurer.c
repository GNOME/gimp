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

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockwindow.h"

#include "display/gimpimagewindow.h"

#include "dialogs/dialogs.h"

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


static void  gimp_ui_configurer_set_property                (GObject           *object,
                                                             guint              property_id,
                                                             const GValue      *value,
                                                             GParamSpec        *pspec);
static void  gimp_ui_configurer_get_property                (GObject           *object,
                                                             guint              property_id,
                                                             GValue            *value,
                                                             GParamSpec        *pspec);
static void  gimp_ui_configurer_move_docks_to_columns       (GimpUIConfigurer  *ui_configurer,
                                                             GimpDialogFactory *dialog_factory,
                                                             GimpDockColumns   *dock_columns);
static void  gimp_ui_configurer_configure_for_single_window (GimpUIConfigurer  *ui_configurer);
static void  gimp_ui_configurer_configure_for_multi_window  (GimpUIConfigurer  *ui_configurer);


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
                                          GimpDialogFactory *dialog_factory,
                                          GimpDockColumns   *dock_columns)
{
  GList *iter        = NULL;
  GList *for_removal = NULL;

  for (iter = dialog_factory->open_dialogs; iter; iter = iter->next)
    {
      GimpDockWindow *dock_window = NULL;
      GimpDock       *dock        = NULL;

      if (!GIMP_IS_DOCK_WINDOW (iter->data))
        continue;

      dock_window = GIMP_DOCK_WINDOW (iter->data);
      dock        = gimp_dock_window_get_dock (dock_window);

      /* Move the dock from the image window to the dock columns
       * widget. Note that we need a ref while the dock is parentless
       */
      g_object_ref (dock);
      gimp_dock_window_remove_dock (dock_window, dock);
      gimp_dock_columns_add_dock (dock_columns, dock, -1);
      g_object_unref (dock);

      /* Queue for removal from the dialog factory. (We can't remove
       * while we iterate)
       */
      for_removal = g_list_prepend (for_removal, dock_window);
    }

  for (iter = for_removal; iter; iter = iter->next)
    {
      GtkWidget *dock_window = GTK_WIDGET (iter->data);

      /* Kill the dock window, we don't need it any longer */
      gimp_dialog_factory_remove_dialog (dialog_factory, dock_window);
      gtk_widget_destroy (GTK_WIDGET (dock_window));
    }
}

static void
gimp_ui_configurer_configure_for_single_window (GimpUIConfigurer *ui_configurer)
{
  Gimp    *gimp        = GIMP (ui_configurer->p->gimp);
  GList   *windows     = gimp_get_image_windows (gimp);
  GList   *iter        = NULL;
  gboolean docks_moved = FALSE;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GimpImageWindow *image_window = GIMP_IMAGE_WINDOW (iter->data);

      /* Move all docks to the first image window */
      if (! docks_moved)
        {
          GimpDockColumns *left_docks  = gimp_image_window_get_left_docks (image_window);
          GimpDockColumns *right_docks = gimp_image_window_get_right_docks (image_window);

          /* First move the toolbox to the left side of the image
           * window
           */
          gimp_ui_configurer_move_docks_to_columns (ui_configurer,
                                                    global_toolbox_factory,
                                                    left_docks);

          /* Then move the other docks to the right side of the image
           * window
           */
          gimp_ui_configurer_move_docks_to_columns (ui_configurer,
                                                    global_dock_factory,
                                                    right_docks);

          /* Don't do it again */
          docks_moved = TRUE;
        }

      /* FIXME: Move all displays to a single window */
      gimp_image_window_set_show_docks (image_window, TRUE);
    }

  g_list_free (windows);
}

static void
gimp_ui_configurer_configure_for_multi_window (GimpUIConfigurer *ui_configurer)
{
  Gimp  *gimp    = GIMP (ui_configurer->p->gimp);
  GList *windows = gimp_get_image_windows (gimp);
  GList *iter    = NULL;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GimpImageWindow *image_window = GIMP_IMAGE_WINDOW (iter->data);

      /* FIXME: Move all displays to a multiple windows */
      gimp_image_window_set_show_docks (image_window, FALSE);
    }

  g_list_free (windows);
}

void
gimp_ui_configurer_configure (GimpUIConfigurer *ui_configurer,
                              gboolean          single_window_mode)
{
  if (single_window_mode)
    gimp_ui_configurer_configure_for_single_window (ui_configurer);
  else
    gimp_ui_configurer_configure_for_multi_window (ui_configurer);
}
