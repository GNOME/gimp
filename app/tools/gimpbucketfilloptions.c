/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"
#include "libligmawidgets/ligmawidgets-private.h"

#include "tools-types.h"

#include "config/ligmacoreconfig.h"
#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmadatafactory.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmapaintinfo.h"
#include "core/ligmastrokeoptions.h"
#include "core/ligmatoolinfo.h"

#include "display/ligmadisplay.h"

#include "paint/ligmasourceoptions.h"

#include "widgets/ligmacontainercombobox.h"
#include "widgets/ligmacontainertreestore.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmaviewablebox.h"
#include "widgets/ligmaviewrenderer.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmabucketfilloptions.h"
#include "ligmapaintoptions-gui.h"

#include "ligma-intl.h"


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

struct _LigmaBucketFillOptionsPrivate
{
  GtkWidget *diagonal_neighbors_checkbox;
  GtkWidget *threshold_scale;

  GtkWidget *similar_color_frame;
  GtkWidget *line_art_settings;
  GtkWidget *fill_as_line_art_frame;
  GtkWidget *line_art_detect_opacity;
};

static void   ligma_bucket_fill_options_config_iface_init (LigmaConfigInterface *config_iface);

static void   ligma_bucket_fill_options_finalize          (GObject               *object);
static void   ligma_bucket_fill_options_set_property      (GObject               *object,
                                                          guint                  property_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void   ligma_bucket_fill_options_get_property      (GObject               *object,
                                                          guint                  property_id,
                                                          GValue                *value,
                                                          GParamSpec            *pspec);
static gboolean
             ligma_bucket_fill_options_select_stroke_tool (LigmaContainerView     *view,
                                                          GList                 *items,
                                                          GList                 *paths,
                                                          LigmaBucketFillOptions *options);
static void  ligma_bucket_fill_options_tool_cell_renderer (GtkCellLayout         *layout,
                                                          GtkCellRenderer       *cell,
                                                          GtkTreeModel          *model,
                                                          GtkTreeIter           *iter,
                                                          gpointer               data);
static void ligma_bucket_fill_options_image_changed       (LigmaContext           *context,
                                                          LigmaImage             *image,
                                                          LigmaBucketFillOptions *options);


static void   ligma_bucket_fill_options_reset             (LigmaConfig            *config);
static void   ligma_bucket_fill_options_update_area       (LigmaBucketFillOptions *options);


G_DEFINE_TYPE_WITH_CODE (LigmaBucketFillOptions, ligma_bucket_fill_options,
                         LIGMA_TYPE_PAINT_OPTIONS,
                         G_ADD_PRIVATE (LigmaBucketFillOptions)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_bucket_fill_options_config_iface_init))

#define parent_class ligma_bucket_fill_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_bucket_fill_options_class_init (LigmaBucketFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_bucket_fill_options_finalize;
  object_class->set_property = ligma_bucket_fill_options_set_property;
  object_class->get_property = ligma_bucket_fill_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FILL_MODE,
                         "fill-mode",
                         _("Fill type"),
                         NULL,
                         LIGMA_TYPE_BUCKET_FILL_MODE,
                         LIGMA_BUCKET_FILL_FG,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FILL_AREA,
                         "fill-area",
                         _("Fill selection"),
                         _("Which area will be filled"),
                         LIGMA_TYPE_BUCKET_FILL_AREA,
                         LIGMA_BUCKET_FILL_SIMILAR_COLORS,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_TRANSPARENT,
                            "fill-transparent",
                            _("Fill transparent areas"),
                            _("Allow completely transparent regions "
                              "to be filled"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            _("Base filled area on all visible layers"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DIAGONAL_NEIGHBORS,
                            "diagonal-neighbors",
                            _("Diagonal neighbors"),
                            _("Treat diagonally neighboring pixels as "
                              "connected"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Base fill opacity on color difference from "
                              "the clicked pixel (see threshold) or on line "
                              " art borders. Disable antialiasing to fill "
                              "the entire area uniformly."),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of fill edges"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                           "threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_LINE_ART_SOURCE,
                         "line-art-source",
                         _("Source"),
                         _("Source image for line art computation"),
                         LIGMA_TYPE_LINE_ART_SOURCE,
                         LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_COLOR_AS_LINE_ART,
                            "fill-color-as-line-art",
                            _("Manual closure in fill layer"),
                            _("Consider pixels of selected layer and filled with the fill color as line art closure"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD,
                           "fill-color-as-line-art-threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_ART_THRESHOLD,
                           "line-art-threshold",
                           _("Line art detection threshold"),
                           _("Threshold to detect contour (higher values will include more pixels)"),
                           0.0, 1.0, 0.92,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GROW,
                        "line-art-max-grow",
                        _("Maximum growing size"),
                        _("Maximum number of pixels grown under the line art"),
                        1, 100, 3,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_STROKE,
                            "line-art-stroke-border",
                            _("Stroke borders"),
                            _("Stroke fill borders with last stroke options"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_LINE_ART_STROKE_TOOL,
                           "line-art-stroke-tool",
                           _("Stroke tool"),
                           _("The tool to stroke the fill borders with"),
                           "ligma-pencil", LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_AUTOMATIC_CLOSURE,
                            "line-art-automatic-closure",
                            _("Automatic closure"),
                            _("Geometric analysis of the stroke contours to close line arts by splines/segments"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GAP_LENGTH,
                        "line-art-max-gap-length",
                        _("Maximum gap length"),
                        _("Maximum gap (in pixels) in line art which can be closed"),
                        0, 1000, 100,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FILL_CRITERION,
                         "fill-criterion",
                         _("Fill by"),
                         _("Criterion used for determining color similarity"),
                         LIGMA_TYPE_SELECT_CRITERION,
                         LIGMA_SELECT_CRITERION_COMPOSITE,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_bucket_fill_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_bucket_fill_options_reset;
}

static void
ligma_bucket_fill_options_init (LigmaBucketFillOptions *options)
{
  options->priv = ligma_bucket_fill_options_get_instance_private (options);

  options->line_art_stroke_tool = NULL;
}

static void
ligma_bucket_fill_options_finalize (GObject *object)
{
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_OPTIONS (object);

  g_clear_object (&options->stroke_options);
  g_clear_pointer (&options->line_art_stroke_tool, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_bucket_fill_options_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaBucketFillOptions *options      = LIGMA_BUCKET_FILL_OPTIONS (object);
  LigmaToolOptions       *tool_options = LIGMA_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      options->fill_mode = g_value_get_enum (value);
      ligma_bucket_fill_options_update_area (options);
      break;
    case PROP_FILL_AREA:
      options->fill_area = g_value_get_enum (value);
      ligma_bucket_fill_options_update_area (options);
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
      ligma_bucket_fill_options_update_area (options);
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
          LigmaPaintInfo *paint_info = NULL;
          Ligma          *ligma       = tool_options->tool_info->ligma;

          if (! options->line_art_stroke_tool)
            options->line_art_stroke_tool = g_strdup ("ligma-pencil");

          paint_info = LIGMA_PAINT_INFO (ligma_container_get_child_by_name (ligma->paint_info_list,
                                                                          options->line_art_stroke_tool));
          if (! paint_info && ! ligma_container_is_empty (ligma->paint_info_list))
            paint_info = LIGMA_PAINT_INFO (ligma_container_get_child_by_index (ligma->paint_info_list, 0));

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
ligma_bucket_fill_options_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_OPTIONS (object);

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
ligma_bucket_fill_options_select_stroke_tool (LigmaContainerView     *view,
                                             GList                 *items,
                                             GList                 *paths,
                                             LigmaBucketFillOptions *options)
{
  GList *iter;

  for (iter = items; iter; iter = iter->next)
    {
      g_object_set (options,
                    "line-art-stroke-tool",
                    iter->data ? ligma_object_get_name (iter->data) : NULL,
                    NULL);
      break;
    }

  return TRUE;
}

static void
ligma_bucket_fill_options_tool_cell_renderer (GtkCellLayout   *layout,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel    *model,
                                             GtkTreeIter     *iter,
                                             gpointer         data)
{
  LigmaViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer->viewable)
    {
      LigmaPaintInfo *info = LIGMA_PAINT_INFO (renderer->viewable);

      if (LIGMA_IS_SOURCE_OPTIONS (info->paint_options))
        gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                            LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE, FALSE,
                            -1);
    }
  g_object_unref (renderer);
}

static void
ligma_bucket_fill_options_image_changed (LigmaContext           *context,
                                        LigmaImage             *image,
                                        LigmaBucketFillOptions *options)
{
  LigmaImage *prev_image;

  prev_image = g_object_get_data (G_OBJECT (options), "ligma-bucket-fill-options-image");

  if (image != prev_image)
    {
      if (prev_image)
        g_signal_handlers_disconnect_by_func (prev_image,
                                              G_CALLBACK (ligma_bucket_fill_options_update_area),
                                              options);
      if (image)
        {
          g_signal_connect_object (image, "selected-channels-changed",
                                   G_CALLBACK (ligma_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "selected-layers-changed",
                                   G_CALLBACK (ligma_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
        }

      g_object_set_data (G_OBJECT (options), "ligma-bucket-fill-options-image", image);
      ligma_bucket_fill_options_update_area (options);
    }
}

static void
ligma_bucket_fill_options_reset (LigmaConfig *config)
{
  LigmaToolOptions *tool_options = LIGMA_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      tool_options->tool_info->ligma->config->default_threshold;

  parent_config_iface->reset (config);
}

static void
ligma_bucket_fill_options_update_area (LigmaBucketFillOptions *options)
{
  LigmaImage   *image;
  GList       *drawables = NULL;
  const gchar *tooltip   = _("Opaque pixels will be considered as line art "
                             "instead of low luminance pixels");

  image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));

  /* GUI not created yet. */
  if (! options->priv->threshold_scale)
    return;

  if (image)
    drawables = ligma_image_get_selected_drawables (image);

  switch (options->fill_area)
    {
    case LIGMA_BUCKET_FILL_LINE_ART:
      gtk_widget_hide (options->priv->similar_color_frame);
      gtk_widget_show (options->priv->line_art_settings);
      if ((options->fill_mode == LIGMA_BUCKET_FILL_FG ||
           options->fill_mode == LIGMA_BUCKET_FILL_BG) &&
          (options->line_art_source == LIGMA_LINE_ART_SOURCE_LOWER_LAYER ||
           options->line_art_source == LIGMA_LINE_ART_SOURCE_UPPER_LAYER))
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, TRUE);
      else
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, FALSE);

      if (image != NULL                                                  &&
          options->line_art_source != LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED &&
          g_list_length (drawables) == 1)
        {
          LigmaDrawable  *source = NULL;
          LigmaItem      *parent;
          LigmaContainer *container;
          gint           index;

          parent = ligma_item_get_parent (LIGMA_ITEM (drawables->data));
          if (parent)
            container = ligma_viewable_get_children (LIGMA_VIEWABLE (parent));
          else
            container = ligma_image_get_layers (image);

          index = ligma_item_get_index (LIGMA_ITEM (drawables->data));
          if (options->line_art_source == LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER)
            source = drawables->data;
          else if (options->line_art_source == LIGMA_LINE_ART_SOURCE_LOWER_LAYER)
            source = LIGMA_DRAWABLE (ligma_container_get_child_by_index (container, index + 1));
          else if (options->line_art_source == LIGMA_LINE_ART_SOURCE_UPPER_LAYER)
            source = LIGMA_DRAWABLE (ligma_container_get_child_by_index (container, index - 1));

          gtk_widget_set_sensitive (options->priv->line_art_detect_opacity,
                                    source != NULL &&
                                    ligma_drawable_has_alpha (source));
          if (source == NULL)
            tooltip = _("No valid source drawable selected");
          else if (! ligma_drawable_has_alpha (source))
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
    case LIGMA_BUCKET_FILL_SIMILAR_COLORS:
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
ligma_bucket_fill_options_gui (LigmaToolOptions *tool_options)
{
  LigmaBucketFillOptions *options    = LIGMA_BUCKET_FILL_OPTIONS (tool_options);
  GObject               *config     = G_OBJECT (tool_options);
  Ligma                  *ligma       = tool_options->tool_info->ligma;
  GtkWidget             *vbox       = ligma_paint_options_gui (tool_options);
  GtkWidget             *box2;
  GtkWidget             *frame;
  GtkWidget             *hbox;
  GtkWidget             *widget;
  GtkWidget             *scale;
  GtkWidget             *combo;
  gchar                 *str;
  gboolean               bold;
  GdkModifierType        extend_mask = ligma_get_extend_selection_mask ();
  GdkModifierType        toggle_mask = GDK_MOD1_MASK;

  /*  fill type  */
  str = g_strdup_printf (_("Fill Type  (%s)"),
                         ligma_get_mod_string (toggle_mask)),
  frame = ligma_prop_enum_radio_frame_new (config, "fill-mode", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  hbox = ligma_prop_pattern_box_new (NULL, LIGMA_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  ligma_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             LIGMA_BUCKET_FILL_PATTERN, TRUE);

  /*  fill selection  */
  str = g_strdup_printf (_("Affected Area  (%s)"),
                         ligma_get_mod_string (extend_mask));
  frame = ligma_prop_enum_radio_frame_new (config, "fill-area", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  /* Similar color frame */
  frame = ligma_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  options->priv->similar_color_frame = frame;
  gtk_widget_show (frame);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  the fill transparent areas toggle  */
  widget = ligma_prop_check_button_new (config, "fill-transparent", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the sample merged toggle  */
  widget = ligma_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the diagonal neighbors toggle  */
  widget = ligma_prop_check_button_new (config, "diagonal-neighbors", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
  options->priv->diagonal_neighbors_checkbox = widget;

  /*  the antialias toggle  */
  widget = ligma_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the threshold scale  */
  scale = ligma_prop_spin_scale_new (config, "threshold",
                                    1.0, 16.0, 1);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);
  options->priv->threshold_scale = scale;

  /*  the fill criterion combo  */
  combo = ligma_prop_enum_combo_box_new (config, "fill-criterion", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Fill by"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /* Line art settings */
  options->priv->line_art_settings = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), options->priv->line_art_settings, FALSE, FALSE, 0);
  ligma_widget_set_identifier (options->priv->line_art_settings, "line-art-settings");

  frame = ligma_frame_new (NULL);
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
  ligma_label_set_attributes (GTK_LABEL (widget),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_widget_show (widget);

  options->line_art_busy_box = ligma_busy_box_new (_("(computing...)"));
  gtk_box_pack_start (GTK_BOX (box2), options->line_art_busy_box,
                      FALSE, FALSE, 0);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_show (box2);

  /*  Line Art: source combo (replace sample merged!) */
  combo = ligma_prop_enum_combo_box_new (config, "line-art-source", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Source"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /*  the fill transparent areas toggle  */
  widget = ligma_prop_check_button_new (config, "fill-transparent",
                                       _("Detect opacity rather than grayscale"));
  options->priv->line_art_detect_opacity = widget;
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  Line Art: stroke threshold */
  scale = ligma_prop_spin_scale_new (config, "line-art-threshold",
                                    0.05, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /* Line Art Closure frame */
  frame = ligma_frame_new (NULL);
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
  scale = ligma_prop_spin_scale_new (config, "line-art-max-gap-length",
                                    1, 5, 0);
  frame = ligma_prop_expanding_frame_new (config, "line-art-automatic-closure", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Closure: manual line art closure */
  scale = ligma_prop_spin_scale_new (config, "fill-color-as-line-art-threshold",
                                    1.0, 16.0, 1);

  frame = ligma_prop_expanding_frame_new (config, "fill-color-as-line-art", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  options->priv->fill_as_line_art_frame = frame;

  /* Line Art Borders frame */

  frame = ligma_frame_new (NULL);
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
  scale = ligma_prop_spin_scale_new (config, "line-art-max-grow",
                                    1, 5, 0);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /*  Line Art Borders: feather radius scale  */
  scale = ligma_prop_spin_scale_new (config, "feather-radius",
                                    1.0, 10.0, 1);

  frame = ligma_prop_expanding_frame_new (config, "feather", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Borders: stroke border with paint brush */

  options->stroke_options = ligma_stroke_options_new (ligma,
                                                     ligma_get_user_context (ligma),
                                                     TRUE);
  ligma_config_sync (G_OBJECT (LIGMA_DIALOG_CONFIG (ligma->config)->stroke_options),
                    G_OBJECT (options->stroke_options), 0);

  widget = ligma_container_combo_box_new (ligma->paint_info_list,
                                         LIGMA_CONTEXT (options->stroke_options),
                                         16, 0);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (widget),
                                      LIGMA_CONTAINER_COMBO_BOX (widget)->viewable_renderer,
                                      ligma_bucket_fill_options_tool_cell_renderer,
                                      options, NULL);
  g_signal_connect (widget, "select-items",
                    G_CALLBACK (ligma_bucket_fill_options_select_stroke_tool),
                    options);

  frame = ligma_prop_expanding_frame_new (config, "line-art-stroke-border", NULL,
                                         widget, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  ligma_bucket_fill_options_update_area (options);

  g_signal_connect (ligma_get_user_context (LIGMA_CONTEXT (tool_options)->ligma),
                    "image-changed",
                    G_CALLBACK (ligma_bucket_fill_options_image_changed),
                    tool_options);

  return vbox;
}
