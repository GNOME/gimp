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

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppattern.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpdatafactoryview.h"

#include "dialog_handler.h"
#include "pattern-select.h"

#include "appenv.h"
#include "app_procs.h"

#include "libgimp/gimpintl.h"


#define MIN_CELL_SIZE       32
#define STD_PATTERN_COLUMNS  6
#define STD_PATTERN_ROWS     5 


/*  local function prototypes  */
static void     pattern_select_change_callbacks       (PatternSelect *psp,
						       gboolean       closing);

static void     pattern_select_pattern_changed        (GimpContext   *context,
						       GimpPattern   *pattern,
						       PatternSelect *psp);

static void     pattern_select_close_callback         (GtkWidget     *widget,
						       gpointer       data);


/*  The main pattern selection dialog  */
PatternSelect   *pattern_select_dialog = NULL;

/*  local variables  */

/*  List of active dialogs  */
GSList *pattern_active_dialogs = NULL;


GtkWidget *
pattern_dialog_create (void)
{
  if (! pattern_select_dialog)
    {
      pattern_select_dialog = pattern_select_new (NULL, NULL);
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
pattern_select_new (gchar *title,
		    gchar *initial_pattern)
{
  PatternSelect *psp;
  GtkWidget     *vbox;

  GimpPattern *active = NULL;

  static gboolean first_call = TRUE;

  psp = g_new0 (PatternSelect, 1);

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

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox)), 0)));

  gtk_widget_hide (GTK_DIALOG (psp->shell)->action_area);

  if (title)
    {
      psp->context = gimp_create_context (the_gimp, title, NULL);
    }
  else
    {
      psp->context = gimp_get_user_context (the_gimp);

      dialog_register (psp->shell);
    }

  if (no_data && first_call)
    gimp_data_factory_data_init (the_gimp->pattern_factory, FALSE);

  first_call = FALSE;

  if (title && initial_pattern && strlen (initial_pattern))
    {
      active = (GimpPattern *)
	gimp_container_get_child_by_name (the_gimp->pattern_factory->container,
					  initial_pattern);
    }
  else
    {
      active = gimp_context_get_pattern (gimp_get_user_context (the_gimp));
    }

  if (!active)
    active = gimp_context_get_pattern (gimp_get_standard_context (the_gimp));

  if (title)
    gimp_context_set_pattern (psp->context, active);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), vbox);

  /*  The Brush Grid  */
  psp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_GRID,
					  the_gimp->pattern_factory,
					  NULL,
					  psp->context,
					  MIN_CELL_SIZE,
					  STD_PATTERN_COLUMNS,
					  STD_PATTERN_ROWS);
  gtk_box_pack_start (GTK_BOX (vbox), psp->view, TRUE, TRUE, 0);
  gtk_widget_show (psp->view);

  gtk_widget_show (vbox);

  gtk_widget_show (psp->shell);

  gtk_signal_connect (GTK_OBJECT (psp->context), "pattern_changed",
		      GTK_SIGNAL_FUNC (pattern_select_pattern_changed),
		      psp);

  /*  Add to active pattern dialogs list  */
  pattern_active_dialogs = g_slist_append (pattern_active_dialogs, psp);

  return psp;
}

void
pattern_select_free (PatternSelect *psp)
{
  if (!psp)
    return;

  /* remove from active list */
  pattern_active_dialogs = g_slist_remove (pattern_active_dialogs, psp);

  gtk_signal_disconnect_by_data (GTK_OBJECT (psp->context), psp);

  if (psp->callback_name)
    {
      g_free (psp->callback_name);
      gtk_object_unref (GTK_OBJECT (psp->context));
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
      gtk_widget_destroy (psp->shell); 
      pattern_select_free (psp); 
    }
}
