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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets/widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "file/file-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scroll.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimpnavigationpreview.h"

#include "app_procs.h"
#include "gimprc.h"
#include "nav_window.h"

#include "libgimp/gimpintl.h"


#define BORDER_PEN_WIDTH 3
#define MAX_SCALE_BUF    20


struct _NavigationDialog
{
  GtkWidget        *shell;

  GtkWidget        *preview;

  GtkWidget        *zoom_label;
  GtkObject        *zoom_adjustment;
  GimpDisplayShell *disp_shell;

  gboolean          block_adj_sig; 
  gboolean          frozen;
};


static NavigationDialog * nav_dialog_new       (GimpDisplayShell   *shell);

static gchar     * nav_dialog_title            (GimpDisplayShell   *shell);
static GtkWidget * nav_dialog_create_preview   (NavigationDialog   *nav_dialog,
                                                GimpImage          *gimage);
static void        nav_dialog_update_marker    (NavigationDialog   *nav_dialog);
static void        nav_dialog_close_callback   (GtkWidget          *widget,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_marker_changed   (GimpNavigationPreview *preview,
                                                gint                x,
                                                gint                y,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_zoom             (GimpNavigationPreview *preview,
                                                GimpZoomType        direction,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_scroll           (GimpNavigationPreview *preview,
                                                GdkScrollDirection  direction,
                                                NavigationDialog   *nav_dialog);
static gboolean    nav_dialog_button_release   (GtkWidget          *widget,
                                                GdkEventButton     *bevent,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_zoom_in          (GtkWidget          *widget,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_zoom_out         (GtkWidget          *widget,
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_zoom_adj_changed (GtkAdjustment      *adj, 
                                                NavigationDialog   *nav_dialog);
static void        nav_dialog_display_changed  (GimpContext        *context,
                                                GimpDisplay        *gdisp,
                                                NavigationDialog   *nav_dialog);


static NavigationDialog *nav_window_auto = NULL;


/*  public functions  */

NavigationDialog *
nav_dialog_create (GimpDisplayShell *shell)
{
  NavigationDialog *nav_dialog;
  GtkWidget        *abox;
  GtkWidget        *frame;
  GtkWidget        *hbox;
  GtkWidget        *button;
  GtkWidget        *image;
  GtkWidget        *hscale;
  gchar            *title;
  gchar             scale_str[MAX_SCALE_BUF];

  title = nav_dialog_title (shell);

  nav_dialog = nav_dialog_new (shell);

  nav_dialog->shell = gimp_dialog_new (title, "navigation_dialog",
				       gimp_standard_help_func,
				       "dialogs/navigation_window.html",
				       GTK_WIN_POS_MOUSE,
				       FALSE, TRUE, TRUE,

				       "_delete_event_", 
                                       nav_dialog_close_callback,
				       nav_dialog, NULL, NULL, TRUE, TRUE,

				       NULL);

  g_free (title);

  gtk_dialog_set_has_separator (GTK_DIALOG (nav_dialog->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (nav_dialog->shell)->action_area);

  g_object_weak_ref (G_OBJECT (nav_dialog->shell),
                     (GWeakNotify) g_free,
                     nav_dialog);

  /* the preview */

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox), abox,
		      TRUE, TRUE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  nav_dialog->preview = nav_dialog_create_preview (nav_dialog,
                                                   shell->gdisp->gimage);
  gtk_container_add (GTK_CONTAINER (frame), nav_dialog->preview);
  gtk_widget_show (nav_dialog->preview);

  nav_dialog_update_marker (nav_dialog);

  /* the zoom scale */

  nav_dialog->zoom_adjustment =
    gtk_adjustment_new (0.0, -15.0, 16.0, 1.0, 1.0, 1.0);

  g_signal_connect (G_OBJECT (nav_dialog->zoom_adjustment), "value_changed",
                    G_CALLBACK (nav_dialog_zoom_adj_changed),
                    nav_dialog);

  hscale = gtk_hscale_new (GTK_ADJUSTMENT (nav_dialog->zoom_adjustment));
  gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
  gtk_scale_set_digits (GTK_SCALE (hscale), 0);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox), hscale,
                    FALSE, FALSE, 0);
  gtk_widget_show (hscale);

  /* the zoom buttons */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox), hbox,
                    FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked", 
                    G_CALLBACK (nav_dialog_zoom_out),
                    nav_dialog);

  /*  user zoom ratio  */
  g_snprintf (scale_str, sizeof (scale_str), "%d:%d",
	      SCALEDEST (nav_dialog->disp_shell->gdisp), 
	      SCALESRC (nav_dialog->disp_shell->gdisp));
  
  nav_dialog->zoom_label = gtk_label_new (scale_str);
  gtk_box_pack_start (GTK_BOX (hbox), nav_dialog->zoom_label, TRUE, TRUE, 0);
  gtk_widget_show (nav_dialog->zoom_label);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked", 
                    G_CALLBACK (nav_dialog_zoom_in),
                    nav_dialog);

  return nav_dialog;
}

void
nav_dialog_create_popup (GimpDisplayShell *shell,
                         GtkWidget        *widget,
                         gint              button_x,
                         gint              button_y)
{
  NavigationDialog      *nav_dialog;
  GimpNavigationPreview *preview;
  gint                   x, y;
  gint                   x_org, y_org;
  gint                   scr_w, scr_h;

  if (! shell->nav_popup)
    {
      GtkWidget *frame;
      GtkWidget *preview_frame;

      shell->nav_popup = nav_dialog = nav_dialog_new (shell);

      nav_dialog->shell = gtk_window_new (GTK_WINDOW_POPUP);

      g_object_weak_ref (G_OBJECT (nav_dialog->shell),
                         (GWeakNotify) g_free,
                         nav_dialog);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (nav_dialog->shell), frame);
      gtk_widget_show (frame);

      preview_frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (preview_frame), GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame), preview_frame);
      gtk_widget_show (preview_frame);

      nav_dialog->preview = nav_dialog_create_preview (nav_dialog,
                                                       shell->gdisp->gimage);
      gtk_container_add (GTK_CONTAINER (preview_frame), nav_dialog->preview);
      gtk_widget_show (nav_dialog->preview);

      g_signal_connect (G_OBJECT (nav_dialog->preview), "button_release_event",
                        G_CALLBACK (nav_dialog_button_release),
                        nav_dialog);
    }
  else
    {
      nav_dialog = shell->nav_popup;

      gtk_widget_hide (nav_dialog->shell);
    }

  nav_dialog_update_marker (nav_dialog);

  preview = GIMP_NAVIGATION_PREVIEW (nav_dialog->preview);

  /* decide where to put the popup */
  gdk_window_get_origin (widget->window, &x_org, &y_org);

  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();

  x = (x_org + button_x -
       preview->p_x -
       ((preview->p_width  - BORDER_PEN_WIDTH + 1) * 0.5) - 2);

  y = (y_org + button_y -
       preview->p_y -
       ((preview->p_height - BORDER_PEN_WIDTH + 1) * 0.5) - 2);

  /* If the popup doesn't fit into the screen, we have a problem.
   * We move the popup onscreen and risk that the pointer is not
   * in the square representing the viewable area anymore. Moving
   * the pointer will make the image scroll by a large amount,
   * but then it works as usual. Probably better than a popup that
   * is completely unusable in the lower right of the screen.
   *
   * Warping the pointer would be another solution ... 
   */
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + preview->p_width  > scr_w) ? scr_w - preview->p_width  - 2 : x;
  y = (y + preview->p_height > scr_h) ? scr_h - preview->p_height - 2 : y;

  gtk_window_move (GTK_WINDOW (nav_dialog->shell), x, y);
  gtk_widget_show (nav_dialog->shell);

  gdk_flush ();

  /* fill in then grab pointer */
  preview->motion_offset_x =
    (preview->p_width  - BORDER_PEN_WIDTH + 1) * 0.5;

  preview->motion_offset_y =
    (preview->p_height - BORDER_PEN_WIDTH + 1) * 0.5;

  gimp_navigation_preview_grab_pointer (preview);
}

void
nav_dialog_free (GimpDisplayShell *del_shell,
		 NavigationDialog *nav_dialog)
{
  /* So this functions works both ways..
   * it will come in here with nav_dialog == null
   * if the auto mode is on...
   */

  if (nav_dialog)
    {
      gtk_widget_destroy (nav_dialog->shell);
    }
  else if (nav_window_auto)
    {
      GimpDisplay *gdisp = NULL;
      GList       *list;

      /* Only freeze if we are displaying the image we have deleted */
      if (nav_window_auto->disp_shell != del_shell)
        return;

      for (list = GIMP_LIST (the_gimp->images)->list;
           list;
           list = g_list_next (list))
        {
          gdisp = gdisplays_check_valid (NULL, GIMP_IMAGE (list->data));

          if (gdisp)
            break;
        }

      if (gdisp)
        {
          nav_dialog_display_changed (NULL, gdisp, nav_window_auto);
        }
      else
        {
          /* Clear window and freeze */
          nav_window_auto->frozen = TRUE;
          gtk_window_set_title (GTK_WINDOW (nav_window_auto->shell), 
                                _("Navigation: No Image"));

          gtk_widget_set_sensitive (nav_window_auto->shell, FALSE);
          nav_window_auto->disp_shell = NULL;
          gtk_widget_hide (GTK_WIDGET (nav_window_auto->shell));
        }
    }
}

void
nav_dialog_show (NavigationDialog *nav_dialog)
{
  g_return_if_fail (nav_dialog != NULL);

  if (! GTK_WIDGET_VISIBLE (nav_dialog->shell))
    {
      gtk_widget_show (nav_dialog->shell);
    }
  else
    {
      gdk_window_raise (nav_dialog->shell->window);
    }
}

void
nav_dialog_show_auto (Gimp *gimp)
{
  GimpContext *context;
  GimpDisplay *gdisp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  context = gimp_get_user_context (gimp);

  gdisp = gimp_context_get_display (context);

  if (! gdisp)
    return;

  if (! nav_window_auto)
    {
      nav_window_auto = nav_dialog_create (GIMP_DISPLAY_SHELL (gdisp->shell));

      g_signal_connect (G_OBJECT (context), "display_changed",
                        G_CALLBACK (nav_dialog_display_changed),
                        nav_window_auto);
    }

  nav_dialog_show (nav_window_auto);

  gtk_widget_set_sensitive (nav_window_auto->shell, TRUE);

  nav_window_auto->frozen = FALSE;
}

void
nav_dialog_update (NavigationDialog *nav_dialog)
{
  /* So this functions works both ways..
   * it will come in here with info_win == null
   * if the auto mode is on...
   */

  if (! nav_dialog)
    {
      if (nav_window_auto && ! nav_window_auto->frozen)
        {
          nav_dialog_update (nav_window_auto);
        }

      return;
    }

  if (! GTK_WIDGET_VISIBLE (nav_dialog->shell))
    return;

  if (nav_dialog->zoom_label)
    {
      gchar scale_str[MAX_SCALE_BUF];

      /* Update the zoom scale string */
      g_snprintf (scale_str, sizeof (scale_str), "%d:%d",
                  SCALEDEST (nav_dialog->disp_shell->gdisp), 
                  SCALESRC (nav_dialog->disp_shell->gdisp));

      gtk_label_set_text (GTK_LABEL (nav_dialog->zoom_label), scale_str);
    }

  if (nav_dialog->zoom_adjustment)
    {
      GtkAdjustment *adj;
      gdouble        f;
      gdouble        val;

      adj = GTK_ADJUSTMENT (nav_dialog->zoom_adjustment);

      f = (((gdouble) SCALEDEST (nav_dialog->disp_shell->gdisp)) / 
           ((gdouble) SCALESRC (nav_dialog->disp_shell->gdisp)));
  
      if (f < 1.0)
        val = -1.0 / f;
      else
        val = f;

      if (abs ((gint) adj->value) != (gint) (val - 1) &&
          ! nav_dialog->block_adj_sig)
        {
          adj->value = val;

          gtk_adjustment_changed (GTK_ADJUSTMENT (nav_dialog->zoom_adjustment));
        }
    }

  nav_dialog_update_marker (nav_dialog);
}

void 
nav_dialog_preview_resized (NavigationDialog *nav_dialog)
{
  if (! nav_dialog)
    return;
}


/*  private functions  */

static NavigationDialog *
nav_dialog_new (GimpDisplayShell *shell)
{
  NavigationDialog *nav_dialog;

  nav_dialog = g_new0 (NavigationDialog, 1);

  nav_dialog->shell           = NULL;
  nav_dialog->preview         = NULL;
  nav_dialog->zoom_label      = NULL;
  nav_dialog->zoom_adjustment = NULL;
  nav_dialog->disp_shell      = shell;

  nav_dialog->block_adj_sig   = FALSE;
  nav_dialog->frozen          = FALSE;

  return nav_dialog;
}

static gchar *
nav_dialog_title (GimpDisplayShell *shell)
{
  gchar *basename;
  gchar *title;

  basename = file_utils_uri_to_utf8_basename (gimp_image_get_uri (shell->gdisp->gimage));

  title = g_strdup_printf (_("Navigation: %s-%d.%d"), 
			   basename,
			   gimp_image_get_ID (shell->gdisp->gimage),
			   shell->gdisp->instance);

  g_free (basename);

  return title;
}

static GtkWidget *
nav_dialog_create_preview (NavigationDialog *nav_dialog,
                           GimpImage        *gimage)
{
  GtkWidget *preview;

  preview = gimp_navigation_preview_new (gimage, gimprc.nav_preview_size);

  g_signal_connect (G_OBJECT (preview), "marker_changed",
                    G_CALLBACK (nav_dialog_marker_changed),
                    nav_dialog);
  g_signal_connect (G_OBJECT (preview), "zoom",
                    G_CALLBACK (nav_dialog_zoom),
                    nav_dialog);
  g_signal_connect (G_OBJECT (preview), "scroll",
                    G_CALLBACK (nav_dialog_scroll),
                    nav_dialog);

  return preview;
}

static void
nav_dialog_update_marker (NavigationDialog *nav_dialog)
{
  GimpImage *gimage;
  gdouble    xratio;
  gdouble    yratio;     /* Screen res ratio */

  /* Calculate preview size */
  gimage = nav_dialog->disp_shell->gdisp->gimage;

  xratio = SCALEFACTOR_X (nav_dialog->disp_shell->gdisp);
  yratio = SCALEFACTOR_Y (nav_dialog->disp_shell->gdisp);

  if (GIMP_PREVIEW (nav_dialog->preview)->dot_for_dot !=
      nav_dialog->disp_shell->gdisp->dot_for_dot)
    gimp_preview_set_dot_for_dot (GIMP_PREVIEW (nav_dialog->preview),
                                  nav_dialog->disp_shell->gdisp->dot_for_dot);

  gimp_navigation_preview_set_marker
    (GIMP_NAVIGATION_PREVIEW (nav_dialog->preview),
     RINT (nav_dialog->disp_shell->offset_x    / xratio),
     RINT (nav_dialog->disp_shell->offset_y    / yratio),
     RINT (nav_dialog->disp_shell->disp_width  / xratio),
     RINT (nav_dialog->disp_shell->disp_height / yratio));
}

static void
nav_dialog_close_callback (GtkWidget        *widget,
			   NavigationDialog *nav_dialog)
{
  gtk_widget_hide (nav_dialog->shell);
}

static void
nav_dialog_marker_changed (GimpNavigationPreview *nav_preview,
			   gint                   x,
			   gint                   y,
			   NavigationDialog      *nav_dialog)
{
  gdouble xratio;
  gdouble yratio;
  gint    xoffset;
  gint    yoffset;

  xratio = SCALEFACTOR_X (nav_dialog->disp_shell->gdisp);
  yratio = SCALEFACTOR_Y (nav_dialog->disp_shell->gdisp);

  xoffset = x * xratio - nav_dialog->disp_shell->offset_x;
  yoffset = y * yratio - nav_dialog->disp_shell->offset_y;

  gimp_display_shell_scroll (nav_dialog->disp_shell, xoffset, yoffset);
}

static void
nav_dialog_zoom (GimpNavigationPreview *nav_preview,
		 GimpZoomType           direction,
		 NavigationDialog      *nav_dialog)
{
  gimp_display_shell_scale (nav_dialog->disp_shell, direction);
}

static void
nav_dialog_scroll (GimpNavigationPreview *nav_preview,
		   GdkScrollDirection     direction,
		   NavigationDialog      *nav_dialog)
{
  GtkAdjustment *adj = NULL;
  gdouble        value;

  switch (direction)
    {
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_RIGHT:
      adj = nav_dialog->disp_shell->hsbdata;
      break;

    case GDK_SCROLL_UP:
    case GDK_SCROLL_DOWN:
      adj = nav_dialog->disp_shell->vsbdata;
      break;
    }

  g_assert (adj != NULL);

  value = adj->value;

  switch (direction)
    {
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      value -= adj->page_increment / 2;
      break;

    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      value += adj->page_increment / 2;
      break;
    }

  value = CLAMP (value, adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_set_value (adj, value);
}

static gboolean
nav_dialog_button_release (GtkWidget        *widget,
                           GdkEventButton   *bevent,
                           NavigationDialog *nav_dialog)
{
  if (bevent->button == 1)
    {
      gtk_widget_hide (nav_dialog->shell);
    }

  return FALSE;
}

static void
nav_dialog_zoom_in (GtkWidget        *widget,
                    NavigationDialog *nav_dialog)
{
  gimp_display_shell_scale (nav_dialog->disp_shell, GIMP_ZOOM_IN);
}

static void
nav_dialog_zoom_out (GtkWidget        *widget,
                     NavigationDialog *nav_dialog)
{
  gimp_display_shell_scale (nav_dialog->disp_shell, GIMP_ZOOM_OUT);
}

static void
nav_dialog_zoom_adj_changed (GtkAdjustment    *adj, 
                             NavigationDialog *nav_dialog)
{
  gint scalesrc;
  gint scaledest;

  if (adj->value < 0.0)
    {
      scalesrc = abs ((gint) adj->value - 1);
      scaledest = 1;
    }
  else
    {
      scaledest = abs ((gint) adj->value + 1);
      scalesrc = 1;
    }

  nav_dialog->block_adj_sig = TRUE;
  gimp_display_shell_scale (nav_dialog->disp_shell,
                            (scaledest * 100) + scalesrc);
  nav_dialog->block_adj_sig = FALSE;
}

static void
nav_dialog_display_changed (GimpContext      *context,
			    GimpDisplay      *gdisp,
			    NavigationDialog *nav_dialog)
{
  GimpDisplay *old_gdisp = NULL;
  GimpImage   *gimage;
  gchar       *title;

  if (nav_dialog->disp_shell)
    old_gdisp = nav_dialog->disp_shell->gdisp;

  if (gdisp == old_gdisp || ! gdisp)
    return;

  gtk_widget_set_sensitive (nav_window_auto->shell, TRUE);

  nav_dialog->frozen = FALSE;

  title = nav_dialog_title (GIMP_DISPLAY_SHELL (gdisp->shell));

  gtk_window_set_title (GTK_WINDOW (nav_dialog->shell), title);

  g_free (title);

  gimage = gdisp->gimage;

  if (gimage && gimp_container_have (context->gimp->images,
				     GIMP_OBJECT (gimage)))
    {
      nav_dialog->disp_shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      gimp_preview_set_viewable (GIMP_PREVIEW (nav_dialog->preview),
                                 GIMP_VIEWABLE (gdisp->gimage));

      nav_dialog_update (nav_dialog);
    }
}
