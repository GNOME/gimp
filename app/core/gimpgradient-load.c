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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimp-utils.h"
#include "gimpgradient.h"
#include "gimpgradient-load.h"

#include "gimp-intl.h"


GList *
gimp_gradient_load (GimpContext   *context,
                    GFile         *file,
                    GInputStream  *input,
                    GError       **error)
{
  GimpGradient        *gradient = NULL;
  GimpGradientSegment *prev;
  gint                 num_segments;
  gint                 i;
  GDataInputStream    *data_input;
  gchar               *line;
  gsize                line_len;
  gint                 linenum;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data_input = g_data_input_stream_new (input);

  linenum = 1;
  line_len = 1024;
  line = gimp_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    goto failed;

  if (! g_str_has_prefix (line, "GIMP Gradient"))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Not a GIMP gradient file."));
      g_free (line);
      goto failed;
    }

  g_free (line);

  gradient = g_object_new (GIMP_TYPE_GRADIENT,
                           "mime-type", "application/x-gimp-gradient",
                           NULL);

  linenum++;
  line_len = 1024;
  line = gimp_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    goto failed;

  if (g_str_has_prefix (line, "Name: "))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (g_strstrip (line + strlen ("Name: ")), -1,
                               _("Invalid UTF-8 string in gradient file '%s'."),
                               gimp_file_get_utf8_name (file));
      gimp_object_take_name (GIMP_OBJECT (gradient), utf8);

      g_free (line);

      linenum++;
      line_len = 1024;
      line = gimp_data_input_stream_read_line_always (data_input, &line_len,
                                                      NULL, error);
      if (! line)
        goto failed;
    }
  else /* old gradient format */
    {
      gimp_object_take_name (GIMP_OBJECT (gradient),
                             g_path_get_basename (gimp_file_get_utf8_name (file)));
    }

  num_segments = atoi (line);

  g_free (line);

  if (num_segments < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("File is corrupt."));
      goto failed;
    }

  prev = NULL;

  for (i = 0; i < num_segments; i++)
    {
      GimpGradientSegment *seg;
      gchar               *end;
      gint                 color;
      gint                 type;
      gint                 left_color_type;
      gint                 right_color_type;

      seg = gimp_gradient_segment_new ();

      seg->prev = prev;

      if (prev)
        prev->next = seg;
      else
        gradient->segments = seg;

      linenum++;
      line_len = 1024;
      line = gimp_data_input_stream_read_line_always (data_input, &line_len,
                                                      NULL, error);
      if (! line)
        goto failed;

      if (! gimp_ascii_strtod (line, &end, &seg->left)          ||
          ! gimp_ascii_strtod (end,  &end, &seg->middle)        ||
          ! gimp_ascii_strtod (end,  &end, &seg->right)         ||

          ! gimp_ascii_strtod (end,  &end, &seg->left_color.r)  ||
          ! gimp_ascii_strtod (end,  &end, &seg->left_color.g)  ||
          ! gimp_ascii_strtod (end,  &end, &seg->left_color.b)  ||
          ! gimp_ascii_strtod (end,  &end, &seg->left_color.a)  ||

          ! gimp_ascii_strtod (end,  &end, &seg->right_color.r) ||
          ! gimp_ascii_strtod (end,  &end, &seg->right_color.g) ||
          ! gimp_ascii_strtod (end,  &end, &seg->right_color.b) ||
          ! gimp_ascii_strtod (end,  &end, &seg->right_color.a))
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Corrupt segment %d."), i);
          g_free (line);
          goto failed;
        }

      switch (sscanf (end, "%d %d %d %d",
                      &type, &color,
                      &left_color_type, &right_color_type))
        {
        case 4:
          seg->left_color_type  = (GimpGradientColor) left_color_type;
          if (seg->left_color_type < GIMP_GRADIENT_COLOR_FIXED ||
              seg->left_color_type > GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Corrupt segment %d."), i);
              g_free (line);
              goto failed;
            }

          seg->right_color_type = (GimpGradientColor) right_color_type;
          if (seg->right_color_type < GIMP_GRADIENT_COLOR_FIXED ||
              seg->right_color_type > GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Corrupt segment %d."), i);
              g_free (line);
              goto failed;
            }
          /* fall thru */

        case 2:
          seg->type  = (GimpGradientSegmentType) type;
          if (seg->type < GIMP_GRADIENT_SEGMENT_LINEAR ||
              seg->type > GIMP_GRADIENT_SEGMENT_STEP)
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Corrupt segment %d."), i);
              g_free (line);
              goto failed;
            }

          seg->color = (GimpGradientSegmentColor) color;
          if (seg->color < GIMP_GRADIENT_SEGMENT_RGB ||
              seg->color > GIMP_GRADIENT_SEGMENT_HSV_CW)
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Corrupt segment %d."), i);
              g_free (line);
              goto failed;
            }
          break;

        default:
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Corrupt segment %d."), i);
          g_free (line);
          goto failed;
        }

      g_free (line);

      if (seg->left   > seg->middle              ||
          seg->middle > seg->right               ||
          (  prev && (prev->right != seg->left)) ||
          (! prev && (0.0         != seg->left)))
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Segments do not span the range 0-1."));
          goto failed;
        }

      prev = seg;
    }

  if (prev->right != 1.0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Segments do not span the range 0-1."));
      goto failed;
    }

  g_object_unref (data_input);

  return g_list_prepend (NULL, gradient);

 failed:

  g_object_unref (data_input);

  if (gradient)
    g_object_unref (gradient);

  g_prefix_error (error, _("In line %d of gradient file: "), linenum);

  return NULL;
}


/*  SVG gradient parser  */

typedef struct
{
  GimpGradient *gradient;  /*  current gradient    */
  GList        *gradients; /*  finished gradients  */
  GList        *stops;
} SvgParser;

typedef struct
{
  gdouble       offset;
  GimpRGB       color;
} SvgStop;


static void     svg_parser_start_element  (GMarkupParseContext  *context,
                                           const gchar          *element_name,
                                           const gchar         **attribute_names,
                                           const gchar         **attribute_values,
                                           gpointer              user_data,
                                           GError              **error);
static void      svg_parser_end_element   (GMarkupParseContext  *context,
                                           const gchar          *element_name,
                                           gpointer              user_data,
                                           GError              **error);

static GimpGradientSegment *
                 svg_parser_gradient_segments   (GList          *stops);

static SvgStop * svg_parse_gradient_stop        (const gchar   **names,
                                                 const gchar   **values);


static const GMarkupParser markup_parser =
{
  svg_parser_start_element,
  svg_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


GList *
gimp_gradient_load_svg (GimpContext   *context,
                        GFile         *file,
                        GInputStream  *input,
                        GError       **error)
{
  GimpXmlParser *xml_parser;
  SvgParser      parser = { NULL, };
  gboolean       success;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* FIXME input */
  g_input_stream_close (input, NULL, NULL);

  xml_parser = gimp_xml_parser_new (&markup_parser, &parser);

  success = gimp_xml_parser_parse_gfile (xml_parser, file, error);

  gimp_xml_parser_free (xml_parser);

  if (success && ! parser.gradients)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("No linear gradients found."));
    }

  if (parser.gradient)
    g_object_unref (parser.gradient);

  if (parser.stops)
    {
      GList *list;

      for (list = parser.stops; list; list = list->next)
        g_slice_free (SvgStop, list->data);

      g_list_free (parser.stops);
    }

  return g_list_reverse (parser.gradients);
}

static void
svg_parser_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **attribute_names,
                          const gchar         **attribute_values,
                          gpointer              user_data,
                          GError              **error)
{
  SvgParser *parser = user_data;

  if (! parser->gradient && strcmp (element_name, "linearGradient") == 0)
    {
      const gchar *name = NULL;

      while (*attribute_names && *attribute_values)
        {
          if (strcmp (*attribute_names, "id") == 0 && *attribute_values)
            name = *attribute_values;

          attribute_names++;
          attribute_values++;
        }

      parser->gradient = g_object_new (GIMP_TYPE_GRADIENT,
                                       "name",      name,
                                       "mime-type", "image/svg+xml",
                                       NULL);
    }
  else if (parser->gradient && strcmp (element_name, "stop") == 0)
    {
      SvgStop *stop = svg_parse_gradient_stop (attribute_names,
                                               attribute_values);

      /*  The spec clearly states that each gradient stop's offset
       *  value is required to be equal to or greater than the
       *  previous gradient stop's offset value.
       */
      if (parser->stops)
        stop->offset = MAX (stop->offset,
                            ((SvgStop *) parser->stops->data)->offset);

      parser->stops = g_list_prepend (parser->stops, stop);
    }
}

static void
svg_parser_end_element (GMarkupParseContext  *context,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error)
{
  SvgParser *parser = user_data;

  if (parser->gradient &&
      strcmp (element_name, "linearGradient") == 0)
    {
      GList *list;

      parser->gradient->segments = svg_parser_gradient_segments (parser->stops);

      for (list = parser->stops; list; list = list->next)
        g_slice_free (SvgStop, list->data);

      g_list_free (parser->stops);
      parser->stops = NULL;

      if (parser->gradient->segments)
        parser->gradients = g_list_prepend (parser->gradients,
                                            parser->gradient);
      else
        g_object_unref (parser->gradient);

      parser->gradient = NULL;
    }
}

static GimpGradientSegment *
svg_parser_gradient_segments (GList *stops)
{
  GimpGradientSegment *segment;
  SvgStop             *stop;
  GList               *list;

  if (! stops)
    return NULL;

  stop = stops->data;

  segment = gimp_gradient_segment_new ();

  segment->left_color  = stop->color;
  segment->right_color = stop->color;

  /*  the list of offsets is sorted from largest to smallest  */
  for (list = g_list_next (stops); list; list = g_list_next (list))
    {
      GimpGradientSegment *next = segment;

      segment->left   = stop->offset;
      segment->middle = (segment->left + segment->right) / 2.0;

      segment = gimp_gradient_segment_new ();

      segment->next = next;
      next->prev    = segment;

      segment->right       = stop->offset;
      segment->right_color = stop->color;

      stop = list->data;

      segment->left_color  = stop->color;
    }

  segment->middle = (segment->left + segment->right) / 2.0;

  if (stop->offset > 0.0)
    segment->right_color = stop->color;

  /*  FIXME: remove empty segments here or add a GimpGradient API to do that
   */

  return segment;
}

static void
svg_parse_gradient_stop_style_prop (SvgStop     *stop,
                                    const gchar *name,
                                    const gchar *value)
{
  if (strcmp (name, "stop-color") == 0)
    {
      gimp_rgb_parse_css (&stop->color, value, -1);
    }
  else if (strcmp (name, "stop-opacity") == 0)
    {
      gdouble opacity = g_ascii_strtod (value, NULL);

      if (errno != ERANGE)
        gimp_rgb_set_alpha (&stop->color, CLAMP (opacity, 0.0, 1.0));
    }
}

/*  very simplistic CSS style parser  */
static void
svg_parse_gradient_stop_style (SvgStop     *stop,
                               const gchar *style)
{
  const gchar *end;
  const gchar *sep;

  while (*style)
    {
      while (g_ascii_isspace (*style))
        style++;

      for (end = style; *end && *end != ';'; end++)
        /* do nothing */;

      for (sep = style; sep < end && *sep != ':'; sep++)
        /* do nothing */;

      if (end > sep && sep > style)
        {
          gchar *name;
          gchar *value;

          name = g_strndup (style, sep - style);
          sep++;
          value = g_strndup (sep, end - sep - (*end == ';' ? 1 : 0));

          svg_parse_gradient_stop_style_prop (stop, name, value);

          g_free (value);
          g_free (name);
        }

      style = end;

      if (*style == ';')
        style++;
    }
}

static SvgStop *
svg_parse_gradient_stop (const gchar **names,
                         const gchar **values)
{
  SvgStop *stop = g_slice_new0 (SvgStop);

  gimp_rgb_set_alpha (&stop->color, 1.0);

  while (*names && *values)
    {
      if (strcmp (*names, "offset") == 0)
        {
          gchar *end;

          stop->offset = g_ascii_strtod (*values, &end);

          if (end && *end == '%')
            stop->offset /= 100.0;

          stop->offset = CLAMP (stop->offset, 0.0, 1.0);
        }
      else if (strcmp (*names, "style") == 0)
        {
          svg_parse_gradient_stop_style (stop, *values);
        }
      else
        {
          svg_parse_gradient_stop_style_prop (stop, *names, *values);
        }

      names++;
      values++;
    }

  return stop;
}
