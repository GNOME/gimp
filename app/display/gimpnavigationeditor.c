/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpnavigationview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpdocked.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpnavigationpreview.h"
#include "widgets/gimppreviewrenderer.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpnavigationview.h"

#include "gimp-intl.h"


#define MAX_SCALE_BUF 20


static void   gimp_navigation_view_class_init (GimpNavigationViewClass *klass);
static void   gimp_navigation_view_init       (GimpNavigationView      *view);

static void   gimp_navigation_view_docked_iface_init  (GimpDockedInterface *docked_iface);
static void   gimp_navigation_view_set_docked_context (GimpDocked       *docked,
                                                       GimpContext      *context,
                                                       GimpContext      *prev_context);

static void   gimp_navigation_view_destroy          (GtkObject          *object);

static GtkWidget * gimp_navigation_view_new_private (GimpDisplayShell   *shell,
                                                     GimpDisplayConfig  *config,
                                                     gboolean            popup);

static gboolean gimp_navigation_view_button_release (GtkWidget          *widget,
                                                     GdkEventButton     *bevent,
                                                     GimpDisplayShell   *shell);
static void   gimp_navigation_view_marker_changed   (GimpNavigationPreview *preview,
                                                     gdouble             x,
                                                     gdouble             y,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_zoom             (GimpNavigationPreview *preview,
                                                     GimpZoomType        direction,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_scroll           (GimpNavigationPreview *preview,
                                                     GdkScrollDirection  direction,
                                                     GimpNavigationView *view);

static void   gimp_navigation_view_zoom_adj_changed (GtkAdjustment      *adj,
                                                     GimpNavigationView *view);

static void   gimp_navigation_view_zoom_out_clicked (GtkWidget          *widget,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_zoom_in_clicked  (GtkWidget          *widget,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_zoom_100_clicked (GtkWidget          *widget,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_zoom_fit_clicked (GtkWidget          *widget,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_shrink_clicked   (GtkWidget          *widget,
                                                     GimpNavigationView *view);

static void   gimp_navigation_view_shell_scaled     (GimpDisplayShell   *shell,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_shell_scrolled   (GimpDisplayShell   *shell,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_shell_reconnect  (GimpDisplayShell   *shell,
                                                     GimpNavigationView *view);
static void   gimp_navigation_view_update_marker    (GimpNavigationView *view);


static GimpEditorClass *parent_class = NULL;


GType
gimp_navigation_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpNavigationViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_navigation_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_navigation */
        sizeof (GimpNavigationView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_navigation_view_init,
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_navigation_view_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpNavigationView",
                                     &view_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_navigation_view_class_init (GimpNavigationViewClass *klass)
{
  GObjectClass   *object_class;
  GtkObjectClass *gtk_object_class;

  object_class     = G_OBJECT_CLASS (klass);
  gtk_object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gtk_object_class->destroy = gimp_navigation_view_destroy;
}

static void
gimp_navigation_view_init (GimpNavigationView *view)
{
  GtkWidget *frame;

  view->shell = NULL;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (view), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  view->preview = gimp_preview_new_by_types (GIMP_TYPE_NAVIGATION_PREVIEW,
                                             GIMP_TYPE_IMAGE,
                                             GIMP_PREVIEW_SIZE_MEDIUM, 0, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), view->preview);
  gtk_widget_show (view->preview);

  g_signal_connect (view->preview, "marker_changed",
                    G_CALLBACK (gimp_navigation_view_marker_changed),
                    view);
  g_signal_connect (view->preview, "zoom",
                    G_CALLBACK (gimp_navigation_view_zoom),
                    view);
  g_signal_connect (view->preview, "scroll",
                    G_CALLBACK (gimp_navigation_view_scroll),
                    view);

  gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);
}

static void
gimp_navigation_view_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->set_context = gimp_navigation_view_set_docked_context;
}

static void
gimp_navigation_view_docked_context_changed (GimpContext        *context,
                                             GimpDisplay        *gdisp,
                                             GimpNavigationView *view)
{
  GimpDisplayShell *shell = NULL;

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_navigation_view_set_shell (view, shell);
}

static void
gimp_navigation_view_set_docked_context (GimpDocked  *docked,
                                         GimpContext *context,
                                         GimpContext *prev_context)
{
  GimpNavigationView *view  = GIMP_NAVIGATION_VIEW (docked);
  GimpDisplay        *gdisp = NULL;
  GimpDisplayShell   *shell = NULL;

  if (prev_context)
    g_signal_handlers_disconnect_by_func (prev_context,
                                          gimp_navigation_view_docked_context_changed,
                                          view);

  if (context)
    {
      g_signal_connect (context, "display_changed",
                        G_CALLBACK (gimp_navigation_view_docked_context_changed),
                        view);

      gdisp = gimp_context_get_display (context);
    }

  if (gdisp)
    shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_navigation_view_set_shell (view, shell);
}

static void
gimp_navigation_view_destroy (GtkObject *object)
{
  GimpNavigationView *view;

  view = GIMP_NAVIGATION_VIEW (object);

  if (view->shell)
    gimp_navigation_view_set_shell (view, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*  public functions  */

GtkWidget *
gimp_navigation_view_new (GimpDisplayShell  *shell,
                          GimpDisplayConfig *config)
{
  return gimp_navigation_view_new_private (shell, config, FALSE);
}

void
gimp_navigation_view_set_shell (GimpNavigationView *view,
                                GimpDisplayShell   *shell)
{
  g_return_if_fail (GIMP_IS_NAVIGATION_VIEW (view));
  g_return_if_fail (! shell || GIMP_IS_DISPLAY_SHELL (shell));

  if (shell == view->shell)
    return;

  if (view->shell)
    {
      g_signal_handlers_disconnect_by_func (view->shell,
                                            gimp_navigation_view_shell_scaled,
                                            view);
      g_signal_handlers_disconnect_by_func (view->shell,
                                            gimp_navigation_view_shell_scrolled,
                                            view);
      g_signal_handlers_disconnect_by_func (view->shell,
                                            gimp_navigation_view_shell_reconnect,
                                            view);
    }
  else if (shell)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (view), TRUE);
    }

  view->shell = shell;

  if (view->shell)
    {
      gimp_preview_set_viewable (GIMP_PREVIEW (view->preview),
                                 GIMP_VIEWABLE (shell->gdisp->gimage));

      g_signal_connect (view->shell, "scaled",
                        G_CALLBACK (gimp_navigation_view_shell_scaled),
                        view);
      g_signal_connect (view->shell, "scrolled",
                        G_CALLBACK (gimp_navigation_view_shell_scrolled),
                        view);
      g_signal_connect (view->shell, "reconnect",
                        G_CALLBACK (gimp_navigation_view_shell_reconnect),
                        view);

      gimp_navigation_view_shell_scaled (view->shell, view);
    }
  else
    {
      gimp_preview_set_viewable (GIMP_PREVIEW (view->preview), NULL);
      gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);
    }
}

void
gimp_navigation_view_popup (GimpDisplayShell *shell,
                            GtkWidget        *widget,
                            gint              click_x,
                            gint              click_y)
{
  GimpNavigationView    *view;
  GimpNavigationPreview *preview;
  GdkScreen             *screen;
  gint                   x, y;
  gint                   x_org, y_org;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (! shell->nav_popup)
    {
      GimpDisplayConfig *config;
      GtkWidget         *frame;

      shell->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (shell->nav_popup), frame);
      gtk_widget_show (frame);

      config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

      view = GIMP_NAVIGATION_VIEW (gimp_navigation_view_new_private (shell,
                                                                     config,
                                                                     TRUE));
      gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (view));
      gtk_widget_show (GTK_WIDGET (view));

      g_signal_connect (view->preview, "button_release_event",
                        G_CALLBACK (gimp_navigation_view_button_release),
                        shell);
    }
  else
    {
      view = GIMP_NAVIGATION_VIEW (GTK_BIN (GTK_BIN (shell->nav_popup)->child)->child);
    }

  screen = gtk_widget_get_screen (widget);

  gtk_window_set_screen (GTK_WINDOW (shell->nav_popup), screen);

  preview = GIMP_NAVIGATION_PREVIEW (view->preview);

  /* decide where to put the popup */
  gdk_window_get_origin (widget->window, &x_org, &y_org);

#define BORDER_PEN_WIDTH  3

  x = (x_org + click_x -
       preview->p_x -
       0.5 * (preview->p_width  - BORDER_PEN_WIDTH) -
       2   * widget->style->xthickness);

  y = (y_org + click_y -
       preview->p_y -
       0.5 * (preview->p_height - BORDER_PEN_WIDTH) -
       2   * widget->style->ythickness);

  /* If the popup doesn't fit into the screen, we have a problem.
   * We move the popup onscreen and risk that the pointer is not
   * in the square representing the viewable area anymore. Moving
   * the pointer will make the image scroll by a large amount,
   * but then it works as usual. Probably better than a popup that
   * is completely unusable in the lower right of the screen.
   *
   * Warping the pointer would be another solution ...
   */

  x = CLAMP (x, 0, (gdk_screen_get_width (screen)  -
                    GIMP_PREVIEW (preview)->renderer->width  -
                    4 * widget->style->xthickness));
  y = CLAMP (y, 0, (gdk_screen_get_height (screen) -
                    GIMP_PREVIEW (preview)->renderer->height -
                    4 * widget->style->ythickness));

  gtk_window_move (GTK_WINDOW (shell->nav_popup), x, y);
  gtk_widget_show (shell->nav_popup);

  gdk_flush ();

  /* fill in then grab pointer */
  preview->motion_offset_x = 0.5 * (preview->p_width  - BORDER_PEN_WIDTH);
  preview->motion_offset_y = 0.5 * (preview->p_height - BORDER_PEN_WIDTH);

#undef BORDER_PEN_WIDTH

  gimp_navigation_preview_grab_pointer (preview);
}


/*  private functions  */

static GtkWidget *
gimp_navigation_view_new_private (GimpDisplayShell  *shell,
                                  GimpDisplayConfig *config,
                                  gboolean           popup)
{
  GimpNavigationView *view;

  g_return_val_if_fail (! shell || GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY_CONFIG (config), NULL);
  g_return_val_if_fail (! popup || (popup && shell), NULL);

  view = g_object_new (GIMP_TYPE_NAVIGATION_VIEW, NULL);

  if (popup)
    {
      GimpPreview *preview = GIMP_PREVIEW (view->preview);

      gimp_preview_renderer_set_size (preview->renderer,
                                      config->nav_preview_size * 3,
                                      preview->renderer->border_width);
    }
  else
    {
      GtkWidget *hscale;

      gtk_widget_set_size_request (view->preview,
                                   GIMP_PREVIEW_SIZE_HUGE,
                                   GIMP_PREVIEW_SIZE_HUGE);
      gimp_preview_set_expand (GIMP_PREVIEW (view->preview), TRUE);

      /* the editor buttons */

      view->zoom_out_button =
        gimp_editor_add_button (GIMP_EDITOR (view),
                                GTK_STOCK_ZOOM_OUT, _("Zoom out"),
                                GIMP_HELP_VIEW_ZOOM_OUT,
                                G_CALLBACK (gimp_navigation_view_zoom_out_clicked),
                                NULL,
                                view);

      view->zoom_in_button =
        gimp_editor_add_button (GIMP_EDITOR (view),
                                GTK_STOCK_ZOOM_IN, _("Zoom in"),
                                GIMP_HELP_VIEW_ZOOM_IN,
                                G_CALLBACK (gimp_navigation_view_zoom_in_clicked),
                                NULL,
                                view);

      view->zoom_100_button =
        gimp_editor_add_button (GIMP_EDITOR (view),
                                GTK_STOCK_ZOOM_100, _("Zoom 1:1"),
                                GIMP_HELP_VIEW_ZOOM_100,
                                G_CALLBACK (gimp_navigation_view_zoom_100_clicked),
                                NULL,
                                view);

      view->zoom_fit_button =
        gimp_editor_add_button (GIMP_EDITOR (view),
                                GTK_STOCK_ZOOM_FIT, _("Zoom to fit window"),
                                GIMP_HELP_VIEW_ZOOM_FIT,
                                G_CALLBACK (gimp_navigation_view_zoom_fit_clicked),
                                NULL,
                                view);

      view->shrink_wrap_button =
        gimp_editor_add_button (GIMP_EDITOR (view),
                                GTK_STOCK_ZOOM_FIT, _("Shrink Wrap"),
                                GIMP_HELP_VIEW_SHRINK_WRAP,
                                G_CALLBACK (gimp_navigation_view_shrink_clicked),
                                NULL,
                                view);

      /* the zoom scale */

      view->zoom_adjustment =
        GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -15.0, 15.0, 1.0, 1.0, 0.0));

      g_signal_connect (view->zoom_adjustment, "value_changed",
                        G_CALLBACK (gimp_navigation_view_zoom_adj_changed),
                        view);

      hscale = gtk_hscale_new (GTK_ADJUSTMENT (view->zoom_adjustment));
      gtk_range_set_update_policy (GTK_RANGE (hscale), GTK_UPDATE_DELAYED);
      gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
      gtk_scale_set_digits (GTK_SCALE (hscale), 0);
      gtk_box_pack_end (GTK_BOX (view), hscale, FALSE, FALSE, 0);
      gtk_widget_show (hscale);

      /* the zoom label */

      view->zoom_label = gtk_label_new ("1:1");
      gtk_box_pack_end (GTK_BOX (view), view->zoom_label, FALSE, FALSE, 0);
      gtk_widget_show (view->zoom_label);

      /* eek */
      {
        GtkRequisition requisition;

        gtk_widget_size_request (view->zoom_label, &requisition);
        gtk_widget_set_size_request (view->zoom_label,
                                     4 * requisition.width,
                                     requisition.height);
      }
    }

  if (shell)
    gimp_navigation_view_set_shell (view, shell);

  gimp_preview_renderer_set_background (GIMP_PREVIEW (view->preview)->renderer,
                                        GIMP_STOCK_TEXTURE);

  return GTK_WIDGET (view);
}

static gboolean
gimp_navigation_view_button_release (GtkWidget        *widget,
                                     GdkEventButton   *bevent,
                                     GimpDisplayShell *shell)
{
  if (bevent->button == 1)
    {
      gtk_widget_hide (shell->nav_popup);
    }

  return FALSE;
}

static void
gimp_navigation_view_marker_changed (GimpNavigationPreview *preview,
                                     gdouble                x,
                                     gdouble                y,
                                     GimpNavigationView    *view)
{
  if (view->shell)
    {
      gdouble xratio;
      gdouble yratio;
      gint    xoffset;
      gint    yoffset;

      xratio = SCALEFACTOR_X (view->shell);
      yratio = SCALEFACTOR_Y (view->shell);

      xoffset = RINT (x * xratio - view->shell->offset_x);
      yoffset = RINT (y * yratio - view->shell->offset_y);

      gimp_display_shell_scroll (view->shell, xoffset, yoffset);
    }
}

static void
gimp_navigation_view_zoom (GimpNavigationPreview *preview,
                           GimpZoomType           direction,
                           GimpNavigationView    *view)
{
  if (view->shell)
    {
      gimp_display_shell_scale (view->shell, direction);
    }
}

static void
gimp_navigation_view_scroll (GimpNavigationPreview *preview,
                             GdkScrollDirection     direction,
                             GimpNavigationView    *view)
{
  if (view->shell)
    {
      GtkAdjustment *adj = NULL;
      gdouble        value;

      switch (direction)
        {
        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_RIGHT:
          adj = view->shell->hsbdata;
          break;

        case GDK_SCROLL_UP:
        case GDK_SCROLL_DOWN:
          adj = view->shell->vsbdata;
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
}

static void
gimp_navigation_view_zoom_adj_changed (GtkAdjustment      *adj,
                                       GimpNavigationView *view)
{
  gint value;
  gint scalesrc;
  gint scaledest;

  value = RINT (adj->value);

  if (value < 0)
    {
      scalesrc  = - value + 1;
      scaledest = 1;
    }
  else
    {
      scalesrc  = 1;
      scaledest = value + 1;
    }

  g_print ("zoom_adj_changed: %d : %d (%f)\n", scaledest, scalesrc,
           adj->value);

  gimp_display_shell_scale (view->shell, (scaledest * 100) + scalesrc);
}

static void
gimp_navigation_view_zoom_out_clicked (GtkWidget          *widget,
                                       GimpNavigationView *view)
{
  if (view->shell)
    gimp_display_shell_scale (view->shell, GIMP_ZOOM_OUT);
}

static void
gimp_navigation_view_zoom_in_clicked (GtkWidget          *widget,
                                      GimpNavigationView *view)
{
  if (view->shell)
    gimp_display_shell_scale (view->shell, GIMP_ZOOM_IN);
}

static void
gimp_navigation_view_zoom_100_clicked (GtkWidget          *widget,
                                       GimpNavigationView *view)
{
  if (view->shell)
    gimp_display_shell_scale (view->shell, 101);
}

static void
gimp_navigation_view_zoom_fit_clicked (GtkWidget          *widget,
                                       GimpNavigationView *view)
{
  if (view->shell)
    gimp_display_shell_scale_fit (view->shell);
}

static void
gimp_navigation_view_shrink_clicked (GtkWidget          *widget,
                                     GimpNavigationView *view)
{
  if (view->shell)
    gimp_display_shell_scale_shrink_wrap (view->shell);
}

static void
gimp_navigation_view_shell_scaled (GimpDisplayShell   *shell,
                                   GimpNavigationView *view)
{
  if (view->zoom_label)
    {
      gchar scale_str[MAX_SCALE_BUF];

      /* Update the zoom scale string */
      g_snprintf (scale_str, sizeof (scale_str), "%d:%d",
                  SCALEDEST (view->shell), 
                  SCALESRC (view->shell));

      gtk_label_set_text (GTK_LABEL (view->zoom_label), scale_str);
    }

  if (view->zoom_adjustment)
    {
      gdouble f;
      gint    val;

      f = (((gdouble) SCALEDEST (view->shell)) / 
           ((gdouble) SCALESRC (view->shell)));
  
      if (f < 1.0)
        val = - RINT (1.0 / f) + 1;
      else
        val = RINT (f) - 1;

      g_signal_handlers_block_by_func (view->zoom_adjustment,
                                       gimp_navigation_view_zoom_adj_changed,
                                       view);

      gtk_adjustment_set_value (view->zoom_adjustment, val);

      g_signal_handlers_unblock_by_func (view->zoom_adjustment,
                                         gimp_navigation_view_zoom_adj_changed,
                                         view);
    }

  gimp_navigation_view_update_marker (view);
}

static void
gimp_navigation_view_shell_scrolled (GimpDisplayShell   *shell,
                                     GimpNavigationView *view)
{
  gimp_navigation_view_update_marker (view);
}

static void
gimp_navigation_view_shell_reconnect (GimpDisplayShell   *shell,
                                      GimpNavigationView *view)
{
  gimp_preview_set_viewable (GIMP_PREVIEW (view->preview),
                             GIMP_VIEWABLE (shell->gdisp->gimage));
}

static void
gimp_navigation_view_update_marker (GimpNavigationView *view)
{
  GimpPreviewRenderer *renderer;
  gdouble              xratio;
  gdouble              yratio;

  renderer = GIMP_PREVIEW (view->preview)->renderer;

  xratio = SCALEFACTOR_X (view->shell);
  yratio = SCALEFACTOR_Y (view->shell);

  if (renderer->dot_for_dot != view->shell->dot_for_dot)
    gimp_preview_renderer_set_dot_for_dot (renderer,
                                           view->shell->dot_for_dot);

  gimp_navigation_preview_set_marker (GIMP_NAVIGATION_PREVIEW (view->preview),
                                      view->shell->offset_x    / xratio,
                                      view->shell->offset_y    / yratio,
                                      view->shell->disp_width  / xratio,
                                      view->shell->disp_height / yratio);
}
