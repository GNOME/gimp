/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "gui-types.h"

#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpcomponenteditor.h"
#include "widgets/gimpitemlistview.h"
#include "widgets/gimpitemfactory.h"

#include "channels-commands.h"
#include "channels-menu.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


GimpItemFactoryEntry channels_menu_entries[] =
{
  { { N_("/New Channel..."), "<control>N",
      channels_new_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "dialogs/new_channel.html", NULL },
  { { N_("/Raise Channel"), "<control>F",
      channels_raise_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "raise_channel.html", NULL },
  { { N_("/Lower Channel"), "<control>B",
      channels_lower_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "lower_channel.html", NULL },
  { { N_("/Duplicate Channel"), "<control>C",
      channels_duplicate_channel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_channel.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Channel to Selection"), "<control>S",
      channels_channel_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "channel_to_selection.html", NULL },
  { { N_("/Add to Selection"), NULL,
      channels_add_channel_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    "channel_to_selection.html#add", NULL },
  { { N_("/Subtract from Selection"), NULL,
      channels_sub_channel_from_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    "channel_to_selection.html#subtract", NULL },
  { { N_("/Intersect with Selection"), NULL,
      channels_intersect_channel_with_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    "channel_to_selection.html#intersect", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Delete Channel"), "<control>X",
      channels_delete_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_channel.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Edit Channel Attributes..."), NULL,
      channels_edit_channel_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_channel_attributes.html", NULL }
};

gint n_channels_menu_entries = G_N_ELEMENTS (channels_menu_entries);


void
channels_menu_update (GtkItemFactory *factory,
                      gpointer        data)
{
  GimpImage   *gimage    = NULL;
  GimpChannel *channel   = NULL;
  gboolean     fs        = FALSE;
  gboolean     component = FALSE;
  GList       *next      = NULL;
  GList       *prev      = NULL;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      gimage = GIMP_IMAGE_EDITOR (data)->gimage;

      if (GIMP_COMPONENT_EDITOR (data)->clicked_component != -1)
        component = TRUE;
    }
  else
    {
      GList *list;

      gimage = GIMP_ITEM_LIST_VIEW (data)->gimage;

      channel = gimp_image_get_active_channel (gimage);

      for (list = GIMP_LIST (gimage->channels)->list;
           list;
           list = g_list_next (list))
        {
          if (channel == (GimpChannel *) list->data)
            {
              prev = g_list_previous (list);
              next = g_list_next (list);
              break;
            }
        }
    }

  fs = (gimp_image_floating_sel (gimage) != NULL);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Channel...",             !fs);
  SET_SENSITIVE ("/Raise Channel",              !fs && channel && prev);
  SET_SENSITIVE ("/Lower Channel",              !fs && channel && next);
  SET_SENSITIVE ("/Duplicate Channel",          !fs && (channel || component));
  SET_SENSITIVE ("/Channel to Selection",       !fs && (channel || component));
  SET_SENSITIVE ("/Add to Selection",           !fs && (channel || component));
  SET_SENSITIVE ("/Subtract from Selection",    !fs && (channel || component));
  SET_SENSITIVE ("/Intersect with Selection",   !fs && (channel || component));
  SET_SENSITIVE ("/Delete Channel",             !fs && channel);
  SET_SENSITIVE ("/Edit Channel Attributes...", !fs && channel);

#undef SET_SENSITIVE
}
