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
#include "appenv.h"
#include "gdisplay.h"
#include "libgimp/gimpintl.h"
#include "gimpprogress.h"
#include "gimpui.h"

struct gimp_progress_pvt {
  GDisplay      *gdisp;             /* gdisp in use, or NULL*/

  /* next four fields are only valid if gdisp is NULL */
  GtkWidget     *dialog;	    /* progress dialog, NULL if using gdisp */
  GtkWidget     *dialog_label;
  GtkWidget     *progressbar;
  GtkWidget     *cancelbutton;

  GtkSignalFunc  cancel_callback;   /* callback to remove, or NULL */
  gpointer       cancel_data;
};

#define DEFAULT_PROGRESS_MESSAGE _("Please wait...")


/* prototypes */
static void progress_signal_setup (gimp_progress *, GtkSignalFunc, gpointer);


/* These progress bar routines are re-entrant, and so should be
 * thread-safe. */


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
gimp_progress *
progress_start (GDisplay      *gdisp,
		const char    *message,
		gboolean       important,
		GtkSignalFunc  cancel_callback,
		gpointer       cancel_data)
{
  gimp_progress *p;
  guint cid;
  GtkWidget *vbox;

  p = g_new (gimp_progress, 1);

  p->gdisp = gdisp;
  p->dialog = NULL;
  p->cancel_callback = NULL;
  p->cancel_data = NULL;

  /* do we have a useful gdisplay and statusarea? */
  if (gdisp && GTK_WIDGET_VISIBLE (gdisp->statusarea))
    {
      if (message)
	{
	  cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar),
					      "progress");

	  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), cid, message);
	}

      /* really need image locking to stop multiple people going at
       * the image */
      if (gdisp->progressid)
	g_warning("%d progress bars already active for display %p\n",
		  gdisp->progressid, gdisp);
      gdisp->progressid++;
    }
  else
    {
      /* unimporant progress indications are occasionally failed */
      if (!important)
	{
	  g_free (p);
	  return NULL;
	}

      p->gdisp = NULL;
      p->dialog = gimp_dialog_new (_("Progress"), "plug_in_progress",
				   NULL, NULL,
				   GTK_WIN_POS_NONE,
				   FALSE, TRUE, FALSE,

				   _("Cancel"), NULL,
				   NULL, NULL, &p->cancelbutton, TRUE, TRUE,

				   NULL);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (p->dialog)->vbox), vbox);
      gtk_widget_show (vbox);

      p->dialog_label = gtk_label_new (message ? message :
				       DEFAULT_PROGRESS_MESSAGE);
      gtk_misc_set_alignment (GTK_MISC (p->dialog_label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), p->dialog_label, FALSE, TRUE, 0);
      gtk_widget_show (p->dialog_label);

      p->progressbar = gtk_progress_bar_new ();
      gtk_widget_set_usize (p->progressbar, 150, 20);
      gtk_box_pack_start (GTK_BOX (vbox), p->progressbar, TRUE, TRUE, 0);
      gtk_widget_show (p->progressbar);

      gtk_widget_show (p->dialog);
    }

  progress_signal_setup (p, cancel_callback, cancel_data);

  return p;
}


/* Update the message and/or the callbacks for a progress and reset
 * the bar to zero, with the minimum of disturbance to the user. */
gimp_progress *
progress_restart (gimp_progress *p,
		  const char    *message,
		  GtkSignalFunc  cancel_callback,
		  gpointer       cancel_data)
{
  int cid;
  GtkWidget *bar;

  g_return_val_if_fail (p != NULL, p);

  /* change the message */
  if (p->gdisp)
    {
      cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (p->gdisp->statusbar),
					  "progress");
      gtk_statusbar_pop (GTK_STATUSBAR (p->gdisp->statusbar), cid);

      if (message)
	gtk_statusbar_push (GTK_STATUSBAR (p->gdisp->statusbar), cid, message);

      bar = p->gdisp->progressbar;
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (p->dialog_label),
			  message ? message : DEFAULT_PROGRESS_MESSAGE);
      bar = p->progressbar;
    }

  /* reset the progress bar */
  gtk_progress_bar_update (GTK_PROGRESS_BAR (bar), 0.0);

  /* do we need to change the callbacks? */
  progress_signal_setup (p, cancel_callback, cancel_data);

  return p;
}


void
progress_update (gimp_progress *progress,
		 float          percentage)
{
  GtkWidget *bar;

  g_return_if_fail (progress != NULL);

  if (!(percentage >= 0.0 && percentage <= 1.0))
    return;

  /* do we have a dialog box, or are we using the statusbar? */
  if (progress->gdisp)
    bar = progress->gdisp->progressbar;
  else
    bar = progress->progressbar;

  gtk_progress_bar_update (GTK_PROGRESS_BAR (bar), percentage);
}


/* This function's prototype is conveniently the same as progress_func_t */
void
progress_update_and_flush (int       ymin,
			   int       ymax,
			   int       curr_y,
			   gpointer  data)
{
  progress_update ((gimp_progress *)data,
		   (float)(curr_y - ymin) / (float)(ymax - ymin));

  /* HACK until we do long-running operations in the gtk idle thread */
  while (gtk_events_pending())
    gtk_main_iteration();
}


/* Step the progress bar by one percent, wrapping at 100% */
void
progress_step (gimp_progress *progress)
{
  GtkWidget *bar;
  float val;

  g_return_if_fail (progress != NULL);

  if (progress->gdisp)
    bar = progress->gdisp->progressbar;
  else
    bar = progress->progressbar;

  val = gtk_progress_get_current_percentage (GTK_PROGRESS (bar)) + 0.01;
  if (val > 1.0)
    val = 0.0;

  progress_update (progress, val);
}


/* Finish using the progress bar "p" */
void
progress_end (gimp_progress *p)
{
  int cid;

  g_return_if_fail (p != NULL);

  /* remove all callbacks so they don't get called while we're
   * destroying widgets */
  progress_signal_setup (p, NULL, NULL);

  if (p->gdisp)
    {
      cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (p->gdisp->statusbar),
					  "progress");
      gtk_statusbar_pop (GTK_STATUSBAR (p->gdisp->statusbar), cid);

      gtk_progress_bar_update (GTK_PROGRESS_BAR (p->gdisp->progressbar), 0.0);

      if (p->gdisp->progressid > 0)
	p->gdisp->progressid--;
    }
  else
    {
      gtk_widget_destroy (p->dialog);
    }

  g_free (p);
}


/* Helper function to add or remove signals */
static void
progress_signal_setup (gimp_progress *p,
		       GtkSignalFunc  cancel_callback,
		       gpointer       cancel_data)
{
  GtkWidget *button;
  GtkWidget *dialog;

  if (p->cancel_callback == cancel_callback && p->cancel_data == cancel_data)
    return;

  /* are we using the statusbar or a freestanding dialog? */
  if (p->gdisp)
    {
      dialog = NULL;
      button = p->gdisp->cancelbutton;
    }
  else
    {
      dialog = p->dialog;
      button = p->cancelbutton;
    }

  /* remove any existing signal handlers */
  if (p->cancel_callback)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (button),
				     p->cancel_callback, p->cancel_data);
      if (dialog)
	gtk_signal_disconnect_by_func (GTK_OBJECT (dialog),
				       p->cancel_callback, p->cancel_data);
    }

  /* add the new handlers */
  if (cancel_callback)
    {
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  cancel_callback, cancel_data);

      if (dialog)
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    cancel_callback, cancel_data);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (button),
			    cancel_callback ? TRUE : FALSE);

  p->cancel_callback = cancel_callback;
  p->cancel_data     = cancel_data;
}

/* End of gimpprogress.c */
