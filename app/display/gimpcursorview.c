/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcursorview.c
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpitem.h"

#include "widgets/gimpcolorframe.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo-aux.h"

#include "gimpcursorview.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED
};


struct _GimpCursorViewPrivate
{
  GimpEditor        parent_instance;

  GtkWidget        *coord_hbox;
  GtkWidget        *selection_hbox;
  GtkWidget        *color_hbox;

  GtkWidget        *pixel_x_label;
  GtkWidget        *pixel_y_label;
  GtkWidget        *unit_x_label;
  GtkWidget        *unit_y_label;
  GtkWidget        *selection_x_label;
  GtkWidget        *selection_y_label;
  GtkWidget        *selection_width_label;
  GtkWidget        *selection_height_label;
  GtkWidget        *color_frame_1;
  GtkWidget        *color_frame_2;

  gboolean          sample_merged;

  GimpContext      *context;
  GimpDisplayShell *shell;
  GimpImage        *image;
  GimpUnit          unit;

  guint             cursor_idle_id;
  GimpImage        *cursor_image;
  GimpUnit          cursor_unit;
  gdouble           cursor_x;
  gdouble           cursor_y;
};


static void       gimp_cursor_view_docked_iface_init     (GimpDockedInterface *iface);

static void       gimp_cursor_view_dispose               (GObject             *object);
static void       gimp_cursor_view_set_property          (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void       gimp_cursor_view_get_property          (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void       gimp_cursor_view_style_set             (GtkWidget           *widget,
                                                          GtkStyle            *prev_style);

static void       gimp_cursor_view_set_aux_info          (GimpDocked          *docked,
                                                          GList               *aux_info);
static GList    * gimp_cursor_view_get_aux_info          (GimpDocked          *docked);

static void       gimp_cursor_view_set_context           (GimpDocked          *docked,
                                                          GimpContext         *context);
static void       gimp_cursor_view_image_changed         (GimpCursorView      *view,
                                                          GimpImage           *image,
                                                          GimpContext         *context);
static void       gimp_cursor_view_mask_changed          (GimpCursorView      *view,
                                                          GimpImage           *image);
static void       gimp_cursor_view_diplay_changed        (GimpCursorView      *view,
                                                          GimpDisplay         *display,
                                                          GimpContext         *context);
static void       gimp_cursor_view_shell_unit_changed    (GimpCursorView      *view,
                                                          GParamSpec          *pspec,
                                                          GimpDisplayShell    *shell);
static void       gimp_cursor_view_format_as_unit        (GimpUnit             unit,
                                                          gchar               *output_buf,
                                                          gint                 output_buf_size,
                                                          gdouble              pixel_value,
                                                          gdouble              image_res);
static void       gimp_cursor_view_set_label_italic      (GtkWidget           *label,
                                                          gboolean             italic);
static void       gimp_cursor_view_update_selection_info (GimpCursorView      *view,
                                                          GimpImage           *image,
                                                          GimpUnit             unit);
static gboolean   gimp_cursor_view_cursor_idle           (GimpCursorView      *view);


G_DEFINE_TYPE_WITH_CODE (GimpCursorView, gimp_cursor_view, GIMP_TYPE_EDITOR,
                         G_ADD_PRIVATE (GimpCursorView)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_cursor_view_docked_iface_init))

#define parent_class gimp_cursor_view_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_cursor_view_class_init (GimpCursorViewClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = gimp_cursor_view_dispose;
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

  view->priv = gimp_cursor_view_get_instance_private (view);

  view->priv->sample_merged  = TRUE;
  view->priv->context        = NULL;
  view->priv->shell          = NULL;
  view->priv->image          = NULL;
  view->priv->unit           = GIMP_UNIT_PIXEL;
  view->priv->cursor_idle_id = 0;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);


  /* cursor information */

  view->priv->coord_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                        content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->coord_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->coord_hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (view->priv->coord_hbox);

  view->priv->selection_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                            content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->selection_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->selection_hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (view->priv->selection_hbox);


  /* Pixels */

  frame = gimp_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->priv->pixel_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_x_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X"), 0.5, 0.5,
                             view->priv->pixel_x_label, 1, FALSE);

  view->priv->pixel_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_y_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y"), 0.5, 0.5,
                             view->priv->pixel_y_label, 1, FALSE);


  /* Units */

  frame = gimp_frame_new (_("Units"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->priv->unit_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_x_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X"), 0.5, 0.5,
                             view->priv->unit_x_label, 1, FALSE);

  view->priv->unit_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_y_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y"), 0.5, 0.5,
                             view->priv->unit_y_label, 1, FALSE);


  /* Selection Bounding Box */

  frame = gimp_frame_new (_("Selection"));
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gimp_help_set_help_data (frame, _("The selection's bounding box"), NULL);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->priv->selection_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_x_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("X"), 0.5, 0.5,
                             view->priv->selection_x_label, 1, FALSE);

  view->priv->selection_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_y_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Y"), 0.5, 0.5,
                             view->priv->selection_y_label, 1, FALSE);

  frame = gimp_frame_new ("");
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  view->priv->selection_width_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_width_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             /* Width */
                             _("W"), 0.5, 0.5,
                             view->priv->selection_width_label, 1, FALSE);

  view->priv->selection_height_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_height_label), 1.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             /* Height */
                             _("H"), 0.5, 0.5,
                             view->priv->selection_height_label, 1, FALSE);


  /* color information */

  view->priv->color_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                        content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->color_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->color_hbox, FALSE, FALSE, 0);
  gtk_widget_show (view->priv->color_hbox);

  view->priv->color_frame_1 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->priv->color_frame_1),
                             GIMP_COLOR_PICK_MODE_PIXEL);
  gimp_color_frame_set_ellipsize (GIMP_COLOR_FRAME (view->priv->color_frame_1),
                                  PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_1,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->priv->color_frame_1);

  view->priv->color_frame_2 = gimp_color_frame_new ();
  gimp_color_frame_set_mode (GIMP_COLOR_FRAME (view->priv->color_frame_2),
                             GIMP_COLOR_PICK_MODE_RGB_PERCENT);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_2,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->priv->color_frame_2);

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
  iface->set_context  = gimp_cursor_view_set_context;
}

static void
gimp_cursor_view_dispose (GObject *object)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (object);

  if (view->priv->context)
    gimp_docked_set_context (GIMP_DOCKED (view), NULL);

  if (view->priv->cursor_idle_id)
    {
      g_source_remove (view->priv->cursor_idle_id);
      view->priv->cursor_idle_id = 0;
    }

  g_clear_object (&view->priv->cursor_image);

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
      view->priv->sample_merged = g_value_get_boolean (value);
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
      g_value_set_boolean (value, view->priv->sample_merged);
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
        frame = view->priv->color_frame_1;
      else if (! strcmp (aux->name, AUX_INFO_FRAME_2_MODE))
        frame = view->priv->color_frame_2;

      if (frame)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          enum_class = g_type_class_peek (GIMP_TYPE_COLOR_PICK_MODE);
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

  if (gimp_enum_get_value (GIMP_TYPE_COLOR_PICK_MODE,
                           GIMP_COLOR_FRAME (view->priv->color_frame_1)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = gimp_session_info_aux_new (AUX_INFO_FRAME_1_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  if (gimp_enum_get_value (GIMP_TYPE_COLOR_PICK_MODE,
                           GIMP_COLOR_FRAME (view->priv->color_frame_2)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = gimp_session_info_aux_new (AUX_INFO_FRAME_2_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_cursor_view_format_as_unit (GimpUnit  unit,
                                 gchar    *output_buf,
                                 gint      output_buf_size,
                                 gdouble   pixel_value,
                                 gdouble   image_res)
{
  gchar         format_buf[32];
  gdouble       value;
  gint          unit_digits = 0;
  const gchar  *unit_str = "";

  value = gimp_pixels_to_units (pixel_value, unit, image_res);

  if (unit != GIMP_UNIT_PIXEL)
    {
      unit_digits = gimp_unit_get_scaled_digits (unit, image_res);
      unit_str    = gimp_unit_get_abbreviation (unit);
    }

  g_snprintf (format_buf, sizeof (format_buf),
              "%%.%df %s", unit_digits, unit_str);

  g_snprintf (output_buf, output_buf_size, format_buf, value);
}

static void
gimp_cursor_view_set_label_italic (GtkWidget *label,
                                   gboolean   italic)
{
  PangoStyle attribute = italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, attribute,
                             -1);
}

static void
gimp_cursor_view_style_set (GtkWidget *widget,
                            GtkStyle  *prev_style)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (widget);
  gint            content_spacing;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  gtk_box_set_spacing (GTK_BOX (view->priv->coord_hbox),     content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->selection_hbox), content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->color_hbox),     content_spacing);
}

static void
gimp_cursor_view_set_context (GimpDocked  *docked,
                              GimpContext *context)
{
  GimpCursorView  *view    = GIMP_CURSOR_VIEW (docked);
  GimpColorConfig *config  = NULL;
  GimpDisplay     *display = NULL;
  GimpImage       *image   = NULL;

  if (context == view->priv->context)
    return;

  if (view->priv->context)
    {
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            gimp_cursor_view_diplay_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            gimp_cursor_view_image_changed,
                                            view);

      g_object_unref (view->priv->context);
    }

  view->priv->context = context;

  if (view->priv->context)
    {
      g_object_ref (view->priv->context);

      g_signal_connect_swapped (view->priv->context, "display-changed",
                                G_CALLBACK (gimp_cursor_view_diplay_changed),
                                view);

      g_signal_connect_swapped (view->priv->context, "image-changed",
                                G_CALLBACK (gimp_cursor_view_image_changed),
                                view);

      config  = context->gimp->config->color_management;
      display = gimp_context_get_display (context);
      image   = gimp_context_get_image (context);
    }

  gimp_color_frame_set_color_config (GIMP_COLOR_FRAME (view->priv->color_frame_1),
                                     config);
  gimp_color_frame_set_color_config (GIMP_COLOR_FRAME (view->priv->color_frame_2),
                                     config);

  gimp_cursor_view_diplay_changed (view, display, view->priv->context);
  gimp_cursor_view_image_changed (view, image, view->priv->context);
}

static void
gimp_cursor_view_image_changed (GimpCursorView *view,
                                GimpImage      *image,
                                GimpContext    *context)
{
  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));

  if (image == view->priv->image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            gimp_cursor_view_mask_changed,
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      g_signal_connect_swapped (view->priv->image, "mask-changed",
                                G_CALLBACK (gimp_cursor_view_mask_changed),
                                view);
    }

  gimp_cursor_view_mask_changed (view, view->priv->image);
}

static void
gimp_cursor_view_mask_changed (GimpCursorView *view,
                               GimpImage      *image)
{
  gimp_cursor_view_update_selection_info (view,
                                          view->priv->image,
                                          view->priv->unit);
}

static void
gimp_cursor_view_diplay_changed (GimpCursorView *view,
                                 GimpDisplay    *display,
                                 GimpContext    *context)
{
  GimpDisplayShell *shell = NULL;

  if (display)
    shell = gimp_display_get_shell (display);

  if (view->priv->shell)
    {
      g_signal_handlers_disconnect_by_func (view->priv->shell,
                                            gimp_cursor_view_shell_unit_changed,
                                            view);
    }

  view->priv->shell = shell;

  if (view->priv->shell)
    {
      g_signal_connect_swapped (view->priv->shell, "notify::unit",
                                G_CALLBACK (gimp_cursor_view_shell_unit_changed),
                                view);
    }

  gimp_cursor_view_shell_unit_changed (view,
                                       NULL,
                                       view->priv->shell);
}

static void
gimp_cursor_view_shell_unit_changed (GimpCursorView   *view,
                                     GParamSpec       *pspec,
                                     GimpDisplayShell *shell)
{
  GimpUnit new_unit = GIMP_UNIT_PIXEL;

  if (shell)
    {
      new_unit = gimp_display_shell_get_unit (shell);
    }

  if (view->priv->unit != new_unit)
    {
      gimp_cursor_view_update_selection_info (view, view->priv->image, new_unit);
      view->priv->unit = new_unit;
    }
}

static void
gimp_cursor_view_update_selection_info (GimpCursorView *view,
                                        GimpImage      *image,
                                        GimpUnit        unit)
{
  gint x, y, width, height;

  if (image &&
      gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                        &x, &y, &width, &height))
    {
      gdouble xres, yres;
      gchar   buf[32];

      gimp_image_get_resolution (image, &xres, &yres);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_x_label), buf);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_y_label), buf);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), width, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_width_label), buf);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), height, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_height_label), buf);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->priv->selection_x_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_y_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_width_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_height_label),
                          _("n/a"));
    }
}

static gboolean
gimp_cursor_view_cursor_idle (GimpCursorView *view)
{

  if (view->priv->cursor_image)
    {
      GimpImage  *image = view->priv->cursor_image;
      GimpUnit    unit  = view->priv->cursor_unit;
      gdouble     x     = view->priv->cursor_x;
      gdouble     y     = view->priv->cursor_y;
      gboolean    in_image;
      gchar       buf[32];
      const Babl *sample_format;
      guchar      pixel[32];
      GimpRGB     color;
      gdouble     xres;
      gdouble     yres;
      gint        int_x;
      gint        int_y;

      if (unit == GIMP_UNIT_PIXEL)
        unit = gimp_image_get_unit (image);

      gimp_image_get_resolution (image, &xres, &yres);

      in_image = (x >= 0.0 && x < gimp_image_get_width  (image) &&
                  y >= 0.0 && y < gimp_image_get_height (image));

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (x));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_x_label), buf);
      gimp_cursor_view_set_label_italic (view->priv->pixel_x_label, ! in_image);

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (y));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_y_label), buf);
      gimp_cursor_view_set_label_italic (view->priv->pixel_y_label, ! in_image);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_x_label), buf);
      gimp_cursor_view_set_label_italic (view->priv->unit_x_label, ! in_image);

      gimp_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_y_label), buf);
      gimp_cursor_view_set_label_italic (view->priv->unit_y_label, ! in_image);

      int_x = (gint) floor (x);
      int_y = (gint) floor (y);

      if (gimp_image_pick_color (image, NULL,
                                 int_x, int_y,
                                 view->priv->shell->show_all,
                                 view->priv->sample_merged,
                                 FALSE, 0.0,
                                 &sample_format, pixel, &color))
        {
          gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->priv->color_frame_1),
                                      FALSE, sample_format, pixel, &color,
                                      int_x, int_y);
          gimp_color_frame_set_color (GIMP_COLOR_FRAME (view->priv->color_frame_2),
                                      FALSE, sample_format, pixel, &color,
                                      int_x, int_y);
        }
      else
        {
          gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->priv->color_frame_1));
          gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->priv->color_frame_2));
        }

      /* Show the selection info from the image under the cursor if any */
      gimp_cursor_view_update_selection_info (view,
                                              image,
                                              view->priv->cursor_unit);

      g_clear_object (&view->priv->cursor_image);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_x_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_y_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->unit_x_label),  _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->unit_y_label),  _("n/a"));

      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->priv->color_frame_1));
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (view->priv->color_frame_2));

      /* Start showing selection info from the active image again */
      gimp_cursor_view_update_selection_info (view,
                                              view->priv->image,
                                              view->priv->unit);
    }

  view->priv->cursor_idle_id = 0;

  return G_SOURCE_REMOVE;
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

  if (view->priv->sample_merged != sample_merged)
    {
      view->priv->sample_merged = sample_merged;

      g_object_notify (G_OBJECT (view), "sample-merged");
    }
}

gboolean
gimp_cursor_view_get_sample_merged (GimpCursorView *view)
{
  g_return_val_if_fail (GIMP_IS_CURSOR_VIEW (view), FALSE);

  return view->priv->sample_merged;
}

void
gimp_cursor_view_update_cursor (GimpCursorView   *view,
                                GimpImage        *image,
                                GimpUnit          shell_unit,
                                gdouble           x,
                                gdouble           y)
{
  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_clear_object (&view->priv->cursor_image);

  view->priv->cursor_image = g_object_ref (image);
  view->priv->cursor_unit  = shell_unit;
  view->priv->cursor_x     = x;
  view->priv->cursor_y     = y;

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) gimp_cursor_view_cursor_idle, view);
    }
}

void
gimp_cursor_view_clear_cursor (GimpCursorView *view)
{
  g_return_if_fail (GIMP_IS_CURSOR_VIEW (view));

  g_clear_object (&view->priv->cursor_image);

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) gimp_cursor_view_cursor_idle, view);
    }
}
