/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokeoptions.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "paint/gimpsourceoptions.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainertreestore.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpbucketfilloptions.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_FILL_MODE,
  PROP_FILL_AREA,
  PROP_FILL_TRANSPARENT,
  PROP_SAMPLE_MERGED,
  PROP_DIAGONAL_NEIGHBORS,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_THRESHOLD,
  PROP_LINE_ART_SOURCE,
  PROP_LINE_ART_THRESHOLD,
  PROP_LINE_ART_MAX_GROW,
  PROP_LINE_ART_STROKE,
  PROP_LINE_ART_STROKE_TOOL,
  PROP_LINE_ART_AUTOMATIC_CLOSURE,
  PROP_LINE_ART_MAX_GAP_LENGTH,
  PROP_FILL_CRITERION,
  PROP_FILL_COLOR_AS_LINE_ART,
  PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD,
};

struct _GimpBucketFillOptionsPrivate
{
  GtkWidget *diagonal_neighbors_checkbox;
  GtkWidget *threshold_scale;

  GtkWidget *similar_color_frame;
  GtkWidget *line_art_settings;
  GtkWidget *fill_as_line_art_frame;
  GtkWidget *line_art_detect_opacity;
};

static void   gimp_bucket_fill_options_config_iface_init (GimpConfigInterface *config_iface);

static void   gimp_bucket_fill_options_finalize          (GObject               *object);
static void   gimp_bucket_fill_options_set_property      (GObject               *object,
                                                          guint                  property_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void   gimp_bucket_fill_options_get_property      (GObject               *object,
                                                          guint                  property_id,
                                                          GValue                *value,
                                                          GParamSpec            *pspec);
static gboolean
             gimp_bucket_fill_options_select_stroke_tool (GimpContainerView     *view,
                                                          GList                 *items,
                                                          GList                 *paths,
                                                          GimpBucketFillOptions *options);
static void  gimp_bucket_fill_options_tool_cell_renderer (GtkCellLayout         *layout,
                                                          GtkCellRenderer       *cell,
                                                          GtkTreeModel          *model,
                                                          GtkTreeIter           *iter,
                                                          gpointer               data);
static void gimp_bucket_fill_options_image_changed       (GimpContext           *context,
                                                          GimpImage             *image,
                                                          GimpBucketFillOptions *options);


static void   gimp_bucket_fill_options_reset             (GimpConfig            *config);
static void   gimp_bucket_fill_options_update_area       (GimpBucketFillOptions *options);


G_DEFINE_TYPE_WITH_CODE (GimpBucketFillOptions, gimp_bucket_fill_options,
                         GIMP_TYPE_PAINT_OPTIONS,
                         G_ADD_PRIVATE (GimpBucketFillOptions)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_bucket_fill_options_config_iface_init))

#define parent_class gimp_bucket_fill_options_parent_class

static GimpConfigInterface *parent_config_iface = NULL;


static void
gimp_bucket_fill_options_class_init (GimpBucketFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_bucket_fill_options_finalize;
  object_class->set_property = gimp_bucket_fill_options_set_property;
  object_class->get_property = gimp_bucket_fill_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_MODE,
                         "fill-mode",
                         _("Fill type"),
                         NULL,
                         GIMP_TYPE_BUCKET_FILL_MODE,
                         GIMP_BUCKET_FILL_FG,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_AREA,
                         "fill-area",
                         _("Fill selection"),
                         _("Which area will be filled"),
                         GIMP_TYPE_BUCKET_FILL_AREA,
                         GIMP_BUCKET_FILL_SIMILAR_COLORS,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_TRANSPARENT,
                            "fill-transparent",
                            _("Fill transparent areas"),
                            _("Allow completely transparent regions "
                              "to be filled"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            _("Base filled area on all visible layers"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DIAGONAL_NEIGHBORS,
                            "diagonal-neighbors",
                            _("Diagonal neighbors"),
                            _("Treat diagonally neighboring pixels as "
                              "connected"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Base fill opacity on color difference from "
                              "the clicked pixel (see threshold) or on line "
                              " art borders. Disable antialiasing to fill "
                              "the entire area uniformly."),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of fill edges"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                           "threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LINE_ART_SOURCE,
                         "line-art-source",
                         _("Source"),
                         _("Source image for line art computation"),
                         GIMP_TYPE_LINE_ART_SOURCE,
                         GIMP_LINE_ART_SOURCE_SAMPLE_MERGED,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_COLOR_AS_LINE_ART,
                            "fill-color-as-line-art",
                            _("Manual closure in fill layer"),
                            _("Consider pixels of selected layer and filled with the fill color as line art closure"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD,
                           "fill-color-as-line-art-threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_ART_THRESHOLD,
                           "line-art-threshold",
                           _("Line art detection threshold"),
                           _("Threshold to detect contour (higher values will include more pixels)"),
                           0.0, 1.0, 0.92,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GROW,
                        "line-art-max-grow",
                        _("Maximum growing size"),
                        _("Maximum number of pixels grown under the line art"),
                        1, 100, 3,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_STROKE,
                            "line-art-stroke-border",
                            _("Stroke borders"),
                            _("Stroke fill borders with last stroke options"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LINE_ART_STROKE_TOOL,
                           "line-art-stroke-tool",
                           _("Stroke tool"),
                           _("The tool to stroke the fill borders with"),
                           "gimp-pencil", GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_AUTOMATIC_CLOSURE,
                            "line-art-automatic-closure",
                            _("Automatic closure"),
                            _("Geometric analysis of the stroke contours to close line arts by splines/segments"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GAP_LENGTH,
                        "line-art-max-gap-length",
                        _("Maximum gap length"),
                        _("Maximum gap (in pixels) in line art which can be closed"),
                        0, 1000, 100,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_CRITERION,
                         "fill-criterion",
                         _("Fill by"),
                         _("Criterion used for determining color similarity"),
                         GIMP_TYPE_SELECT_CRITERION,
                         GIMP_SELECT_CRITERION_COMPOSITE,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_bucket_fill_options_config_iface_init (GimpConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = gimp_bucket_fill_options_reset;
}

static void
gimp_bucket_fill_options_init (GimpBucketFillOptions *options)
{
  options->priv = gimp_bucket_fill_options_get_instance_private (options);

  options->line_art_stroke_tool = NULL;
}

static void
gimp_bucket_fill_options_finalize (GObject *object)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_OPTIONS (object);

  g_clear_object (&options->stroke_options);
  g_clear_pointer (&options->line_art_stroke_tool, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_bucket_fill_options_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpBucketFillOptions *options      = GIMP_BUCKET_FILL_OPTIONS (object);
  GimpToolOptions       *tool_options = GIMP_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      options->fill_mode = g_value_get_enum (value);
      gimp_bucket_fill_options_update_area (options);
      break;
    case PROP_FILL_AREA:
      options->fill_area = g_value_get_enum (value);
      gimp_bucket_fill_options_update_area (options);
      break;
    case PROP_FILL_TRANSPARENT:
      options->fill_transparent = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_DIAGONAL_NEIGHBORS:
      options->diagonal_neighbors = g_value_get_boolean (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_FEATHER:
      options->feather = g_value_get_boolean (value);
      break;
    case PROP_FEATHER_RADIUS:
      options->feather_radius = g_value_get_double (value);
      break;
    case PROP_THRESHOLD:
      options->threshold = g_value_get_double (value);
      break;
    case PROP_LINE_ART_SOURCE:
      options->line_art_source = g_value_get_enum (value);
      gimp_bucket_fill_options_update_area (options);
      break;
    case PROP_LINE_ART_THRESHOLD:
      options->line_art_threshold = g_value_get_double (value);
      break;
    case PROP_LINE_ART_MAX_GROW:
      options->line_art_max_grow = g_value_get_int (value);
      break;
    case PROP_LINE_ART_STROKE:
      options->line_art_stroke = g_value_get_boolean (value);
      break;
    case PROP_LINE_ART_STROKE_TOOL:
      g_clear_pointer (&options->line_art_stroke_tool, g_free);
      options->line_art_stroke_tool = g_value_dup_string (value);

      if (options->stroke_options)
        {
          GimpPaintInfo *paint_info = NULL;
          Gimp          *gimp       = tool_options->tool_info->gimp;

          if (! options->line_art_stroke_tool)
            options->line_art_stroke_tool = g_strdup ("gimp-pencil");

          paint_info = GIMP_PAINT_INFO (gimp_container_get_child_by_name (gimp->paint_info_list,
                                                                          options->line_art_stroke_tool));
          if (! paint_info && ! gimp_container_is_empty (gimp->paint_info_list))
            paint_info = GIMP_PAINT_INFO (gimp_container_get_child_by_index (gimp->paint_info_list, 0));

          g_object_set (options->stroke_options,
                        "paint-info", paint_info,
                        NULL);
        }
      break;
    case PROP_LINE_ART_AUTOMATIC_CLOSURE:
      options->line_art_automatic_closure = g_value_get_boolean (value);
      break;
    case PROP_LINE_ART_MAX_GAP_LENGTH:
      options->line_art_max_gap_length = g_value_get_int (value);
      break;
    case PROP_FILL_CRITERION:
      options->fill_criterion = g_value_get_enum (value);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART:
      options->fill_as_line_art = g_value_get_boolean (value);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD:
      options->fill_as_line_art_threshold = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_bucket_fill_options_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      g_value_set_enum (value, options->fill_mode);
      break;
    case PROP_FILL_AREA:
      g_value_set_enum (value, options->fill_area);
      break;
    case PROP_FILL_TRANSPARENT:
      g_value_set_boolean (value, options->fill_transparent);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_DIAGONAL_NEIGHBORS:
      g_value_set_boolean (value, options->diagonal_neighbors);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_FEATHER:
      g_value_set_boolean (value, options->feather);
      break;
    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, options->feather_radius);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, options->threshold);
      break;
    case PROP_LINE_ART_SOURCE:
      g_value_set_enum (value, options->line_art_source);
      break;
    case PROP_LINE_ART_THRESHOLD:
      g_value_set_double (value, options->line_art_threshold);
      break;
    case PROP_LINE_ART_MAX_GROW:
      g_value_set_int (value, options->line_art_max_grow);
      break;
    case PROP_LINE_ART_STROKE:
      g_value_set_boolean (value, options->line_art_stroke);
      break;
    case PROP_LINE_ART_STROKE_TOOL:
      g_value_set_string (value, options->line_art_stroke_tool);
      break;
    case PROP_LINE_ART_AUTOMATIC_CLOSURE:
      g_value_set_boolean (value, options->line_art_automatic_closure);
      break;
    case PROP_LINE_ART_MAX_GAP_LENGTH:
      g_value_set_int (value, options->line_art_max_gap_length);
      break;
    case PROP_FILL_CRITERION:
      g_value_set_enum (value, options->fill_criterion);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART:
      g_value_set_boolean (value, options->fill_as_line_art);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD:
      g_value_set_double (value, options->fill_as_line_art_threshold);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_bucket_fill_options_select_stroke_tool (GimpContainerView     *view,
                                             GList                 *items,
                                             GList                 *paths,
                                             GimpBucketFillOptions *options)
{
  GList *iter;

  for (iter = items; iter; iter = iter->next)
    {
      g_object_set (options,
                    "line-art-stroke-tool",
                    iter->data ? gimp_object_get_name (iter->data) : NULL,
                    NULL);
      break;
    }

  return TRUE;
}

static void
gimp_bucket_fill_options_tool_cell_renderer (GtkCellLayout   *layout,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel    *model,
                                             GtkTreeIter     *iter,
                                             gpointer         data)
{
  GimpViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer->viewable)
    {
      GimpPaintInfo *info = GIMP_PAINT_INFO (renderer->viewable);

      if (GIMP_IS_SOURCE_OPTIONS (info->paint_options))
        gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                            GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE, FALSE,
                            -1);
    }
  g_object_unref (renderer);
}

static void
gimp_bucket_fill_options_image_changed (GimpContext           *context,
                                        GimpImage             *image,
                                        GimpBucketFillOptions *options)
{
  GimpImage *prev_image;

  prev_image = g_object_get_data (G_OBJECT (options), "gimp-bucket-fill-options-image");

  if (image != prev_image)
    {
      if (prev_image)
        g_signal_handlers_disconnect_by_func (prev_image,
                                              G_CALLBACK (gimp_bucket_fill_options_update_area),
                                              options);
      if (image)
        {
          g_signal_connect_object (image, "selected-channels-changed",
                                   G_CALLBACK (gimp_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "selected-layers-changed",
                                   G_CALLBACK (gimp_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
        }

      g_object_set_data (G_OBJECT (options), "gimp-bucket-fill-options-image", image);
      gimp_bucket_fill_options_update_area (options);
    }
}

static void
gimp_bucket_fill_options_reset (GimpConfig *config)
{
  GimpToolOptions *tool_options = GIMP_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      tool_options->tool_info->gimp->config->default_threshold;

  parent_config_iface->reset (config);
}

static void
gimp_bucket_fill_options_update_area (GimpBucketFillOptions *options)
{
  GimpImage   *image;
  GList       *drawables = NULL;
  const gchar *tooltip   = _("Opaque pixels will be considered as line art "
                             "instead of low luminance pixels");

  image = gimp_context_get_image (gimp_get_user_context (GIMP_CONTEXT (options)->gimp));

  /* GUI not created yet. */
  if (! options->priv->threshold_scale)
    return;

  if (image)
    drawables = gimp_image_get_selected_drawables (image);

  switch (options->fill_area)
    {
    case GIMP_BUCKET_FILL_LINE_ART:
      gtk_widget_hide (options->priv->similar_color_frame);
      gtk_widget_show (options->priv->line_art_settings);
      if ((options->fill_mode == GIMP_BUCKET_FILL_FG ||
           options->fill_mode == GIMP_BUCKET_FILL_BG) &&
          (options->line_art_source == GIMP_LINE_ART_SOURCE_LOWER_LAYER ||
           options->line_art_source == GIMP_LINE_ART_SOURCE_UPPER_LAYER))
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, TRUE);
      else
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, FALSE);

      if (image != NULL                                                  &&
          options->line_art_source != GIMP_LINE_ART_SOURCE_SAMPLE_MERGED &&
          g_list_length (drawables) == 1)
        {
          GimpDrawable  *source = NULL;
          GimpItem      *parent;
          GimpContainer *container;
          gint           index;

          parent = gimp_item_get_parent (GIMP_ITEM (drawables->data));
          if (parent)
            container = gimp_viewable_get_children (GIMP_VIEWABLE (parent));
          else
            container = gimp_image_get_layers (image);

          index = gimp_item_get_index (GIMP_ITEM (drawables->data));
          if (options->line_art_source == GIMP_LINE_ART_SOURCE_ACTIVE_LAYER)
            source = drawables->data;
          else if (options->line_art_source == GIMP_LINE_ART_SOURCE_LOWER_LAYER)
            source = GIMP_DRAWABLE (gimp_container_get_child_by_index (container, index + 1));
          else if (options->line_art_source == GIMP_LINE_ART_SOURCE_UPPER_LAYER)
            source = GIMP_DRAWABLE (gimp_container_get_child_by_index (container, index - 1));

          gtk_widget_set_sensitive (options->priv->line_art_detect_opacity,
                                    source != NULL &&
                                    gimp_drawable_has_alpha (source));
          if (source == NULL)
            tooltip = _("No valid source drawable selected");
          else if (! gimp_drawable_has_alpha (source))
            tooltip = _("The source drawable has no alpha channel");
        }
      else
        {
          gtk_widget_set_sensitive (options->priv->line_art_detect_opacity,
                                    TRUE);
        }
      gtk_widget_set_tooltip_text (options->priv->line_art_detect_opacity,
                                   tooltip);
      break;
    case GIMP_BUCKET_FILL_SIMILAR_COLORS:
      gtk_widget_show (options->priv->similar_color_frame);
      gtk_widget_hide (options->priv->line_art_settings);
      break;
    default:
      gtk_widget_hide (options->priv->similar_color_frame);
      gtk_widget_hide (options->priv->line_art_settings);
      break;
    }

  g_list_free (drawables);
}

GtkWidget *
gimp_bucket_fill_options_gui (GimpToolOptions *tool_options)
{
  GimpBucketFillOptions *options    = GIMP_BUCKET_FILL_OPTIONS (tool_options);
  GObject               *config     = G_OBJECT (tool_options);
  Gimp                  *gimp       = tool_options->tool_info->gimp;
  GtkWidget             *vbox       = gimp_paint_options_gui (tool_options);
  GtkWidget             *box2;
  GtkWidget             *frame;
  GtkWidget             *hbox;
  GtkWidget             *widget;
  GtkWidget             *scale;
  GtkWidget             *combo;
  gchar                 *str;
  gboolean               bold;
  GdkModifierType        extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType        toggle_mask = GDK_MOD1_MASK;

  /*  fill type  */
  str = g_strdup_printf (_("Fill Type  (%s)"),
                         gimp_get_mod_string (toggle_mask)),
  frame = gimp_prop_enum_radio_frame_new (config, "fill-mode", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             GIMP_BUCKET_FILL_PATTERN, TRUE);

  /*  fill selection  */
  str = g_strdup_printf (_("Affected Area  (%s)"),
                         gimp_get_mod_string (extend_mask));
  frame = gimp_prop_enum_radio_frame_new (config, "fill-area", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  /* Similar color frame */
  frame = gimp_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  options->priv->similar_color_frame = frame;
  gtk_widget_show (frame);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  the fill transparent areas toggle  */
  widget = gimp_prop_check_button_new (config, "fill-transparent", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the sample merged toggle  */
  widget = gimp_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the diagonal neighbors toggle  */
  widget = gimp_prop_check_button_new (config, "diagonal-neighbors", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
  options->priv->diagonal_neighbors_checkbox = widget;

  /*  the antialias toggle  */
  widget = gimp_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the threshold scale  */
  scale = gimp_prop_spin_scale_new (config, "threshold",
                                    1.0, 16.0, 1);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);
  options->priv->threshold_scale = scale;

  /*  the fill criterion combo  */
  combo = gimp_prop_enum_combo_box_new (config, "fill-criterion", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Fill by"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /* Line art settings */
  options->priv->line_art_settings = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), options->priv->line_art_settings, FALSE, FALSE, 0);
  gimp_widget_set_identifier (options->priv->line_art_settings, "line-art-settings");

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Line art: label widget */
  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_frame_set_label_widget (GTK_FRAME (frame), box2);
  gtk_widget_show (box2);

  widget = gtk_label_new (_("Line Art Detection"));
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
  gtk_widget_style_get (GTK_WIDGET (frame),
                        "label-bold", &bold,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (widget),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_widget_show (widget);

  options->line_art_busy_box = gimp_busy_box_new (_("(computing...)"));
  gtk_box_pack_start (GTK_BOX (box2), options->line_art_busy_box,
                      FALSE, FALSE, 0);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  Line Art: source combo (replace sample merged!) */
  combo = gimp_prop_enum_combo_box_new (config, "line-art-source", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Source"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /*  the fill transparent areas toggle  */
  widget = gimp_prop_check_button_new (config, "fill-transparent",
                                       _("Detect opacity rather than grayscale"));
  options->priv->line_art_detect_opacity = widget;
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  Line Art: stroke threshold */
  scale = gimp_prop_spin_scale_new (config, "line-art-threshold",
                                    0.05, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /* Line Art Closure frame */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Line Art Closure: frame label */
  widget = gtk_label_new (_("Line Art Closure"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), widget);
  gtk_widget_show (widget);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  Line Art Closure: max gap length */
  scale = gimp_prop_spin_scale_new (config, "line-art-max-gap-length",
                                    1, 5, 0);
  frame = gimp_prop_expanding_frame_new (config, "line-art-automatic-closure", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Closure: manual line art closure */
  scale = gimp_prop_spin_scale_new (config, "fill-color-as-line-art-threshold",
                                    1.0, 16.0, 1);

  frame = gimp_prop_expanding_frame_new (config, "fill-color-as-line-art", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  options->priv->fill_as_line_art_frame = frame;

  /* Line Art Borders frame */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Line Art Borders: frame label */
  widget = gtk_label_new (_("Fill borders"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), widget);
  gtk_widget_show (widget);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  Line Art Borders: max growing size */
  scale = gimp_prop_spin_scale_new (config, "line-art-max-grow",
                                    1, 5, 0);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /*  Line Art Borders: feather radius scale  */
  scale = gimp_prop_spin_scale_new (config, "feather-radius",
                                    1.0, 10.0, 1);

  frame = gimp_prop_expanding_frame_new (config, "feather", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Borders: stroke border with paint brush */

  options->stroke_options = gimp_stroke_options_new (gimp,
                                                     gimp_get_user_context (gimp),
                                                     TRUE);
  gimp_config_sync (G_OBJECT (GIMP_DIALOG_CONFIG (gimp->config)->stroke_options),
                    G_OBJECT (options->stroke_options), 0);

  widget = gimp_container_combo_box_new (gimp->paint_info_list,
                                         GIMP_CONTEXT (options->stroke_options),
                                         16, 0);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (widget),
                                      GIMP_CONTAINER_COMBO_BOX (widget)->viewable_renderer,
                                      gimp_bucket_fill_options_tool_cell_renderer,
                                      options, NULL);
  g_signal_connect (widget, "select-items",
                    G_CALLBACK (gimp_bucket_fill_options_select_stroke_tool),
                    options);

  frame = gimp_prop_expanding_frame_new (config, "line-art-stroke-border", NULL,
                                         widget, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  gimp_bucket_fill_options_update_area (options);

  g_signal_connect (gimp_get_user_context (GIMP_CONTEXT (tool_options)->gimp),
                    "image-changed",
                    G_CALLBACK (gimp_bucket_fill_options_image_changed),
                    tool_options);

  return vbox;
}
