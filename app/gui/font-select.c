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
#include "text/gimpfont.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimphelp-ids.h"

#include "menus/menus.h"

#include "dialogs-constructors.h"
#include "font-select.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   font_select_change_callbacks (FontSelect  *font_select,
                                            gboolean     closing);
static void   font_select_font_changed     (GimpContext *context,
                                            GimpFont    *font,
                                            FontSelect  *font_select);
static void   font_select_response         (GtkWidget   *widget,
                                            gint         response_id,
                                            FontSelect  *font_select);


/*  list of active dialogs  */
static GSList *font_active_dialogs = NULL;


/*  public functions  */

FontSelect *
font_select_new (Gimp        *gimp,
                 GimpContext *context,
                 const gchar *title,
                 const gchar *initial_font,
                 const gchar *callback_name)
{
  FontSelect *font_select;
  GimpFont   *active = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  if (initial_font && strlen (initial_font))
    {
      active = (GimpFont *)
	gimp_container_get_child_by_name (gimp->fonts, initial_font);
    }

  if (! active)
    active = gimp_context_get_font (context);

  if (! active)
    return NULL;

  font_select = g_new0 (FontSelect, 1);

  /*  Add to active font dialogs list  */
  font_active_dialogs = g_slist_append (font_active_dialogs, font_select);

  font_select->context       = gimp_context_new (gimp, title, NULL);
  font_select->callback_name = g_strdup (callback_name);

  gimp_context_set_font (font_select->context, active);

  g_signal_connect (font_select->context, "font_changed",
                    G_CALLBACK (font_select_font_changed),
                    font_select);

  /*  the shell  */
  font_select->shell = gimp_dialog_new (title, "font_selection",
                                        NULL, 0,
                                        gimp_standard_help_func,
                                        GIMP_HELP_FONT_DIALOG,

                                        GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                        NULL);

  g_signal_connect (font_select->shell, "response",
                    G_CALLBACK (font_select_response),
                    font_select);

  /*  The Font List  */
  font_select->view = gimp_container_tree_view_new (gimp->fonts,
                                                    font_select->context,
                                                    GIMP_PREVIEW_SIZE_MEDIUM, 1,
                                                    FALSE);

  gimp_container_view_set_size_request (GIMP_CONTAINER_VIEW (font_select->view),
                                        5 * (GIMP_PREVIEW_SIZE_MEDIUM + 2),
                                        5 * (GIMP_PREVIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (font_select->view), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (font_select->shell)->vbox),
                     font_select->view);
  gtk_widget_show (font_select->view);

  gtk_widget_show (font_select->shell);

  return font_select;
}

void
font_select_free (FontSelect *font_select)
{
  g_return_if_fail (font_select != NULL);

  gtk_widget_destroy (font_select->shell);

  /* remove from active list */
  font_active_dialogs = g_slist_remove (font_active_dialogs, font_select);

  if (font_select->callback_name)
    g_free (font_select->callback_name);

  if (font_select->context)
    g_object_unref (font_select->context);

  g_free (font_select);
}

FontSelect *
font_select_get_by_callback (const gchar *callback_name)
{
  GSList        *list;
  FontSelect *font_select;

  for (list = font_active_dialogs; list; list = g_slist_next (list))
    {
      font_select = (FontSelect *) list->data;

      if (font_select->callback_name && !
          strcmp (callback_name, font_select->callback_name))
	return font_select;
    }

  return NULL;
}

void
font_select_dialogs_check (void)
{
  FontSelect *font_select;
  GSList        *list;

  list = font_active_dialogs;

  while (list)
    {
      font_select = (FontSelect *) list->data;

      list = g_slist_next (list);

      if (font_select->callback_name)
        {
          if (!  procedural_db_lookup (font_select->context->gimp,
                                       font_select->callback_name))
            font_select_response (NULL, GTK_RESPONSE_CLOSE, font_select);
        }
    }
}


/*  private functions  */

static void
font_select_change_callbacks (FontSelect *font_select,
                              gboolean    closing)
{
  ProcRecord *proc;
  GimpFont   *font;

  static gboolean busy = FALSE;

  if (! (font_select && font_select->callback_name) || busy)
    return;

  busy = TRUE;

  font = gimp_context_get_font (font_select->context);

  /* If its still registered run it */
  proc = procedural_db_lookup (font_select->context->gimp,
                               font_select->callback_name);

  if (proc && font)
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
	procedural_db_run_proc (font_select->context->gimp,
                                font_select->context,
				font_select->callback_name,
				&nreturn_vals,
				GIMP_PDB_STRING, GIMP_OBJECT (font)->name,
				GIMP_PDB_INT32,  closing,
				GIMP_PDB_END);

      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Unable to run font callback. "
                     "The corresponding plug-in may have crashed."));

      if (return_vals)
        procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
}

static void
font_select_font_changed (GimpContext *context,
                          GimpFont    *font,
                          FontSelect  *font_select)
{
  if (font)
    font_select_change_callbacks (font_select, FALSE);
}

static void
font_select_response (GtkWidget  *widget,
                      gint        response_id,
                      FontSelect *font_select)
{
  font_select_change_callbacks (font_select, TRUE);
  font_select_free (font_select);
}
