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

#include "appenv.h"
#include "context_manager.h"
#include "dialog_handler.h"
#include "gimpcontainer.h"
#include "gimpcontainergridview.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimppattern.h"
#include "patterns.h"
#include "pattern_select.h"
#include "session.h"
#include "temp_buf.h"

#include "pdb/procedural_db.h"

#include "libgimp/gimpintl.h"


#define MIN_CELL_SIZE       32
#define STD_PATTERN_COLUMNS  6
#define STD_PATTERN_ROWS     5 


/*  local function prototypes  */
static void     pattern_select_change_callbacks       (PatternSelect *psp,
						       gboolean       closing);

static void     pattern_select_drop_pattern           (GtkWidget     *widget,
						       GimpViewable  *viewable,
						       gpointer       data);
static void     pattern_select_pattern_changed        (GimpContext   *context,
						       GimpPattern   *pattern,
						       PatternSelect *psp);

static void     pattern_select_pattern_dirty_callback (GimpPattern   *brush,
						       PatternSelect *psp);

static void     pattern_select_update_active_pattern_field (PatternSelect *psp);

static void     pattern_select_close_callback         (GtkWidget     *widget,
						       gpointer       data);
static void     pattern_select_refresh_callback       (GtkWidget     *widget,
						       gpointer       data);


/*  The main pattern selection dialog  */
PatternSelect   *pattern_select_dialog = NULL;

/*  local variables  */

/*  List of active dialogs  */
GSList *pattern_active_dialogs = NULL;


void
pattern_dialog_create (void)
{
  if (! pattern_select_dialog)
    {
      pattern_select_dialog = pattern_select_new (NULL, NULL);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (pattern_select_dialog->shell))
	gtk_widget_show (pattern_select_dialog->shell);
      else
	gdk_window_raise (pattern_select_dialog->shell->window);
    }
}

void
pattern_dialog_free (void)
{
  if (pattern_select_dialog)
    {
      session_get_window_info (pattern_select_dialog->shell,
			       &pattern_select_session_info);

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
  GtkWidget     *label_box;

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

				 _("Refresh"), pattern_select_refresh_callback,
				 psp, NULL, NULL, FALSE, FALSE,
				 _("Close"), pattern_select_close_callback,
				 psp, NULL, NULL, TRUE, TRUE,

				 NULL);

  if (title)
    {
      psp->context = gimp_context_new (title, NULL);
    }
  else
    {
      psp->context = gimp_context_get_user ();

      session_set_window_geometry (psp->shell, &pattern_select_session_info,
				   TRUE);
      dialog_register (psp->shell);
    }

  if (no_data && first_call)
    patterns_init (FALSE);

  first_call = FALSE;

  if (title && initial_pattern && strlen (initial_pattern))
    {
      active = (GimpPattern *)
	gimp_container_get_child_by_name (global_pattern_list,
					  initial_pattern);
    }
  else
    {
      active = gimp_context_get_pattern (gimp_context_get_user ());
    }

  if (!active)
    {
      active = gimp_context_get_pattern (gimp_context_get_standard ());
    }

  if (title)
    {
      gimp_context_set_pattern (psp->context, active);
    }

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (psp->shell)->vbox), vbox);

  /*  Options box  */
  psp->options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), psp->options_box, FALSE, FALSE, 0);
  gtk_widget_show (psp->options_box);

  /*  Create the active pattern label  */
  label_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (psp->options_box), label_box, FALSE, FALSE, 2);
  gtk_widget_show (label_box);

  psp->pattern_name = gtk_label_new (_("No Patterns available"));
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_name, FALSE, FALSE, 4);
  psp->pattern_size = gtk_label_new ("(0 X 0)");
  gtk_box_pack_start (GTK_BOX (label_box), psp->pattern_size, FALSE, FALSE, 2);

  gtk_widget_show (psp->pattern_name);
  gtk_widget_show (psp->pattern_size);

  /*  The Brush Grid  */
  psp->view = gimp_container_grid_view_new (global_pattern_list,
					    psp->context,
					    MIN_CELL_SIZE,
					    STD_PATTERN_COLUMNS,
					    STD_PATTERN_ROWS);
  gtk_box_pack_start (GTK_BOX (vbox), psp->view, TRUE, TRUE, 0);
  gtk_widget_show (psp->view);

  gimp_gtk_drag_dest_set_by_type (psp->view,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_PATTERN,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (psp->view),
			      GIMP_TYPE_PATTERN,
			      pattern_select_drop_pattern,
                              psp);

  gtk_widget_show (vbox);

  /*  add callbacks to keep the display area current  */
  psp->name_changed_handler_id =
    gimp_container_add_handler
    (GIMP_CONTAINER (global_pattern_list), "name_changed",
     GTK_SIGNAL_FUNC (pattern_select_pattern_dirty_callback),
     psp);

  gtk_widget_show (psp->shell);

  gtk_signal_connect (GTK_OBJECT (psp->context), "pattern_changed",
		      GTK_SIGNAL_FUNC (pattern_select_pattern_changed),
		      psp);

  if (active)
    pattern_select_update_active_pattern_field (psp);

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

  gimp_container_remove_handler (GIMP_CONTAINER (global_pattern_list),
				 psp->name_changed_handler_id);

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
  prec = procedural_db_lookup (name);

  if (prec && pattern)
    {
      return_vals =
	procedural_db_run_proc (name,
				&nreturn_vals,
				PDB_STRING,    GIMP_OBJECT (pattern)->name,
				PDB_INT32,     pattern->mask->width,
				PDB_INT32,     pattern->mask->height,
				PDB_INT32,     pattern->mask->bytes,
				PDB_INT32,     (pattern->mask->bytes  *
						pattern->mask->height *
						pattern->mask->width),
				PDB_INT8ARRAY, temp_buf_data (pattern->mask),
				PDB_INT32,     (gint) closing,
				PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
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
	  prec = procedural_db_lookup (name);

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
pattern_select_drop_pattern (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  PatternSelect *psp;

  psp = (PatternSelect *) data;

  gimp_context_set_pattern (psp->context, GIMP_PATTERN (viewable));
}


static void
pattern_select_pattern_changed (GimpContext   *context,
				GimpPattern   *pattern,
				PatternSelect *psp)
{
  if (pattern)
    {
      pattern_select_update_active_pattern_field (psp);

      if (psp->callback_name)
	pattern_select_change_callbacks (psp, FALSE);
    }
}

static void
pattern_select_pattern_dirty_callback (GimpPattern   *pattern,
				       PatternSelect *psp)
{
  pattern_select_update_active_pattern_field (psp);
}

static void
pattern_select_update_active_pattern_field (PatternSelect *psp)
{
  GimpPattern *pattern;
  gchar        buf[32];

  pattern = gimp_context_get_pattern (psp->context);

  if (!pattern)
    return;

  /*  Set pattern name  */
  gtk_label_set_text (GTK_LABEL (psp->pattern_name),
		      GIMP_OBJECT (pattern)->name);

  /*  Set pattern size  */
  g_snprintf (buf, sizeof (buf), "(%d X %d)",
	      pattern->mask->width, pattern->mask->height);
  gtk_label_set_text (GTK_LABEL (psp->pattern_size), buf);
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

static void
pattern_select_refresh_callback (GtkWidget *widget,
				 gpointer   data)
{
  patterns_init (FALSE);
}
