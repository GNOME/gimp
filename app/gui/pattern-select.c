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

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppattern.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpcontainerbox.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimphelp-ids.h"

#include "menus/menus.h"

#include "pattern-select.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   pattern_select_change_callbacks (PatternSelect *psp,
                                               gboolean       closing);
static void   pattern_select_pattern_changed  (GimpContext   *context,
                                               GimpPattern   *pattern,
                                               PatternSelect *psp);
static void   pattern_select_response         (GtkWidget     *widget,
                                               gint           response_id,
                                               PatternSelect *psp);


/*  List of active dialogs  */
static GSList *pattern_active_dialogs = NULL;


/*  public functions  */

PatternSelect *
pattern_select_new (Gimp        *gimp,
                    GimpContext *context,
                    const gchar *title,
		    const gchar *initial_pattern,
                    const gchar *callback_name)
{
  PatternSelect *psp;
  GimpPattern   *active = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  if (gimp->no_data)
    {
      static gboolean first_call = TRUE;

      if (first_call)
        gimp_data_factory_data_init (gimp->pattern_factory, FALSE);

      first_call = FALSE;
    }

  if (initial_pattern && strlen (initial_pattern))
    {
      active = (GimpPattern *)
	gimp_container_get_child_by_name (gimp->pattern_factory->container,
					  initial_pattern);
    }

  if (! active)
    active = gimp_context_get_pattern (context);

  if (! active)
    return NULL;

  psp = g_new0 (PatternSelect, 1);

  /*  Add to active pattern dialogs list  */
  pattern_active_dialogs = g_slist_append (pattern_active_dialogs, psp);

  psp->context       = gimp_context_new (gimp, title, NULL);
  psp->callback_name = g_strdup (callback_name);

  gimp_context_set_pattern (psp->context, active);

  g_signal_connect (psp->context, "pattern_changed",
                    G_CALLBACK (pattern_select_pattern_changed),
                    psp);

  /*  the shell  */
  psp->shell = gimp_dialog_new (title, "pattern_selection",
                                NULL, 0,
                                gimp_standard_help_func,
                                GIMP_HELP_PATTERN_DIALOG,

                                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                NULL);

  g_signal_connect (psp->shell, "response",
                    G_CALLBACK (pattern_select_response),
                    psp);

  /*  the pattern grid  */
  psp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                          gimp->pattern_factory,
                                          NULL,
                                          psp->context,
                                          GIMP_PREVIEW_SIZE_MEDIUM, 1,
                                          global_menu_factory, "<Patterns>",
                                          "/patterns-popup",
                                          "patterns");

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (psp->view)->view),
                                       6 * (GIMP_PREVIEW_SIZE_MEDIUM + 2),
                                       6 * (GIMP_PREVIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (psp->view), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), psp->view);
  gtk_widget_show (psp->view);

  gtk_widget_show (psp->shell);

  return psp;
}

void
pattern_select_free (PatternSelect *psp)
{
  g_return_if_fail (psp != NULL);

  gtk_widget_destroy (psp->shell);

  /* remove from active list */
  pattern_active_dialogs = g_slist_remove (pattern_active_dialogs, psp);

  if (psp->callback_name)
    g_free (psp->callback_name);

  if (psp->context)
    g_object_unref (psp->context);

  g_free (psp);
}

PatternSelect *
pattern_select_get_by_callback (const gchar *callback_name)
{
  GSList        *list;
  PatternSelect *psp;

  for (list = pattern_active_dialogs; list; list = g_slist_next (list))
    {
      psp = (PatternSelect *) list->data;

      if (psp->callback_name && ! strcmp (callback_name, psp->callback_name))
	return psp;
    }

  return NULL;
}

void
pattern_select_dialogs_check (void)
{
  PatternSelect *psp;
  GSList        *list;

  list = pattern_active_dialogs;

  while (list)
    {
      psp = (PatternSelect *) list->data;

      list = g_slist_next (list);

      if (psp->callback_name)
        {
          if (! procedural_db_lookup (psp->context->gimp, psp->callback_name))
            pattern_select_response (NULL, GTK_RESPONSE_CLOSE, psp);
        }
    }
}


/*  private functions  */

static void
pattern_select_change_callbacks (PatternSelect *psp,
				 gboolean       closing)
{
  ProcRecord  *proc;
  GimpPattern *pattern;

  static gboolean busy = FALSE;

  if (! (psp && psp->callback_name) || busy)
    return;

  busy = TRUE;

  pattern = gimp_context_get_pattern (psp->context);

  /* If its still registered run it */
  proc = procedural_db_lookup (psp->context->gimp, psp->callback_name);

  if (proc && pattern)
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
	procedural_db_run_proc (psp->context->gimp,
                                psp->context,
				psp->callback_name,
				&nreturn_vals,
				GIMP_PDB_STRING,    GIMP_OBJECT (pattern)->name,
				GIMP_PDB_INT32,     pattern->mask->width,
				GIMP_PDB_INT32,     pattern->mask->height,
				GIMP_PDB_INT32,     pattern->mask->bytes,
				GIMP_PDB_INT32,     (pattern->mask->bytes  *
                                                     pattern->mask->height *
                                                     pattern->mask->width),
				GIMP_PDB_INT8ARRAY, temp_buf_data (pattern->mask),
				GIMP_PDB_INT32,     closing,
				GIMP_PDB_END);

      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Unable to run pattern callback. "
                     "The corresponding plug-in may have crashed."));

      if (return_vals)
        procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
}

static void
pattern_select_pattern_changed (GimpContext   *context,
				GimpPattern   *pattern,
				PatternSelect *psp)
{
  if (pattern)
    pattern_select_change_callbacks (psp, FALSE);
}

static void
pattern_select_response (GtkWidget     *widget,
                         gint           response_id,
                         PatternSelect *psp)
{
  pattern_select_change_callbacks (psp, TRUE);
  pattern_select_free (psp);
}
