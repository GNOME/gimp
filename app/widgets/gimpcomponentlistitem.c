/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcomponentlistitem.c
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpviewable.h"

#include "display/gimpdisplay-foreach.h"

#include "gimpcomponentlistitem.h"
#include "gimpdnd.h"
#include "gimpimagepreview.h"
#include "gimppreview.h"

#include "libgimp/gimpintl.h"


static void    gimp_component_list_item_class_init (GimpComponentListItemClass *klass);
static void    gimp_component_list_item_init       (GimpComponentListItem      *list_item);

static void    gimp_component_list_item_set_viewable (GimpListItem    *list_item,
                                                      GimpViewable    *viewable);

static void    gimp_component_list_item_eye_toggled  (GtkWidget       *widget,
                                                      gpointer         data);

static void    gimp_component_list_item_visibility_changed 
                                                     (GimpImage       *gimage,
                                                      GimpChannelType  channel,
                                                      gpointer         data);

static void    gimp_component_list_item_active_changed
                                                     (GimpImage       *gimage,
                                                      GimpChannelType  channel,
                                                      gpointer         data);

static gchar * gimp_component_list_item_get_name     (GtkWidget       *widget,
                                                      gchar          **tooltip);


static GimpListItemClass *parent_class = NULL;


GType
gimp_component_list_item_get_type (void)
{
  static GType list_item_type = 0;

  if (! list_item_type)
    {
      static const GTypeInfo list_item_info =
      {
        sizeof (GimpComponentListItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_component_list_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpComponentListItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_component_list_item_init,
      };

      list_item_type = g_type_register_static (GIMP_TYPE_LIST_ITEM,
                                               "GimpComponentListItem",
                                               &list_item_info, 0);
    }

  return list_item_type;
}

static void
gimp_component_list_item_class_init (GimpComponentListItemClass *klass)
{
  GimpListItemClass *list_item_class;

  list_item_class = GIMP_LIST_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  list_item_class->set_viewable = gimp_component_list_item_set_viewable;
}

static void
gimp_component_list_item_init (GimpComponentListItem *list_item)
{
  GtkWidget *abox;
  GtkWidget *image;

  list_item->channel = 0;

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GIMP_LIST_ITEM (list_item)->hbox), abox,
                      FALSE, FALSE, 0);
  gtk_widget_show (abox);

  list_item->eye_button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (list_item->eye_button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (abox), list_item->eye_button);
  gtk_widget_show (list_item->eye_button);

  g_signal_connect (G_OBJECT (list_item->eye_button), "realize",
		    G_CALLBACK (gimp_list_item_button_realize),
		    list_item);

  g_signal_connect (G_OBJECT (list_item->eye_button), "state_changed",
		    G_CALLBACK (gimp_list_item_button_state_changed),
		    list_item);

  image = gtk_image_new_from_stock (GIMP_STOCK_VISIBLE,
				    GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list_item->eye_button), image);
  gtk_widget_show (image);
}

GtkWidget *
gimp_component_list_item_new (GimpImage       *gimage,
			      gint             preview_size,
			      GimpChannelType  channel)
{
  GimpListItem *list_item;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 256, NULL);
 
  list_item = g_object_new (GIMP_TYPE_COMPONENT_LIST_ITEM, NULL);

  list_item->preview_size  = preview_size;
  list_item->get_name_func = gimp_component_list_item_get_name;

  GIMP_COMPONENT_LIST_ITEM (list_item)->channel = channel;

  gimp_list_item_set_viewable (list_item, GIMP_VIEWABLE (gimage));

  GTK_PREVIEW (list_item->preview)->bpp  = 1;
  GTK_PREVIEW (list_item->preview)->type = GTK_PREVIEW_GRAYSCALE;

  return GTK_WIDGET (list_item);
}

static void
gimp_component_list_item_set_viewable (GimpListItem *list_item,
				       GimpViewable *viewable)
{
  GimpComponentListItem *component_item;
  GimpImage             *gimage;
  gboolean               visible;
  gboolean               active;
  gint                   pixel;

  if (GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable)
    GIMP_LIST_ITEM_CLASS (parent_class)->set_viewable (list_item, viewable);

  component_item = GIMP_COMPONENT_LIST_ITEM (list_item);
  gimage         = GIMP_IMAGE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible        = gimp_image_get_component_visible (gimage,
						     component_item->channel);
  active         = gimp_image_get_component_active (gimage,
						    component_item->channel);

  switch (component_item->channel)
    {
    case GIMP_RED_CHANNEL:     pixel = RED_PIX;     break;
    case GIMP_GREEN_CHANNEL:   pixel = GREEN_PIX;   break;
    case GIMP_BLUE_CHANNEL:    pixel = BLUE_PIX;    break;
    case GIMP_GRAY_CHANNEL:    pixel = GRAY_PIX;    break;
    case GIMP_INDEXED_CHANNEL: pixel = INDEXED_PIX; break;
    case GIMP_ALPHA_CHANNEL:   pixel = ALPHA_PIX;   break;

    default:
      pixel = 0;
      g_assert_not_reached ();
    }

  GIMP_IMAGE_PREVIEW (list_item->preview)->channel = pixel;

  if (! visible)
    {
      GtkRequisition requisition;

      gtk_widget_size_request (component_item->eye_button, &requisition);

      gtk_widget_set_usize (component_item->eye_button,
                            requisition.width,
                            requisition.height);
      gtk_widget_hide (GTK_BIN (component_item->eye_button)->child);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (component_item->eye_button),
                                visible);

  if (active)
    gtk_item_select (GTK_ITEM (list_item));

  g_signal_connect (G_OBJECT (component_item->eye_button), "toggled",
		    G_CALLBACK (gimp_component_list_item_eye_toggled),
		    list_item);

  g_signal_connect_object (G_OBJECT (viewable), "component_visibility_changed",
			   G_CALLBACK (gimp_component_list_item_visibility_changed),
			   G_OBJECT (list_item),
			   0);

  g_signal_connect_object (G_OBJECT (viewable), "component_active_changed",
			   G_CALLBACK (gimp_component_list_item_active_changed),
			   G_OBJECT (list_item),
			   0);
}

static void
gimp_component_list_item_eye_toggled (GtkWidget *widget,
				      gpointer   data)
{
  GimpComponentListItem *component_item;
  GimpListItem          *list_item;
  GimpImage             *gimage;
  gboolean               visible;

  component_item = GIMP_COMPONENT_LIST_ITEM (data);
  list_item      = GIMP_LIST_ITEM (data);
  gimage         = GIMP_IMAGE (GIMP_PREVIEW (list_item->preview)->viewable);
  visible        = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (visible != gimp_image_get_component_visible (gimage,
						   component_item->channel))
    {
      if (! visible)
        {
          gtk_widget_set_usize (GTK_WIDGET (widget),
                                GTK_WIDGET (widget)->allocation.width,
                                GTK_WIDGET (widget)->allocation.height);
          gtk_widget_hide (GTK_BIN (widget)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (widget)->child);
          gtk_widget_set_usize (GTK_WIDGET (widget), -1, -1);
        }

      g_signal_handlers_block_by_func (G_OBJECT (gimage),
				       gimp_component_list_item_visibility_changed,
				       list_item);

      gimp_image_set_component_visible (gimage, component_item->channel,
					visible);

      g_signal_handlers_unblock_by_func (G_OBJECT (gimage),
					 gimp_component_list_item_visibility_changed,
					 list_item);

      gdisplays_flush ();
    }
}

static void
gimp_component_list_item_visibility_changed (GimpImage       *gimage,
					     GimpChannelType  channel,
					     gpointer         data)
{
  GimpComponentListItem *component_item;
  GimpListItem          *list_item;
  GtkToggleButton       *toggle;
  gboolean               visible;

  component_item = GIMP_COMPONENT_LIST_ITEM (data);

  if (channel != component_item->channel)
    return;

  list_item = GIMP_LIST_ITEM (data);
  toggle    = GTK_TOGGLE_BUTTON (component_item->eye_button);
  visible   = gimp_image_get_component_visible (gimage, 
                                                component_item->channel);

  if (visible != toggle->active)
    {
      if (! visible)
        {
          gtk_widget_set_usize (GTK_WIDGET (toggle),
                                GTK_WIDGET (toggle)->allocation.width,
                                GTK_WIDGET (toggle)->allocation.height);
          gtk_widget_hide (GTK_BIN (toggle)->child);
        }
      else
        {
          gtk_widget_show (GTK_BIN (toggle)->child);
          gtk_widget_set_usize (GTK_WIDGET (toggle), -1, -1);
        }

      g_signal_handlers_block_by_func (G_OBJECT (toggle),
				       gimp_component_list_item_eye_toggled,
				       list_item);

      gtk_toggle_button_set_active (toggle, visible);

      g_signal_handlers_unblock_by_func (G_OBJECT (toggle),
					 gimp_component_list_item_eye_toggled,
					 list_item);
    }
}

static void
gimp_component_list_item_active_changed (GimpImage       *gimage,
					 GimpChannelType  channel,
					 gpointer         data)
{
  GimpComponentListItem *component_item;
  gboolean               active;

  component_item = GIMP_COMPONENT_LIST_ITEM (data);

  if (channel != component_item->channel)
    return;

  active = gimp_image_get_component_active (gimage, component_item->channel);

  if (active)
    gtk_item_select (GTK_ITEM (data));
  else
    gtk_item_deselect (GTK_ITEM (data));
}

static gchar *
gimp_component_list_item_get_name (GtkWidget  *widget,
				   gchar     **tooltip)
{
  GimpComponentListItem *component_item;

  component_item = GIMP_COMPONENT_LIST_ITEM (widget);

  if (tooltip)
    *tooltip = NULL;

  switch (component_item->channel)
    {
    case GIMP_RED_CHANNEL:     return g_strdup (_("Red"));     break;
    case GIMP_GREEN_CHANNEL:   return g_strdup (_("Green"));   break;
    case GIMP_BLUE_CHANNEL:    return g_strdup (_("Blue"));    break;
    case GIMP_GRAY_CHANNEL:    return g_strdup (_("Gray"));    break;
    case GIMP_INDEXED_CHANNEL: return g_strdup (_("Indexed")); break;
    case GIMP_ALPHA_CHANNEL:   return g_strdup (_("Alpha"));   break;

    default:                   
      return g_strdup (_("EEEEK"));
    }
}
