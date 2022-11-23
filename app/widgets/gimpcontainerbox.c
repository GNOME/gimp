/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerbox.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"

#include "ligmacontainerbox.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmapropwidgets.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"


static void   ligma_container_box_view_iface_init   (LigmaContainerViewInterface *iface);
static void   ligma_container_box_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_container_box_constructed       (GObject      *object);

static GtkWidget * ligma_container_box_get_preview  (LigmaDocked   *docked,
                                                    LigmaContext  *context,
                                                    GtkIconSize   size);
static void        ligma_container_box_set_context  (LigmaDocked   *docked,
                                                    LigmaContext  *context);


G_DEFINE_TYPE_WITH_CODE (LigmaContainerBox, ligma_container_box,
                         LIGMA_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_container_box_view_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_container_box_docked_iface_init))

#define parent_class ligma_container_box_parent_class


static void
ligma_container_box_class_init (LigmaContainerBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_container_box_constructed;
  object_class->set_property = ligma_container_view_set_property;
  object_class->get_property = ligma_container_view_get_property;

  ligma_container_view_install_properties (object_class);
}

static void
ligma_container_box_init (LigmaContainerBox *box)
{
  GtkWidget *sb;

  box->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), box->scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (box->scrolled_win);

  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (box->scrolled_win));

  gtk_widget_set_can_focus (sb, FALSE);
}

static void
ligma_container_box_view_iface_init (LigmaContainerViewInterface *iface)
{
}

static void
ligma_container_box_docked_iface_init (LigmaDockedInterface *iface)
{
  iface->get_preview = ligma_container_box_get_preview;
  iface->set_context = ligma_container_box_set_context;
}

static void
ligma_container_box_constructed (GObject *object)
{
  LigmaContainerBox *box = LIGMA_CONTAINER_BOX (object);

  /* This is evil: the hash table of "insert_data" is created on
   * demand when LigmaContainerView API is used, using a
   * value_free_func that is set in the interface_init functions of
   * its implementors. Therefore, no LigmaContainerView API must be
   * called from any init() function, because the interface_init()
   * function of a subclass that sets the right value_free_func might
   * not have been called yet, leaving the insert_data hash table
   * without memory management.
   *
   * Call LigmaContainerView API from GObject::constructed() instead,
   * which runs after everything is set up correctly.
   */
  ligma_container_view_set_dnd_widget (LIGMA_CONTAINER_VIEW (box),
                                      box->scrolled_win);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

void
ligma_container_box_set_size_request (LigmaContainerBox *box,
                                     gint              width,
                                     gint              height)
{
  LigmaContainerView      *view;
  GtkScrolledWindowClass *sw_class;
  GtkStyleContext        *sw_style;
  GtkWidget              *sb;
  GtkRequisition          req;
  GtkBorder               border;
  gint                    view_size;
  gint                    scrollbar_width;
  gint                    border_x;
  gint                    border_y;

  g_return_if_fail (LIGMA_IS_CONTAINER_BOX (box));

  view = LIGMA_CONTAINER_VIEW (box);

  view_size = ligma_container_view_get_view_size (view, NULL);

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

  sw_style = gtk_widget_get_style_context (box->scrolled_win);

  gtk_style_context_get_border (sw_style, gtk_widget_get_state_flags (box->scrolled_win), &border);

  border_x += border.left + border.right + scrollbar_width;
  border_y += border.top + border.bottom;

  gtk_widget_set_size_request (box->scrolled_win,
                               width  > 0 ? width  + border_x : -1,
                               height > 0 ? height + border_y : -1);
}

static void
ligma_container_box_set_context (LigmaDocked  *docked,
                                LigmaContext *context)
{
  ligma_container_view_set_context (LIGMA_CONTAINER_VIEW (docked), context);
}

static GtkWidget *
ligma_container_box_get_preview (LigmaDocked   *docked,
                                LigmaContext  *context,
                                GtkIconSize   size)
{
  LigmaContainerView *view = LIGMA_CONTAINER_VIEW (docked);
  LigmaContainer     *container;
  GtkWidget         *preview;
  gint               width;
  gint               height;
  gint               border_width = 1;
  const gchar       *prop_name;

  container = ligma_container_view_get_container (view);

  g_return_val_if_fail (container != NULL, NULL);

  gtk_icon_size_lookup (size, &width, &height);

  prop_name = ligma_context_type_to_prop_name (ligma_container_get_children_type (container));

  preview = ligma_prop_view_new (G_OBJECT (context), prop_name,
                                context, height);
  LIGMA_VIEW (preview)->renderer->size = -1;

  ligma_container_view_get_view_size (view, &border_width);

  border_width = MIN (1, border_width);

  ligma_view_renderer_set_size_full (LIGMA_VIEW (preview)->renderer,
                                    width, height, border_width);

  return preview;
}
