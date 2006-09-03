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

#include "actions-types.h"

#include "core/gimppalette.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpview.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "palettes-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   palettes_merge_callback (GtkWidget   *widget,
                                       const gchar *palette_name,
                                       gpointer     data);


/*  public functionss */

void
palettes_import_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  gimp_dialog_factory_dialog_new (global_dialog_factory,
                                  gtk_widget_get_screen (widget),
                                  "gimp-palette-import-dialog", -1, TRUE);
}

void
palettes_merge_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GtkWidget           *dialog;

  dialog = gimp_query_string_box (_("Merge Palette"),
                                  GTK_WIDGET (editor),
                                  gimp_standard_help_func,
                                  GIMP_HELP_PALETTE_MERGE,
                                  _("Enter a name for the merged palette"),
                                  NULL,
                                  G_OBJECT (editor), "destroy",
                                  palettes_merge_callback,
                                  editor);
  gtk_widget_show (dialog);
}


/*  private functions  */

static void
palettes_merge_callback (GtkWidget   *widget,
                         const gchar *palette_name,
                         gpointer     data)
{
#ifdef __GNUC__
#warning FIXME: reimplement palettes_merge_callback()
#endif
#if 0
  GimpContainerEditor *editor;
  GimpPalette         *palette;
  GimpPalette         *new_palette;
  GimpPaletteEntry    *entry;
  GList               *sel_list;

  editor = (GimpContainerEditor *) data;

  sel_list = GTK_LIST (GIMP_CONTAINER_LIST_VIEW (editor->view)->gtk_list)->selection;

  if (! sel_list)
    {
      g_message ("Can't merge palettes because there are no palettes selected.");
      return;
    }

  new_palette = GIMP_PALETTE (gimp_palette_new (palette_name, FALSE));

  while (sel_list)
    {
      GimpListItem *list_item;
      GList        *cols;

      list_item = GIMP_LIST_ITEM (sel_list->data);

      palette = (GimpPalette *) GIMP_VIEW (list_item->preview)->viewable;

      if (palette)
        {
          for (cols = palette->colors; cols; cols = g_list_next (cols))
            {
              entry = (GimpPaletteEntry *) cols->data;

              gimp_palette_add_entry (new_palette,
                                      entry->name,
                                      &entry->color);
            }
        }

      sel_list = sel_list->next;
    }

  gimp_container_add (editor->view->container,
                      GIMP_OBJECT (new_palette));
#endif
}
