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

#include "display-types.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpstatusbar.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


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


/*  local function prototypes  */

static void   gimp_progress_signal_setup (GimpProgress *progress,
                                          GCallback     cancel_callback,
                                          gpointer      cancel_data);
static void   gimp_progress_response     (GtkWidget    *dialog,
                                          gint          response_id,
                                          GimpProgress *progress);


/**
 * gimp_progress_start:
 * @gdisp:           The #GimpDisplay to show the progress in.
 * @message:         The message.
 * @important:       Setting this to #FALSE will cause the progress
 *                   to silently fail if the display's statusbar
 *                   is hidden.
 * @cancel_callback: The callback to call if the "Cancel" button is clicked.
 * @cancel_data:     The %cancel_callback's "user_data".
 *
 * Start a progress bar on %gdisp with reason %message.  If %gdisp
 * is #NULL, the progress bar is presented in a new dialog box.  If
 * %message is #NULL, then no message is used.
 *
 * If %cancel_callback is not %NULL, it is attached to the progress
 * bar cancel button's "clicked" signal, with data %cancel_data.  The
 * cancel button is only made sensitive if the callback is set.
 *
 * It is an error to gimp_progress_start() a bar on a %gdisp for which
 * there is already a progress bar active.
 *
 * Progress bars with %important set to #TRUE will be shown to the
 * user in any possible way.  Unimportant progress bars will not be
 * shown to the user if it would mean creating a new window.
 *
 * Return value: The new #GimpProgress.
 **/
GimpProgress *
gimp_progress_start (GimpDisplay *gdisp,
                     const gchar *message,
                     gboolean     important,
                     GCallback    cancel_callback,
                     gpointer     cancel_data)
{
  GimpDisplayShell *shell = NULL;
  GimpProgress     *progress;
  GtkWidget        *vbox;

  g_return_val_if_fail (gdisp == NULL || GIMP_IS_DISPLAY (gdisp), NULL);

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  progress = g_new0 (GimpProgress, 1);

  /*  do we have a useful gdisplay and statusarea?  */
  if (gdisp && GTK_WIDGET_VISIBLE (shell->statusbar))
    {
      progress->gdisp = gdisp;

      if (message)
	{
	  gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar),
                               "progress",
                               message);
	}

      /*  really need image locking to stop multiple people going at
       *  the image
       */
      if (GIMP_STATUSBAR (shell->statusbar)->progressid)
        {
          g_warning ("%s: %d progress bars already active for display %p",
                     G_STRFUNC, GIMP_STATUSBAR (shell->statusbar)->progressid,
                     gdisp);
        }

      GIMP_STATUSBAR (shell->statusbar)->progressid++;
    }
  else
    {
      /*  unimporant progress indications are occasionally failed  */
      if (! important)
	{
	  g_free (progress);
	  return NULL;
	}

      progress->dialog = gimp_dialog_new (_("Progress"), "progress",
                                          NULL, 0,
                                          NULL, NULL,
                                          NULL);

      progress->cancelbutton =
        gtk_dialog_add_button (GTK_DIALOG (progress->dialog),
                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

      gtk_window_set_resizable (GTK_WINDOW (progress->dialog), FALSE);

      vbox = gtk_vbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (progress->dialog)->vbox),
                         vbox);
      gtk_widget_show (vbox);

      progress->dialog_label = gtk_label_new (message ? message :
                                              _("Please wait..."));
      gtk_misc_set_alignment (GTK_MISC (progress->dialog_label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (vbox), progress->dialog_label,
                          FALSE, FALSE, 0);
      gtk_widget_show (progress->dialog_label);

      progress->progressbar = gtk_progress_bar_new ();
      gtk_widget_set_size_request (progress->progressbar, 150, 20);
      gtk_box_pack_start (GTK_BOX (vbox), progress->progressbar,
                          FALSE, FALSE, 0);
      gtk_widget_show (progress->progressbar);

      gtk_widget_show (progress->dialog);
    }

  gimp_progress_signal_setup (progress, cancel_callback, cancel_data);

  return progress;
}


/**
 * gimp_progress_restart:
 * @progress:        The #GimpProgress to restart.
 * @message:         The new message.
 * @cancel_callback: The new cancel_callback
 * @cancel_data:     The new cancel_data
 *
 * Update the message and/or the callbacks for a progress and reset
 * the bar to zero, with the minimum of disturbance to the user.
 *
 * Return value: The same #GimpProgress as passed in as %progress.
 **/
GimpProgress *
gimp_progress_restart (GimpProgress *progress,
                       const char   *message,
                       GCallback     cancel_callback,
                       gpointer      cancel_data)
{
  GtkWidget *bar;

  g_return_val_if_fail (progress != NULL, NULL);

  /*  change the message  */
  if (progress->gdisp)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar), "progress");

      if (message)
	gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar),
                             "progress",
                             message);

      bar = GIMP_STATUSBAR (shell->statusbar)->progressbar;
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (progress->dialog_label),
			  message ? message : _("Please wait..."));

      bar = progress->progressbar;
    }

  /*  reset the progress bar  */
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  /*  do we need to change the callbacks?  */
  gimp_progress_signal_setup (progress, cancel_callback, cancel_data);

  return progress;
}


void
gimp_progress_update (GimpProgress *progress,
                      gdouble       percentage)
{
  GtkWidget *bar;

  g_return_if_fail (progress != NULL);

  if (percentage < 0.0 || percentage > 1.0)
    return;

  /*  do we have a dialog box, or are we using the statusbar?  */
  if (progress->gdisp)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      bar = GIMP_STATUSBAR (shell->statusbar)->progressbar;
    }
  else
    {
      bar = progress->progressbar;
    }

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);

  /* force updates so there's feedback even when the main loop is busy */
  if (GTK_WIDGET_DRAWABLE (bar))
    gdk_window_process_updates (bar->window, TRUE);
}


/**
 * gimp_progress_step:
 * @progress: The #GimpProgress.
 *
 * Step the progress bar by one percent, wrapping at 100%
 **/
void
gimp_progress_step (GimpProgress *progress)
{
  GtkWidget *bar;
  gdouble    val;

  g_return_if_fail (progress != NULL);

  if (progress->gdisp)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      bar = GIMP_STATUSBAR (shell->statusbar)->progressbar;
    }
  else
    {
      bar = progress->progressbar;
    }

  val = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (bar)) + 0.01;
  if (val > 1.0)
    val = 0.0;

  gimp_progress_update (progress, val);
}


/**
 * gimp_progress_end:
 * @progress: The #GimpProgress.
 *
 * Finish using the progress bar.
 **/
void
gimp_progress_end (GimpProgress *progress)
{
  g_return_if_fail (progress != NULL);

  /*  We might get called from gimp_exit() and at that time
   *  the display shell has been destroyed already.
   */
  if (progress->gdisp && !progress->gdisp->shell)
    return;

  /* remove all callbacks so they don't get called while we're
   * destroying widgets
   */
  gimp_progress_signal_setup (progress, NULL, NULL);

  if (progress->gdisp)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);
      GtkProgressBar   *bar;

      gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar), "progress");

      bar = GTK_PROGRESS_BAR (GIMP_STATUSBAR (shell->statusbar)->progressbar);

      gtk_progress_bar_set_fraction (bar, 0.0);

      if (GIMP_STATUSBAR (shell->statusbar)->progressid > 0)
        GIMP_STATUSBAR (shell->statusbar)->progressid--;
    }
  else
    {
      gtk_widget_destroy (progress->dialog);
    }

  g_free (progress);
}


/*  This function's prototype is conveniently
 *  the same as progress_func_t
 */

/**
 * gimp_progress_update_and_flush:
 * @min:  The minimum, ...
 * @max:  ... the maximum, ...
 * @current: ... and the current progress of your operation.
 * @data: The #GimpProgress you want to update.
 *
 * This function's prototype is conveniently
 * the same as #GimpProgressFunc from libgimpcolor.
 **/
void
gimp_progress_update_and_flush (gint      min,
                                gint      max,
                                gint      current,
                                gpointer  data)
{
  gimp_progress_update ((GimpProgress *) data,
                        (gfloat) (current - min) / (gfloat) (max - min));

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

/*  Helper function to add or remove signals  */
static void
gimp_progress_signal_setup (GimpProgress *progress,
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
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      dialog = NULL;
      button = GIMP_STATUSBAR (shell->statusbar)->cancelbutton;
    }
  else
    {
      dialog = progress->dialog;
      button = progress->cancelbutton;
    }

  /* remove any existing signal handlers */
  if (progress->cancel_callback)
    {
      if (dialog)
	g_signal_handlers_disconnect_by_func (dialog,
                                              progress->cancel_callback,
                                              progress->cancel_data);
      else
        g_signal_handlers_disconnect_by_func (button,
                                              progress->cancel_callback,
                                              progress->cancel_data);
    }

  /* add the new handlers */
  if (cancel_callback)
    {
      if (dialog)
	g_signal_connect (dialog, "response",
                          G_CALLBACK (gimp_progress_response),
                          progress);
      else
        g_signal_connect (button, "clicked",
                          G_CALLBACK (cancel_callback),
                          cancel_data);

    }

  gtk_widget_set_sensitive (GTK_WIDGET (button),
			    cancel_callback ? TRUE : FALSE);

  progress->cancel_callback = cancel_callback;
  progress->cancel_data     = cancel_data;
}

static void
gimp_progress_response (GtkWidget    *dialog,
                        gint          response_id,
                        GimpProgress *progress)
{
  typedef void (* cb) (GtkWidget*, gpointer);

  GtkWidget *button;
  cb         callback;

  callback = (cb) progress->cancel_callback;

  if (progress->gdisp)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (progress->gdisp->shell);

      button = GIMP_STATUSBAR (shell->statusbar)->cancelbutton;
    }
  else
    {
      button = progress->cancelbutton;
    }

  callback (button, progress->cancel_data);
}
