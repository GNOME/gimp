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

#include "widgets/gimpdatafactoryview.h"

#include "pattern-select.h"

#include "libgimp/gimpintl.h"


#define MIN_CELL_SIZE       32
#define STD_PATTERN_COLUMNS  6
#define STD_PATTERN_ROWS     5 


/*  local function prototypes  */
static void  pattern_select_change_callbacks (PatternSelect *psp,
                                              gboolean       closing);

static void  pattern_select_pattern_changed  (GimpContext   *context,
                                              GimpPattern   *pattern,
                                              PatternSelect *psp);

static void  pattern_select_close_callback   (GtkWidget     *widget,
                                              gpointer       data);


/*  The main pattern selection dialog  */
static PatternSelect *pattern_select_dialog = NULL;

/*  List of active dialogs  */
static GSList *pattern_active_dialogs = NULL;


GtkWidget *
pattern_dialog_create (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (! pattern_select_dialog)
    {
      pattern_select_dialog = pattern_select_new (gimp, NULL, NULL, NULL);
    }

  return pattern_select_dialog->shell;
}

void
pattern_dialog_free (void)
{
  if (pattern_select_dialog)
    {
      pattern_select_free (pattern_select_dialog);
      pattern_select_dialog = NULL;
    }
}


/*  If title == NULL then it is the main pattern dialog  */
PatternSelect *
pattern_select_new (Gimp        *gimp,
                    const gchar *title,
		    const gchar *initial_pattern,
                    const gchar *callback_name)
{
  PatternSelect *psp;
  GtkWidget     *vbox;

  GimpPattern *active = NULL;

  static gboolean first_call = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  psp = g_new0 (PatternSelect, 1);

  psp->callback_name = g_strdup (callback_name);

  /*  The shell  */
  psp->shell = 	gimp_dialog_new (title ? title : _("Pattern Selection"),
				 "pattern_selection",
				 gimp_standard_help_func,
				 "dialogs/pattern_selection.html",
				 title ? GTK_WIN_POS_MOUSE : GTK_WIN_POS_NONE,
				 FALSE, TRUE, FALSE,

				 "_delete_event_", pattern_select_close_callback,
				 psp, NULL, NULL, TRUE, TRUE,

				 NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (psp->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (psp->shell)->action_area);

  if (title)
    {
      psp->context = gimp_create_context (gimp, title, NULL);
    }
  else
    {
      psp->context = gimp_get_user_context (gimp);
    }

  if (gimp->no_data && first_call)
    gimp_data_factory_data_init (gimp->pattern_factory, FALSE);

  first_call = FALSE;

  if (title && initial_pattern && strlen (initial_pattern))
    {
      active = (GimpPattern *)
	gimp_container_get_child_by_name (gimp->pattern_factory->container,
					  initial_pattern);
    }
  else
    {
      active = gimp_context_get_pattern (gimp_get_user_context (gimp));
    }

  if (!active)
    active = gimp_context_get_pattern (gimp_get_standard_context (gimp));

  if (title)
    gimp_context_set_pattern (psp->context, active);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), vbox);

  /*  The Brush Grid  */
  psp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
					  gimp->pattern_factory,
					  NULL,
					  psp->context,
					  MIN_CELL_SIZE,
					  STD_PATTERN_COLUMNS,
					  STD_PATTERN_ROWS,
					  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), psp->view, TRUE, TRUE, 0);
  gtk_widget_show (psp->view);

  gtk_widget_show (vbox);

  gtk_widget_show (psp->shell);

  g_signal_connect (G_OBJECT (psp->context), "pattern_changed",
                    G_CALLBACK (pattern_select_pattern_changed),
                    psp);

  /*  Add to active pattern dialogs list  */
  pattern_active_dialogs = g_slist_append (pattern_active_dialogs, psp);

  return psp;
}

void
pattern_select_free (PatternSelect *psp)
{
  g_return_if_fail (psp != NULL);

  gtk_widget_destroy (psp->shell); 

  /* remove from active list */
  pattern_active_dialogs = g_slist_remove (pattern_active_dialogs, psp);

  g_signal_handlers_disconnect_by_func (G_OBJECT (psp->context), 
                                        pattern_select_pattern_changed,
                                        psp);

  if (psp->callback_name)
    {
      g_free (psp->callback_name);
      g_object_unref (G_OBJECT (psp->context));
    }

  g_free (psp);
}

/*  Call this dialog's PDB callback  */

static void
pattern_select_change_callbacks (PatternSelect *psp,
				 gboolean       closing)
{
  gchar       *name;
  ProcRecord  *prec = NULL;
  GimpPattern *pattern;
  gint         nreturn_vals;

  static gboolean busy = FALSE;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!psp || !psp->callback_name || busy)
    return;

  busy = TRUE;

  name = psp->callback_name;
  pattern = gimp_context_get_pattern (psp->context);

  /* If its still registered run it */
  prec = procedural_db_lookup (psp->context->gimp, name);

  if (prec && pattern)
    {
      return_vals =
	procedural_db_run_proc (psp->context->gimp,
				name,
				&nreturn_vals,
				GIMP_PDB_STRING,    GIMP_OBJECT (pattern)->name,
				GIMP_PDB_INT32,     pattern->mask->width,
				GIMP_PDB_INT32,     pattern->mask->height,
				GIMP_PDB_INT32,     pattern->mask->bytes,
				GIMP_PDB_INT32,     (pattern->mask->bytes  *
						pattern->mask->height *
						pattern->mask->width),
				GIMP_PDB_INT8ARRAY, temp_buf_data (pattern->mask),
				GIMP_PDB_INT32,     (gint) closing,
				GIMP_PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_warning ("failed to run pattern callback function");

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
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

/*  Close active dialogs that no longer have PDB registered for them  */

void
pattern_select_dialogs_check (void)
{
  PatternSelect *psp;
  GSList        *list;
  gchar         *name;
  ProcRecord    *prec = NULL;

  list = pattern_active_dialogs;

  while (list)
    {
      psp = (PatternSelect *) list->data;
      list = g_slist_next (list);

      name = psp->callback_name;

      if (name)
	{
	  prec = procedural_db_lookup (psp->context->gimp, name);

	  if (!prec)
	    {
	      /*  Can alter pattern_active_dialogs list  */
	      pattern_select_close_callback (NULL, psp); 
	    }
	}
    }
}

/*
 *  Local functions
 */

static void
pattern_select_pattern_changed (GimpContext   *context,
				GimpPattern   *pattern,
				PatternSelect *psp)
{
  if (pattern)
    {
      if (psp->callback_name)
	pattern_select_change_callbacks (psp, FALSE);
    }
}

static void
pattern_select_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PatternSelect *psp;

  psp = (PatternSelect *) data;

  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if (psp != pattern_select_dialog)
    {
      /* Send data back */
      pattern_select_change_callbacks (psp, TRUE);
      pattern_select_free (psp); 
    }
}
