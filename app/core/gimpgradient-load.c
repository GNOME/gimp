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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimpgradient.h"
#include "gimpgradient-load.h"

#include "gimp-intl.h"


GimpData *
gimp_gradient_load (const gchar  *filename,
                    gboolean      stingy_memory_use,
                    GError      **error)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpGradientSegment *prev;
  gint                 num_segments;
  gint                 i;
  gint                 type, color;
  FILE                *file;
  gchar                line[1024];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = fopen (filename, "rb");
  if (!file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  fgets (line, 1024, file);
  if (strcmp (line, "GIMP Gradient\n") != 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "Not a GIMP gradient file."),
                   gimp_filename_to_utf8 (filename));
      fclose (file);
      return NULL;
    }

  gradient = g_object_new (GIMP_TYPE_GRADIENT, NULL);

  fgets (line, 1024, file);
  if (! strncmp (line, "Name: ", strlen ("Name: ")))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (&line[strlen ("Name: ")], -1,
                               _("Invalid UTF-8 string in gradient file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_strstrip (utf8);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);

      fgets (line, 1024, file);
    }
  else /* old gradient format */
    {
      gchar *basename;
      gchar *utf8;

      basename = g_path_get_basename (filename);

      utf8 = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
      g_free (basename);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);
    }

  num_segments = atoi (line);

  if (num_segments < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "File is corrupt."),
                   gimp_filename_to_utf8 (filename));
      g_object_unref (gradient);
      fclose (file);
      return NULL;
    }

  prev = NULL;

  for (i = 0; i < num_segments; i++)
    {
      gchar *end;

      seg = gimp_gradient_segment_new ();

      seg->prev = prev;

      if (prev)
	prev->next = seg;
      else
	gradient->segments = seg;

      fgets (line, 1024, file);

      seg->left = g_ascii_strtod (line, &end);
      if (end && errno != ERANGE)
        seg->middle = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->left_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.a = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->right_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.a = g_ascii_strtod (end, &end);

      if (errno != ERANGE &&
          sscanf (end, "%d %d", &type, &color) == 2)
        {
	  seg->type  = (GimpGradientSegmentType) type;
	  seg->color = (GimpGradientSegmentColor) color;
        }
      else
        {
	  g_message (_("Corrupt segment %d in gradient file '%s'."),
		     i, gimp_filename_to_utf8 (filename));
	}

      prev = seg;
    }

  fclose (file);

  return GIMP_DATA (gradient);
}


/*  SVG gradient parser  */

typedef enum
{
  SVG_STATE_OUT,  /* not inside an <svg />  */
  SVG_STATE_IN,   /* inside an <svg />      */
  SVG_STATE_DONE  /* finished, don't care about the rest of the file
                   *   (this is stupid, we should continue parsing
                   *    and return multiple gradients if there are any)
                   */
} SvgParserState;

typedef struct
{
  SvgParserState  state;
  GimpGradient   *gradient;
  GList          *stops;
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

static SvgStop * svg_parse_gradient_stop  (const gchar         **names,
                                           const gchar         **values);


static const GMarkupParser markup_parser =
{
  svg_parser_start_element,
  svg_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


GimpData *
gimp_gradient_load_svg (const gchar  *filename,
                        gboolean      stingy_memory_use,
                        GError      **error)
{
  GimpData      *data = NULL;
  GimpXmlParser *xml_parser;
  SvgParser      parser;
  gboolean       success;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  parser.state    = SVG_STATE_OUT;
  parser.gradient = NULL;
  parser.stops    = NULL;

  xml_parser = gimp_xml_parser_new (&markup_parser, &parser);

  success = gimp_xml_parser_parse_file (xml_parser, filename, error);

  gimp_xml_parser_free (xml_parser);

  if (success && parser.state == SVG_STATE_DONE)
    {
      g_return_val_if_fail (parser.gradient != NULL, FALSE);

      data = g_object_ref (parser.gradient);

      /*  FIXME: this will be overwritten by GimpDataFactory  */
      data->writable = FALSE;
    }
  else if (success)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("No gradients found in '%s'"),
                   gimp_filename_to_utf8 (filename));
    }
  else if (error && *error) /*  parser reported an error  */
    {
      gchar *msg = (*error)->message;

      (*error)->message =
        g_strdup_printf (_("Failed to import gradient from '%s': %s"),
                         gimp_filename_to_utf8 (filename), msg);

      g_free (msg);
    }

  if (parser.gradient)
    g_object_unref (parser.gradient);

  g_list_foreach (parser.stops, (GFunc) g_free, NULL);
  g_list_free (parser.stops);

  return data;
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

  switch (parser->state)
    {
    case SVG_STATE_OUT:
      if (strcmp (element_name, "svg") == 0)
        parser->state = SVG_STATE_IN;
      break;

    case SVG_STATE_IN:
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
                                           "name", name,
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
      break;

    case SVG_STATE_DONE:
      break;
    }
}

static void
svg_parser_end_element (GMarkupParseContext  *context,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error)
{
  SvgParser *parser = user_data;

  if (parser->state != SVG_STATE_IN)
    return;

  if (strcmp (element_name, "svg") == 0)
    {
      parser->state = SVG_STATE_OUT;
    }
  else if (parser->gradient && strcmp (element_name, "linearGradient") == 0)
    {
      GimpGradientSegment *seg  = gimp_gradient_segment_new ();
      GimpGradientSegment *next = NULL;
      SvgStop             *stop = parser->stops->data;
      GList               *list;

      seg->left_color  = stop->color;
      seg->right_color = stop->color;

      /*  the list of offsets is sorted from largest to smallest  */
      for (list = g_list_next (parser->stops); list; list = g_list_next (list))
        {
          seg->left   = stop->offset;
          seg->middle = (seg->left + seg->right) / 2.0;

          next = seg;
          seg  = gimp_gradient_segment_new ();

          seg->next  = next;
          next->prev = seg;

          seg->right       = stop->offset;
          seg->right_color = stop->color;

          stop = list->data;

          seg->left_color = stop->color;
        }

      seg->middle = (seg->left + seg->right) / 2.0;

      parser->gradient->segments = seg;

      parser->state = SVG_STATE_DONE;
    }
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
          gchar *name  = g_strndup (style, sep - style);
          gchar *value = g_strndup (++sep, end - sep - (*end == ';' ? 1 : 0));

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
  SvgStop *stop = g_new0 (SvgStop, 1);

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
