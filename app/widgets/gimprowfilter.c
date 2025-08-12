/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowfilter.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimp-utils.h"
#include "core/gimpfilter.h"

#include "gimprowfilter.h"


typedef struct _GimpRowFilterPrivate GimpRowFilterPrivate;

struct _GimpRowFilterPrivate
{
  GtkWidget *active_toggle;
  GtkWidget *active_icon;
};

#define GET_PRIVATE(obj) \
  ((GimpRowFilterPrivate *) \
   gimp_row_filter_get_instance_private ((GimpRowFilter *) obj))


static void   gimp_row_filter_set_viewable        (GimpRow          *row,
                                                   GimpViewable     *viewable);
static void   gimp_row_filter_set_view_size       (GimpRow          *row);

static void   gimp_row_filter_real_active_toggled (GimpRowFilter    *row,
                                                   gboolean          active);

static void   gimp_row_filter_active_changed      (GimpFilter       *filter,
                                                   GimpRowFilter    *row);
static void   gimp_row_filter_active_toggled      (GtkToggleButton  *button,
                                                   GimpRowFilter    *row);


G_DEFINE_TYPE_WITH_PRIVATE (GimpRowFilter,
                            gimp_row_filter,
                            GIMP_TYPE_ROW)

#define parent_class gimp_row_filter_parent_class


static void
gimp_row_filter_class_init (GimpRowFilterClass *klass)
{
  GimpRowClass *row_class = GIMP_ROW_CLASS (klass);

  row_class->set_viewable      = gimp_row_filter_set_viewable;
  row_class->set_view_size     = gimp_row_filter_set_view_size;

  klass->active_property       = "active";
  klass->active_changed_signal = "active-changed";

  klass->active_toggled        = gimp_row_filter_real_active_toggled;
}

static void
gimp_row_filter_init (GimpRowFilter *row)
{
  GimpRowFilterPrivate *priv = GET_PRIVATE (row);
  GtkWidget            *box;

  box = _gimp_row_get_box (GIMP_ROW (row));

  priv->active_toggle = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (box), priv->active_toggle, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (box), priv->active_toggle, 0);
  gtk_widget_set_visible (priv->active_toggle, TRUE);

  g_signal_connect (priv->active_toggle, "toggled",
                    G_CALLBACK (gimp_row_filter_active_toggled),
                    row);

  priv->active_icon = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE,
                                                    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (priv->active_toggle),
                     priv->active_icon);
  gtk_widget_set_visible (priv->active_icon, TRUE);
}

static void
gimp_row_filter_set_viewable (GimpRow      *row,
                              GimpViewable *viewable)
{
  GimpViewable *old_viewable = gimp_row_get_viewable (row);

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            gimp_row_filter_active_changed,
                                            row);
    }

  GIMP_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      g_signal_connect (viewable,
                        GIMP_ROW_FILTER_GET_CLASS (row)->active_changed_signal,
                        G_CALLBACK (gimp_row_filter_active_changed),
                        row);

      gimp_row_filter_active_changed (GIMP_FILTER (viewable),
                                      GIMP_ROW_FILTER (row));
    }
}

static void
gimp_row_filter_set_view_size (GimpRow *row)
{
  GimpRowFilterPrivate *priv = GET_PRIVATE (row);
  gint                  view_size;

  GIMP_ROW_CLASS (parent_class)->set_view_size (row);

  view_size = gimp_row_get_view_size (row, NULL);
  view_size = gimp_view_size_get_smaller (view_size);

  gtk_image_set_pixel_size (GTK_IMAGE (priv->active_icon),
                            view_size);
}

static void
gimp_row_filter_real_active_toggled (GimpRowFilter *row,
                                     gboolean       active)
{
  GimpViewable *viewable = gimp_row_get_viewable (GIMP_ROW (row));

  if (viewable)
    g_object_set (viewable,
                  GIMP_ROW_FILTER_GET_CLASS (row)->active_property, active,
                  NULL);
}

static void
gimp_row_filter_active_changed (GimpFilter    *filter,
                                GimpRowFilter *row)
{
  GimpRowFilterPrivate *priv = GET_PRIVATE (row);
  gboolean              active;

  g_signal_handlers_block_by_func (priv->active_toggle,
                                   gimp_row_filter_active_toggled,
                                   row);

  g_object_get (filter,
                GIMP_ROW_FILTER_GET_CLASS (row)->active_property, &active,
                NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->active_toggle),
                                active);
  gtk_widget_set_opacity (priv->active_icon, active ? 1.0 : 0.0);

  g_signal_handlers_unblock_by_func (priv->active_toggle,
                                     gimp_row_filter_active_toggled,
                                     row);
}

static void
gimp_row_filter_active_toggled (GtkToggleButton *button,
                                GimpRowFilter   *row)
{
  GimpRowFilterPrivate *priv = GET_PRIVATE (row);
  gboolean              active;

  active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->active_toggle));

  GIMP_ROW_FILTER_GET_CLASS  (row)->active_toggled (row, active);
}
