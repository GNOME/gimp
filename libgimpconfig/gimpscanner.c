/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscanner.c
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpconfig-error.h"
#include "gimpscanner.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpscanner
 * @title: GimpScanner
 * @short_description: A wrapper around #GScanner with some convenience API.
 *
 * A wrapper around #GScanner with some convenience API.
 **/


typedef struct
{
  gint          ref_count;
  gchar        *name;
  GMappedFile  *mapped;
  gchar        *text;
  GError      **error;
} GimpScannerData;


G_DEFINE_BOXED_TYPE (GimpScanner, gimp_scanner,
                     gimp_scanner_ref, gimp_scanner_unref)



/*  local function prototypes  */

static GimpScanner * gimp_scanner_new                    (const gchar  *name,
                                                          GMappedFile  *mapped,
                                                          gchar        *text,
                                                          GError      **error);
static void          gimp_scanner_message                (GimpScanner  *scanner,
                                                          gchar        *message,
                                                          gboolean      is_error);
static GTokenType    gimp_scanner_parse_deprecated_color (GimpScanner  *scanner,
                                                          GeglColor   **color);


/*  public functions  */

/**
 * gimp_scanner_new_file:
 * @file: a #GFile
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #GimpScanner.
 *
 * Since: 2.10
 **/
GimpScanner *
gimp_scanner_new_file (GFile   *file,
                       GError **error)
{
  GimpScanner *scanner;
  gchar       *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (path)
    {
      GMappedFile *mapped;

      mapped = g_mapped_file_new (path, FALSE, error);
      g_free (path);

      if (! mapped)
        {
          if (error)
            {
              (*error)->domain = GIMP_CONFIG_ERROR;
              (*error)->code   = ((*error)->code == G_FILE_ERROR_NOENT ?
                                  GIMP_CONFIG_ERROR_OPEN_ENOENT :
                                  GIMP_CONFIG_ERROR_OPEN);
            }

          return NULL;
        }

      /*  gimp_scanner_new() takes a "name" for the scanner, not a filename  */
      scanner = gimp_scanner_new (gimp_file_get_utf8_name (file),
                                  mapped, NULL, error);

      g_scanner_input_text (scanner,
                            g_mapped_file_get_contents (mapped),
                            g_mapped_file_get_length (mapped));
    }
  else
    {
      GInputStream *input;

      input = G_INPUT_STREAM (g_file_read (file, NULL, error));

      if (! input)
        {
          if (error)
            {
              (*error)->domain = GIMP_CONFIG_ERROR;
              (*error)->code   = ((*error)->code == G_IO_ERROR_NOT_FOUND ?
                                  GIMP_CONFIG_ERROR_OPEN_ENOENT :
                                  GIMP_CONFIG_ERROR_OPEN);
            }

          return NULL;
        }

      g_object_set_data (G_OBJECT (input), "gimp-data", file);

      scanner = gimp_scanner_new_stream (input, error);

      g_object_unref (input);
    }

  return scanner;
}

/**
 * gimp_scanner_new_stream:
 * @input: a #GInputStream
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #GimpScanner.
 *
 * Since: 2.10
 **/
GimpScanner *
gimp_scanner_new_stream (GInputStream  *input,
                         GError       **error)
{
  GimpScanner *scanner;
  GFile       *file;
  const gchar *path;
  GString     *string;
  gchar        buffer[4096];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_object_get_data (G_OBJECT (input), "gimp-file");
  if (file)
    path = gimp_file_get_utf8_name (file);
  else
    path = "stream";

  string = g_string_new (NULL);

  do
    {
      GError   *my_error = NULL;
      gboolean  success;

      success = g_input_stream_read_all (input, buffer, sizeof (buffer),
                                         &bytes_read, NULL, &my_error);

      if (bytes_read > 0)
        g_string_append_len (string, buffer, bytes_read);

      if (! success)
        {
          if (string->len > 0)
            {
              g_printerr ("%s: read error in '%s', trying to scan "
                          "partial content: %s",
                          G_STRFUNC, path, my_error->message);
              g_clear_error (&my_error);
              break;
            }

          g_string_free (string, TRUE);

          g_propagate_error (error, my_error);

          return NULL;
        }
    }
  while (bytes_read == sizeof (buffer));

  /*  gimp_scanner_new() takes a "name" for the scanner, not a filename  */
  scanner = gimp_scanner_new (path, NULL, string->str, error);

  bytes_read = string->len;

  g_scanner_input_text (scanner, g_string_free (string, FALSE), bytes_read);

  return scanner;
}

/**
 * gimp_scanner_new_string:
 * @text: (array length=text_len):
 * @text_len: The length of @text, or -1 if NULL-terminated
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #GimpScanner.
 *
 * Since: 2.4
 **/
GimpScanner *
gimp_scanner_new_string (const gchar  *text,
                         gint          text_len,
                         GError      **error)
{
  GimpScanner *scanner;

  g_return_val_if_fail (text != NULL || text_len <= 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (text_len < 0)
    text_len = text ? strlen (text) : 0;

  scanner = gimp_scanner_new (NULL, NULL, NULL, error);

  g_scanner_input_text (scanner, text, text_len);

  return scanner;
}

static GimpScanner *
gimp_scanner_new (const gchar  *name,
                  GMappedFile  *mapped,
                  gchar        *text,
                  GError      **error)
{
  GimpScanner     *scanner;
  GimpScannerData *data;

  scanner = g_scanner_new (NULL);

  data = g_slice_new0 (GimpScannerData);

  data->ref_count = 1;
  data->name      = g_strdup (name);
  data->mapped    = mapped;
  data->text      = text;
  data->error     = error;

  scanner->user_data   = data;
  scanner->msg_handler = gimp_scanner_message;

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z G_CSET_A_2_Z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z G_CSET_A_2_Z
                                             G_CSET_DIGITS "-_" );
  scanner->config->scan_identifier_1char = TRUE;

  scanner->config->store_int64           = TRUE;

  return scanner;
}

/**
 * gimp_scanner_ref:
 * @scanner: #GimpScanner to ref
 *
 * Adds a reference to a #GimpScanner.
 *
 * Returns: the same @scanner.
 *
 * Since: 3.0
 */
GimpScanner *
gimp_scanner_ref (GimpScanner *scanner)
{
  GimpScannerData *data;

  g_return_val_if_fail (scanner != NULL, NULL);

  data = scanner->user_data;

  data->ref_count++;

  return scanner;
}

/**
 * gimp_scanner_unref:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 *
 * Unref a #GimpScanner. If the reference count drops to zero, the
 * scanner is freed.
 *
 * Since: 3.0
 **/
void
gimp_scanner_unref (GimpScanner *scanner)
{
  GimpScannerData *data;

  g_return_if_fail (scanner != NULL);

  data = scanner->user_data;

  data->ref_count--;

  if (data->ref_count < 1)
    {
      if (data->mapped)
        g_mapped_file_unref (data->mapped);

      if (data->text)
        g_free (data->text);

      g_free (data->name);
      g_slice_free (GimpScannerData, data);

      g_scanner_destroy (scanner);
    }
}

/**
 * gimp_scanner_parse_token:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @token: the #GTokenType expected as next token.
 *
 * Returns: %TRUE if the next token is @token, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_token (GimpScanner *scanner,
                          GTokenType   token)
{
  if (g_scanner_peek_next_token (scanner) != token)
    return FALSE;

  g_scanner_get_next_token (scanner);

  return TRUE;
}

/**
 * gimp_scanner_parse_identifier:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @identifier: (out): the expected identifier.
 *
 * Returns: %TRUE if the next token is an identifier and if its
 * value matches @identifier.
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_identifier (GimpScanner *scanner,
                               const gchar *identifier)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (strcmp (scanner->value.v_identifier, identifier))
    return FALSE;

  return TRUE;
}

/**
 * gimp_scanner_parse_string:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed string
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_string (GimpScanner  *scanner,
                           gchar       **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    {
      if (! g_utf8_validate (scanner->value.v_string, -1, NULL))
        {
          g_scanner_warn (scanner, _("invalid UTF-8 string"));
          return FALSE;
        }

      *dest = g_strdup (scanner->value.v_string);
    }
  else
    {
      *dest = NULL;
    }

  return TRUE;
}

/**
 * gimp_scanner_parse_string_no_validate:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed string
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_string_no_validate (GimpScanner  *scanner,
                                       gchar       **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    *dest = g_strdup (scanner->value.v_string);
  else
    *dest = NULL;

  return TRUE;
}

/**
 * gimp_scanner_parse_data:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @length: Length of the data to parse
 * @dest: (out) (array length=length): Return location for the parsed data
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_data (GimpScanner  *scanner,
                         gint          length,
                         guint8      **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (scanner->value.v_string)
    *dest = g_memdup2 (scanner->value.v_string, length);
  else
    *dest = NULL;

  return TRUE;
}

/**
 * gimp_scanner_parse_int:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed integer
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_int (GimpScanner *scanner,
                        gint        *dest)
{
  gboolean negate = FALSE;

  if (g_scanner_peek_next_token (scanner) == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (negate)
    *dest = -scanner->value.v_int64;
  else
    *dest = scanner->value.v_int64;

  return TRUE;
}

/**
 * gimp_scanner_parse_int64:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed integer
 *
 * Returns: %TRUE on success
 *
 * Since: 2.8
 **/
gboolean
gimp_scanner_parse_int64 (GimpScanner *scanner,
                          gint64      *dest)
{
  gboolean negate = FALSE;

  if (g_scanner_peek_next_token (scanner) == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (negate)
    *dest = -scanner->value.v_int64;
  else
    *dest = scanner->value.v_int64;

  return TRUE;
}

/**
 * gimp_scanner_parse_double:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed double
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_double (GimpScanner *scanner,
                           gdouble     *dest)
{
  gboolean negate = FALSE;

  if (g_scanner_peek_next_token (scanner) == '-')
    {
      negate = TRUE;
      g_scanner_get_next_token (scanner);
    }

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
    {
      g_scanner_get_next_token (scanner);

      if (negate)
        *dest = -scanner->value.v_float;
      else
        *dest = scanner->value.v_float;

      return TRUE;
    }
  else if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
    {
      /* v_int is unsigned so we need to cast to
       *
       * gint64 first for negative values.
       */

      g_scanner_get_next_token (scanner);

      if (negate)
        *dest = - (gint64) scanner->value.v_int;
      else
        *dest = scanner->value.v_int;

      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_scanner_parse_boolean:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out): Return location for the parsed boolean
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_boolean (GimpScanner *scanner,
                            gboolean    *dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (! g_ascii_strcasecmp (scanner->value.v_identifier, "yes") ||
      ! g_ascii_strcasecmp (scanner->value.v_identifier, "true"))
    {
      *dest = TRUE;
    }
  else if (! g_ascii_strcasecmp (scanner->value.v_identifier, "no") ||
           ! g_ascii_strcasecmp (scanner->value.v_identifier, "false"))
    {
      *dest = FALSE;
    }
  else
    {
      g_scanner_error
        (scanner,
         /* please don't translate 'yes' and 'no' */
         _("expected 'yes' or 'no' for boolean token, got '%s'"),
         scanner->value.v_identifier);

      return FALSE;
    }

  return TRUE;
}

enum
{
  COLOR_RGB  = 1,
  COLOR_RGBA,
  COLOR_HSV,
  COLOR_HSVA,
  COLOR
};

/**
 * gimp_scanner_parse_color:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @color: (out callee-allocates): Pointer to a color to store the result
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_color (GimpScanner  *scanner,
                          GeglColor   **color)
{
  guint      scope_id;
  guint      old_scope_id;
  GTokenType token;
  gboolean   success = TRUE;

  scope_id = g_quark_from_static_string ("gimp_scanner_parse_color");
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  if (! g_scanner_scope_lookup_symbol (scanner, scope_id, "color"))
    {
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color", GINT_TO_POINTER (COLOR));
      /* Deprecated. Kept for backward compatibility. */
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-rgb", GINT_TO_POINTER (COLOR_RGB));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-rgba", GINT_TO_POINTER (COLOR_RGBA));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-hsv", GINT_TO_POINTER (COLOR_HSV));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-hsva", GINT_TO_POINTER (COLOR_HSVA));
    }

  token = g_scanner_peek_next_token (scanner);

  if (token == G_TOKEN_IDENTIFIER)
    {
      g_scanner_get_next_token (scanner);

      if (g_ascii_strcasecmp (scanner->value.v_identifier, "null") != 0)
        /* Do not fail the whole color parsing. Just output to stderr and assume
         * a NULL color property.
         */
        g_printerr ("%s: expected NULL identifier for serialized color, got '%s'. "
                    "Assuming NULL instead.\n",
                    G_STRFUNC, scanner->value.v_identifier);

      *color = NULL;

      token = g_scanner_peek_next_token (scanner);
      if (token == G_TOKEN_RIGHT_PAREN)
        token = G_TOKEN_NONE;
      else
        token = G_TOKEN_RIGHT_PAREN;
    }
  else if (token == G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      token = g_scanner_peek_next_token (scanner);

      if (token == G_TOKEN_SYMBOL)
        {
          if (GPOINTER_TO_INT (scanner->next_value.v_symbol) != COLOR)
            {
              /* Support historical GimpRGB format which may be stored in various config
               * files, but even some data (such as GTP tool presets which contains
               * tool-options which are GimpContext).
               */
              if (gimp_scanner_parse_deprecated_color (scanner, color))
                token = G_TOKEN_RIGHT_PAREN;
              else
                success = FALSE;
            }
          else
            {
              const Babl *format;
              gchar      *encoding;
              guint8     *data;
              gint        data_length;
              gint        profile_data_length;

              g_scanner_get_next_token (scanner);

              if (! gimp_scanner_parse_string (scanner, &encoding))
                {
                  token = G_TOKEN_STRING;
                  goto color_parsed;
                }

              if (! babl_format_exists (encoding))
                {
                  g_scanner_error (scanner,
                                   "%s: format \"%s\" for serialized color is not a valid babl format.",
                                   G_STRFUNC, encoding);
                  g_free (encoding);
                  success = FALSE;
                  goto color_parsed;
                }

              format = babl_format (encoding);
              g_free (encoding);

              if (! gimp_scanner_parse_int (scanner, &data_length))
                {
                  token = G_TOKEN_INT;
                  goto color_parsed;
                }

              if (data_length != babl_format_get_bytes_per_pixel (format))
                {
                  g_scanner_error (scanner,
                                   "%s: format \"%s\" expects %d bpp but color was serialized with %d bpp.",
                                   G_STRFUNC, babl_get_name (format),
                                   babl_format_get_bytes_per_pixel (format),
                                   data_length);
                  success = FALSE;
                  goto color_parsed;
                }

              if (! gimp_scanner_parse_data (scanner, data_length, &data))
                {
                  token = G_TOKEN_STRING;
                  goto color_parsed;
                }

              if (! gimp_scanner_parse_int (scanner, &profile_data_length))
                {
                  g_free (data);
                  token = G_TOKEN_INT;
                  goto color_parsed;
                }

              if (profile_data_length > 0)
                {
                  const Babl       *space = NULL;
                  GimpColorProfile *profile;
                  guint8           *profile_data;
                  GError           *error = NULL;

                  if (! gimp_scanner_parse_data (scanner, profile_data_length, &profile_data))
                    {
                      g_free (data);
                      token = G_TOKEN_STRING;
                      goto color_parsed;
                    }

                  profile = gimp_color_profile_new_from_icc_profile (profile_data, profile_data_length, &error);

                  if (profile)
                    {
                      space = gimp_color_profile_get_space (profile,
                                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                            &error);

                      if (! space)
                        {
                          g_scanner_error (scanner,
                                           "%s: failed to create Babl space for serialized color from profile: %s\n",
                                           G_STRFUNC, error->message);
                          g_clear_error (&error);
                        }
                      g_object_unref (profile);
                    }
                  else
                    {
                      g_scanner_error (scanner,
                                       "%s: invalid profile data for serialized color: %s",
                                       G_STRFUNC, error->message);
                      g_error_free (error);
                    }
                  format = babl_format_with_space (babl_format_get_encoding (format), space);

                  g_free (profile_data);
                }

              *color = gegl_color_new (NULL);
              gegl_color_set_pixel (*color, format, data);

              token = G_TOKEN_RIGHT_PAREN;
              g_free (data);
            }
        }
      else
        {
          token = G_TOKEN_SYMBOL;
        }

      if (success && token == G_TOKEN_RIGHT_PAREN)
        {
          token = g_scanner_peek_next_token (scanner);
          if (token == G_TOKEN_RIGHT_PAREN)
            {
              g_scanner_get_next_token (scanner);
              token = G_TOKEN_NONE;
            }
          else
            {
               g_clear_object (color);
               token = G_TOKEN_RIGHT_PAREN;
            }
        }
    }
  else
    {
      token = G_TOKEN_LEFT_PAREN;
    }

color_parsed:

  if (success && token != G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  g_scanner_set_scope (scanner, old_scope_id);

  return (success && token == G_TOKEN_NONE);
}

/**
 * gimp_scanner_parse_matrix2:
 * @scanner: A #GimpScanner created by gimp_scanner_new_file() or
 *           gimp_scanner_new_string()
 * @dest: (out caller-allocates): Pointer to a matrix to store the result
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
gimp_scanner_parse_matrix2 (GimpScanner *scanner,
                            GimpMatrix2 *dest)
{
  guint        scope_id;
  guint        old_scope_id;
  GTokenType   token;
  GimpMatrix2  matrix;

  scope_id = g_quark_from_static_string ("gimp_scanner_parse_matrix");
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  if (! g_scanner_scope_lookup_symbol (scanner, scope_id, "matrix"))
    g_scanner_scope_add_symbol (scanner, scope_id,
                                "matrix", GINT_TO_POINTER (0));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          {
            token = G_TOKEN_FLOAT;

            if (! gimp_scanner_parse_double (scanner, &matrix.coeff[0][0]))
              goto finish;
            if (! gimp_scanner_parse_double (scanner, &matrix.coeff[0][1]))
              goto finish;
            if (! gimp_scanner_parse_double (scanner, &matrix.coeff[1][0]))
              goto finish;
            if (! gimp_scanner_parse_double (scanner, &matrix.coeff[1][1]))
              goto finish;

            token = G_TOKEN_RIGHT_PAREN;
          }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_NONE; /* indicates success */
          goto finish;

        default: /* do nothing */
          break;
        }
    }

 finish:

  if (token != G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }
  else
    {
      *dest = matrix;
    }

  g_scanner_set_scope (scanner, old_scope_id);

  return (token == G_TOKEN_NONE);
}


/*  private functions  */

static void
gimp_scanner_message (GimpScanner *scanner,
                      gchar       *message,
                      gboolean     is_error)
{
  GimpScannerData *data = scanner->user_data;

  /* we don't expect warnings */
  g_return_if_fail (is_error);

  if (data->name)
    g_set_error (data->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                 _("Error while parsing '%s' in line %d: %s"),
                 data->name, scanner->line, message);
  else
    /*  should never happen, thus not marked for translation  */
    g_set_error (data->error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                 "Error parsing internal buffer: %s", message);
}

static GTokenType
gimp_scanner_parse_deprecated_color (GimpScanner  *scanner,
                                     GeglColor   **color)
{
  guint      scope_id;
  guint      old_scope_id;
  GTokenType token;

  scope_id = g_quark_from_static_string ("gimp_scanner_parse_deprecated_color");
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  if (! g_scanner_scope_lookup_symbol (scanner, scope_id, "color-rgb"))
    {
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-rgb", GINT_TO_POINTER (COLOR_RGB));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-rgba", GINT_TO_POINTER (COLOR_RGBA));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-hsv", GINT_TO_POINTER (COLOR_HSV));
      g_scanner_scope_add_symbol (scanner, scope_id,
                                  "color-hsva", GINT_TO_POINTER (COLOR_HSVA));
    }

  token = G_TOKEN_SYMBOL;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_SYMBOL:
          {
            gdouble  col[4]     = { 0.0, 0.0, 0.0, 1.0 };
            gfloat   col_f[4]   = { 0.0f, 0.0f, 0.0f, 1.0f };
            gint     n_channels = 4;
            gboolean is_hsv     = FALSE;
            gint     i;

            switch (GPOINTER_TO_INT (scanner->value.v_symbol))
              {
              case COLOR_RGB:
                n_channels = 3;
                /* fallthrough */
              case COLOR_RGBA:
                break;

              case COLOR_HSV:
                n_channels = 3;
                /* fallthrough */
              case COLOR_HSVA:
                is_hsv = TRUE;
                break;
              }

            token = G_TOKEN_FLOAT;

            for (i = 0; i < n_channels; i++)
              {
                if (! gimp_scanner_parse_double (scanner, &col[i]))
                  goto finish;

                col_f[i] = (gfloat) col[i];
              }

            *color = gegl_color_new (NULL);
            if (is_hsv)
              gegl_color_set_pixel (*color, babl_format ("HSVA float"), col_f);
            else
              gegl_color_set_pixel (*color, babl_format ("R'G'B'A double"), col);

            /* Indicates success. */
            token = G_TOKEN_NONE;
          }
          break;

        default: /* do nothing */
          break;
        }
    }

finish:

  g_scanner_set_scope (scanner, old_scope_id);

  return token;
}
