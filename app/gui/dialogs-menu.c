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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpitemfactory.h"

#include "actions/dockable-commands.h"

#include "dialogs-menu.h"
#include "menus.h"

#include "gimp-intl.h"


#define ADD_TAB(path,id,stock_id,help_id) \
  { { (path), "", dockable_add_tab_cmd_callback, 0, \
      "<StockItem>", (stock_id) }, \
    (id), (help_id), NULL }
#define PREVIEW_SIZE(path,size) \
  { { (path), NULL, dockable_preview_size_cmd_callback, \
      (size), "/Preview Size/Tiny" }, \
    NULL, GIMP_HELP_DOCK_PREVIEW_SIZE, NULL }
#define TAB_STYLE(path,style) \
  { { (path), NULL, dockable_tab_style_cmd_callback, \
      (style), "/Tab Style/Icon" }, \
    NULL, GIMP_HELP_DOCK_TAB_STYLE, NULL }


GimpItemFactoryEntry dialogs_menu_entries[] =
{
  { { "/dialog-menu", NULL, NULL, 0,
      "<StockItem>", GTK_STOCK_MISSING_IMAGE },
    NULL, NULL, NULL },

  MENU_BRANCH (N_("/_Add Tab")),

  ADD_TAB (N_("/Add Tab/Tool _Options"),     "gimp-tool-options",
           GIMP_STOCK_TOOL_OPTIONS,          GIMP_HELP_TOOL_OPTIONS_DIALOG),
  ADD_TAB (N_("/Add Tab/_Device Status"),    "gimp-device-status",
           GIMP_STOCK_DEVICE_STATUS,         GIMP_HELP_DEVICE_STATUS_DIALOG),

  MENU_SEPARATOR ("/Add Tab/---"),

  ADD_TAB (N_("/Add Tab/_Layers"),           "gimp-layer-list",
           GIMP_STOCK_LAYERS,                GIMP_HELP_LAYER_DIALOG),
  ADD_TAB (N_("/Add Tab/_Channels"),         "gimp-channel-list",
           GIMP_STOCK_CHANNELS,              GIMP_HELP_CHANNEL_DIALOG),
  ADD_TAB (N_("/Add Tab/_Paths"),            "gimp-vectors-list",
           GIMP_STOCK_PATHS,                 GIMP_HELP_PATH_DIALOG),
  ADD_TAB (N_("/Add Tab/Inde_xed Palette"),  "gimp-indexed-palette",
           GIMP_STOCK_INDEXED_PALETTE,       GIMP_HELP_INDEXED_PALETTE_DIALOG),
  ADD_TAB (N_("/Add Tab/Histogra_m"),        "gimp-histogram-editor",
           GIMP_STOCK_HISTOGRAM,             GIMP_HELP_HISTOGRAM_DIALOG),
  ADD_TAB (N_("/Add Tab/_Selection Editor"), "gimp-selection-editor",
           GIMP_STOCK_TOOL_RECT_SELECT,      GIMP_HELP_SELECTION_DIALOG),
  ADD_TAB (N_("/Add Tab/Na_vigation"),       "gimp-navigation-view",
           GIMP_STOCK_NAVIGATION,            GIMP_HELP_NAVIGATION_DIALOG),
  ADD_TAB (N_("/Add Tab/_Undo History"),     "gimp-undo-history",
           GIMP_STOCK_UNDO_HISTORY,          GIMP_HELP_UNDO_DIALOG),

  MENU_SEPARATOR ("/Add Tab/---"),

  ADD_TAB (N_("/Add Tab/Colo_rs"),           "gimp-color-editor",
           GIMP_STOCK_DEFAULT_COLORS,        GIMP_HELP_COLOR_DIALOG),
  ADD_TAB (N_("/Add Tab/Brus_hes"),          "gimp-brush-grid",
           GIMP_STOCK_BRUSH,                 GIMP_HELP_BRUSH_DIALOG),
  ADD_TAB (N_("/Add Tab/P_atterns"),         "gimp-pattern-grid",
           GIMP_STOCK_PATTERN,               GIMP_HELP_PATTERN_DIALOG),
  ADD_TAB (N_("/Add Tab/_Gradients"),        "gimp-gradient-list",
           GIMP_STOCK_GRADIENT,              GIMP_HELP_GRADIENT_DIALOG),
  ADD_TAB (N_("/Add Tab/Pal_ettes"),         "gimp-palette-list",
           GIMP_STOCK_PALETTE,               GIMP_HELP_PALETTE_DIALOG),
  ADD_TAB (N_("/Add Tab/_Fonts"),            "gimp-font-list",
           GIMP_STOCK_FONT,                  GIMP_HELP_FONT_DIALOG),
  ADD_TAB (N_("/Add Tab/_Buffers"),          "gimp-buffer-list",
           GIMP_STOCK_BUFFER,                GIMP_HELP_BUFFER_DIALOG),

  MENU_SEPARATOR ("/Add Tab/---"),

  ADD_TAB (N_("/Add Tab/_Images"),           "gimp-image-list",
           GIMP_STOCK_IMAGES,                GIMP_HELP_IMAGE_DIALOG),
  ADD_TAB (N_("/Add Tab/Document Histor_y"), "gimp-document-list",
           GTK_STOCK_OPEN,                   GIMP_HELP_DOCUMENT_DIALOG),
  ADD_TAB (N_("/Add Tab/_Templates"),        "gimp-template-list",
           GIMP_STOCK_TEMPLATE,              GIMP_HELP_TEMPLATE_DIALOG),
  ADD_TAB (N_("/Add Tab/T_ools"),            "gimp-tool-list",
           GIMP_STOCK_TOOLS,                 GIMP_HELP_TOOLS_DIALOG),
  ADD_TAB (N_("/Add Tab/Error Co_nsole"),    "gimp-error-console",
           GIMP_STOCK_WARNING,               GIMP_HELP_ERRORS_DIALOG),

  { { N_("/_Close Tab"), "",
      dockable_close_tab_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLOSE },
    NULL,
    GIMP_HELP_DOCK_TAB_CLOSE, NULL },
  { { N_("/_Detach Tab"), "",
      dockable_detach_tab_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CONVERT },
    NULL,
    GIMP_HELP_DOCK_TAB_DETACH, NULL },

  MENU_SEPARATOR ("/properties-separator"),

  MENU_BRANCH (N_("/Preview Si_ze")),

  { { N_("/Preview Size/_Tiny"), NULL,
      dockable_preview_size_cmd_callback,
      GIMP_PREVIEW_SIZE_TINY, "<RadioItem>" },
    NULL,
    GIMP_HELP_DOCK_PREVIEW_SIZE, NULL },

  PREVIEW_SIZE (N_("/Preview Size/E_xtra Small"), GIMP_PREVIEW_SIZE_EXTRA_SMALL),
  PREVIEW_SIZE (N_("/Preview Size/_Small"),       GIMP_PREVIEW_SIZE_SMALL),
  PREVIEW_SIZE (N_("/Preview Size/_Medium"),      GIMP_PREVIEW_SIZE_MEDIUM),
  PREVIEW_SIZE (N_("/Preview Size/_Large"),       GIMP_PREVIEW_SIZE_LARGE),
  PREVIEW_SIZE (N_("/Preview Size/Ex_tra Large"), GIMP_PREVIEW_SIZE_EXTRA_LARGE),
  PREVIEW_SIZE (N_("/Preview Size/_Huge"),        GIMP_PREVIEW_SIZE_HUGE),
  PREVIEW_SIZE (N_("/Preview Size/_Enormous"),    GIMP_PREVIEW_SIZE_ENORMOUS),
  PREVIEW_SIZE (N_("/Preview Size/_Gigantic"),    GIMP_PREVIEW_SIZE_GIGANTIC),

  MENU_BRANCH (N_("/_Tab Style")),

  { { N_("/Tab Style/_Icon"), NULL,
      dockable_tab_style_cmd_callback,
      GIMP_TAB_STYLE_ICON, "<RadioItem>" },
    NULL,
    GIMP_HELP_DOCK_TAB_STYLE, NULL },

  TAB_STYLE (N_("/Tab Style/Current _Status"), GIMP_TAB_STYLE_PREVIEW),
  TAB_STYLE (N_("/Tab Style/_Text"),           GIMP_TAB_STYLE_NAME),
  TAB_STYLE (N_("/Tab Style/I_con & Text"),    GIMP_TAB_STYLE_ICON_NAME),
  TAB_STYLE (N_("/Tab Style/St_atus & Text"),  GIMP_TAB_STYLE_PREVIEW_NAME),

  { { N_("/View as _List"), NULL,
      dockable_toggle_view_cmd_callback, GIMP_VIEW_TYPE_LIST, "<RadioItem>" },
    NULL,
    GIMP_HELP_DOCK_VIEW_AS_LIST, NULL },
  { { N_("/View as _Grid"), NULL,
      dockable_toggle_view_cmd_callback, GIMP_VIEW_TYPE_GRID, "/View as List" },
    NULL,
    GIMP_HELP_DOCK_VIEW_AS_GRID, NULL },

  MENU_SEPARATOR ("/image-menu-separator"),

  { { N_("/Show Image _Menu"), NULL,
      dockable_toggle_image_menu_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_DOCK_IMAGE_MENU, NULL },
  { { N_("/Auto Follow Active _Image"), NULL,
      dockable_toggle_auto_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_DOCK_AUTO_BUTTON, NULL },
  { { N_("/Move to Screen..."), NULL,
      dockable_change_screen_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MOVE_TO_SCREEN },
    NULL,
    GIMP_HELP_DOCK_CHANGE_SCREEN, NULL }
};

#undef ADD_TAB
#undef PREVIEW_SIZE
#undef TAB_STYLE

gint n_dialogs_menu_entries = G_N_ELEMENTS (dialogs_menu_entries);


void
dialogs_menu_update (GtkItemFactory *factory,
                     gpointer        data)
{
  GimpDockable           *dockable;
  GimpDockbook           *dockbook;
  GimpDialogFactoryEntry *entry;
  GimpContainerView      *view;
  GimpViewType            view_type           = -1;
  gboolean                list_view_available = FALSE;
  gboolean                grid_view_available = FALSE;
  GimpPreviewSize         preview_size        = -1;
  GimpTabStyle            tab_style           = -1;
  gint                    n_pages             = 0;
  gint                    n_screens           = 1;

  if (GIMP_IS_DOCKBOOK (data))
    {
      gint page_num;

      dockbook = GIMP_DOCKBOOK (data);

      page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

      dockable = (GimpDockable *)
	gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);
    }
  else if (GIMP_IS_DOCKABLE (data))
    {
      dockable = GIMP_DOCKABLE (data);
      dockbook = dockable->dockbook;
    }
  else
    {
      return;
    }

  gimp_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

  if (entry)
    {
      gchar *identifier;
      gchar *substring = NULL;

      identifier = g_strdup (entry->identifier);

      if ((substring = strstr (identifier, "grid")))
        view_type = GIMP_VIEW_TYPE_GRID;
      else if ((substring = strstr (identifier, "list")))
        view_type = GIMP_VIEW_TYPE_LIST;

      if (substring)
        {
          memcpy (substring, "list", 4);
          if (gimp_dialog_factory_find_entry (dockbook->dock->dialog_factory,
                                              identifier))
            list_view_available = TRUE;

          memcpy (substring, "grid", 4);
          if (gimp_dialog_factory_find_entry (dockbook->dock->dialog_factory,
                                              identifier))
            grid_view_available = TRUE;
        }

      g_free (identifier);
    }

  view = gimp_container_view_get_by_dockable (dockable);

  if (view)
    preview_size = view->preview_size;

  tab_style = dockable->tab_style;

  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook));

#define SET_ACTIVE(path,active) \
        gimp_item_factory_set_active (factory, (path), (active) != 0)
#define SET_VISIBLE(path,active) \
        gimp_item_factory_set_visible (factory, (path), (active) != 0)
#define SET_SENSITIVE(path,sensitive) \
        gimp_item_factory_set_sensitive (factory, (path), (sensitive) != 0)

  SET_VISIBLE ("/properties-separator",
               preview_size != -1 ||
               n_pages       > 1  ||
               view_type    != -1);

  SET_VISIBLE ("/Preview Size", preview_size != -1);

  if (preview_size != -1)
    {
      if (preview_size >= GIMP_PREVIEW_SIZE_GIGANTIC)
        {
          SET_ACTIVE ("/Preview Size/Gigantic", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_ENORMOUS)
        {
          SET_ACTIVE ("/Preview Size/Enormous", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_HUGE)
        {
          SET_ACTIVE ("/Preview Size/Huge", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_EXTRA_LARGE)
        {
          SET_ACTIVE ("/Preview Size/Extra Large", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_LARGE)
        {
          SET_ACTIVE ("/Preview Size/Large", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_MEDIUM)
        {
          SET_ACTIVE ("/Preview Size/Medium", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_SMALL)
        {
          SET_ACTIVE ("/Preview Size/Small", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_EXTRA_SMALL)
        {
          SET_ACTIVE ("/Preview Size/Extra Small", TRUE);
        }
      else if (preview_size >= GIMP_PREVIEW_SIZE_TINY)
        {
          SET_ACTIVE ("/Preview Size/Tiny", TRUE);
        }
    }

  SET_VISIBLE ("/Tab Style", n_pages > 1);

  if (n_pages > 1)
    {
      GimpDockedInterface *docked_iface;

      docked_iface = GIMP_DOCKED_GET_INTERFACE (GTK_BIN (dockable)->child);

      if (tab_style == GIMP_TAB_STYLE_ICON)
        SET_ACTIVE ("/Tab Style/Icon", TRUE);
      else if (tab_style == GIMP_TAB_STYLE_PREVIEW)
        SET_ACTIVE ("/Tab Style/Current Status", TRUE);
      else if (tab_style == GIMP_TAB_STYLE_NAME)
        SET_ACTIVE ("/Tab Style/Text", TRUE);
      else if (tab_style == GIMP_TAB_STYLE_ICON_NAME)
        SET_ACTIVE ("/Tab Style/Icon & Text", TRUE);
      else if (tab_style == GIMP_TAB_STYLE_PREVIEW_NAME)
        SET_ACTIVE ("/Tab Style/Status & Text", TRUE);

      SET_SENSITIVE ("/Tab Style/Current Status", docked_iface->get_preview);
      SET_SENSITIVE ("/Tab Style/Status & Text",  docked_iface->get_preview);
    }

  SET_VISIBLE ("/View as Grid", view_type != -1);
  SET_VISIBLE ("/View as List", view_type != -1);

  if (view_type != -1)
    {
      if (view_type == GIMP_VIEW_TYPE_LIST)
        SET_ACTIVE ("/View as List", TRUE);
      else
        SET_ACTIVE ("/View as Grid", TRUE);

      SET_SENSITIVE ("/View as Grid", grid_view_available);
      SET_SENSITIVE ("/View as List", list_view_available);
    }

  n_screens = gdk_display_get_n_screens
    (gtk_widget_get_display (GTK_WIDGET (dockbook->dock)));

  if (GIMP_IS_IMAGE_DOCK (dockbook->dock))
    {
      GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dockbook->dock);

      SET_VISIBLE ("/image-menu-separator",     TRUE);
      SET_VISIBLE ("/Show Image Menu",          TRUE);
      SET_VISIBLE ("/Auto Follow Active Image", TRUE);

      SET_ACTIVE ("/Show Image Menu",          image_dock->show_image_menu);
      SET_ACTIVE ("/Auto Follow Active Image", image_dock->auto_follow_active);
    }
  else
    {
      SET_VISIBLE ("/image-menu-separator",     n_screens > 1);
      SET_VISIBLE ("/Show Image Menu",          FALSE);
      SET_VISIBLE ("/Auto Follow Active Image", FALSE);
    }

  SET_VISIBLE ("/Move to Screen...", n_screens > 1);

#undef SET_ACTIVE
#undef SET_VISIBLE
#undef SET_SENSITIVE
}
