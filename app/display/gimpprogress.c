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

#include "display/display-types.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpprogress.h"

#include "libgimp/gimpintl.h"


struct _GimpProgress
{
  GimpDisplay *gdisp;            /* gdisp in use, or NULL*/

  /* next four fields are only valid if gdisp is NULL */
  GtkWidget   *dialog;           /* progress dialog, NULL if using gdisp */
  GtkWidget   *dialog_label;
  GtkWidget   *progressbar;
  GtkWidget   *cancelbutton;

  GCallback    cancel_callback;  /* callback to remove, or NULL */
  gpointer     cancel_data;
};

/* prototypes */
static void   progress_signal_setup (GimpProgress *progress,
				     GCallback     cancel_callback,
				     gpointer      cancel_data);


/* These progress bar routines are re-entrant, and so should be
 * thread-safe.
 */


/* Start a progress bar on "gdisp" with reason "message".  If "gdisp"
 * is NULL, the progress bar is presented in a new dialog box.  If
 * "message" is NULL, then no message is used.
 *
 * If "cancel_callback" is not NULL, it is attached to the progress
 * bar cancel button's "clicked" signal, with data "cancel_data".  The
 * cancel button is only made sensitive if the callback is set.
 *
 * It is an error to progress_start() a bar on a "gdisp" for which
 * there is already a progress bar active.
 *
 * Progress bars with "important" set to TRUE will be shown to the
 * user in any possible way.  Unimportant progress bars will not be
 * shown to the user if it would mean creating a new window.
 */
GimpProgress *
progress_start (GimpDisplay *gdisp,
		const gchar *message,
		gboolean     important,
		GCallback    cancel_callback,
		gpointer     cancel_data)
{
  GimpDisplayShell *shell = NULL;
  GimpProgress     *progress;
  guint             cid;
  GtkWidget        *vbox;

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  progress = g_new0 (GimpProgress, 1);

  progress->gdisp           = gdisp;
  progress->dialog          = NULL;
  progress->cancel_callback = NULL;
  progress->cancel_data     = NULL;

  /* do we have a useful gdisplay and statusarea? */
  if (gdisp && GTK_WIDGET_VISIBLE (shell->statusarea))
    {
      if (message)
	{
	  cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar),
					      "progress");

	  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar), cid, message);
	}

      /*  really need image locking to stop multiple people going at
       *  the image
       */
      if (shell->progressid)
        {
          g_warning("%d progress bars already active for display %p\n",
                    shell->progressid, gdisp);
        }

      shell->progressid++;
    }
  else
    {
      /* unimporant progress indications are occasionally failed */
      if (! important)
	{
	  g_free (progress);
	  return NULL;
	}

      progress->gdisp  = NULL;
      progress->dialog = gimp_dialog_new (_("Progress"), "plug_in_progress",
                                          NULL, NULL,
                                          GTK_WIN_POS_NONE,
                                          FALSE, TRUE, FALSE,

                                          GTK_STOCK_CANCEL, NULL,
                                          NULL, NULL, &progress->cancelbutton,
                                          TRUE, TRUE,

                                          NULL);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (progress->dialog)->vbox),
                         vbox);
      gtk_widget_show (vbox);

      progress->dialog_label = gtk_label_new (message ? message :
                                              _("Please wait..."));
      gtk_misc_set_alignment (GTK_MISC (progress->dialog_label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), progress->dialog_label,
                          FALSE, TRUE, 0);
      gtk_widget_show (progress->dialog_label);

      progress->progressbar = gtk_progress_bar_new ();
      gtk_widget_set_size_request (progress->progressbar, 150, 20);
      gtk_box_pack_start (GTK_BOX (vbox), progress->progressbar, TRUE, TRUE, 0);
      gtk_widget_show (progress->progressbar);

      gtk_widget_show (progress->dialog);
    }

  progress_signal_setup (progress, cancel_callback, cancel_data);

  return progress;
}


/* Update the message and/or the callbacks for a progress and reset
 * the bar to zero, with the minimum of disturbance to the user.
 */
GimpProgress *
progress_restart (GimpProgress *progress,
		  const char   *message,
		  GCallback     cancel_callback,
		  gpointer      cancel_data)
{
  GtkWidget *bar;
  gint       cid;

  g_return_val_if_fail (progress != NULL, progress);

  /* change the message */
  if (progress->gdisp)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar),
					  "progress");
      gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar), cid);

      if (message)
	gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar), cid, message);

      bar = shell->progressbar;
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (progress->dialog_label),
			  message ? message : _("Please wait..."));

      bar = progress->progressbar;
    }

  /* reset the progress bar */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  /* do we need to change the callbacks? */
  progress_signal_setup (progress, cancel_callback, cancel_data);

  return progress;
}


void
progress_update (GimpProgress *progress,
		 gdouble       percentage)
{
  GtkWidget *bar;

  g_return_if_fail (progress != NULL);

  if (percentage < 0.0 || percentage > 1.0)
    return;

  /* do we have a dialog box, or are we using the statusbar? */
  if (progress->gdisp)
    {
      bar = GIMP_DISPLAY_SHELL (progress->gdisp->shell)->progressbar;
    }
  else
    {
      bar = progress->progressbar;
    }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);
}


/* This function's prototype is conveniently the same as progress_func_t */
void
progress_update_and_flush (gint      ymin,
			   gint      ymax,
			   gint      curr_y,
			   gpointer  data)
{
  progress_update ((GimpProgress *) data,
		   (float)(curr_y - ymin) / (float)(ymax - ymin));

  /* HACK until we do long-running operations in the gtk idle thread */
  while (gtk_events_pending())
    gtk_main_iteration();
}


/* Step the progress bar by one percent, wrapping at 100% */
void
progress_step (GimpProgress *progress)
{
  GtkWidget *bar;
  gdouble    val;

  g_return_if_fail (progress != NULL);

  if (progress->gdisp)
    {
      bar = GIMP_DISPLAY_SHELL (progress->gdisp->shell)->progressbar;
    }
  else
    {
      bar = progress->progressbar;
    }

  val = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (bar)) + 0.01;
  if (val > 1.0)
    val = 0.0;

  progress_update (progress, val);
}


/* Finish using the progress bar "p" */
void
progress_end (GimpProgress *progress)
{
  gint cid;

  g_return_if_fail (progress != NULL);

  /* remove all callbacks so they don't get called while we're
   * destroying widgets
   */
  progress_signal_setup (progress, NULL, NULL);

  if (progress->gdisp)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar),
					  "progress");
      gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar), cid);

      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (shell->progressbar), 0.0);

      if (shell->progressid > 0)
        shell->progressid--;
    }
  else
    {
      gtk_widget_destroy (progress->dialog);
    }

  g_free (progress);
}


/* Helper function to add or remove signals */
static void
progress_signal_setup (GimpProgress *progress,
                       GCallback     cancel_callback,
		       gpointer      cancel_data)
{
  GtkWidget *button;
  GtkWidget *dialog;

  if (progress->cancel_callback == cancel_callback &&
      progress->cancel_data     == cancel_data)
    return;

  /* are we using the statusbar or a freestanding dialog? */
  if (progress->gdisp)
    {
      dialog = NULL;
      button = GIMP_DISPLAY_SHELL (progress->gdisp->shell)->cancelbutton;
    }
  else
    {
      dialog = progress->dialog;
      button = progress->cancelbutton;
    }

  /* remove any existing signal handlers */
  if (progress->cancel_callback)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (button),
                                            progress->cancel_callback, 
                                            progress->cancel_data);
      if (dialog)
	g_signal_handlers_disconnect_by_func (G_OBJECT (dialog),
                                              progress->cancel_callback, 
                                              progress->cancel_data);
    }

  /* add the new handlers */
  if (cancel_callback)
    {
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (cancel_callback), 
                        cancel_data);

      if (dialog)
	g_signal_connect (G_OBJECT (dialog), "destroy",
                          G_CALLBACK (cancel_callback), 
                          cancel_data);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (button),
			    cancel_callback ? TRUE : FALSE);

  progress->cancel_callback = cancel_callback;
  progress->cancel_data     = cancel_data;
}
