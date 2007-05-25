/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpitem.h"
#include "core/gimpunit.h"

#include "gimpcolorframe.h"
#include "gimpcursorview.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo-aux.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED
};


static void   gimp_cursor_view_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_cursor_view_set_property      (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_cursor_view_get_property      (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

static void   gimp_cursor_view_style_set         (GtkWidget    *widget,
                                                  GtkStyle      *prev_style);

static void   gimp_cursor_view_set_aux_info      (GimpDocked   *docked,
                                                  GList        *aux_info);
static GList *gimp_cursor_view_get_aux_info      (GimpDocked   *docked);


G_DEFINE_TYPE_WITH_CODE (GimpCursorView, gimp_cursor_view, GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_cursor_view_docked_iface_init))

#define parent_class gimp_cursor_view_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_cursor_view_class_init (GimpCursorViewClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gimp_cursor_view_get_property;
  object_class->set_property = gimp_cursor_view_set_property;

  widget_class->style_set    = gimp_cursor_view_style_set;

  g_object_class_install_property (object_class, PROP_SAMPLE_MERGED,
                                   g_param_spec_boolean ("sample-merged",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_cursor_view_init (GimpCursorView *view)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  gint       content_spacing;

  view->sample_merged = TRUE;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);


  /* cursor information */

  view->coord_hbox = gtk_hbox_new (TRUE, content_spacing);
  gtk_box_pack_start (GTK_BOX (view), view->coord_hbox, FALSE, FALSE, 0);
  gtk_widget_show (view->coord_hbox);

  frame = gimp_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (view->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
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
  gtk_box_pack_start (GTK_BOX (view->coord_hbox), frame, TRUE, TRUE, 0);
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

  view->color_hbox = gtk_hbox_new (TRUE, content_spacing);
  gtk_box_pack_start (GTK_BOX (view), view->color_hbox, FALSE, FALSE, 0);
  gtk_widget_show (view->color_hbox);

  view->color_frame_1 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->color_frame_1),
                             GIMP_COLOR_FRAME_MODE_PIXEL);
  gtk_box_pack_start (GTK_BOX (view->color_hbox), view->color_frame_1,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->color_frame_1);

  view->color_frame_2 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->color_frame_2),
                             GIMP_COLOR_FRAME_MODE_RGB);
  gtk_box_pack_start (GTK_BOX (view->color_hbox), view->color_frame_2,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->color_frame_2);

  /* sample merged toggle */

  toggle = gimp_prop_check_button_new (G_OBJECT (view), "sample-merged",
                                       _("_Sample Merged"));
  gtk_box_pack_start (GTK_BOX (view), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
}

static void
gimp_cursor_view_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_aux_info = gimp_cursor_view_set_aux_info;
  iface->get_aux_info = gimp_cursor_view_get_aux_info;
}

static void
gimp_cursor_view_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      view->sample_merged = g_value_get_boolean (value);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_cursor_view_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, view->sample_merged);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

#define AUX_INFO_FRAME_1_MODE "frame-1-mode"
#define AUX_INFO_FRAME_2_MODE "frame-2-mode"

static void
gimp_cursor_view_set_aux_info (GimpDocked *docked,
                               GList      *aux_info)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (docked);
  GList          *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux   = list->data;
      GtkWidget          *frame = NULL;

      if (! strcmp (aux->name, AUX_INFO_FRAME_1_MODE))
        frame = view->color_frame_1;
      else if (! strcmp (aux->name, AUX_INFO_FRAME_2_MODE))
        frame = view->color_frame_2;

      if (frame)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          enum_class = g_type_class_peek (GIMP_TYPE_COLOR_FRAME_MODE);
          enum_value = g_enum_get_value_by_nick (enum_class, aux->value);

          if (enum_value)
            gimp_color_frame_set_mode (GIMP_COLOR_FRAME (frame),
                                       enum_value->value);
        }
    }
}

static GList *
gimp_cursor_view_get_aux_info (GimpDocked *docked)
{
  GimpCursorView     *view = GIMP_CURSOR_VIEW (docked);
  GList              *aux_info;
  const gchar        *nick;
  GimpSessionInfoAux *aux;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (gimp_enum_get_value (GIMP_TYPE_COLOR_FRAME_MODE,
                           GIMP_COLOR_FRAME (view->color_frame_1)->frame_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = gimp_session_info_aux_new (AUX_INFO_FRAME_1_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  if (gimp_enum_get_value (GIMP_TYPE_COLOR_FRAME_MODE,
                           GIMP_COLOR_FRAME (view->color_frame_2)->frame_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = gimp_session_info_aux_new (AUX_INFO_FRAME_2_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_cursor_view_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (widget);
  gint            content_spacing;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  gtk_box_set_spacing (GTK_BOX (view->coord_hbox), content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->color_hbox), content_spacing);
}


/*  public functions  */

GtkWidget *
gimp_cursor_view_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_CURSOR_VIEW,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<CursorInfo>",
                       "ui-path",         "/cursor-info-popup",
                       NULL);
}

void
gimp_cursor_view_set_sample_merged (GimpCursorView *view,
                                    gboolean        sample_merged)
{
  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));

  sample_merged = sample_merged ? TRUE : FALSE;

  if (view->sample_merged != sample_merged)
    {
      view->sample_merged = sample_merged;

      g_object_notify (G_OBJECT (view), "sample-merged");
    }
}

gboolean
gimp_cursor_view_get_sample_merged (GimpCursorView *view)
{
  g_return_val_if_fail (GIMP_IS_CURSOR_VIEW (view), FALSE);

  return view->sample_merged;
}

void
gimp_cursor_view_update_cursor (GimpCursorView   *view,
                                GimpImage        *image,
                                GimpUnit          unit,
                                gdouble           x,
                                gdouble           y)
{
  gboolean      in_image;
  gdouble       unit_factor;
  gint          unit_digits;
  const gchar  *unit_str;
  gchar         format_buf[32];
  gchar         buf[32];
  GimpImageType sample_type;
  GimpRGB       color;
  gint          color_index;

  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (unit == GIMP_UNIT_PIXEL)
    unit = gimp_image_get_unit (image);

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

  if (gimp_image_pick_color (image, NULL,
                             (gint) floor (x),
                             (gint) floor (y),
                             view->sample_merged,
                             FALSE, 0.0,
                             &sample_type, &color, &color_index))
    {
      gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->color_frame_1),
                                  sample_type, &color, color_index);
      gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->color_frame_2),
                                  sample_type, &color, color_index);
    }
  else
    {
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_1));
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_2));
    }
}

void
gimp_cursor_view_clear_cursor (GimpCursorView *view)
{
  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));

  gtk_label_set_text (GTK_LABEL (view->pixel_x_label), _("n/a"));
  gtk_label_set_text (GTK_LABEL (view->pixel_y_label), _("n/a"));
  gtk_label_set_text (GTK_LABEL (view->unit_x_label),  _("n/a"));
  gtk_label_set_text (GTK_LABEL (view->unit_y_label),  _("n/a"));

  gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_1));
  gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->color_frame_2));
}
