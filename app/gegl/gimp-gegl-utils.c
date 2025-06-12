/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-plugin.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimp-gegl-types.h"

#include "core/gimppattern.h"
#include "core/gimpprogress.h"

#include "gimp-babl.h"
#include "gimp-gegl-loops.h"
#include "gimp-gegl-utils.h"


/*  local function prototypes  */

static gboolean   gimp_gegl_op_blacklisted    (const gchar        *name,
                                               const gchar        *categories,
                                               gboolean            block_gimp_ops);
static GList    * gimp_gegl_get_op_subclasses (GType               type,
                                               GList              *classes,
                                               gboolean            block_gimp_ops);
static gint       gimp_gegl_compare_op_names  (GeglOperationClass *a,
                                               GeglOperationClass *b);


/*  public functions  */

GList *
gimp_gegl_get_op_classes (gboolean block_gimp_ops)
{
  GList *operations;

  operations = gimp_gegl_get_op_subclasses (GEGL_TYPE_OPERATION, NULL, block_gimp_ops);

  operations = g_list_sort (operations,
                            (GCompareFunc)
                            gimp_gegl_compare_op_names);

  return operations;
}

GType
gimp_gegl_get_op_enum_type (const gchar *operation,
                            const gchar *property)
{
  GeglNode   *node;
  GObject    *op;
  GParamSpec *pspec;

  g_return_val_if_fail (operation != NULL, G_TYPE_NONE);
  g_return_val_if_fail (property != NULL, G_TYPE_NONE);

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation,
                       NULL);
  g_object_get (node, "gegl-operation", &op, NULL);
  g_object_unref (node);

  g_return_val_if_fail (op != NULL, G_TYPE_NONE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (op), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (pspec), G_TYPE_NONE);

  g_object_unref (op);

  return G_TYPE_FROM_CLASS (G_PARAM_SPEC_ENUM (pspec)->enum_class);
}

static void
gimp_gegl_progress_callback (GObject      *object,
                             gdouble       value,
                             GimpProgress *progress)
{
  if (value == 0.0)
    {
      const gchar *text = g_object_get_data (object, "gimp-progress-text");

      if (gimp_progress_is_active (progress))
        gimp_progress_set_text (progress, "%s", text);
      else
        gimp_progress_start (progress, FALSE, "%s", text);
    }
  else
    {
      gimp_progress_set_value (progress, value);

      if (value == 1.0)
        gimp_progress_end (progress);
    }
}

void
gimp_gegl_progress_connect (GeglNode     *node,
                            GimpProgress *progress,
                            const gchar  *text)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (GIMP_IS_PROGRESS (progress));
  g_return_if_fail (text != NULL);

  g_signal_connect_object (node, "progress",
                           G_CALLBACK (gimp_gegl_progress_callback),
                           progress, 0);

  g_object_set_data_full (G_OBJECT (node),
                          "gimp-progress-text", g_strdup (text),
                          (GDestroyNotify) g_free);
}

void
gimp_gegl_progress_disconnect (GeglNode     *node,
                               GimpProgress *progress)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  g_signal_handlers_disconnect_by_func (node,
                                        gimp_gegl_progress_callback,
                                        progress);
}

gboolean
gimp_gegl_node_is_source_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_SOURCE (operation);
}

gboolean
gimp_gegl_node_is_point_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_POINT_RENDER    (operation) ||
         GEGL_IS_OPERATION_POINT_FILTER    (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER  (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER3 (operation);
}

gboolean
gimp_gegl_node_is_area_filter_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_AREA_FILTER (operation) ||
         /* be conservative and return TRUE for meta ops, since they may
          * involve an area op
          */
         GEGL_IS_OPERATION_META (operation);
}

const gchar *
gimp_gegl_node_get_key (GeglNode    *node,
                        const gchar *key)
{
  const gchar *operation_name;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  operation_name = gegl_node_get_operation (node);

  if (operation_name)
    return gegl_operation_get_key (operation_name, key);
  else
    return NULL;
}

gboolean
gimp_gegl_node_has_key (GeglNode    *node,
                        const gchar *key)
{
  return gimp_gegl_node_get_key (node, key) != NULL;
}

const Babl *
gimp_gegl_node_get_format (GeglNode    *node,
                           const gchar *pad_name)
{
  GeglOperation *op;
  const Babl    *format = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);
  g_return_val_if_fail (pad_name != NULL, NULL);

  g_object_get (node, "gegl-operation", &op, NULL);

  if (op)
    {
      format = gegl_operation_get_format (op, pad_name);

      g_object_unref (op);
    }

  if (! format)
    format = babl_format ("RGBA float");

  return format;
}

void
gimp_gegl_node_set_underlying_operation (GeglNode *node,
                                         GeglNode *operation)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (operation == NULL || GEGL_IS_NODE (operation));

  g_object_set_data (G_OBJECT (node),
                     "gimp-gegl-node-underlying-operation", operation);
}

GeglNode *
gimp_gegl_node_get_underlying_operation (GeglNode *node)
{
  GeglNode *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  operation = g_object_get_data (G_OBJECT (node),
                                 "gimp-gegl-node-underlying-operation");

  if (operation)
    return gimp_gegl_node_get_underlying_operation (operation);
  else
    return node;
}

gboolean
gimp_gegl_param_spec_has_key (GParamSpec  *pspec,
                              const gchar *key,
                              const gchar *value)
{
  const gchar *v = gegl_param_spec_get_property_key (pspec, key);

  if (v && ! strcmp (v, value))
    return TRUE;

  return FALSE;
}

GeglBuffer *
gimp_gegl_buffer_dup (GeglBuffer *buffer)
{
  GeglBuffer          *new_buffer;
  const GeglRectangle *extent;
  const GeglRectangle *abyss;
  GeglRectangle        rect;
  gint                 shift_x;
  gint                 shift_y;
  gint                 tile_width;
  gint                 tile_height;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  extent = gegl_buffer_get_extent (buffer);
  abyss  = gegl_buffer_get_abyss  (buffer);

  g_object_get (buffer,
                "shift-x",     &shift_x,
                "shift-y",     &shift_y,
                "tile-width",  &tile_width,
                "tile-height", &tile_height,
                NULL);

  new_buffer = g_object_new (GEGL_TYPE_BUFFER,
                             "format",       gegl_buffer_get_format (buffer),
                             "x",            extent->x,
                             "y",            extent->y,
                             "width",        extent->width,
                             "height",       extent->height,
                             "abyss-x",      abyss->x,
                             "abyss-y",      abyss->y,
                             "abyss-width",  abyss->width,
                             "abyss-height", abyss->height,
                             "shift-x",      shift_x,
                             "shift-y",      shift_y,
                             "tile-width",   tile_width,
                             "tile-height",  tile_height,
                             NULL);

  gegl_rectangle_align_to_buffer (&rect, extent, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  gimp_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                         new_buffer, &rect);

  return new_buffer;
}

GeglBuffer *
gimp_gegl_buffer_resize (GeglBuffer   *buffer,
                         gint          new_width,
                         gint          new_height,
                         gint          offset_x,
                         gint          offset_y,
                         GeglColor    *color,
                         GimpPattern  *pattern,
                         gint          pattern_offset_x,
                         gint          pattern_offset_y)
{
  GeglBuffer          *new_buffer;
  gboolean             intersect;
  GeglRectangle        copy_rect;
  const GeglRectangle *extent;
  const Babl          *format;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  extent     = gegl_buffer_get_extent (buffer);
  format     = gegl_buffer_get_format (buffer);
  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, new_width, new_height),
                                format);

  intersect = gegl_rectangle_intersect (&copy_rect,
                                        GEGL_RECTANGLE (0, 0, extent->width,
                                                        extent->height),
                                        GEGL_RECTANGLE (offset_x, offset_y,
                                                        new_width, new_height));

  if (! intersect                   ||
      copy_rect.width  != new_width ||
      copy_rect.height != new_height)
    {
      /*  Clear the new buffer if needed and color/pattern is given */
      if (pattern)
        {
          GeglBuffer       *src_buffer;
          GeglBuffer       *dest_buffer;
          GimpColorProfile *src_profile;
          GimpColorProfile *dest_profile;

          src_buffer = gimp_pattern_create_buffer (pattern);

          src_profile  = gimp_babl_format_get_color_profile (
                           gegl_buffer_get_format (src_buffer));
          dest_profile = gimp_babl_format_get_color_profile (
                           gegl_buffer_get_format (new_buffer));

          if (gimp_color_transform_can_gegl_copy (src_profile, dest_profile))
            {
              dest_buffer = g_object_ref (src_buffer);
            }
          else
            {
              dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                             gegl_buffer_get_format (new_buffer));

              gimp_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                               dest_buffer, NULL, dest_profile,
                                               GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                               TRUE, NULL);
            }

          g_object_unref (src_profile);
          g_object_unref (dest_profile);

          gegl_buffer_set_pattern (new_buffer, NULL, dest_buffer,
                                   pattern_offset_x, pattern_offset_y);

          g_object_unref (src_buffer);
          g_object_unref (dest_buffer);
        }
      else if (color)
        {
          gegl_buffer_set_color (new_buffer, NULL, color);
        }
    }

  if (intersect && copy_rect.width && copy_rect.height)
    {
      /*  Copy the pixels in the intersection  */
      gimp_gegl_buffer_copy (buffer, &copy_rect, GEGL_ABYSS_NONE, new_buffer,
                             GEGL_RECTANGLE (copy_rect.x - offset_x,
                                             copy_rect.y - offset_y, 0, 0));
    }

  return new_buffer;
}

gboolean
gimp_gegl_buffer_set_extent (GeglBuffer          *buffer,
                             const GeglRectangle *extent)
{
  GeglRectangle aligned_old_extent;
  GeglRectangle aligned_extent;
  GeglRectangle old_extent_rem;
  GeglRectangle diff_rects[4];
  gint          n_diff_rects;
  gint          i;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);
  g_return_val_if_fail (extent != NULL, FALSE);

  gegl_rectangle_align_to_buffer (&aligned_old_extent,
                                  gegl_buffer_get_extent (buffer), buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);
  gegl_rectangle_align_to_buffer (&aligned_extent,
                                  extent, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                          &aligned_old_extent,
                                          &aligned_extent);

  for (i = 0; i < n_diff_rects; i++)
    gegl_buffer_clear (buffer, &diff_rects[i]);

  if (gegl_rectangle_intersect (&old_extent_rem,
                                gegl_buffer_get_extent (buffer),
                                &aligned_extent))
    {
      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &old_extent_rem,
                                              extent);

      for (i = 0; i < n_diff_rects; i++)
        gegl_buffer_clear (buffer, &diff_rects[i]);
    }

  return gegl_buffer_set_extent (buffer, extent);
}


/*  private functions  */

static gboolean
gimp_gegl_op_blacklisted (const gchar *name,
                          const gchar *categories_str,
                          gboolean     block_gimp_ops)
{
  static const gchar * const category_blacklist[] =
  {
    "compositors",
    "core",
    "debug",
    "display",
    "hidden",
    "input",
    "output",
    "programming",
    "transform",
    "video"
  };
  static const gchar * const name_blacklist[] =
  {
    /* these ops are blacklisted for other reasons */
    "gegl:color", /* pointless */
    "gegl:contrast-curve",
    "gegl:convert-format", /* pointless */
    "gegl:ditto", /* pointless */
    "gegl:fill-path",
    "gegl:gray", /* we use gimp's op */
    "gegl:hstack", /* deleted from GEGL and replaced by gegl:pack */
    "gegl:introspect", /* pointless */
    "gegl:json:dropshadow2", /* has shortcomings, and duplicates gegl:dropshadow */
    "gegl:json:grey2", /* has shortcomings, and duplicates gegl:gray */
    "gegl:layer", /* we use gimp's ops */
    "gegl:lcms-from-profile", /* not usable here */
    "gegl:linear-gradient", /* we use the blend tool */
    "gegl:map-absolute", /* pointless */
    "gegl:map-relative", /* pointless */
    "gegl:matting-global", /* used in the foreground select tool */
    "gegl:matting-levin", /* used in the foreground select tool */
    "gegl:opacity", /* poinless */
    "gegl:pack", /* pointless */
    "gegl:path",
    "gegl:posterize", /* we use gimp's op */
    "gegl:radial-gradient", /* we use the blend tool */
    "gegl:rectangle", /* pointless */
    "gegl:seamless-clone", /* used in the seamless clone tool */
    "gegl:text", /* we use gimp's text rendering */
    "gegl:threshold", /* we use gimp's op */
    "gegl:tile", /* pointless */
    "gegl:unpremul", /* pointless */
    "gegl:vector-stroke",
    "gegl:wavelet-blur", /* we use gimp's op wavelet-decompose */
  };

  gchar **categories;
  gint    i;

  /* Operations with no name are abstract base classes */
  if (! name)
    return TRUE;

  /* use this flag to include all ops for testing */
  if (g_getenv ("GIMP_TESTING_NO_GEGL_BLACKLIST"))
    return FALSE;

  if (block_gimp_ops && g_str_has_prefix (name, "gimp"))
    return TRUE;

  for (i = 0; i < G_N_ELEMENTS (name_blacklist); i++)
    {
      if (! strcmp (name, name_blacklist[i]))
        return TRUE;
    }

  if (! categories_str)
    return FALSE;

  categories = g_strsplit (categories_str, ":", 0);

  for (i = 0; i < G_N_ELEMENTS (category_blacklist); i++)
    {
      gint j;

      for (j = 0; categories[j]; j++)
        if (! strcmp (categories[j], category_blacklist[i]))
          {
            g_strfreev (categories);
            return TRUE;
          }
    }

  g_strfreev (categories);

  return FALSE;
}

/* Builds a GList of the class structures of all subtypes of type.
 */
static GList *
gimp_gegl_get_op_subclasses (GType     type,
                             GList    *classes,
                             gboolean  block_gimp_ops)
{
  GeglOperationClass *klass;
  GType              *ops;
  const gchar        *categories;
  guint               n_ops;
  gint                i;

  if (! type)
    return classes;

  klass = GEGL_OPERATION_CLASS (g_type_class_ref (type));
  ops = g_type_children (type, &n_ops);

  categories = gegl_operation_class_get_key (klass, "categories");

  if (! gimp_gegl_op_blacklisted (klass->name, categories, block_gimp_ops))
    classes = g_list_prepend (classes, klass);

  for (i = 0; i < n_ops; i++)
    classes = gimp_gegl_get_op_subclasses (ops[i], classes, block_gimp_ops);

  if (ops)
    g_free (ops);

  return classes;
}

static gint
gimp_gegl_compare_op_names (GeglOperationClass *a,
                            GeglOperationClass *b)
{
  const gchar *name_a = gegl_operation_class_get_key (a, "title");
  const gchar *name_b = gegl_operation_class_get_key (b, "title");

  if (! name_a) name_a = a->name;
  if (! name_b) name_b = b->name;

  return strcmp (name_a, name_b);
}
