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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpcontainerbox.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimphelp-ids.h"

#include "menus/menus.h"

#include "dialogs-constructors.h"
#include "palette-select.h"

#include "gimp-intl.h"


#define STD_PALETTE_COLUMNS  6
#define STD_PALETTE_ROWS     5


/*  local function prototypes  */

static void   palette_select_change_callbacks (PaletteSelect *psp,
                                               gboolean       closing);
static void   palette_select_palette_changed  (GimpContext   *context,
                                               GimpPalette   *palette,
                                               PaletteSelect *psp);
static void   palette_select_response         (GtkWidget     *widget,
                                               gint           response_id,
                                               PaletteSelect *psp);


/*  list of active dialogs  */
static GSList *palette_active_dialogs = NULL;


/*  public functions  */

PaletteSelect *
palette_select_new (Gimp        *gimp,
                    GimpContext *context,
                    const gchar *title,
		    const gchar *initial_palette,
                    const gchar *callback_name)
{
  PaletteSelect *psp;
  GimpPalette   *active = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  if (gimp->no_data)
    {
      static gboolean first_call = TRUE;

      if (first_call)
        gimp_data_factory_data_init (gimp->palette_factory, FALSE);

      first_call = FALSE;
    }

  if (initial_palette && strlen (initial_palette))
    {
      active = (GimpPalette *)
	gimp_container_get_child_by_name (gimp->palette_factory->container,
					  initial_palette);
    }

  if (! active)
    active = gimp_context_get_palette (context);

  if (! active)
    return NULL;

  psp = g_new0 (PaletteSelect, 1);

  /*  Add to active palette dialogs list  */
  palette_active_dialogs = g_slist_append (palette_active_dialogs, psp);

  psp->context       = gimp_context_new (gimp, title, NULL);
  psp->callback_name = g_strdup (callback_name);

  gimp_context_set_palette (psp->context, active);

  g_signal_connect (psp->context, "palette_changed",
                    G_CALLBACK (palette_select_palette_changed),
                    psp);

  /*  the shell  */
  psp->shell = gimp_dialog_new (title, "palette_selection",
                                NULL, 0,
				gimp_standard_help_func,
				GIMP_HELP_PALETTE_DIALOG,

				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                NULL);

  g_signal_connect (psp->shell, "response",
                    G_CALLBACK (palette_select_response),
                    psp);

  /*  The Palette List  */
  psp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                          gimp->palette_factory,
                                          dialogs_edit_palette_func,
                                          psp->context,
                                          GIMP_PREVIEW_SIZE_MEDIUM, 1,
                                          global_menu_factory, "<Palettes>",
                                          "/palettes-popup",
                                          "palettes");

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (psp->view)->view),
                                       5 * (GIMP_PREVIEW_SIZE_MEDIUM + 2),
                                       8 * (GIMP_PREVIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (psp->view), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), psp->view);
  gtk_widget_show (psp->view);

  gtk_widget_show (psp->shell);

  return psp;
}

void
palette_select_free (PaletteSelect *psp)
{
  g_return_if_fail (psp != NULL);

  gtk_widget_destroy (psp->shell);

  /* remove from active list */
  palette_active_dialogs = g_slist_remove (palette_active_dialogs, psp);

  if (psp->callback_name)
    g_free (psp->callback_name);

  if (psp->context)
    g_object_unref (psp->context);

  g_free (psp);
}

PaletteSelect *
palette_select_get_by_callback (const gchar *callback_name)
{
  GSList *list;

  for (list = palette_active_dialogs; list; list = g_slist_next (list))
    {
      PaletteSelect *psp = list->data;

      if (psp->callback_name && ! strcmp (callback_name, psp->callback_name))
	return psp;
    }

  return NULL;
}

void
palette_select_dialogs_check (void)
{
  GSList *list;

  list = palette_active_dialogs;

  while (list)
    {
     PaletteSelect *psp = list->data;

      list = g_slist_next (list);

      if (psp->callback_name)
        {
          if (! procedural_db_lookup (psp->context->gimp, psp->callback_name))
            palette_select_response (NULL, GTK_RESPONSE_CLOSE, psp);
        }
    }
}


/*  private functions  */

static void
palette_select_change_callbacks (PaletteSelect *psp,
				 gboolean       closing)
{
  ProcRecord  *proc;
  GimpPalette *palette;

  static gboolean busy = FALSE;

  if (! psp->callback_name || busy)
    return;

  busy = TRUE;

  palette = gimp_context_get_palette (psp->context);

  /* If its still registered run it */
  proc = procedural_db_lookup (psp->context->gimp, psp->callback_name);

  if (proc && palette)
    {
      Argument *return_vals;
      gint      n_return_vals;

      return_vals =
	procedural_db_run_proc (psp->context->gimp,
                                psp->context,
				psp->callback_name,
				&n_return_vals,
				GIMP_PDB_STRING, GIMP_OBJECT (palette)->name,
				GIMP_PDB_INT32,  palette->n_colors,
				GIMP_PDB_INT32,  closing,
				GIMP_PDB_END);

      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Unable to run palette callback. "
                     "The corresponding plug-in may have crashed."));

      if (return_vals)
        procedural_db_destroy_args (return_vals, n_return_vals);
    }

  busy = FALSE;
}

static void
palette_select_palette_changed (GimpContext   *context,
				GimpPalette   *palette,
				PaletteSelect *psp)
{
  if (palette)
    palette_select_change_callbacks (psp, FALSE);
}

static void
palette_select_response (GtkWidget     *widget,
                         gint           response_id,
                         PaletteSelect *psp)
{
  palette_select_change_callbacks (psp, TRUE);
  palette_select_free (psp);
}
