/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacursorview.c
 * Copyright (C) 2005-2016 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-color.h"
#include "core/ligmaitem.h"

#include "widgets/ligmacolorframe.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmasessioninfo-aux.h"

#include "ligmacursorview.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_SAMPLE_MERGED
};


struct _LigmaCursorViewPrivate
{
  LigmaEditor        parent_instance;

  Ligma             *ligma;

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

  LigmaContext      *context;
  LigmaDisplayShell *shell;
  LigmaImage        *image;

  LigmaUnit          unit;
  guint             cursor_idle_id;
  LigmaImage        *cursor_image;
  LigmaUnit          cursor_unit;
  gdouble           cursor_x;
  gdouble           cursor_y;
};


static void       ligma_cursor_view_docked_iface_init     (LigmaDockedInterface *iface);

static void       ligma_cursor_view_dispose               (GObject             *object);
static void       ligma_cursor_view_constructed           (GObject             *object);
static void       ligma_cursor_view_set_property          (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void       ligma_cursor_view_get_property          (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void       ligma_cursor_view_style_updated         (GtkWidget           *widget);

static void       ligma_cursor_view_set_aux_info          (LigmaDocked          *docked,
                                                          GList               *aux_info);
static GList    * ligma_cursor_view_get_aux_info          (LigmaDocked          *docked);

static void       ligma_cursor_view_set_context           (LigmaDocked          *docked,
                                                          LigmaContext         *context);
static void       ligma_cursor_view_image_changed         (LigmaCursorView      *view,
                                                          LigmaImage           *image,
                                                          LigmaContext         *context);
static void       ligma_cursor_view_mask_changed          (LigmaCursorView      *view,
                                                          LigmaImage           *image);
static void       ligma_cursor_view_diplay_changed        (LigmaCursorView      *view,
                                                          LigmaDisplay         *display,
                                                          LigmaContext         *context);
static void       ligma_cursor_view_shell_unit_changed    (LigmaCursorView      *view,
                                                          GParamSpec          *pspec,
                                                          LigmaDisplayShell    *shell);
static void       ligma_cursor_view_format_as_unit        (LigmaUnit             unit,
                                                          gchar               *output_buf,
                                                          gint                 output_buf_size,
                                                          gdouble              pixel_value,
                                                          gdouble              image_res);
static void       ligma_cursor_view_set_label_italic      (GtkWidget           *label,
                                                          gboolean             italic);
static void       ligma_cursor_view_update_selection_info (LigmaCursorView      *view,
                                                          LigmaImage           *image,
                                                          LigmaUnit             unit);
static gboolean   ligma_cursor_view_cursor_idle           (LigmaCursorView      *view);


G_DEFINE_TYPE_WITH_CODE (LigmaCursorView, ligma_cursor_view, LIGMA_TYPE_EDITOR,
                         G_ADD_PRIVATE (LigmaCursorView)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_cursor_view_docked_iface_init))

#define parent_class ligma_cursor_view_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_cursor_view_class_init (LigmaCursorViewClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = ligma_cursor_view_constructed;
  object_class->dispose       = ligma_cursor_view_dispose;
  object_class->get_property  = ligma_cursor_view_get_property;
  object_class->set_property  = ligma_cursor_view_set_property;

  widget_class->style_updated = ligma_cursor_view_style_updated;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SAMPLE_MERGED,
                                   g_param_spec_boolean ("sample-merged",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_cursor_view_init (LigmaCursorView *view)
{
  GtkWidget *frame;
  GtkWidget *grid;
  GtkWidget *toggle;
  gint       content_spacing;

  view->priv = ligma_cursor_view_get_instance_private (view);

  view->priv->sample_merged  = TRUE;
  view->priv->context        = NULL;
  view->priv->shell          = NULL;
  view->priv->image          = NULL;
  view->priv->unit           = LIGMA_UNIT_PIXEL;
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

  frame = ligma_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  view->priv->pixel_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_x_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->pixel_x_label, 1);

  view->priv->pixel_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_y_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->pixel_y_label, 1);


  /* Units */

  frame = ligma_frame_new (_("Units"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  view->priv->unit_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_x_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->unit_x_label, 1);

  view->priv->unit_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_y_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->unit_y_label, 1);


  /* Selection Bounding Box */

  frame = ligma_frame_new (_("Selection"));
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  ligma_help_set_help_data (frame, _("The selection's bounding box"), NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  view->priv->selection_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_x_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->selection_x_label, 1);

  view->priv->selection_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_y_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->selection_y_label, 1);

  frame = ligma_frame_new ("");
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  view->priv->selection_width_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_width_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            /* Width */
                            _("W"), 0.5, 0.5,
                            view->priv->selection_width_label, 1);

  view->priv->selection_height_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_height_label), 1.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            /* Height */
                            _("H"), 0.5, 0.5,
                            view->priv->selection_height_label, 1);

  /* sample merged toggle */

  toggle = ligma_prop_check_button_new (G_OBJECT (view), "sample-merged",
                                       _("_Sample Merged"));
  gtk_box_pack_start (GTK_BOX (view), toggle, FALSE, FALSE, 0);
}

static void
ligma_cursor_view_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_aux_info = ligma_cursor_view_set_aux_info;
  iface->get_aux_info = ligma_cursor_view_get_aux_info;
  iface->set_context  = ligma_cursor_view_set_context;
}

static void
ligma_cursor_view_constructed (GObject             *object)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (object);
  gint            content_spacing;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  /* color information */
  view->priv->color_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                        content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->color_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->color_hbox, FALSE, FALSE, 0);
  gtk_widget_show (view->priv->color_hbox);

  view->priv->color_frame_1 = ligma_color_frame_new (view->priv->ligma);
  ligma_color_frame_set_mode (LIGMA_COLOR_FRAME (view->priv->color_frame_1),
                             LIGMA_COLOR_PICK_MODE_PIXEL);
  ligma_color_frame_set_ellipsize (LIGMA_COLOR_FRAME (view->priv->color_frame_1),
                                  PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_1,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->priv->color_frame_1);

  view->priv->color_frame_2 = ligma_color_frame_new (view->priv->ligma);
  ligma_color_frame_set_mode (LIGMA_COLOR_FRAME (view->priv->color_frame_2),
                             LIGMA_COLOR_PICK_MODE_RGB_PERCENT);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_2,
                      TRUE, TRUE, 0);
  gtk_widget_show (view->priv->color_frame_2);

}

static void
ligma_cursor_view_dispose (GObject *object)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (object);

  if (view->priv->context)
    ligma_docked_set_context (LIGMA_DOCKED (view), NULL);

  if (view->priv->cursor_idle_id)
    {
      g_source_remove (view->priv->cursor_idle_id);
      view->priv->cursor_idle_id = 0;
    }

  view->priv->color_frame_1 = NULL;
  view->priv->color_frame_2 = NULL;

  g_clear_object (&view->priv->cursor_image);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_cursor_view_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      view->priv->ligma = g_value_get_object (value);
      break;

    case PROP_SAMPLE_MERGED:
      view->priv->sample_merged = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_cursor_view_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, view->priv->ligma);
      break;

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
ligma_cursor_view_set_aux_info (LigmaDocked *docked,
                               GList      *aux_info)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (docked);
  GList          *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux   = list->data;
      GtkWidget          *frame = NULL;

      if (! strcmp (aux->name, AUX_INFO_FRAME_1_MODE))
        frame = view->priv->color_frame_1;
      else if (! strcmp (aux->name, AUX_INFO_FRAME_2_MODE))
        frame = view->priv->color_frame_2;

      if (frame)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          enum_class = g_type_class_peek (LIGMA_TYPE_COLOR_PICK_MODE);
          enum_value = g_enum_get_value_by_nick (enum_class, aux->value);

          if (enum_value)
            ligma_color_frame_set_mode (LIGMA_COLOR_FRAME (frame),
                                       enum_value->value);
        }
    }
}

static GList *
ligma_cursor_view_get_aux_info (LigmaDocked *docked)
{
  LigmaCursorView     *view = LIGMA_CURSOR_VIEW (docked);
  GList              *aux_info;
  const gchar        *nick;
  LigmaSessionInfoAux *aux;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (ligma_enum_get_value (LIGMA_TYPE_COLOR_PICK_MODE,
                           LIGMA_COLOR_FRAME (view->priv->color_frame_1)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = ligma_session_info_aux_new (AUX_INFO_FRAME_1_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  if (ligma_enum_get_value (LIGMA_TYPE_COLOR_PICK_MODE,
                           LIGMA_COLOR_FRAME (view->priv->color_frame_2)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = ligma_session_info_aux_new (AUX_INFO_FRAME_2_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
ligma_cursor_view_format_as_unit (LigmaUnit  unit,
                                 gchar    *output_buf,
                                 gint      output_buf_size,
                                 gdouble   pixel_value,
                                 gdouble   image_res)
{
  gchar         format_buf[32];
  gdouble       value;
  gint          unit_digits = 0;
  const gchar  *unit_str = "";

  value = ligma_pixels_to_units (pixel_value, unit, image_res);

  if (unit != LIGMA_UNIT_PIXEL)
    {
      unit_digits = ligma_unit_get_scaled_digits (unit, image_res);
      unit_str    = ligma_unit_get_abbreviation (unit);
    }

  g_snprintf (format_buf, sizeof (format_buf),
              "%%.%df %s", unit_digits, unit_str);

  g_snprintf (output_buf, output_buf_size, format_buf, value);
}

static void
ligma_cursor_view_set_label_italic (GtkWidget *label,
                                   gboolean   italic)
{
  PangoStyle attribute = italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;

  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, attribute,
                             -1);
}

static void
ligma_cursor_view_style_updated (GtkWidget *widget)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (widget);
  gint            content_spacing;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  gtk_box_set_spacing (GTK_BOX (view->priv->coord_hbox),     content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->selection_hbox), content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->color_hbox),     content_spacing);
}

static void
ligma_cursor_view_set_context (LigmaDocked  *docked,
                              LigmaContext *context)
{
  LigmaCursorView  *view    = LIGMA_CURSOR_VIEW (docked);
  LigmaColorConfig *config  = NULL;
  LigmaDisplay     *display = NULL;
  LigmaImage       *image   = NULL;

  if (context == view->priv->context)
    return;

  if (view->priv->context)
    {
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            ligma_cursor_view_diplay_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            ligma_cursor_view_image_changed,
                                            view);

      g_object_unref (view->priv->context);
    }

  view->priv->context = context;

  if (view->priv->context)
    {
      g_object_ref (view->priv->context);

      g_signal_connect_swapped (view->priv->context, "display-changed",
                                G_CALLBACK (ligma_cursor_view_diplay_changed),
                                view);

      g_signal_connect_swapped (view->priv->context, "image-changed",
                                G_CALLBACK (ligma_cursor_view_image_changed),
                                view);

      config  = context->ligma->config->color_management;
      display = ligma_context_get_display (context);
      image   = ligma_context_get_image (context);
    }

  ligma_color_frame_set_color_config (LIGMA_COLOR_FRAME (view->priv->color_frame_1),
                                     config);
  ligma_color_frame_set_color_config (LIGMA_COLOR_FRAME (view->priv->color_frame_2),
                                     config);

  ligma_cursor_view_diplay_changed (view, display, view->priv->context);
  ligma_cursor_view_image_changed (view, image, view->priv->context);
}

static void
ligma_cursor_view_image_changed (LigmaCursorView *view,
                                LigmaImage      *image,
                                LigmaContext    *context)
{
  g_return_if_fail (LIGMA_IS_CURSOR_VIEW (view));

  if (image == view->priv->image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            ligma_cursor_view_mask_changed,
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      g_signal_connect_swapped (view->priv->image, "mask-changed",
                                G_CALLBACK (ligma_cursor_view_mask_changed),
                                view);
    }

  ligma_cursor_view_mask_changed (view, view->priv->image);
}

static void
ligma_cursor_view_mask_changed (LigmaCursorView *view,
                               LigmaImage      *image)
{
  ligma_cursor_view_update_selection_info (view,
                                          view->priv->image,
                                          view->priv->unit);
}

static void
ligma_cursor_view_diplay_changed (LigmaCursorView *view,
                                 LigmaDisplay    *display,
                                 LigmaContext    *context)
{
  LigmaDisplayShell *shell = NULL;

  if (display)
    shell = ligma_display_get_shell (display);

  if (view->priv->shell)
    {
      g_signal_handlers_disconnect_by_func (view->priv->shell,
                                            ligma_cursor_view_shell_unit_changed,
                                            view);
    }

  view->priv->shell = shell;

  if (view->priv->shell)
    {
      g_signal_connect_swapped (view->priv->shell, "notify::unit",
                                G_CALLBACK (ligma_cursor_view_shell_unit_changed),
                                view);
    }

  ligma_cursor_view_shell_unit_changed (view,
                                       NULL,
                                       view->priv->shell);
}

static void
ligma_cursor_view_shell_unit_changed (LigmaCursorView   *view,
                                     GParamSpec       *pspec,
                                     LigmaDisplayShell *shell)
{
  LigmaUnit new_unit = LIGMA_UNIT_PIXEL;

  if (shell)
    {
      new_unit = ligma_display_shell_get_unit (shell);
    }

  if (view->priv->unit != new_unit)
    {
      ligma_cursor_view_update_selection_info (view, view->priv->image, new_unit);
      view->priv->unit = new_unit;
    }
}

static void
ligma_cursor_view_update_selection_info (LigmaCursorView *view,
                                        LigmaImage      *image,
                                        LigmaUnit        unit)
{
  gint x, y, width, height;

  if (image &&
      ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                        &x, &y, &width, &height))
    {
      gdouble xres, yres;
      gchar   buf[32];

      ligma_image_get_resolution (image, &xres, &yres);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_x_label), buf);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_y_label), buf);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), width, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_width_label), buf);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), height, yres);
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
ligma_cursor_view_cursor_idle (LigmaCursorView *view)
{

  if (view->priv->cursor_image)
    {
      LigmaImage  *image = view->priv->cursor_image;
      LigmaUnit    unit  = view->priv->cursor_unit;
      gdouble     x     = view->priv->cursor_x;
      gdouble     y     = view->priv->cursor_y;
      gboolean    in_image;
      gchar       buf[32];
      const Babl *sample_format;
      gdouble     pixel[4];
      LigmaRGB     color;
      gdouble     xres;
      gdouble     yres;
      gint        int_x;
      gint        int_y;

      if (unit == LIGMA_UNIT_PIXEL)
        unit = ligma_image_get_unit (image);

      ligma_image_get_resolution (image, &xres, &yres);

      in_image = (x >= 0.0 && x < ligma_image_get_width  (image) &&
                  y >= 0.0 && y < ligma_image_get_height (image));

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (x));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_x_label), buf);
      ligma_cursor_view_set_label_italic (view->priv->pixel_x_label, ! in_image);

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (y));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_y_label), buf);
      ligma_cursor_view_set_label_italic (view->priv->pixel_y_label, ! in_image);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_x_label), buf);
      ligma_cursor_view_set_label_italic (view->priv->unit_x_label, ! in_image);

      ligma_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_y_label), buf);
      ligma_cursor_view_set_label_italic (view->priv->unit_y_label, ! in_image);

      int_x = (gint) floor (x);
      int_y = (gint) floor (y);

      if (ligma_image_pick_color (image, NULL,
                                 int_x, int_y,
                                 view->priv->shell->show_all,
                                 view->priv->sample_merged,
                                 FALSE, 0.0,
                                 &sample_format, pixel, &color))
        {
          ligma_color_frame_set_color (LIGMA_COLOR_FRAME (view->priv->color_frame_1),
                                      FALSE, sample_format, pixel, &color,
                                      int_x, int_y);
          ligma_color_frame_set_color (LIGMA_COLOR_FRAME (view->priv->color_frame_2),
                                      FALSE, sample_format, pixel, &color,
                                      int_x, int_y);
        }
      else
        {
          ligma_color_frame_set_invalid (LIGMA_COLOR_FRAME (view->priv->color_frame_1));
          ligma_color_frame_set_invalid (LIGMA_COLOR_FRAME (view->priv->color_frame_2));
        }

      /* Show the selection info from the image under the cursor if any */
      ligma_cursor_view_update_selection_info (view,
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

      ligma_color_frame_set_invalid (LIGMA_COLOR_FRAME (view->priv->color_frame_1));
      ligma_color_frame_set_invalid (LIGMA_COLOR_FRAME (view->priv->color_frame_2));

      /* Start showing selection info from the active image again */
      ligma_cursor_view_update_selection_info (view,
                                              view->priv->image,
                                              view->priv->unit);
    }

  view->priv->cursor_idle_id = 0;

  return G_SOURCE_REMOVE;
}


/*  public functions  */

GtkWidget *
ligma_cursor_view_new (Ligma            *ligma,
                      LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_CURSOR_VIEW,
                       "ligma",            ligma,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<CursorInfo>",
                       "ui-path",         "/cursor-info-popup",
                       NULL);
}

void
ligma_cursor_view_set_sample_merged (LigmaCursorView *view,
                                    gboolean        sample_merged)
{
  g_return_if_fail (LIGMA_IS_CURSOR_VIEW (view));

  sample_merged = sample_merged ? TRUE : FALSE;

  if (view->priv->sample_merged != sample_merged)
    {
      view->priv->sample_merged = sample_merged;

      g_object_notify (G_OBJECT (view), "sample-merged");
    }
}

gboolean
ligma_cursor_view_get_sample_merged (LigmaCursorView *view)
{
  g_return_val_if_fail (LIGMA_IS_CURSOR_VIEW (view), FALSE);

  return view->priv->sample_merged;
}

void
ligma_cursor_view_update_cursor (LigmaCursorView   *view,
                                LigmaImage        *image,
                                LigmaUnit          shell_unit,
                                gdouble           x,
                                gdouble           y)
{
  g_return_if_fail (LIGMA_IS_CURSOR_VIEW (view));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_clear_object (&view->priv->cursor_image);

  view->priv->cursor_image = g_object_ref (image);
  view->priv->cursor_unit  = shell_unit;
  view->priv->cursor_x     = x;
  view->priv->cursor_y     = y;

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) ligma_cursor_view_cursor_idle, view);
    }
}

void
ligma_cursor_view_clear_cursor (LigmaCursorView *view)
{
  g_return_if_fail (LIGMA_IS_CURSOR_VIEW (view));

  g_clear_object (&view->priv->cursor_image);

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) ligma_cursor_view_cursor_idle, view);
    }
}
