/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerbox.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  gimp_container_view_install_properties (object_class);
}

static void
gimp_container_box_init (GimpContainerBox *box)
{
  GimpContainerView *view = GIMP_CONTAINER_VIEW (box);

  box->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), box->scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (box->scrolled_win);

  GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (box->scrolled_win)->vscrollbar,
                          GTK_CAN_FOCUS);

  gimp_container_view_set_dnd_widget (view, box->scrolled_win);
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

void
gimp_container_box_set_size_request (GimpContainerBox *box,
                                     gint              width,
                                     gint              height)
{
  GimpContainerView      *view;
  GtkScrolledWindowClass *sw_class;
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

  gtk_widget_size_request (GTK_SCROLLED_WINDOW (box->scrolled_win)->vscrollbar,
                           &req);
  scrollbar_width += req.width;

  border_x = GTK_CONTAINER (box)->border_width;
  border_y = GTK_CONTAINER (box)->border_width;

  border_x += box->scrolled_win->style->xthickness * 2 + scrollbar_width;
  border_y += box->scrolled_win->style->ythickness * 2;

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

  prop_name = gimp_context_type_to_prop_name (container->children_type);

  preview = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                context, height);
  GIMP_VIEW (preview)->renderer->size = -1;

  gimp_container_view_get_view_size (view, &border_width);

  border_width = MIN (1, border_width);

  gimp_view_renderer_set_size_full (GIMP_VIEW (preview)->renderer,
                                    width, height, border_width);

  return preview;
}
