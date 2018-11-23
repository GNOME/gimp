/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectors Import
 * Copyright (C) 2003-2004  Sven Neumann <sven@gimp.org>
 *
 * Some code here is based on code from librsvg that was originally
 * written by Raph Levien <raph@artofcode.com> for Gill.
 *
 * This SVG path importer implements a subset of SVG that is
 * sufficient to parse path elements and basic shapes and to apply
 * transformations as described by the SVG specification:
 * http://www.w3.org/TR/SVG/.  It must handle the SVG files exported
 * by GIMP but it is also supposed to be able to extract paths and
 * shapes from foreign SVG documents.
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
#include <errno.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "vectors-types.h"

#include "config/gimpxmlparser.h"

#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "gimpbezierstroke.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-import.h"

#include "gimp-intl.h"


#define COORDS_INIT   \
  {                   \
    .x         = 0.0, \
    .y         = 0.0, \
    .pressure  = 1.0, \
    .xtilt     = 0.0, \
    .ytilt     = 0.0, \
    .wheel     = 0.5, \
    .velocity  = 0.0, \
    .direction = 0.0  \
  }


typedef struct
{
  GQueue       *stack;
  GimpImage    *image;
  gboolean      scale;
  gint          svg_depth;
} SvgParser;


typedef struct _SvgHandler SvgHandler;

struct _SvgHandler
{
  const gchar  *name;

  void (* start) (SvgHandler   *handler,
                  const gchar **names,
                  const gchar **values,
                  SvgParser    *parser);
  void (* end)   (SvgHandler   *handler,
                  SvgParser    *parser);

  gdouble       width;
  gdouble       height;
  gchar        *id;
  GList        *paths;
  GimpMatrix3  *transform;
};


typedef struct
{
  gchar        *id;
  GList        *strokes;
} SvgPath;


static gboolean  gimp_vectors_import  (GimpImage            *image,
                                       GFile                *file,
                                       const gchar          *str,
                                       gsize                 len,
                                       gboolean              merge,
                                       gboolean              scale,
                                       GimpVectors          *parent,
                                       gint                  position,
                                       GList               **ret_vectors,
                                       GError              **error);

static void  svg_parser_start_element (GMarkupParseContext  *context,
                                       const gchar          *element_name,
                                       const gchar         **attribute_names,
                                       const gchar         **attribute_values,
                                       gpointer              user_data,
                                       GError              **error);
static void  svg_parser_end_element   (GMarkupParseContext  *context,
                                       const gchar          *element_name,
                                       gpointer              user_data,
                                       GError              **error);

static const GMarkupParser markup_parser =
{
  svg_parser_start_element,
  svg_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


static void  svg_handler_svg_start     (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_svg_end       (SvgHandler   *handler,
                                        SvgParser    *parser);
static void  svg_handler_group_start   (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_path_start    (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_rect_start    (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_ellipse_start (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_line_start    (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);
static void  svg_handler_poly_start    (SvgHandler   *handler,
                                        const gchar **names,
                                        const gchar **values,
                                        SvgParser    *parser);

static const SvgHandler svg_handlers[] =
{
  { "svg",      svg_handler_svg_start,     svg_handler_svg_end },
  { "g",        svg_handler_group_start,   NULL                },
  { "path",     svg_handler_path_start,    NULL                },
  { "rect",     svg_handler_rect_start,    NULL                },
  { "circle",   svg_handler_ellipse_start, NULL                },
  { "ellipse",  svg_handler_ellipse_start, NULL                },
  { "line",     svg_handler_line_start,    NULL                },
  { "polyline", svg_handler_poly_start,    NULL                },
  { "polygon",  svg_handler_poly_start,    NULL                }
};


static gboolean   parse_svg_length    (const gchar  *value,
                                       gdouble       reference,
                                       gdouble       resolution,
                                       gdouble      *length);
static gboolean   parse_svg_viewbox   (const gchar  *value,
                                       gdouble      *width,
                                       gdouble      *height,
                                       GimpMatrix3  *matrix);
static gboolean   parse_svg_transform (const gchar  *value,
                                       GimpMatrix3  *matrix);
static GList    * parse_path_data     (const gchar  *data);


/**
 * gimp_vectors_import_file:
 * @image:    the #GimpImage to add the paths to
 * @file:     a SVG file
 * @merge:    should multiple paths be merged into a single #GimpVectors object
 * @scale:    should the SVG be scaled to fit the image dimensions
 * @position: position in the image's vectors stack where to add the vectors
 * @error:    location to store possible errors
 *
 * Imports one or more paths and basic shapes from a SVG file.
 *
 * Return value: %TRUE on success, %FALSE if an error occurred
 **/
gboolean
gimp_vectors_import_file (GimpImage    *image,
                          GFile        *file,
                          gboolean      merge,
                          gboolean      scale,
                          GimpVectors  *parent,
                          gint          position,
                          GList       **ret_vectors,
                          GError      **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        GIMP_IS_VECTORS (parent), FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_item_is_attached (GIMP_ITEM (parent)), FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_item_get_image (GIMP_ITEM (parent)) == image,
                        FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_viewable_get_children (GIMP_VIEWABLE (parent)),
                        FALSE);
  g_return_val_if_fail (ret_vectors == NULL || *ret_vectors == NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return gimp_vectors_import (image, file, NULL, 0, merge, scale,
                              parent, position,
                              ret_vectors, error);
}

/**
 * gimp_vectors_import_string:
 * @image:  the #GimpImage to add the paths to
 * @buffer: a character buffer to parse
 * @len:    number of bytes in @str or -1 if @str is %NUL-terminated
 * @merge:  should multiple paths be merged into a single #GimpVectors object
 * @scale:  should the SVG be scaled to fit the image dimensions
 * @error:  location to store possible errors
 *
 * Imports one or more paths and basic shapes from a SVG file.
 *
 * Return value: %TRUE on success, %FALSE if an error occurred
 **/
gboolean
gimp_vectors_import_buffer (GimpImage    *image,
                            const gchar  *buffer,
                            gsize         len,
                            gboolean      merge,
                            gboolean      scale,
                            GimpVectors  *parent,
                            gint          position,
                            GList       **ret_vectors,
                            GError      **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (buffer != NULL || len == 0, FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        GIMP_IS_VECTORS (parent), FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_item_is_attached (GIMP_ITEM (parent)), FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_item_get_image (GIMP_ITEM (parent)) == image,
                        FALSE);
  g_return_val_if_fail (parent == NULL ||
                        parent == GIMP_IMAGE_ACTIVE_PARENT ||
                        gimp_viewable_get_children (GIMP_VIEWABLE (parent)),
                        FALSE);
  g_return_val_if_fail (ret_vectors == NULL || *ret_vectors == NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return gimp_vectors_import (image, NULL, buffer, len, merge, scale,
                              parent, position,
                              ret_vectors, error);
}

static gboolean
gimp_vectors_import (GimpImage    *image,
                     GFile        *file,
                     const gchar  *str,
                     gsize         len,
                     gboolean      merge,
                     gboolean      scale,
                     GimpVectors  *parent,
                     gint          position,
                     GList       **ret_vectors,
                     GError      **error)
{
  GimpXmlParser *xml_parser;
  SvgParser      parser;
  GList         *paths;
  SvgHandler    *base;
  gboolean       success = TRUE;

  parser.stack     = g_queue_new ();
  parser.image     = image;
  parser.scale     = scale;
  parser.svg_depth = 0;

  /*  the base of the stack, defines the size of the view-port  */
  base = g_slice_new0 (SvgHandler);
  base->name   = "image";
  base->width  = gimp_image_get_width  (image);
  base->height = gimp_image_get_height (image);

  g_queue_push_head (parser.stack, base);

  xml_parser = gimp_xml_parser_new (&markup_parser, &parser);

  if (file)
    success = gimp_xml_parser_parse_gfile (xml_parser, file, error);
  else
    success = gimp_xml_parser_parse_buffer (xml_parser, str, len, error);

  gimp_xml_parser_free (xml_parser);

  if (success)
    {
      if (base->paths)
        {
          GimpVectors *vectors = NULL;

          base->paths = g_list_reverse (base->paths);

          merge = merge && base->paths->next;

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_VECTORS_IMPORT,
                                       _("Import Paths"));

          for (paths = base->paths; paths; paths = paths->next)
            {
              SvgPath *path = paths->data;
              GList   *list;

              if (! merge || ! vectors)
                {
                  vectors = gimp_vectors_new (image,
                                              ((merge || ! path->id) ?
                                               _("Imported Path") : path->id));
                  gimp_image_add_vectors (image, vectors,
                                          parent, position, TRUE);
                  gimp_vectors_freeze (vectors);

                  if (ret_vectors)
                    *ret_vectors = g_list_prepend (*ret_vectors, vectors);

                  if (position != -1)
                    position++;
                }

              for (list = path->strokes; list; list = list->next)
                gimp_vectors_stroke_add (vectors, GIMP_STROKE (list->data));

              if (! merge)
                gimp_vectors_thaw (vectors);

              g_list_free_full (path->strokes, g_object_unref);
              path->strokes = NULL;
            }

          if (merge)
            gimp_vectors_thaw (vectors);

          gimp_image_undo_group_end (image);
        }
      else
        {
          if (file)
            g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                         _("No paths found in '%s'"),
                         gimp_file_get_utf8_name (file));
          else
            g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                 _("No paths found in the buffer"));

          success = FALSE;
        }
    }
  else if (error && *error && file) /*  parser reported an error  */
    {
      gchar *msg = (*error)->message;

      (*error)->message =
        g_strdup_printf (_("Failed to import paths from '%s': %s"),
                         gimp_file_get_utf8_name (file), msg);

      g_free (msg);
    }

  while ((base = g_queue_pop_head (parser.stack)) != NULL)
    {
      for (paths = base->paths; paths; paths = paths->next)
        {
          SvgPath *path = paths->data;
          GList   *list;

          g_free (path->id);

          for (list = path->strokes; list; list = list->next)
            g_object_unref (list->data);

          g_list_free (path->strokes);

          g_slice_free (SvgPath, path);
        }

      g_list_free (base->paths);

      g_slice_free (GimpMatrix3, base->transform);
      g_slice_free (SvgHandler, base);
    }

  g_queue_free (parser.stack);

  return success;
}

static void
svg_parser_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **attribute_names,
                          const gchar         **attribute_values,
                          gpointer              user_data,
                          GError              **error)
{
  SvgParser  *parser = user_data;
  SvgHandler *handler;
  SvgHandler *base;
  gint        i = 0;

  handler = g_slice_new0 (SvgHandler);
  base    = g_queue_peek_head (parser->stack);

  /* if the element is not rendered, always use the generic handler */
  if (base->width <= 0.0 || base->height <= 0.0)
    i = G_N_ELEMENTS (svg_handlers);

  for (; i < G_N_ELEMENTS (svg_handlers); i++)
    if (strcmp (svg_handlers[i].name, element_name) == 0)
      {
        handler->name  = svg_handlers[i].name;
        handler->start = svg_handlers[i].start;
        break;
      }

  handler->width  = base->width;
  handler->height = base->height;

  g_queue_push_head (parser->stack, handler);

  if (handler->start)
    handler->start (handler, attribute_names, attribute_values, parser);
}

static void
svg_parser_end_element (GMarkupParseContext  *context,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error)
{
  SvgParser  *parser = user_data;
  SvgHandler *handler;
  SvgHandler *base;
  GList      *paths;

  handler = g_queue_pop_head (parser->stack);

  g_return_if_fail (handler != NULL &&
                    (handler->name == NULL ||
                     strcmp (handler->name, element_name) == 0));

  if (handler->end)
    handler->end (handler, parser);

  if (handler->paths)
    {
      if (handler->transform)
        {
          for (paths = handler->paths; paths; paths = paths->next)
            {
              SvgPath *path = paths->data;
              GList   *list;

              for (list = path->strokes; list; list = list->next)
                gimp_stroke_transform (GIMP_STROKE (list->data),
                                       handler->transform, NULL);
            }

          g_slice_free (GimpMatrix3, handler->transform);
        }

      base = g_queue_peek_head (parser->stack);
      base->paths = g_list_concat (base->paths, handler->paths);
    }

  g_slice_free (SvgHandler, handler);
}

static void
svg_handler_svg_start (SvgHandler   *handler,
                       const gchar **names,
                       const gchar **values,
                       SvgParser    *parser)
{
  GimpMatrix3 *matrix;
  GimpMatrix3  box;
  const gchar *viewbox = NULL;
  gdouble      x = 0;
  gdouble      y = 0;
  gdouble      w = handler->width;
  gdouble      h = handler->height;
  gdouble      xres;
  gdouble      yres;

  matrix = g_slice_new (GimpMatrix3);
  gimp_matrix3_identity (matrix);

  gimp_image_get_resolution (parser->image, &xres, &yres);

  while (*names)
    {
      switch (*names[0])
        {
        case 'x':
          if (strcmp (*names, "x") == 0)
            parse_svg_length (*values, handler->width, xres, &x);
          break;

        case 'y':
          if (strcmp (*names, "y") == 0)
            parse_svg_length (*values, handler->height, yres, &y);
          break;

        case 'w':
          if (strcmp (*names, "width") == 0)
            parse_svg_length (*values, handler->width, xres, &w);
          break;

        case 'h':
          if (strcmp (*names, "height") == 0)
            parse_svg_length (*values, handler->height, yres, &h);
          break;

        case 'v':
          if (strcmp (*names, "viewBox") == 0)
            viewbox = *values;
          break;
        }

      names++;
      values++;
    }

  if (x || y)
    {
      /* according to the spec offsets are meaningless on the outermost svg */
      if (parser->svg_depth > 0)
        gimp_matrix3_translate (matrix, x, y);
    }

  if (viewbox && parse_svg_viewbox (viewbox, &w, &h, &box))
    {
      gimp_matrix3_mult (&box, matrix);
    }

  /*  optionally scale the outermost svg to image size  */
  if (parser->scale && parser->svg_depth == 0)
    {
      if (w > 0.0 && h > 0.0)
        gimp_matrix3_scale (matrix,
                            gimp_image_get_width  (parser->image) / w,
                            gimp_image_get_height (parser->image) / h);
    }

  handler->width  = w;
  handler->height = h;

  handler->transform = matrix;

  parser->svg_depth++;
}

static void
svg_handler_svg_end (SvgHandler   *handler,
                     SvgParser    *parser)
{
  parser->svg_depth--;
}

static void
svg_handler_group_start (SvgHandler   *handler,
                         const gchar **names,
                         const gchar **values,
                         SvgParser    *parser)
{
  while (*names)
    {
      if (strcmp (*names, "transform") == 0 && ! handler->transform)
        {
          GimpMatrix3  matrix;

          if (parse_svg_transform (*values, &matrix))
            {
              handler->transform = g_slice_dup (GimpMatrix3, &matrix);

#ifdef DEBUG_VECTORS_IMPORT
              g_printerr ("transform %s: %g %g %g   %g %g %g   %g %g %g\n",
                          handler->id ? handler->id : "(null)",
                          handler->transform->coeff[0][0],
                          handler->transform->coeff[0][1],
                          handler->transform->coeff[0][2],
                          handler->transform->coeff[1][0],
                          handler->transform->coeff[1][1],
                          handler->transform->coeff[1][2],
                          handler->transform->coeff[2][0],
                          handler->transform->coeff[2][1],
                          handler->transform->coeff[2][2]);
#endif
            }
        }

      names++;
      values++;
    }
}

static void
svg_handler_path_start (SvgHandler   *handler,
                        const gchar **names,
                        const gchar **values,
                        SvgParser    *parser)
{
  SvgPath *path = g_slice_new0 (SvgPath);

  while (*names)
    {
      switch (*names[0])
        {
        case 'i':
          if (strcmp (*names, "id") == 0 && ! path->id)
            path->id = g_strdup (*values);
          break;

        case 'd':
          if (strcmp (*names, "d") == 0 && ! path->strokes)
            path->strokes = parse_path_data (*values);
          break;

        case 't':
          if (strcmp (*names, "transform") == 0 && ! handler->transform)
            {
              GimpMatrix3  matrix;

              if (parse_svg_transform (*values, &matrix))
                {
                  handler->transform = g_slice_dup (GimpMatrix3, &matrix);
                }
            }
          break;
        }

      names++;
      values++;
    }

  handler->paths = g_list_prepend (handler->paths, path);
}

static void
svg_handler_rect_start (SvgHandler   *handler,
                        const gchar **names,
                        const gchar **values,
                        SvgParser    *parser)
{
  SvgPath *path   = g_slice_new0 (SvgPath);
  gdouble  x      = 0.0;
  gdouble  y      = 0.0;
  gdouble  width  = 0.0;
  gdouble  height = 0.0;
  gdouble  rx     = 0.0;
  gdouble  ry     = 0.0;
  gdouble  xres;
  gdouble  yres;

  gimp_image_get_resolution (parser->image, &xres, &yres);

  while (*names)
    {
      switch (*names[0])
        {
        case 'i':
          if (strcmp (*names, "id") == 0 && ! path->id)
            path->id = g_strdup (*values);
          break;

        case 'x':
          if (strcmp (*names, "x") == 0)
            parse_svg_length (*values, handler->width, xres, &x);
          break;

        case 'y':
          if (strcmp (*names, "y") == 0)
            parse_svg_length (*values, handler->height, yres, &y);
          break;

        case 'w':
          if (strcmp (*names, "width") == 0)
            parse_svg_length (*values, handler->width, xres, &width);
          break;

        case 'h':
          if (strcmp (*names, "height") == 0)
            parse_svg_length (*values, handler->height, yres, &height);
          break;

        case 'r':
          if (strcmp (*names, "rx") == 0)
            parse_svg_length (*values, handler->width, xres, &rx);
          else if (strcmp (*names, "ry") == 0)
            parse_svg_length (*values, handler->height, yres, &ry);
          break;

        case 't':
          if (strcmp (*names, "transform") == 0 && ! handler->transform)
            {
              GimpMatrix3  matrix;

              if (parse_svg_transform (*values, &matrix))
                {
                  handler->transform = g_slice_dup (GimpMatrix3, &matrix);
                }
            }
          break;
        }

      names++;
      values++;
    }

  if (width > 0.0 && height > 0.0 && rx >= 0.0 && ry >= 0.0)
    {
      GimpStroke *stroke;
      GimpCoords  point = COORDS_INIT;

      if (rx == 0.0)
        rx = ry;
      if (ry == 0.0)
        ry = rx;

      rx = MIN (rx, width / 2);
      ry = MIN (ry, height / 2);

      point.x = x + width - rx;
      point.y = y;
      stroke = gimp_bezier_stroke_new_moveto (&point);

      if (rx)
        {
          GimpCoords  end = COORDS_INIT;

          end.x = x + width;
          end.y = y + ry;

          gimp_bezier_stroke_arcto (stroke, rx, ry, 0, FALSE, TRUE, &end);
        }

      point.x = x + width;
      point.y = y + height - ry;
      gimp_bezier_stroke_lineto (stroke, &point);

      if (rx)
        {
          GimpCoords  end = COORDS_INIT;

          end.x = x + width - rx;
          end.y = y + height;

          gimp_bezier_stroke_arcto (stroke, rx, ry, 0, FALSE, TRUE, &end);
        }

      point.x = x + rx;
      point.y = y + height;
      gimp_bezier_stroke_lineto (stroke, &point);

      if (rx)
        {
          GimpCoords  end = COORDS_INIT;

          end.x = x;
          end.y = y + height - ry;

          gimp_bezier_stroke_arcto (stroke, rx, ry, 0, FALSE, TRUE, &end);
        }

      point.x = x;
      point.y = y + ry;
      gimp_bezier_stroke_lineto (stroke, &point);

      if (rx)
        {
          GimpCoords  end = COORDS_INIT;

          end.x = x + rx;
          end.y = y;

          gimp_bezier_stroke_arcto (stroke, rx, ry, 0, FALSE, TRUE, &end);
        }

      /* the last line is handled by closing the stroke */
      gimp_stroke_close (stroke);

      path->strokes = g_list_prepend (path->strokes, stroke);
    }

  handler->paths = g_list_prepend (handler->paths, path);
}

static void
svg_handler_ellipse_start (SvgHandler   *handler,
                           const gchar **names,
                           const gchar **values,
                           SvgParser    *parser)
{
  SvgPath    *path   = g_slice_new0 (SvgPath);
  GimpCoords  center = COORDS_INIT;
  gdouble     rx     = 0.0;
  gdouble     ry     = 0.0;
  gdouble     xres;
  gdouble     yres;

  gimp_image_get_resolution (parser->image, &xres, &yres);

  while (*names)
    {
      switch (*names[0])
        {
        case 'i':
          if (strcmp (*names, "id") == 0 && ! path->id)
            path->id = g_strdup (*values);
          break;

        case 'c':
          if (strcmp (*names, "cx") == 0)
            parse_svg_length (*values, handler->width, xres, &center.x);
          else if (strcmp (*names, "cy") == 0)
            parse_svg_length (*values, handler->height, yres, &center.y);
          break;

        case 'r':
          if (strcmp (*names, "r") == 0)
            {
              parse_svg_length (*values, handler->width,  xres, &rx);
              parse_svg_length (*values, handler->height, yres, &ry);
            }
          else if (strcmp (*names, "rx") == 0)
            {
              parse_svg_length (*values, handler->width, xres, &rx);
            }
          else if (strcmp (*names, "ry") == 0)
            {
              parse_svg_length (*values, handler->height, yres, &ry);
            }
          break;

        case 't':
          if (strcmp (*names, "transform") == 0 && ! handler->transform)
            {
              GimpMatrix3  matrix;

              if (parse_svg_transform (*values, &matrix))
                {
                  handler->transform = g_slice_dup (GimpMatrix3, &matrix);
                }
            }
          break;
        }

      names++;
      values++;
    }

  if (rx >= 0.0 && ry >= 0.0)
    path->strokes = g_list_prepend (path->strokes,
                                    gimp_bezier_stroke_new_ellipse (&center,
                                                                    rx, ry,
                                                                    0.0));

  handler->paths = g_list_prepend (handler->paths, path);
}

static void
svg_handler_line_start (SvgHandler   *handler,
                        const gchar **names,
                        const gchar **values,
                        SvgParser    *parser)
{
  SvgPath    *path  = g_slice_new0 (SvgPath);
  GimpCoords  start = COORDS_INIT;
  GimpCoords  end   = COORDS_INIT;
  GimpStroke *stroke;
  gdouble     xres;
  gdouble     yres;

  gimp_image_get_resolution (parser->image, &xres, &yres);

  while (*names)
    {
      switch (*names[0])
        {
        case 'i':
          if (strcmp (*names, "id") == 0 && ! path->id)
            path->id = g_strdup (*values);
          break;

        case 'x':
          if (strcmp (*names, "x1") == 0)
            parse_svg_length (*values, handler->width, xres, &start.x);
          else if (strcmp (*names, "x2") == 0)
            parse_svg_length (*values, handler->width, xres, &end.x);
          break;

        case 'y':
          if (strcmp (*names, "y1") == 0)
            parse_svg_length (*values, handler->height, yres, &start.y);
          else if (strcmp (*names, "y2") == 0)
            parse_svg_length (*values, handler->height, yres, &end.y);
          break;

        case 't':
          if (strcmp (*names, "transform") == 0 && ! handler->transform)
            {
              GimpMatrix3  matrix;

              if (parse_svg_transform (*values, &matrix))
                {
                  handler->transform = g_slice_dup (GimpMatrix3, &matrix);
                }
            }
          break;
        }

      names++;
      values++;
    }

  stroke = gimp_bezier_stroke_new_moveto (&start);
  gimp_bezier_stroke_lineto (stroke, &end);

  path->strokes = g_list_prepend (path->strokes, stroke);

  handler->paths = g_list_prepend (handler->paths, path);
}

static void
svg_handler_poly_start (SvgHandler   *handler,
                        const gchar **names,
                        const gchar **values,
                        SvgParser    *parser)
{
  SvgPath *path   = g_slice_new0 (SvgPath);
  GString *points = NULL;

  while (*names)
    {
      switch (*names[0])
        {
        case 'i':
          if (strcmp (*names, "id") == 0 && ! path->id)
            path->id = g_strdup (*values);
          break;

        case 'p':
          if (strcmp (*names, "points") == 0 && ! points)
            {
              const gchar *p = *values;
              const gchar *m = NULL;
              const gchar *l = NULL;
              gint         n = 0;

              while (*p)
                {
                  while (g_ascii_isspace (*p) || *p == ',')
                    p++;

                  switch (n)
                    {
                    case 0:
                      m = p;
                      break;
                    case 2:
                      l = p;
                      break;
                    }

                  if (*p)
                    n++;

                  while (*p && ! g_ascii_isspace (*p) && *p != ',')
                    p++;
                }

              if ((n > 3) && (n % 2 == 0))
                {
                  points = g_string_sized_new (p - *values + 8);

                  g_string_append_len (points, "M ", 2);
                  g_string_append_len (points, m, l - m);

                  g_string_append_len (points, "L ", 2);
                  g_string_append_len (points, l, p - l);

                  if (strcmp (handler->name, "polygon") == 0)
                    g_string_append_c (points, 'Z');
                }
            }
          break;

        case 't':
          if (strcmp (*names, "transform") == 0 && ! handler->transform)
            {
              GimpMatrix3  matrix;

              if (parse_svg_transform (*values, &matrix))
                {
                  handler->transform = g_slice_dup (GimpMatrix3, &matrix);
                }
            }
          break;
        }

      names++;
      values++;
    }

  if (points)
    {
      path->strokes = parse_path_data (points->str);
      g_string_free (points, TRUE);
    }

  handler->paths = g_list_prepend (handler->paths, path);
}

static gboolean
parse_svg_length (const gchar *value,
                  gdouble      reference,
                  gdouble      resolution,
                  gdouble     *length)
{
  GimpUnit  unit = GIMP_UNIT_PIXEL;
  gdouble   len;
  gchar    *ptr;

  len = g_ascii_strtod (value, &ptr);

  while (g_ascii_isspace (*ptr))
    ptr++;

  switch (ptr[0])
    {
    case '\0':
      break;

    case 'p':
      switch (ptr[1])
        {
        case 'x':                         break;
        case 't': unit = GIMP_UNIT_POINT; break;
        case 'c': unit = GIMP_UNIT_PICA;  break;
        default:
          return FALSE;
        }
      ptr += 2;
      break;

    case 'c':
      if (ptr[1] == 'm')
        len *= 10.0, unit = GIMP_UNIT_MM;
      else
        return FALSE;
      ptr += 2;
      break;

    case 'm':
      if (ptr[1] == 'm')
        unit = GIMP_UNIT_MM;
      else
        return FALSE;
      ptr += 2;
      break;

    case 'i':
      if (ptr[1] == 'n')
        unit = GIMP_UNIT_INCH;
      else
        return FALSE;
      ptr += 2;
      break;

    case '%':
      unit = GIMP_UNIT_PERCENT;
      ptr += 1;
      break;

    default:
      return FALSE;
    }

  while (g_ascii_isspace (*ptr))
    ptr++;

  if (*ptr)
    return FALSE;

  switch (unit)
    {
    case GIMP_UNIT_PERCENT:
      *length = len * reference / 100.0;
      break;

    case GIMP_UNIT_PIXEL:
      *length = len;
      break;

    default:
      *length = len * resolution / gimp_unit_get_factor (unit);
      break;
    }

  return TRUE;
}

static gboolean
parse_svg_viewbox (const gchar *value,
                   gdouble     *width,
                   gdouble     *height,
                   GimpMatrix3 *matrix)
{
  gdouble   x, y, w, h;
  gchar    *tok;
  gchar    *str     = g_strdup (value);
  gboolean  success = FALSE;

  x = y = w = h = 0;

  tok = strtok (str, ", \t");
  if (tok)
    {
      x = g_ascii_strtod (tok, NULL);
      tok = strtok (NULL, ", \t");
      if (tok)
        {
          y = g_ascii_strtod (tok, NULL);
          tok = strtok (NULL, ", \t");
          if (tok != NULL)
            {
              w = g_ascii_strtod (tok, NULL);
              tok = strtok (NULL, ", \t");
              if (tok)
                {
                  h = g_ascii_strtod (tok, NULL);
                  success = TRUE;
                }
            }
        }
    }

  g_free (str);

  if (success)
    {
      gimp_matrix3_identity (matrix);
      gimp_matrix3_translate (matrix, -x,  -y);

      if (w > 0.0 && h > 0.0)
        {
          gimp_matrix3_scale (matrix, *width / w, *height / h);
        }
      else  /* disable rendering of the element */
        {
#ifdef DEBUG_VECTORS_IMPORT
          g_printerr ("empty viewBox");
#endif
          *width = *height = 0.0;
        }
    }
  else
    {
      g_printerr ("SVG import: cannot parse viewBox attribute\n");
    }

  return success;
}

static gboolean
parse_svg_transform (const gchar *value,
                     GimpMatrix3 *matrix)
{
  gint i;

  gimp_matrix3_identity (matrix);

  for (i = 0; value[i]; i++)
    {
      GimpMatrix3  trafo;
      gchar        keyword[32];
      gdouble      args[6];
      gint         n_args;
      gint         key_len;

      gimp_matrix3_identity (&trafo);

      /* skip initial whitespace */
      while (g_ascii_isspace (value[i]))
        i++;

      /* parse keyword */
      for (key_len = 0; key_len < sizeof (keyword); key_len++)
        {
          gchar c = value[i];

          if (g_ascii_isalpha (c) || c == '-')
            keyword[key_len] = value[i++];
          else
            break;
        }

      if (key_len >= sizeof (keyword))
        return FALSE;

      keyword[key_len] = '\0';

      /* skip whitespace */
      while (g_ascii_isspace (value[i]))
        i++;

      if (value[i] != '(')
        return FALSE;
      i++;

      for (n_args = 0; ; n_args++)
        {
          gchar  c;
          gchar *end_ptr;

          /* skip whitespace */
          while (g_ascii_isspace (value[i]))
            i++;

          c = value[i];
          if (g_ascii_isdigit (c) || c == '+' || c == '-' || c == '.')
            {
              if (n_args == G_N_ELEMENTS (args))
                return FALSE; /* too many args */

              args[n_args] = g_ascii_strtod (value + i, &end_ptr);
              i = end_ptr - value;

              while (g_ascii_isspace (value[i]))
                i++;

              /* skip optional comma */
              if (value[i] == ',')
                i++;
            }
          else if (c == ')')
            break;
          else
            return FALSE;
        }

      /* OK, have parsed keyword and args, now calculate the transform matrix */

      if (strcmp (keyword, "matrix") == 0)
        {
          if (n_args != 6)
            return FALSE;

          gimp_matrix3_affine (&trafo,
                               args[0], args[1],
                               args[2], args[3],
                               args[4], args[5]);
        }
      else if (strcmp (keyword, "translate") == 0)
        {
          if (n_args == 1)
            args[1] = 0.0;
          else if (n_args != 2)
            return FALSE;

          gimp_matrix3_translate (&trafo, args[0], args[1]);
        }
      else if (strcmp (keyword, "scale") == 0)
        {
          if (n_args == 1)
            args[1] = args[0];
          else if (n_args != 2)
            return FALSE;

          gimp_matrix3_scale (&trafo, args[0], args[1]);
        }
      else if (strcmp (keyword, "rotate") == 0)
        {
          if (n_args == 1)
            {
              gimp_matrix3_rotate (&trafo, gimp_deg_to_rad (args[0]));
            }
          else if (n_args == 3)
            {
              gimp_matrix3_translate (&trafo, -args[1], -args[2]);
              gimp_matrix3_rotate (&trafo, gimp_deg_to_rad (args[0]));
              gimp_matrix3_translate (&trafo, args[1], args[2]);
            }
          else
            return FALSE;
        }
      else if (strcmp (keyword, "skewX") == 0)
        {
          if (n_args != 1)
            return FALSE;

          gimp_matrix3_xshear (&trafo, tan (gimp_deg_to_rad (args[0])));
        }
      else if (strcmp (keyword, "skewY") == 0)
        {
          if (n_args != 1)
            return FALSE;

          gimp_matrix3_yshear (&trafo, tan (gimp_deg_to_rad (args[0])));
        }
      else
        {
          return FALSE; /* unknown keyword */
        }

      gimp_matrix3_invert (&trafo);
      gimp_matrix3_mult (&trafo, matrix);
    }

  gimp_matrix3_invert (matrix);

  return TRUE;
}


/**********************************************************/
/*  Below is the code that parses the actual path data.   */
/*                                                        */
/*  This code is taken from librsvg and was originally    */
/*  written by Raph Levien <raph@artofcode.com> for Gill. */
/**********************************************************/

typedef struct
{
  GList       *strokes;
  GimpStroke  *stroke;
  gdouble      cpx, cpy;  /* current point                               */
  gdouble      rpx, rpy;  /* reflection point (for 's' and 't' commands) */
  gchar        cmd;       /* current command (lowercase)                 */
  gint         param;     /* number of parameters                        */
  gboolean     rel;       /* true if relative coords                     */
  gdouble      params[7]; /* parameters that have been parsed            */
} ParsePathContext;


static void  parse_path_default_xy (ParsePathContext *ctx,
                                    gint              n_params);
static void  parse_path_do_cmd     (ParsePathContext *ctx,
                                    gboolean          final);


static GList *
parse_path_data (const gchar *data)
{
  ParsePathContext ctx;

  gboolean  in_num        = FALSE;
  gboolean  in_frac       = FALSE;
  gboolean  in_exp        = FALSE;
  gboolean  exp_wait_sign = FALSE;
  gdouble   val           = 0.0;
  gchar     c             = 0;
  gint      sign          = 0;
  gint      exp           = 0;
  gint      exp_sign      = 0;
  gdouble   frac          = 0.0;
  gint      i;

  memset (&ctx, 0, sizeof (ParsePathContext));

  for (i = 0; ; i++)
    {
      c = data[i];

      if (c >= '0' && c <= '9')
        {
          /* digit */
          if (in_num)
            {
              if (in_exp)
                {
                  exp = (exp * 10) + c - '0';
                  exp_wait_sign = FALSE;
                }
              else if (in_frac)
                val += (frac *= 0.1) * (c - '0');
              else
                val = (val * 10) + c - '0';
            }
          else
            {
              in_num = TRUE;
              in_frac = FALSE;
              in_exp = FALSE;
              exp = 0;
              exp_sign = 1;
              exp_wait_sign = FALSE;
              val = c - '0';
              sign = 1;
            }
        }
      else if (c == '.')
        {
          if (! in_num)
            {
              in_num = TRUE;
              val = 0;
            }

          in_frac = TRUE;
          frac = 1;
        }
      else if ((c == 'E' || c == 'e') && in_num)
        {
          in_exp = TRUE;
          exp_wait_sign = TRUE;
          exp = 0;
          exp_sign = 1;
        }
      else if ((c == '+' || c == '-') && in_exp)
        {
          exp_sign = c == '+' ? 1 : -1;
        }
      else if (in_num)
        {
          /* end of number */

          val *= sign * pow (10, exp_sign * exp);

          if (ctx.rel)
            {
              /* Handle relative coordinates. This switch statement attempts
                 to determine _what_ the coords are relative to. This is
                 underspecified in the 12 Apr working draft. */
              switch (ctx.cmd)
                {
                case 'l':
                case 'm':
                case 'c':
                case 's':
                case 'q':
                case 't':
                  /* rule: even-numbered params are x-relative, odd-numbered
                     are y-relative */
                  if ((ctx.param & 1) == 0)
                    val += ctx.cpx;
                  else if ((ctx.param & 1) == 1)
                    val += ctx.cpy;
                  break;

                case 'a':
                  /* rule: sixth and seventh are x and y, rest are not
                     relative */
                  if (ctx.param == 5)
                    val += ctx.cpx;
                  else if (ctx.param == 6)
                    val += ctx.cpy;
                  break;

                case 'h':
                  /* rule: x-relative */
                  val += ctx.cpx;
                  break;

                case 'v':
                  /* rule: y-relative */
                  val += ctx.cpy;
                  break;
                }
            }

          ctx.params[ctx.param++] = val;
          parse_path_do_cmd (&ctx, FALSE);
          in_num = FALSE;
        }

      if (c == '\0')
        {
          break;
        }
      else if ((c == '+' || c == '-') && ! exp_wait_sign)
        {
          sign = c == '+' ? 1 : -1;
          val = 0;
          in_num = TRUE;
          in_frac = FALSE;
          in_exp = FALSE;
          exp = 0;
          exp_sign = 1;
          exp_wait_sign = FALSE;
        }
      else if (c == 'z' || c == 'Z')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);

          if (ctx.stroke)
            gimp_stroke_close (ctx.stroke);
        }
      else if (c >= 'A' && c <= 'Z' && c != 'E')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);

          ctx.cmd = c + 'a' - 'A';
          ctx.rel = FALSE;
        }
      else if (c >= 'a' && c <= 'z' && c != 'e')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);

          ctx.cmd = c;
          ctx.rel = TRUE;
        }
      /* else c _should_ be whitespace or , */
    }

  return g_list_reverse (ctx.strokes);
}

/* supply defaults for missing parameters, assuming relative coordinates
   are to be interpreted as x,y */
static void
parse_path_default_xy (ParsePathContext *ctx,
                       gint              n_params)
{
  gint i;

  if (ctx->rel)
    {
      for (i = ctx->param; i < n_params; i++)
        {
          if (i > 2)
            ctx->params[i] = ctx->params[i - 2];
          else if (i == 1)
            ctx->params[i] = ctx->cpy;
          else if (i == 0)
            /* we shouldn't get here (ctx->param > 0 as precondition) */
            ctx->params[i] = ctx->cpx;
        }
    }
  else
    {
      for (i = ctx->param; i < n_params; i++)
        ctx->params[i] = 0.0;
    }
}

static void
parse_path_do_cmd (ParsePathContext *ctx,
                   gboolean          final)
{
  GimpCoords coords = COORDS_INIT;

  switch (ctx->cmd)
    {
    case 'm':
      /* moveto */
      if (ctx->param == 2 || final)
        {
          parse_path_default_xy (ctx, 2);

          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

          ctx->stroke = gimp_bezier_stroke_new_moveto (&coords);
          ctx->strokes = g_list_prepend (ctx->strokes, ctx->stroke);

          ctx->param = 0;

          /* If a moveto is followed by multiple pairs of coordinates,
           * the subsequent pairs are treated as implicit lineto commands.
           */
          ctx->cmd = 'l';
        }
      break;

    case 'l':
      /* lineto */
      if (ctx->param == 2 || final)
        {
          parse_path_default_xy (ctx, 2);

          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

          ctx->param = 0;
        }
      break;

    case 'c':
      /* curveto */
      if (ctx->param == 6 || final)
        {
          GimpCoords ctrl1 = COORDS_INIT;
          GimpCoords ctrl2 = COORDS_INIT;

          parse_path_default_xy (ctx, 6);

          ctrl1.x  = ctx->params[0];
          ctrl1.y  = ctx->params[1];
          ctrl2.x  = ctx->rpx = ctx->params[2];
          ctrl2.y  = ctx->rpy = ctx->params[3];
          coords.x = ctx->cpx = ctx->params[4];
          coords.y = ctx->cpy = ctx->params[5];

          gimp_bezier_stroke_cubicto (ctx->stroke, &ctrl1, &ctrl2, &coords);

          ctx->param = 0;
        }
      break;

    case 's':
      /* smooth curveto */
      if (ctx->param == 4 || final)
        {
          GimpCoords ctrl1 = COORDS_INIT;
          GimpCoords ctrl2 = COORDS_INIT;

          parse_path_default_xy (ctx, 4);

          ctrl1.x  = 2 * ctx->cpx - ctx->rpx;
          ctrl1.y  = 2 * ctx->cpy - ctx->rpy;
          ctrl2.x  = ctx->rpx = ctx->params[0];
          ctrl2.y  = ctx->rpy = ctx->params[1];
          coords.x = ctx->cpx = ctx->params[2];
          coords.y = ctx->cpy = ctx->params[3];

          gimp_bezier_stroke_cubicto (ctx->stroke, &ctrl1, &ctrl2, &coords);

          ctx->param = 0;
        }
      break;

    case 'h':
      /* horizontal lineto */
      if (ctx->param == 1)
        {
          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy;

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

          ctx->param = 0;
        }
      break;

    case 'v':
      /* vertical lineto */
      if (ctx->param == 1)
        {
          coords.x = ctx->cpx;
          coords.y = ctx->cpy = ctx->rpy = ctx->params[0];

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

          ctx->param = 0;
        }
      break;

    case 'q':
      /* quadratic bezier curveto */
      if (ctx->param == 4 || final)
        {
          GimpCoords ctrl = COORDS_INIT;

          parse_path_default_xy (ctx, 4);

          ctrl.x   = ctx->rpx = ctx->params[0];
          ctrl.y   = ctx->rpy = ctx->params[1];
          coords.x = ctx->cpx = ctx->params[2];
          coords.y = ctx->cpy = ctx->params[3];

          gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);

          ctx->param = 0;
        }
      break;

    case 't':
      /* truetype quadratic bezier curveto */
      if (ctx->param == 2 || final)
        {
          GimpCoords ctrl = COORDS_INIT;

          parse_path_default_xy (ctx, 2);

          ctrl.x   = ctx->rpx = 2 * ctx->cpx - ctx->rpx;
          ctrl.y   = ctx->rpy = 2 * ctx->cpy - ctx->rpy;
          coords.x = ctx->cpx = ctx->params[0];
          coords.y = ctx->cpy = ctx->params[1];

          gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);

          ctx->param = 0;
        }
      else if (final)
        {
          if (ctx->param > 2)
            {
              GimpCoords ctrl = COORDS_INIT;

              parse_path_default_xy (ctx, 4);

              ctrl.x   = ctx->rpx = ctx->params[0];
              ctrl.y   = ctx->rpy = ctx->params[1];
              coords.x = ctx->cpx = ctx->params[2];
              coords.y = ctx->cpy = ctx->params[3];

              gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);
            }
          else
            {
              parse_path_default_xy (ctx, 2);

              coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
              coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

              gimp_bezier_stroke_lineto (ctx->stroke, &coords);
            }

          ctx->param = 0;
        }
      break;

    case 'a':
      if (ctx->param == 7 || final)
        {
          coords.x = ctx->cpx = ctx->rpx = ctx->params[5];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[6];

          gimp_bezier_stroke_arcto (ctx->stroke,
                                    ctx->params[0], ctx->params[1],
                                    gimp_deg_to_rad (ctx->params[2]),
                                    ctx->params[3], ctx->params[4],
                                    &coords);
          ctx->param = 0;
        }
      break;

    default:
      ctx->param = 0;
      break;
    }
}
