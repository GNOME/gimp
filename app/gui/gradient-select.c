/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1998 Andy Thomas.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURIGHTE.  See the
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
#include "core/gimpgradient.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimphelp-ids.h"

#include "dialogs-constructors.h"
#include "gradient-select.h"
#include "menus.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradient_select_change_callbacks (GradientSelect *gsp,
                                                gboolean        closing);
static void   gradient_select_gradient_changed (GimpContext    *context,
                                                GimpGradient   *gradient,
                                                GradientSelect *gsp);
static void   gradient_select_response         (GtkWidget      *widget,
                                                gint            response_id,
                                                GradientSelect *gsp);


/*  list of active dialogs   */
static GSList *gradient_active_dialogs = NULL;


/*  public functions  */

GradientSelect *
gradient_select_new (Gimp        *gimp,
                     GimpContext *context,
                     const gchar *title,
		     const gchar *initial_gradient,
                     const gchar *callback_name,
                     gint         sample_size)
{
  GradientSelect *gsp;
  GimpGradient   *active = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  if (gimp->no_data)
    {
      static gboolean first_call = TRUE;

      if (first_call)
        gimp_data_factory_data_init (gimp->gradient_factory, FALSE);

      first_call = FALSE;
    }

  if (initial_gradient && strlen (initial_gradient))
    {
      active = (GimpGradient *)
	gimp_container_get_child_by_name (gimp->gradient_factory->container,
					  initial_gradient);
    }

  if (! active)
    active = gimp_context_get_gradient (context);

  if (! active)
    return NULL;

  gsp = g_new0 (GradientSelect, 1);

  /*  Add to active gradient dialogs list  */
  gradient_active_dialogs = g_slist_append (gradient_active_dialogs, gsp);

  gsp->context       = gimp_context_new (gimp, title, NULL);
  gsp->callback_name = g_strdup (callback_name);
  gsp->sample_size   = sample_size;

  gimp_context_set_gradient (gsp->context, active);

  g_signal_connect (gsp->context, "gradient_changed",
                    G_CALLBACK (gradient_select_gradient_changed),
                    gsp);

  /*  the shell  */
  gsp->shell = gimp_dialog_new (title, "gradient_selection",
                                NULL, 0,
                                gimp_standard_help_func,
                                GIMP_HELP_GRADIENT_DIALOG,

                                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                NULL);

  g_signal_connect (gsp->shell, "response",
                    G_CALLBACK (gradient_select_response),
                    gsp);

  /*  the gradient list  */
  gsp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
                                          gimp->gradient_factory,
                                          dialogs_edit_gradient_func,
                                          gsp->context,
                                          GIMP_PREVIEW_SIZE_MEDIUM, 1,
                                          global_menu_factory, "<Gradients>");

  gimp_container_view_set_size_request (GIMP_CONTAINER_VIEW (GIMP_CONTAINER_EDITOR (gsp->view)->view),
                                        6 * (GIMP_PREVIEW_SIZE_MEDIUM + 2),
                                        6 * (GIMP_PREVIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (gsp->view), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gsp->shell)->vbox), gsp->view);
  gtk_widget_show (gsp->view);

  gtk_widget_show (gsp->shell);

  return gsp;
}

void
gradient_select_free (GradientSelect *gsp)
{
  g_return_if_fail (gsp != NULL);

  gtk_widget_destroy (gsp->shell);

  /* remove from active list */
  gradient_active_dialogs = g_slist_remove (gradient_active_dialogs, gsp);

  if (gsp->callback_name)
    g_free (gsp->callback_name);

  if (gsp->context)
    g_object_unref (gsp->context);

   g_free (gsp);
}

GradientSelect *
gradient_select_get_by_callback (const gchar *callback_name)
{
  GSList         *list;
  GradientSelect *gsp;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      if (gsp->callback_name && ! strcmp (callback_name, gsp->callback_name))
	return gsp;
    }

  return NULL;
}

void
gradient_select_dialogs_check (void)
{
  GradientSelect *gsp;
  GSList         *list;

  list = gradient_active_dialogs;

  while (list)
    {
      gsp = (GradientSelect *) list->data;

      list = g_slist_next (list);

      if (gsp->callback_name)
        {
          if (! procedural_db_lookup (gsp->context->gimp, gsp->callback_name))
            gradient_select_response (NULL, GTK_RESPONSE_CLOSE, gsp);
        }
    }
}


/*  private functions  */

static void
gradient_select_change_callbacks (GradientSelect *gsp,
				  gboolean        closing)
{
  ProcRecord   *proc = NULL;
  GimpGradient *gradient;

  static gboolean  busy = FALSE;

  if (! (gsp && gsp->callback_name) || busy)
    return;

  busy = TRUE;

  gradient = gimp_context_get_gradient (gsp->context);

  /*  If its still registered run it  */
  proc = procedural_db_lookup (gsp->context->gimp, gsp->callback_name);

  if (proc && gradient)
    {
      Argument *return_vals;
      gint      nreturn_vals;
      gdouble  *values, *pv;
      double    pos, delta;
      GimpRGB   color;
      gint      i;

      i      = gsp->sample_size;
      pos    = 0.0;
      delta  = 1.0 / (i - 1);

      values = g_new (gdouble, 4 * i);
      pv     = values;

      while (i--)
	{
	  gimp_gradient_get_color_at (gradient, pos, FALSE, &color);

	  *pv++ = color.r;
	  *pv++ = color.g;
	  *pv++ = color.b;
	  *pv++ = color.a;

	  pos += delta;
	}

      return_vals =
	procedural_db_run_proc (gsp->context->gimp,
                                gsp->context,
				gsp->callback_name,
				&nreturn_vals,
				GIMP_PDB_STRING,     GIMP_OBJECT (gradient)->name,
				GIMP_PDB_INT32,      gsp->sample_size * 4,
				GIMP_PDB_FLOATARRAY, values,
				GIMP_PDB_INT32,      (gint) closing,
				GIMP_PDB_END);

      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Unable to run gradient callback. "
                     "The corresponding plug-in may have crashed."));

      if (return_vals)
	procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
}

static void
gradient_select_gradient_changed (GimpContext    *context,
				  GimpGradient   *gradient,
				  GradientSelect *gsp)
{
  if (gradient)
    gradient_select_change_callbacks (gsp, FALSE);
}

static void
gradient_select_response (GtkWidget      *widget,
                          gint            response_id,
                          GradientSelect *gsp)
{
  gradient_select_change_callbacks (gsp, TRUE);
  gradient_select_free (gsp);
}
