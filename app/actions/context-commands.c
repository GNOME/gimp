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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimplist.h"

#include "actions.h"
#include "context-commands.h"


/*  local function prototypes  */

static void      context_select_object (GimpContext           *context,
                                        GimpContainer         *container,
                                        GimpContextSelectType  select_type);
static gdouble   context_select_value  (GimpContextSelectType  select_type,
                                        gdouble                value,
                                        gdouble                min,
                                        gdouble                max,
                                        gdouble                inc,
                                        gdouble                skip_inc,
                                        gboolean               wrap);


/*  public functions  */

void
context_colors_default_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  gimp_context_set_default_colors (context);
}

void
context_colors_swap_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  gimp_context_swap_colors (context);
}

void
context_opacity_cmd_callback (GtkAction *action,
                              gint       value,
                              gpointer   data)
{
  GimpContext *context;
  gdouble      opacity;
  return_if_no_context (context, data);

  opacity = context_select_value ((GimpContextSelectType) value,
                                  gimp_context_get_opacity (context),
                                  GIMP_OPACITY_TRANSPARENT,
                                  GIMP_OPACITY_OPAQUE,
                                  0.01, 0.1, FALSE);
  gimp_context_set_opacity (context, opacity);
}

void
context_tool_select_cmd_callback (GtkAction *action,
                                  gint       value,
                                  gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->tool_info_list,
                         (GimpContextSelectType) value);
}

void
context_brush_select_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->brush_factory->container,
                         (GimpContextSelectType) value);
}

void
context_pattern_select_cmd_callback (GtkAction *action,
                                     gint       value,
                                     gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->pattern_factory->container,
                         (GimpContextSelectType) value);
}

void
context_palette_select_cmd_callback (GtkAction *action,
                                     gint       value,
                                     gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->palette_factory->container,
                         (GimpContextSelectType) value);
}

void
context_gradient_select_cmd_callback (GtkAction *action,
                                      gint       value,
                                      gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->gradient_factory->container,
                         (GimpContextSelectType) value);
}

void
context_font_select_cmd_callback (GtkAction *action,
                                  gint       value,
                                  gpointer   data)
{
  GimpContext *context;
  return_if_no_context (context, data);

  context_select_object (context, context->gimp->fonts,
                         (GimpContextSelectType) value);
}

void
context_brush_radius_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpContext *context;
  GimpBrush   *brush;
  return_if_no_context (context, data);

  brush = gimp_context_get_brush (context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      GimpBrushGenerated *generated = GIMP_BRUSH_GENERATED (brush);
      gdouble             radius;

      radius = gimp_brush_generated_get_radius (generated);

      radius = context_select_value ((GimpContextSelectType) value,
                                     radius,
                                     1.0, 4096.0,
                                     1.0, 10.0, FALSE);
      gimp_brush_generated_set_radius (generated, radius);
    }
}

void
context_brush_hardness_cmd_callback (GtkAction *action,
                                     gint       value,
                                     gpointer   data)
{
  GimpContext *context;
  GimpBrush   *brush;
  return_if_no_context (context, data);

  brush = gimp_context_get_brush (context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      GimpBrushGenerated *generated = GIMP_BRUSH_GENERATED (brush);
      gdouble             hardness;

      hardness = gimp_brush_generated_get_hardness (generated);

      hardness = context_select_value ((GimpContextSelectType) value,
                                       hardness,
                                       0.0, 1.0,
                                       0.01, 0.1, FALSE);
      gimp_brush_generated_set_hardness (generated, hardness);
    }
}

void
context_brush_aspect_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpContext *context;
  GimpBrush   *brush;
  return_if_no_context (context, data);

  brush = gimp_context_get_brush (context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      GimpBrushGenerated *generated = GIMP_BRUSH_GENERATED (brush);
      gdouble             aspect;

      aspect = gimp_brush_generated_get_aspect_ratio (generated);

      aspect = context_select_value ((GimpContextSelectType) value,
                                     aspect,
                                     1.0, 1000.0,
                                     1.0, 4.0, FALSE);
      gimp_brush_generated_set_aspect_ratio (generated, aspect);
    }
}

void
context_brush_angle_cmd_callback (GtkAction *action,
                                  gint       value,
                                  gpointer   data)
{
  GimpContext *context;
  GimpBrush   *brush;
  return_if_no_context (context, data);

  brush = gimp_context_get_brush (context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      GimpBrushGenerated *generated = GIMP_BRUSH_GENERATED (brush);
      gdouble             angle;

      angle = gimp_brush_generated_get_angle (generated);

      if (value == GIMP_CONTEXT_SELECT_FIRST)
        angle = 0.0;
      else if (value == GIMP_CONTEXT_SELECT_LAST)
        angle = 90.0;
      else
        angle = context_select_value ((GimpContextSelectType) value,
                                      angle,
                                      0.0, 180.0,
                                      1.0, 15.0, TRUE);

      gimp_brush_generated_set_angle (generated, angle);
    }
}


/*  private functions  */

static void
context_select_object (GimpContext           *context,
                       GimpContainer         *container,
                       GimpContextSelectType  select_type)
{
  GimpObject *current;
  gint        select_index;
  gint        n_children;

  current = gimp_context_get_by_type (context, container->children_type);

  if (! current)
    return;

  n_children = gimp_container_num_children (container);

  if (n_children == 0)
    return;

  switch (select_type)
    {
    case GIMP_CONTEXT_SELECT_FIRST:
      select_index = 0;
      break;

    case GIMP_CONTEXT_SELECT_LAST:
      select_index = n_children - 1;
      break;

    case GIMP_CONTEXT_SELECT_PREVIOUS:
      select_index = gimp_container_get_child_index (container, current) - 1;
      break;

    case GIMP_CONTEXT_SELECT_NEXT:
      select_index = gimp_container_get_child_index (container, current) + 1;
      break;

    case GIMP_CONTEXT_SELECT_SKIP_PREVIOUS:
      select_index = gimp_container_get_child_index (container, current) - 10;
      break;

    case GIMP_CONTEXT_SELECT_SKIP_NEXT:
      select_index = gimp_container_get_child_index (container, current) + 10;
      break;

    default:
      g_return_if_reached ();
      break;
    }

  select_index = CLAMP (select_index, 0, n_children - 1);

  current = gimp_container_get_child_by_index (container, select_index);

  if (current)
    gimp_context_set_by_type (context, container->children_type, current);
}

static gdouble
context_select_value (GimpContextSelectType  select_type,
                      gdouble                value,
                      gdouble                min,
                      gdouble                max,
                      gdouble                inc,
                      gdouble                skip_inc,
                      gboolean               wrap)
{
  switch (select_type)
    {
    case GIMP_CONTEXT_SELECT_FIRST:
      value = min;
      break;

    case GIMP_CONTEXT_SELECT_LAST:
      value = max;
      break;

    case GIMP_CONTEXT_SELECT_PREVIOUS:
      value -= inc;
      break;

    case GIMP_CONTEXT_SELECT_NEXT:
      value += inc;
      break;

    case GIMP_CONTEXT_SELECT_SKIP_PREVIOUS:
      value -= skip_inc;
      break;

    case GIMP_CONTEXT_SELECT_SKIP_NEXT:
      value += skip_inc;
      break;

    default:
      g_return_val_if_reached (min);
      break;
    }

  if (! wrap)
    return CLAMP (value, min, max);

  if (value < min)
    return max - (min - value);
  else if (value > max)
    return min + (max - value);

  return value;
}
