/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmascanner.c
 * Copyright (C) 2002  Sven Neumann <sven@ligma.org>
 *                     Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "ligmaconfig-error.h"
#include "ligmascanner.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmascanner
 * @title: LigmaScanner
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
} LigmaScannerData;


G_DEFINE_BOXED_TYPE (LigmaScanner, ligma_scanner,
                     ligma_scanner_ref, ligma_scanner_unref)



/*  local function prototypes  */

static LigmaScanner * ligma_scanner_new     (const gchar  *name,
                                           GMappedFile  *mapped,
                                           gchar        *text,
                                           GError      **error);
static void          ligma_scanner_message (LigmaScanner  *scanner,
                                           gchar        *message,
                                           gboolean      is_error);


/*  public functions  */

/**
 * ligma_scanner_new_file:
 * @file: a #GFile
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #LigmaScanner.
 *
 * Since: 2.10
 **/
LigmaScanner *
ligma_scanner_new_file (GFile   *file,
                       GError **error)
{
  LigmaScanner *scanner;
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
              (*error)->domain = LIGMA_CONFIG_ERROR;
              (*error)->code   = ((*error)->code == G_FILE_ERROR_NOENT ?
                                  LIGMA_CONFIG_ERROR_OPEN_ENOENT :
                                  LIGMA_CONFIG_ERROR_OPEN);
            }

          return NULL;
        }

      /*  ligma_scanner_new() takes a "name" for the scanner, not a filename  */
      scanner = ligma_scanner_new (ligma_file_get_utf8_name (file),
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
              (*error)->domain = LIGMA_CONFIG_ERROR;
              (*error)->code   = ((*error)->code == G_IO_ERROR_NOT_FOUND ?
                                  LIGMA_CONFIG_ERROR_OPEN_ENOENT :
                                  LIGMA_CONFIG_ERROR_OPEN);
            }

          return NULL;
        }

      g_object_set_data (G_OBJECT (input), "ligma-data", file);

      scanner = ligma_scanner_new_stream (input, error);

      g_object_unref (input);
    }

  return scanner;
}

/**
 * ligma_scanner_new_stream:
 * @input: a #GInputStream
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #LigmaScanner.
 *
 * Since: 2.10
 **/
LigmaScanner *
ligma_scanner_new_stream (GInputStream  *input,
                         GError       **error)
{
  LigmaScanner *scanner;
  GFile       *file;
  const gchar *path;
  GString     *string;
  gchar        buffer[4096];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_object_get_data (G_OBJECT (input), "ligma-file");
  if (file)
    path = ligma_file_get_utf8_name (file);
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

  /*  ligma_scanner_new() takes a "name" for the scanner, not a filename  */
  scanner = ligma_scanner_new (path, NULL, string->str, error);

  bytes_read = string->len;

  g_scanner_input_text (scanner, g_string_free (string, FALSE), bytes_read);

  return scanner;
}

/**
 * ligma_scanner_new_string:
 * @text: (array length=text_len):
 * @text_len: The length of @text, or -1 if NULL-terminated
 * @error: return location for #GError, or %NULL
 *
 * Returns: (transfer full): The new #LigmaScanner.
 *
 * Since: 2.4
 **/
LigmaScanner *
ligma_scanner_new_string (const gchar  *text,
                         gint          text_len,
                         GError      **error)
{
  LigmaScanner *scanner;

  g_return_val_if_fail (text != NULL || text_len <= 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (text_len < 0)
    text_len = text ? strlen (text) : 0;

  scanner = ligma_scanner_new (NULL, NULL, NULL, error);

  g_scanner_input_text (scanner, text, text_len);

  return scanner;
}

static LigmaScanner *
ligma_scanner_new (const gchar  *name,
                  GMappedFile  *mapped,
                  gchar        *text,
                  GError      **error)
{
  LigmaScanner     *scanner;
  LigmaScannerData *data;

  scanner = g_scanner_new (NULL);

  data = g_slice_new0 (LigmaScannerData);

  data->ref_count = 1;
  data->name      = g_strdup (name);
  data->mapped    = mapped;
  data->text      = text;
  data->error     = error;

  scanner->user_data   = data;
  scanner->msg_handler = ligma_scanner_message;

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z G_CSET_A_2_Z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z G_CSET_A_2_Z
                                             G_CSET_DIGITS "-_" );
  scanner->config->scan_identifier_1char = TRUE;

  scanner->config->store_int64           = TRUE;

  return scanner;
}

/**
 * ligma_scanner_ref:
 * @scanner: #LigmaScanner to ref
 *
 * Adds a reference to a #LigmaScanner.
 *
 * Returns: the same @scanner.
 *
 * Since: 3.0
 */
LigmaScanner *
ligma_scanner_ref (LigmaScanner *scanner)
{
  LigmaScannerData *data;

  g_return_val_if_fail (scanner != NULL, NULL);

  data = scanner->user_data;

  data->ref_count++;

  return scanner;
}

/**
 * ligma_scanner_unref:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 *
 * Unref a #LigmaScanner. If the reference count drops to zero, the
 * scanner is freed.
 *
 * Since: 3.0
 **/
void
ligma_scanner_unref (LigmaScanner *scanner)
{
  LigmaScannerData *data;

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
      g_slice_free (LigmaScannerData, data);

      g_scanner_destroy (scanner);
    }
}

/**
 * ligma_scanner_parse_token:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @token: the #GTokenType expected as next token.
 *
 * Returns: %TRUE if the next token is @token, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_token (LigmaScanner *scanner,
                          GTokenType   token)
{
  if (g_scanner_peek_next_token (scanner) != token)
    return FALSE;

  g_scanner_get_next_token (scanner);

  return TRUE;
}

/**
 * ligma_scanner_parse_identifier:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @identifier: (out): the expected identifier.
 *
 * Returns: %TRUE if the next token is an identifier and if its
 * value matches @identifier.
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_identifier (LigmaScanner *scanner,
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
 * ligma_scanner_parse_string:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed string
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_string (LigmaScanner  *scanner,
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
 * ligma_scanner_parse_string_no_validate:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed string
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_string_no_validate (LigmaScanner  *scanner,
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
 * ligma_scanner_parse_data:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @length: Length of the data to parse
 * @dest: (out) (array length=length): Return location for the parsed data
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_data (LigmaScanner  *scanner,
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
 * ligma_scanner_parse_int:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed integer
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_int (LigmaScanner *scanner,
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
 * ligma_scanner_parse_int64:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed integer
 *
 * Returns: %TRUE on success
 *
 * Since: 2.8
 **/
gboolean
ligma_scanner_parse_int64 (LigmaScanner *scanner,
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
 * ligma_scanner_parse_float:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed float
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_float (LigmaScanner *scanner,
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
 * ligma_scanner_parse_boolean:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out): Return location for the parsed boolean
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_boolean (LigmaScanner *scanner,
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
  COLOR_HSVA
};

/**
 * ligma_scanner_parse_color:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out caller-allocates): Pointer to a color to store the result
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_color (LigmaScanner *scanner,
                          LigmaRGB     *dest)
{
  guint      scope_id;
  guint      old_scope_id;
  GTokenType token;
  LigmaRGB    color = { 0.0, 0.0, 0.0, 1.0 };

  scope_id = g_quark_from_static_string ("ligma_scanner_parse_color");
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
            gdouble  col[4]     = { 0.0, 0.0, 0.0, 1.0 };
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
                if (! ligma_scanner_parse_float (scanner, &col[i]))
                  goto finish;
              }

            if (is_hsv)
              {
                LigmaHSV hsv;

                ligma_hsva_set (&hsv, col[0], col[1], col[2], col[3]);
                ligma_hsv_to_rgb (&hsv, &color);
              }
            else
              {
                ligma_rgba_set (&color, col[0], col[1], col[2], col[3]);
              }

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
      *dest = color;
    }

  g_scanner_set_scope (scanner, old_scope_id);

  return (token == G_TOKEN_NONE);
}

/**
 * ligma_scanner_parse_matrix2:
 * @scanner: A #LigmaScanner created by ligma_scanner_new_file() or
 *           ligma_scanner_new_string()
 * @dest: (out caller-allocates): Pointer to a matrix to store the result
 *
 * Returns: %TRUE on success
 *
 * Since: 2.4
 **/
gboolean
ligma_scanner_parse_matrix2 (LigmaScanner *scanner,
                            LigmaMatrix2 *dest)
{
  guint        scope_id;
  guint        old_scope_id;
  GTokenType   token;
  LigmaMatrix2  matrix;

  scope_id = g_quark_from_static_string ("ligma_scanner_parse_matrix");
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

            if (! ligma_scanner_parse_float (scanner, &matrix.coeff[0][0]))
              goto finish;
            if (! ligma_scanner_parse_float (scanner, &matrix.coeff[0][1]))
              goto finish;
            if (! ligma_scanner_parse_float (scanner, &matrix.coeff[1][0]))
              goto finish;
            if (! ligma_scanner_parse_float (scanner, &matrix.coeff[1][1]))
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
ligma_scanner_message (LigmaScanner *scanner,
                      gchar       *message,
                      gboolean     is_error)
{
  LigmaScannerData *data = scanner->user_data;

  /* we don't expect warnings */
  g_return_if_fail (is_error);

  if (data->name)
    g_set_error (data->error, LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_PARSE,
                 _("Error while parsing '%s' in line %d: %s"),
                 data->name, scanner->line, message);
  else
    /*  should never happen, thus not marked for translation  */
    g_set_error (data->error, LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_PARSE,
                 "Error parsing internal buffer: %s", message);
}
