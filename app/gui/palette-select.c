/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1998 Andy Thomas (alt@picnic.demon.co.uk)
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

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdnd.h"

#include "dialog_handler.h"
#include "dialogs-constructors.h"
#include "palette-editor.h"
#include "palette-select.h"

#include "appenv.h"
#include "context_manager.h"

#include "libgimp/gimpintl.h"


#define STD_PALETTE_COLUMNS  6
#define STD_PALETTE_ROWS     5 


/*  local function prototypes  */

static void   palette_select_drop_palette   (GtkWidget      *widget,
					     GimpViewable   *viewable,
					     gpointer        data);
static void   palette_select_close_callback (GtkWidget      *widget,
					     gpointer        data);


/*  list of active dialogs  */
static GSList *palette_active_dialogs = NULL;


/*  public functions  */

PaletteSelect *
palette_select_new (const gchar *title,
		    const gchar *initial_palette)
{
  PaletteSelect *psp;
  GtkWidget     *vbox;
  GimpPalette   *active = NULL;

  static gboolean first_call = TRUE;

  psp = g_new0 (PaletteSelect, 1);

  psp->callback_name = NULL;
  
  /*  The shell and main vbox  */
  psp->shell = gimp_dialog_new (title ? title : _("Palette Selection"),
				"palette_selection",
				gimp_standard_help_func,
				"dialogs/palette_selection.html",
				GTK_WIN_POS_MOUSE,
				FALSE, TRUE, FALSE,

				"_delete_event_", palette_select_close_callback,
				psp, NULL, NULL, FALSE, TRUE,

				NULL);

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox)), 0)));

  gtk_widget_hide (GTK_DIALOG (psp->shell)->action_area);

  if (title)
    {
      psp->context = gimp_context_new (title, NULL);
    }
  else
    {
      psp->context = gimp_context_get_user ();

      /*
      session_set_window_geometry (psp->shell, &palette_select_session_info,
				   TRUE);
      */

      dialog_register (psp->shell);
    }

  if (no_data && first_call)
    gimp_data_factory_data_init (global_palette_factory, FALSE);

  first_call = FALSE;

  if (title && initial_palette && strlen (initial_palette))
    {
      active = (GimpPalette *)
	gimp_container_get_child_by_name (global_palette_factory->container,
					  initial_palette);
    }
  else
    {
      active = gimp_context_get_palette (gimp_context_get_user ());
    }

  if (!active)
    active = gimp_context_get_palette (gimp_context_get_standard ());

  if (title)
    gimp_context_set_palette (psp->context, active);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), vbox);

  /*  The Palette List  */
  psp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
					  global_palette_factory,
					  dialogs_edit_palette_func,
					  psp->context,
					  32,
					  5, 3);
  gtk_box_pack_start (GTK_BOX (vbox), psp->view, TRUE, TRUE, 0);
  gtk_widget_show (psp->view);

  gimp_gtk_drag_dest_set_by_type (psp->view,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_PALETTE,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (psp->view),
			      GIMP_TYPE_PALETTE,
			      palette_select_drop_palette,
                              psp);

  gtk_widget_show (vbox);
  gtk_widget_show (psp->shell);

  palette_active_dialogs = g_slist_append (palette_active_dialogs, psp);

  return psp;
}


/*  local functions  */

static void
palette_select_drop_palette (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  PaletteSelect *psp;

  psp = (PaletteSelect *) data;

  gimp_context_set_palette (psp->context, GIMP_PALETTE (viewable));
}

static void
palette_select_free (PaletteSelect *psp)
{
  if (psp)
    {
      /*
      if(psp->callback_name)
	g_free (gsp->callback_name);
      */

      /* remove from active list */
      palette_active_dialogs = g_slist_remove (palette_active_dialogs, psp);

      g_free (psp);
    }
}

static void
palette_select_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteSelect *psp;

  psp = (PaletteSelect *) data;

  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);

  gtk_widget_destroy (psp->shell); 
  palette_select_free (psp); 
}
