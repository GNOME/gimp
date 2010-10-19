/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerbox.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpcontainerbox.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimppropwidgets.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"


static void   gimp_container_box_view_iface_init   (GimpContainerViewInterface *iface);
static void   gimp_container_box_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_container_box_constructed       (GObject      *object);

static GtkWidget * gimp_container_box_get_preview  (GimpDocked   *docked,
                                                    GimpContext  *context,
                                                    GtkIconSize   size);
static void        gimp_container_box_set_context  (GimpDocked   *docked,
                                                    GimpContext  *context);


G_DEFINE_TYPE_WITH_CODE (GimpContainerBox, gimp_container_box,
                         GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_box_view_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_container_box_docked_iface_init))

#define parent_class gimp_container_box_parent_class


static void
gimp_container_box_class_init (GimpContainerBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_container_box_constructed;
  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  gimp_container_view_install_properties (object_class);
}

static void
gimp_container_box_init (GimpContainerBox *box)
{
  GtkWidget *sb;

  box->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), box->scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (box->scrolled_win);

  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (box->scrolled_win));

  gtk_widget_set_can_focus (sb, FALSE);
}

static void
gimp_container_box_view_iface_init (GimpContainerViewInterface *iface)
{
}

static void
gimp_container_box_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_preview = gimp_container_box_get_preview;
  iface->set_context = gimp_container_box_set_context;
}

static void
gimp_container_box_constructed (GObject *object)
{
  GimpContainerBox *box = GIMP_CONTAINER_BOX (object);

  /* This is evil: the hash table of "insert_data" is created on
   * demand when GimpContainerView API is used, using a
   * value_free_func that is set in the interface_init functions of
   * its implementors. Therefore, no GimpContainerView API must be
   * called from any init() function, because the interface_init()
   * function of a subclass that sets the right value_free_func might
   * not have been called yet, leaving the insert_data hash table
   * without memory management.
   *
   * Call GimpContainerView API from GObject::constructed() instead,
   * which runs after everything is set up correctly.
   */
  gimp_container_view_set_dnd_widget (GIMP_CONTAINER_VIEW (box),
                                      box->scrolled_win);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

void
gimp_container_box_set_size_request (GimpContainerBox *box,
                                     gint              width,
                                     gint              height)
{
  GimpContainerView      *view;
  GtkScrolledWindowClass *sw_class;
  GtkStyle               *sw_style;
  GtkWidget              *sb;
  GtkRequisition          req;
  gint                    view_size;
  gint                    scrollbar_width;
  gint                    border_x;
  gint                    border_y;

  g_return_if_fail (GIMP_IS_CONTAINER_BOX (box));

  view = GIMP_CONTAINER_VIEW (box);

  view_size = gimp_container_view_get_view_size (view, NULL);

  g_return_if_fail (width  <= 0 || width  >= view_size);
  g_return_if_fail (height <= 0 || height >= view_size);

  sw_class = GTK_SCROLLED_WINDOW_GET_CLASS (box->scrolled_win);

  if (sw_class->scrollbar_spacing >= 0)
    scrollbar_width = sw_class->scrollbar_spacing;
  else
    gtk_widget_style_get (GTK_WIDGET (box->scrolled_win),
                          "scrollbar-spacing", &scrollbar_width,
                          NULL);

  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (box->scrolled_win));

  gtk_widget_get_preferred_size (sb, &req, NULL);
  scrollbar_width += req.width;

  border_x = border_y = gtk_container_get_border_width (GTK_CONTAINER (box));

  sw_style = gtk_widget_get_style (box->scrolled_win);

  border_x += sw_style->xthickness * 2 + scrollbar_width;
  border_y += sw_style->ythickness * 2;

  gtk_widget_set_size_request (box->scrolled_win,
                               width  > 0 ? width  + border_x : -1,
                               height > 0 ? height + border_y : -1);
}

static void
gimp_container_box_set_context (GimpDocked  *docked,
                                GimpContext *context)
{
  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (docked), context);
}

static GtkWidget *
gimp_container_box_get_preview (GimpDocked   *docked,
                                GimpContext  *context,
                                GtkIconSize   size)
{
  GimpContainerBox  *box  = GIMP_CONTAINER_BOX (docked);
  GimpContainerView *view = GIMP_CONTAINER_VIEW (docked);
  GimpContainer     *container;
  GtkWidget         *preview;
  gint               width;
  gint               height;
  gint               border_width = 1;
  const gchar       *prop_name;

  container = gimp_container_view_get_container (view);

  g_return_val_if_fail (container != NULL, NULL);

  gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (box)),
                                     size, &width, &height);

  prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

  preview = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                context, height);
  GIMP_VIEW (preview)->renderer->size = -1;

  gimp_container_view_get_view_size (view, &border_width);

  border_width = MIN (1, border_width);

  gimp_view_renderer_set_size_full (GIMP_VIEW (preview)->renderer,
                                    width, height, border_width);

  return preview;
}
