/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcursorview.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpunit.h"

#include "gimpcolorframe.h"
#include "gimpcursorview.h"
#include "gimpdocked.h"
#include "gimpsessioninfo.h"

#include "gimp-intl.h"


static void   gimp_cursor_view_class_init        (GimpCursorViewClass *klass);
static void   gimp_cursor_view_init              (GimpCursorView      *view);
static void   gimp_cursor_view_docked_iface_init (GimpDockedInterface  *docked_iface);

static void   gimp_cursor_view_destroy         (GtkObject         *object);
static void   gimp_cursor_view_style_set       (GtkWidget         *widget,
                                                GtkStyle          *prev_style);

static void   gimp_cursor_view_set_aux_info    (GimpDocked        *docked,
                                                GList             *aux_info);
static GList *gimp_cursor_view_get_aux_info     (GimpDocked       *docked);
static void   gimp_cursor_view_set_context     (GimpDocked        *docked,
                                                GimpContext       *context);

static void   gimp_cursor_view_destroy         (GtkObject         *object);

#if 0
static void   gimp_cursor_view_fg_changed      (GimpContext       *context,
                                                 const GimpRGB     *rgb,
                                                 GimpCursorView   *view);
static void   gimp_cursor_view_bg_changed      (GimpContext       *context,
                                                 const GimpRGB     *rgb,
                                                 GimpCursorView   *view);
#endif


static GimpEditorClass *parent_class = NULL;


GType
gimp_cursor_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpCursorViewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_cursor_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpCursorView),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_cursor_view_init,
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_cursor_view_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpCursorView",
                                     &view_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_cursor_view_class_init (GimpCursorViewClass* klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_cursor_view_destroy;

  widget_class->style_set = gimp_cursor_view_style_set;
}

static void
gimp_cursor_view_init (GimpCursorView *view)
{
  GtkWidget   *hbox;
  GtkWidget   *frame;
  GtkWidget   *table;
  gint         content_spacing;
  gint         button_spacing;
  GtkIconSize  button_icon_size;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content_spacing",  &content_spacing,
			"button_spacing",   &button_spacing,
                        "button_icon_size", &button_icon_size,
			NULL);


  /* cursor information */

  hbox = gtk_hbox_new (TRUE, content_spacing);
  gtk_box_pack_start (GTK_BOX (view), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->pixel_x_label = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (view->pixel_x_label), 1.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X"), 0.5, 0.5,
                             view->pixel_x_label, 1, FALSE);

  view->pixel_y_label = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (view->pixel_y_label), 1.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y"), 0.5, 0.5,
                             view->pixel_y_label, 1, FALSE);

  frame = gimp_frame_new (_("Units"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->unit_x_label = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (view->unit_x_label), 1.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X"), 0.5, 0.5,
                             view->unit_x_label, 1, FALSE);

  view->unit_y_label = gtk_label_new (_("n/a"));
  gtk_misc_set_alignment (GTK_MISC (view->unit_y_label), 1.0, 0.5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y"), 0.5, 0.5,
                             view->unit_y_label, 1, FALSE);


  /* color information */

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start (GTK_BOX (view), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  view->color_frame_1 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->color_frame_1),
                             GIMP_COLOR_FRAME_MODE_PIXEL);
  gtk_box_pack_start (GTK_BOX (hbox), view->color_frame_1, TRUE, TRUE, 0);
  gtk_widget_show (view->color_frame_1);

  view->color_frame_2 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->color_frame_2),
                             GIMP_COLOR_FRAME_MODE_RGB);
  gtk_box_pack_start (GTK_BOX (hbox), view->color_frame_2, TRUE, TRUE, 0);
  gtk_widget_show (view->color_frame_2);
}

static void
gimp_cursor_view_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->set_aux_info = gimp_cursor_view_set_aux_info;
  docked_iface->get_aux_info = gimp_cursor_view_get_aux_info;
  docked_iface->set_context  = gimp_cursor_view_set_context;
}

#define AUX_INFO_CURRENT_PAGE "current-page"

static void
gimp_cursor_view_set_aux_info (GimpDocked *docked,
                               GList      *aux_info)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (docked);
  GList          *list;

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_CURRENT_PAGE))
        {
        }
    }
}

static GList *
gimp_cursor_view_get_aux_info (GimpDocked *docked)
{
  GimpCursorView *view   = GIMP_CURSOR_VIEW (docked);
  GList          *aux_info = NULL;

#if 0
  if (notebook->cur_page)
    {
      GimpSessionInfoAux *aux;

      aux = gimp_session_info_aux_new (AUX_INFO_CURRENT_PAGE,
                                       G_OBJECT_TYPE_NAME (notebook->cur_page));
      aux_info = g_list_append (aux_info, aux);
    }
#endif

  return aux_info;
}

static void
gimp_cursor_view_set_context (GimpDocked  *docked,
                               GimpContext *context)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (docked);

  if (context == view->context)
    return;

  if (view->context)
    {
#if 0
      g_signal_handlers_disconnect_by_func (view->context,
					    gimp_cursor_view_fg_changed,
					    view);
      g_signal_handlers_disconnect_by_func (view->context,
					    gimp_cursor_view_bg_changed,
					    view);
#endif

      g_object_unref (view->context);
      view->context = NULL;
    }

  if (context)
    {
#if 0
      GimpRGB rgb;
#endif

      view->context = g_object_ref (context);

#if 0
      g_signal_connect (view->context, "foreground_changed",
			G_CALLBACK (gimp_cursor_view_fg_changed),
			view);
      g_signal_connect (view->context, "background_changed",
			G_CALLBACK (gimp_cursor_view_bg_changed),
			view);

      if (view->edit_bg)
        {
          gimp_context_get_background (view->context, &rgb);
          gimp_cursor_view_bg_changed (view->context, &rgb, view);
        }
      else
        {
          gimp_context_get_foreground (view->context, &rgb);
          gimp_cursor_view_fg_changed (view->context, &rgb, view);
        }
#endif
    }
}

static void
gimp_cursor_view_destroy (GtkObject *object)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (object);

  if (view->context)
    gimp_docked_set_context (GIMP_DOCKED (view), NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_cursor_view_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (widget);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}


/*  public functions  */

GtkWidget *
gimp_cursor_view_new (GimpContext *context)
{
  GimpCursorView *view;

  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  view = g_object_new (GIMP_TYPE_CURSOR_VIEW, NULL);

  if (context)
    gimp_docked_set_context (GIMP_DOCKED (view), context);

  return GTK_WIDGET (view);
}

void
gimp_cursor_view_update_cursor (GimpCursorView   *view,
                                GimpImage        *image,
                                GimpUnit          unit,
                                gdouble           x,
                                gdouble           y)
{
  GimpPickable *pickable = NULL;
  guchar       *color    = NULL;

  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  if (image && unit == GIMP_UNIT_PIXEL)
    unit = gimp_image_get_unit (image);

  if (image)
    {
      gboolean     in_image;
      gdouble      unit_factor;
      gint         unit_digits;
      const gchar *unit_str;
      gchar        format_buf[32];
      gchar        buf[32];

      in_image = (x >= 0.0 && x < gimp_image_get_width  (image) &&
                  y >= 0.0 && y < gimp_image_get_height (image));

      unit_factor = _gimp_unit_get_factor (image->gimp, unit);
      unit_digits = _gimp_unit_get_digits (image->gimp, unit);
      unit_str    = _gimp_unit_get_abbreviation (image->gimp, unit);

#define FORMAT_STRING(s) (in_image ? (s) : "("s")")

      g_snprintf (buf, sizeof (buf), FORMAT_STRING ("%d"), (gint) floor (x));
      gtk_label_set_text (GTK_LABEL (view->pixel_x_label), buf);

      g_snprintf (buf, sizeof (buf), FORMAT_STRING ("%d"), (gint) floor (y));
      gtk_label_set_text (GTK_LABEL (view->pixel_y_label), buf);

      g_snprintf (format_buf, sizeof (format_buf),
                  FORMAT_STRING ("%%.%df %s"), unit_digits, unit_str);

      g_snprintf (buf, sizeof (buf), format_buf,
                  x * unit_factor / image->xresolution);
      gtk_label_set_text (GTK_LABEL (view->unit_x_label), buf);

      g_snprintf (buf, sizeof (buf), format_buf,
                  y * unit_factor / image->yresolution);
      gtk_label_set_text (GTK_LABEL (view->unit_y_label), buf);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->pixel_x_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->pixel_y_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->unit_x_label),  _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->unit_y_label),  _("n/a"));
    }

  if (image)
    {
      pickable = GIMP_PICKABLE (image->projection);

      color = gimp_pickable_get_color_at (pickable,
                                          (gint) floor (x), (gint) floor (y));
    }

  if (color)
    {
      GimpImageType sample_type;
      GimpRGB       rgb;

      sample_type = gimp_pickable_get_image_type (pickable);

      gimp_rgba_set_uchar (&rgb,
                           color[RED_PIX],
                           color[GREEN_PIX],
                           color[BLUE_PIX],
                           color[ALPHA_PIX]);

      gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->color_frame_1),
                                  sample_type, &rgb, -1);
      gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->color_frame_2),
                                  sample_type, &rgb, -1);

      g_free (color);
    }
  else
    {
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_1));
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_2));
    }
}


/*  private functions  */

#if 0
static void
gimp_cursor_view_fg_changed (GimpContext     *context,
                             const GimpRGB   *rgb,
                             GimpCursorView *view)
{
  if (! view->edit_bg)
    {
      GimpHSV hsv;

      gimp_rgb_to_hsv (rgb, &hsv);

      g_signal_handlers_block_by_func (view->notebook,
                                       gimp_cursor_view_info_changed,
                                       view);

      gimp_info_selector_set_info (GIMP_INFO_SELECTOR (view->notebook),
                                   rgb, &hsv);

      g_signal_handlers_unblock_by_func (view->notebook,
                                         gimp_cursor_view_info_changed,
                                         view);
    }
}

static void
gimp_cursor_view_bg_changed (GimpContext     *context,
                             const GimpRGB   *rgb,
                             GimpCursorView *view)
{
  if (view->edit_bg)
    {
      GimpHSV hsv;

      gimp_rgb_to_hsv (rgb, &hsv);

      g_signal_handlers_block_by_func (view->notebook,
                                       gimp_cursor_view_info_changed,
                                       view);

      gimp_info_selector_set_info (GIMP_INFO_SELECTOR (view->notebook),
                                   rgb, &hsv);

      g_signal_handlers_unblock_by_func (view->notebook,
                                         gimp_cursor_view_info_changed,
                                         view);
    }
}
#endif
