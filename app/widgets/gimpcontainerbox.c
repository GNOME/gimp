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
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimppropwidgets.h"


static void   gimp_container_box_class_init      (GimpContainerBoxClass *klass);
static void   gimp_container_box_init            (GimpContainerBox      *box);
static void   gimp_container_box_view_iface_init   (GimpContainerViewInterface *view_iface);
static void   gimp_container_box_docked_iface_init (GimpDockedInterface *docked_iface);

static GtkWidget * gimp_container_box_get_preview  (GimpDocked   *docked,
                                                    GimpContext  *context,
                                                    GtkIconSize   size);
static void        gimp_container_box_set_context  (GimpDocked   *docked,
                                                    GimpContext  *context);


static GimpEditorClass *parent_class = NULL;


GType
gimp_container_box_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo box_info =
      {
        sizeof (GimpContainerBoxClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_box_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerBox),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_box_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_container_box_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_container_box_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpContainerBox",
                                     &box_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_container_box_class_init (GimpContainerBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  g_object_class_override_property (object_class,
                                    GIMP_CONTAINER_VIEW_PROP_CONTAINER,
                                    "container");
  g_object_class_override_property (object_class,
                                    GIMP_CONTAINER_VIEW_PROP_CONTEXT,
                                    "context");
  g_object_class_override_property (object_class,
                                    GIMP_CONTAINER_VIEW_PROP_REORDERABLE,
                                    "reorderable");
  g_object_class_override_property (object_class,
                                    GIMP_CONTAINER_VIEW_PROP_PREVIEW_SIZE,
                                    "preview-size");
  g_object_class_override_property (object_class,
                                    GIMP_CONTAINER_VIEW_PROP_PREVIEW_BORDER_WIDTH,
                                    "preview-border-width");
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
gimp_container_box_view_iface_init (GimpContainerViewInterface *view_iface)
{
}

static void
gimp_container_box_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->get_preview = gimp_container_box_get_preview;
  docked_iface->set_context = gimp_container_box_set_context;
}

void
gimp_container_box_set_size_request (GimpContainerBox *box,
                                     gint              width,
                                     gint              height)
{
  GimpContainerView      *view;
  GtkScrolledWindowClass *sw_class;
  GtkRequisition          req;
  gint                    preview_size;
  gint                    scrollbar_width;
  gint                    border_x;
  gint                    border_y;

  g_return_if_fail (GIMP_IS_CONTAINER_BOX (box));

  view = GIMP_CONTAINER_VIEW (box);

  preview_size = gimp_container_view_get_preview_size (view, NULL);

  g_return_if_fail (width  <= 0 || width  >= preview_size);
  g_return_if_fail (height <= 0 || height >= preview_size);

  sw_class = GTK_SCROLLED_WINDOW_GET_CLASS (box->scrolled_win);

  if (sw_class->scrollbar_spacing >= 0)
    scrollbar_width = sw_class->scrollbar_spacing;
  else
    gtk_widget_style_get (GTK_WIDGET (box->scrolled_win),
                          "scrollbar_spacing", &scrollbar_width,
                          NULL);

  gtk_widget_size_request (GTK_SCROLLED_WINDOW (box->scrolled_win)->vscrollbar,
                           &req);
  scrollbar_width += req.width;

  border_x = box->scrolled_win->style->xthickness * 2 + scrollbar_width;
  border_y = box->scrolled_win->style->ythickness * 2;

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
  GdkScreen         *screen;
  gint               width;
  gint               height;
  gint               border_width = 1;
  const gchar       *prop_name;

  container = gimp_container_view_get_container (view);

  g_return_val_if_fail (container != NULL, NULL);

  screen = gtk_widget_get_screen (GTK_WIDGET (box));
  gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                     size, &width, &height);

  prop_name = gimp_context_type_to_prop_name (container->children_type);

  preview = gimp_prop_preview_new (G_OBJECT (context), prop_name, height);
  GIMP_VIEW (preview)->renderer->size = -1;

  gimp_container_view_get_preview_size (view, &border_width);

  border_width = MIN (1, border_width);

  gimp_view_renderer_set_size_full (GIMP_VIEW (preview)->renderer,
                                    width, height, border_width);

  return preview;
}
