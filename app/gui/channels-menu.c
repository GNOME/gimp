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
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemtreeview.h"

#include "actions/channels-commands.h"

#include "channels-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry channels_menu_entries[] =
{
  { { N_("/_Edit Channel Attributes..."), NULL,
      channels_edit_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_CHANNEL_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Channel..."), "",
      channels_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_CHANNEL_NEW, NULL },
  { { N_("/_Raise Channel"), "",
      channels_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    GIMP_HELP_CHANNEL_RAISE, NULL },
  { { N_("/_Lower Channel"), "",
      channels_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    GIMP_HELP_CHANNEL_LOWER, NULL },
  { { N_("/D_uplicate Channel"), NULL,
      channels_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_CHANNEL_DUPLICATE, NULL },
  { { N_("/_Delete Channel"), "",
      channels_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_CHANNEL_DELETE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Channel to Sele_ction"), NULL,
      channels_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_CHANNEL_SELECTION_REPLACE, NULL },
  { { N_("/_Add to Selection"), NULL,
      channels_to_selection_cmd_callback, GIMP_CHANNEL_OP_ADD,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    GIMP_HELP_CHANNEL_SELECTION_ADD, NULL },
  { { N_("/_Subtract from Selection"), NULL,
      channels_to_selection_cmd_callback, GIMP_CHANNEL_OP_SUBTRACT,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    GIMP_HELP_CHANNEL_SELECTION_SUBTRACT, NULL },
  { { N_("/_Intersect with Selection"), NULL,
      channels_to_selection_cmd_callback, GIMP_CHANNEL_OP_INTERSECT,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    GIMP_HELP_CHANNEL_SELECTION_INTERSECT, NULL }
};

gint n_channels_menu_entries = G_N_ELEMENTS (channels_menu_entries);


void
channels_menu_update (GtkItemFactory *factory,
                      gpointer        data)
{
  GimpImage   *gimage;
  GimpChannel *channel   = NULL;
  gboolean     fs        = FALSE;
  gboolean     component = FALSE;
  GList       *next      = NULL;
  GList       *prev      = NULL;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      gimage = GIMP_IMAGE_EDITOR (data)->gimage;

      if (gimage)
        {
          if (GIMP_COMPONENT_EDITOR (data)->clicked_component != -1)
            component = TRUE;
        }
    }
  else
    {
      gimage = GIMP_ITEM_TREE_VIEW (data)->gimage;

      if (gimage)
        {
          GList *list;

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
    }

  if (gimage)
    fs = (gimp_image_floating_sel (gimage) != NULL);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Channel Attributes...",  !fs && channel);

  SET_SENSITIVE ("/New Channel...",           !fs && gimage);
  SET_SENSITIVE ("/Raise Channel",            !fs && channel && prev);
  SET_SENSITIVE ("/Lower Channel",            !fs && channel && next);
  SET_SENSITIVE ("/Duplicate Channel",        !fs && (channel || component));
  SET_SENSITIVE ("/Delete Channel",           !fs && channel);

  SET_SENSITIVE ("/Channel to Selection",     !fs && (channel || component));
  SET_SENSITIVE ("/Add to Selection",         !fs && (channel || component));
  SET_SENSITIVE ("/Subtract from Selection",  !fs && (channel || component));
  SET_SENSITIVE ("/Intersect with Selection", !fs && (channel || component));

#undef SET_SENSITIVE
}
