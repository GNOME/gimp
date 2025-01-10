/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "tools-types.h"

#include "gimpfilteroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PREVIEW,
  PROP_PREVIEW_SPLIT,
  PROP_PREVIEW_SPLIT_ALIGNMENT,
  PROP_PREVIEW_SPLIT_POSITION,
  PROP_CONTROLLER,
  PROP_BLENDING_OPTIONS_EXPANDED,
  PROP_MERGE_FILTER
};


static void   gimp_filter_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_filter_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (GimpFilterOptions, gimp_filter_options,
               GIMP_TYPE_COLOR_OPTIONS)

#define parent_class gimp_filter_options_parent_class


static void
gimp_filter_options_class_init (GimpFilterOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_filter_options_set_property;
  object_class->get_property = gimp_filter_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PREVIEW,
                            "preview",
                            _("_Preview"),
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT,
                                   g_param_spec_boolean ("preview-split",
                                                         _("Split _view"),
                                                         NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT_ALIGNMENT,
                                   g_param_spec_enum ("preview-split-alignment",
                                                      NULL, NULL,
                                                      GIMP_TYPE_ALIGNMENT_TYPE,
                                                      GIMP_ALIGN_LEFT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT_POSITION,
                                   g_param_spec_int ("preview-split-position",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_CONTROLLER,
                            "controller",
                            _("On-canvas con_trols"),
                            _("Show on-canvas filter controls"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_BLENDING_OPTIONS_EXPANDED,
                            "blending-options-expanded",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_MERGE_FILTER,
                            "merge-filter",
                            _("_Merge filter"),
                            _("If enabled, the filter is immediately merged.\n"
                              "Otherwise, it becomes an editable non-destructive filter."),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_filter_options_init (GimpFilterOptions *options)
{
}

static void
gimp_filter_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpFilterOptions *options = GIMP_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      options->preview = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_SPLIT:
      options->preview_split = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_SPLIT_ALIGNMENT:
      options->preview_split_alignment = g_value_get_enum (value);
      break;

    case PROP_PREVIEW_SPLIT_POSITION:
      options->preview_split_position = g_value_get_int (value);
      break;

    case PROP_CONTROLLER:
      options->controller = g_value_get_boolean (value);
      break;

    case PROP_BLENDING_OPTIONS_EXPANDED:
      options->blending_options_expanded = g_value_get_boolean (value);
      break;

    case PROP_MERGE_FILTER:
      options->merge_filter = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_filter_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpFilterOptions *options = GIMP_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      g_value_set_boolean (value, options->preview);
      break;

    case PROP_PREVIEW_SPLIT:
      g_value_set_boolean (value, options->preview_split);
      break;

    case PROP_PREVIEW_SPLIT_ALIGNMENT:
      g_value_set_enum (value, options->preview_split_alignment);
      break;

    case PROP_PREVIEW_SPLIT_POSITION:
      g_value_set_int (value, options->preview_split_position);
      break;

    case PROP_CONTROLLER:
      g_value_set_boolean (value, options->controller);
      break;

    case PROP_BLENDING_OPTIONS_EXPANDED:
      g_value_set_boolean (value, options->blending_options_expanded);
      break;

    case PROP_MERGE_FILTER:
      g_value_set_boolean (value, options->merge_filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_filter_options_switch_preview_side (GimpFilterOptions *options)
{
  GimpAlignmentType alignment;

  g_return_if_fail (GIMP_IS_FILTER_OPTIONS (options));

  switch (options->preview_split_alignment)
    {
    case GIMP_ALIGN_LEFT:   alignment = GIMP_ALIGN_RIGHT;  break;
    case GIMP_ALIGN_RIGHT:  alignment = GIMP_ALIGN_LEFT;   break;
    case GIMP_ALIGN_TOP:    alignment = GIMP_ALIGN_BOTTOM; break;
    case GIMP_ALIGN_BOTTOM: alignment = GIMP_ALIGN_TOP;    break;
    default:
      g_return_if_reached ();
    }

  g_object_set (options, "preview-split-alignment", alignment, NULL);
}

void
gimp_filter_options_switch_preview_orientation (GimpFilterOptions *options,
                                                gint               position_x,
                                                gint               position_y)
{
  GimpAlignmentType alignment;
  gint              position;

  g_return_if_fail (GIMP_IS_FILTER_OPTIONS (options));

  switch (options->preview_split_alignment)
    {
    case GIMP_ALIGN_LEFT:   alignment = GIMP_ALIGN_TOP;    break;
    case GIMP_ALIGN_RIGHT:  alignment = GIMP_ALIGN_BOTTOM; break;
    case GIMP_ALIGN_TOP:    alignment = GIMP_ALIGN_LEFT;   break;
    case GIMP_ALIGN_BOTTOM: alignment = GIMP_ALIGN_RIGHT;  break;
    default:
      g_return_if_reached ();
    }

  if (alignment == GIMP_ALIGN_LEFT ||
      alignment == GIMP_ALIGN_RIGHT)
    {
      position = position_x;
    }
  else
    {
      position = position_y;
    }

  g_object_set (options,
                "preview-split-alignment", alignment,
                "preview-split-position",  position,
                NULL);
}
