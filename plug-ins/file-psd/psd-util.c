/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-util.h"

#include "libgimp/stdplugins-intl.h"

/*  Local constants  */
#define MIN_RUN     3

/*  Local types  */
typedef struct
{
  const gchar   *name;
  const gchar   *psd_mode;
  GimpLayerMode  gimp_mode;
  gboolean       exact; /* does the modes behave (more-or-less) the same in
                         * Photoshop and in GIMP?
                         */
} LayerModeMapping;

/*  Local function prototypes  */
static const gchar * get_enum_value_nick (GType type,
                                          gint  value);

/*  Local variables  */

/* mapping table between Photoshop and GIMP modes.  in case a mode matches more
 * than one entry (in either direction), the first entry wins.
 */
static const LayerModeMapping layer_mode_map[] =
{
/*  Name             PSD     GIMP                                   Exact?  */

  /* Normal (ps3) */
  { "Normal",        "norm", GIMP_LAYER_MODE_NORMAL,                TRUE },
  { "Normal",        "norm", GIMP_LAYER_MODE_NORMAL_LEGACY,         TRUE },

  /* Dissolve (ps3) */
  { "Dissolve",      "diss", GIMP_LAYER_MODE_DISSOLVE,              TRUE },

  /* Multiply (ps3) */
  { "Multiply",      "mul ", GIMP_LAYER_MODE_MULTIPLY,              TRUE },
  { "Multiply",      "mul ", GIMP_LAYER_MODE_MULTIPLY_LEGACY,       TRUE },

  /* Screen (ps3) */
  { "Screen",        "scrn", GIMP_LAYER_MODE_SCREEN,                TRUE },
  { "Screen",        "scrn", GIMP_LAYER_MODE_SCREEN_LEGACY,         TRUE },

  /* Overlay (ps3) */
  { "Overlay",       "over", GIMP_LAYER_MODE_OVERLAY,               TRUE },

  /* Difference (ps3) */
  { "Difference",    "diff", GIMP_LAYER_MODE_DIFFERENCE,            TRUE },
  { "Difference",    "diff", GIMP_LAYER_MODE_DIFFERENCE_LEGACY,     TRUE },

  /* Linear Dodge (cs2) */
  { "Linear Dodge",  "lddg", GIMP_LAYER_MODE_ADDITION,              FALSE },
  { "Linear Dodge",  "lddg", GIMP_LAYER_MODE_ADDITION_LEGACY,       TRUE },

  /* Subtract (??) */
  { "Subtract",      "fsub", GIMP_LAYER_MODE_SUBTRACT,              FALSE },
  { "Subtract",      "fsub", GIMP_LAYER_MODE_SUBTRACT_LEGACY,       TRUE },

  /* Darken (ps3) */
  { "Darken",        "dark", GIMP_LAYER_MODE_DARKEN_ONLY,           TRUE },
  { "Darken",        "dark", GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY,    TRUE },

  /* Lighten (ps3) */
  { "Ligten",        "lite", GIMP_LAYER_MODE_LIGHTEN_ONLY,          TRUE },
  { "Ligten",        "lite", GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY,   TRUE },

  /* Hue (ps3) */
  { "Hue",           "hue ", GIMP_LAYER_MODE_LCH_HUE,               FALSE },
  { "Hue",           "hue ", GIMP_LAYER_MODE_HSV_HUE,               FALSE },
  { "Hue",           "hue ", GIMP_LAYER_MODE_HSV_HUE_LEGACY,        FALSE },

  /* Saturation (ps3) */
  { "Saturation",    "sat ", GIMP_LAYER_MODE_LCH_CHROMA,            FALSE },
  { "Saturation",    "sat ", GIMP_LAYER_MODE_HSV_SATURATION,        FALSE },
  { "Saturation",    "sat ", GIMP_LAYER_MODE_HSV_SATURATION_LEGACY, FALSE },

  /* Color (ps3) */
  { "Color",         "colr", GIMP_LAYER_MODE_LCH_COLOR,             FALSE },
  { "Color",         "colr", GIMP_LAYER_MODE_HSL_COLOR,             FALSE },
  { "Color",         "colr", GIMP_LAYER_MODE_HSL_COLOR_LEGACY,      FALSE },

  /* Luminosity (ps3) */
  { "Luminosity",    "lum ", GIMP_LAYER_MODE_LCH_LIGHTNESS,         FALSE },
  { "Luminosity",    "lum ", GIMP_LAYER_MODE_HSV_VALUE,             FALSE },
  { "Luminosity",    "lum ", GIMP_LAYER_MODE_HSV_VALUE_LEGACY,      FALSE },
  { "Luminosity",    "lum ", GIMP_LAYER_MODE_LUMINANCE,             FALSE },

  /* Divide (??) */
  { "Divide",        "fdiv", GIMP_LAYER_MODE_DIVIDE,                FALSE },
  { "Divide",        "fdiv", GIMP_LAYER_MODE_DIVIDE_LEGACY,         TRUE },

  /* Color Dodge (ps6) */
  { "Color Dodge",   "div ", GIMP_LAYER_MODE_DODGE,                 FALSE },
  { "Color Dodge",   "div ", GIMP_LAYER_MODE_DODGE_LEGACY,          TRUE },

  /* Color Burn (ps6) */
  { "Color Burn",    "idiv", GIMP_LAYER_MODE_BURN,                  FALSE },
  { "Color Burn",    "idiv", GIMP_LAYER_MODE_BURN_LEGACY,           TRUE },

  /* Hard Light (ps3) */
  { "Hard Light",    "hLit", GIMP_LAYER_MODE_HARDLIGHT,             TRUE },
  { "Hard Light",    "hLit", GIMP_LAYER_MODE_HARDLIGHT_LEGACY,      TRUE },

  /* Soft Light (ps3) */
  { "Soft Light",    "sLit", GIMP_LAYER_MODE_SOFTLIGHT,             FALSE },
  { "Soft Light",    "sLit", GIMP_LAYER_MODE_SOFTLIGHT_LEGACY,      FALSE },
  { "Soft Light",    "sLit", GIMP_LAYER_MODE_OVERLAY_LEGACY,        FALSE },

  /* Vivid Light (ps7)*/
  { "Vivid Light",   "vLit", GIMP_LAYER_MODE_VIVID_LIGHT,           TRUE },

  /* Pin Light (ps7)*/
  { "Pin Light",     "pLit", GIMP_LAYER_MODE_PIN_LIGHT,             TRUE },

  /* Linear Light (ps7)*/
  { "Linear Light",  "lLit", GIMP_LAYER_MODE_LINEAR_LIGHT,          FALSE },

  /* Hard Mix (CS)*/
  { "Hard Mix",      "hMix", GIMP_LAYER_MODE_HARD_MIX,              FALSE },

  /* Exclusion (ps6) */
  { "Exclusion",     "smud", GIMP_LAYER_MODE_EXCLUSION,             TRUE },

  /* Linear Burn (ps7)*/
  { "Linear Burn",   "lbrn", GIMP_LAYER_MODE_LINEAR_BURN,           FALSE },

  /* Darker Color (??)*/
  { "Darker Color",  "dkCl", GIMP_LAYER_MODE_LUMA_DARKEN_ONLY,      FALSE },

  /* Lighter Color (??)*/
  { "Lighter Color", "lgCl", GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY,     FALSE },

  /* Pass Through (CS)*/
  { "Pass Through",  "pass", GIMP_LAYER_MODE_PASS_THROUGH,          TRUE },
};


/* Utility function */
void
psd_set_error (GError  **error)
{
  if (! error || ! *error)
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                 _("Error reading data. Most likely unexpected end of file."));

  return;
}

gint
psd_read (GInputStream  *input,
          gconstpointer  data,
          gint           count,
          GError       **error)
{
  gsize bytes_read = 0;

  /* we allow for 'data == NULL && count == 0', which g_input_stream_read_all()
   * rejects.
   */
  if (count > 0)
    {
      /* We consider reading less bytes than we want an error. */
      if (g_input_stream_read_all (input, (void *) data, count,
                                   &bytes_read, NULL, error) &&
          bytes_read < count)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Unexpected end of file"));
    }

  return bytes_read;
}

gboolean
psd_read_len (GInputStream  *input,
              guint64       *data,
              gint           psd_version,
              GError       **error)
{
  gint block_len_size = (psd_version == 1 ? 4 : 8);

  if (psd_read (input, data, block_len_size, error) < block_len_size)
    {
      psd_set_error (error);
      return FALSE;
    }

  if (psd_version == 1)
    *data = GUINT32_FROM_BE (*data);
  else
    *data = GUINT64_FROM_BE (*data);
  return TRUE;
}

gboolean
psd_seek (GInputStream  *input,
          goffset        offset,
          GSeekType      type,
          GError       **error)
{
  return g_seekable_seek (G_SEEKABLE (input),
                          offset, type,
                          NULL, error);
}

gchar *
fread_pascal_string (gint32        *bytes_read,
                     gint32        *bytes_written,
                     guint16        mod_len,
                     GInputStream  *input,
                     GError       **error)
{
  /*
   * Reads a pascal string from the file padded to a multiple of mod_len
   * and returns a utf-8 string.
   */

  gchar        *str;
  gchar        *utf8_str;
  guchar        len = 0;
  gint32        padded_len;

  *bytes_read = 0;
  *bytes_written = -1;

  if (psd_read (input, &len, 1, error) < 1)
    {
      psd_set_error (error);
      return NULL;
    }
  (*bytes_read)++;
  IFDBG(3) g_debug ("Pascal string length %d", len);

  if (len == 0)
    {
      if (! psd_seek (input, mod_len - 1, G_SEEK_CUR, error))
        {
          psd_set_error (error);
          return NULL;
        }
      *bytes_read += (mod_len - 1);
      *bytes_written = 0;
      return NULL;
    }

  str = g_malloc (len);
  if (psd_read (input, str, len, error) < len)
    {
      psd_set_error (error);
      g_free (str);
      return NULL;
    }
  *bytes_read += len;

  if (mod_len > 0)
    {
      padded_len = len + 1;
      while (padded_len % mod_len != 0)
        {
          if (! psd_seek (input, 1, G_SEEK_CUR, error))
            {
              psd_set_error (error);
              g_free (str);
              return NULL;
            }
          (*bytes_read)++;
          padded_len++;
        }
    }

  utf8_str = gimp_any_to_utf8 (str, len, NULL);
  *bytes_written = strlen (utf8_str);
  g_free (str);

  IFDBG(3) g_debug ("Pascal string: %s, bytes_read: %d, bytes_written: %d",
                    utf8_str, *bytes_read, *bytes_written);

  return utf8_str;
}

gint32
fwrite_pascal_string (const gchar    *src,
                      guint16         mod_len,
                      GOutputStream  *output,
                      GError        **error)
{
  /*
   *  Converts utf-8 string to current locale and writes as pascal
   *  string with padding to a multiple of mod_len.
   */

  gchar        *str;
  gchar        *pascal_str;
  const gchar   null_str = 0x0;
  guchar        pascal_len;
  gsize         bytes_written = 0;
  gsize         len;

  g_debug ("fwrite_pascal_string %s!", src);

  if (src == NULL)
    {
       /* Write null string as two null bytes (0x0) */
      if (! g_output_stream_write_all (output, &null_str, 1,
                                       &bytes_written, NULL, error) ||
          ! g_output_stream_write_all (output, &null_str, 1,
                                       &bytes_written, NULL, error))
        {
          psd_set_error (error);
          return -1;
        }
      bytes_written += 2;
    }
  else
    {
      str = g_locale_from_utf8 (src, -1, NULL, &len, NULL);
      if (len > 255)
        pascal_len = 255;
      else
        pascal_len = len;
      pascal_str = g_strndup (str, pascal_len);
      g_free (str);

      if (! g_output_stream_write_all (output, &pascal_len, 1,
                                       &bytes_written, NULL, error) ||
          ! g_output_stream_write_all (output, pascal_str, pascal_len,
                                       &bytes_written, NULL, error))
        {
          g_free (pascal_str);
          return -1;
        }
      bytes_written++;
      bytes_written += pascal_len;

      IFDBG(2) g_debug ("Pascal string: %s, bytes_written: %" G_GSIZE_FORMAT,
                        pascal_str, bytes_written);
      g_free (pascal_str);
    }

  /* Pad with nulls */
  if (mod_len > 0)
    {
      while (bytes_written % mod_len != 0)
        {

          if (! g_output_stream_write_all (output, &null_str, 1,
                                           &bytes_written, NULL, error))
            {
              return -1;
            }
          bytes_written++;
        }
    }

  return bytes_written;
}

gchar *
fread_unicode_string (gint32        *bytes_read,
                      gint32        *bytes_written,
                      guint16        mod_len,
                      gboolean       ibm_pc_format,
                      GInputStream  *input,
                      GError       **error)
{
  /*
   * Reads a utf-16 string from the file padded to a multiple of mod_len
   * and returns a utf-8 string.
   */

  gchar     *utf8_str;
  gunichar2 *utf16_str;
  gint32     len = 0;
  gint32     i;
  gint32     padded_len;
  glong      utf8_str_len;

  *bytes_read = 0;
  *bytes_written = -1;

  if (psd_read (input, &len, 4, error) < 4)
    {
      psd_set_error (error);
      return NULL;
    }
  *bytes_read += 4;

  if (! ibm_pc_format)
    len = GINT32_FROM_BE (len);
  else
    len = GINT32_FROM_LE (len);

  IFDBG(3) g_debug ("Unicode string length %d", len);

  if (len == 0)
    {
      if (! psd_seek (input, mod_len - 1, G_SEEK_CUR, error))
        {
          psd_set_error (error);
          return NULL;
        }
      *bytes_read += (mod_len - 1);
      *bytes_written = 0;
      return NULL;
    }

  utf16_str = g_malloc (len * 2);
  for (i = 0; i < len; ++i)
    {
      if (psd_read (input, &utf16_str[i], 2, error) < 2)
        {
          psd_set_error (error);
          g_free (utf16_str);
          return NULL;
        }
      *bytes_read += 2;
      if (! ibm_pc_format)
        utf16_str[i] = GINT16_FROM_BE (utf16_str[i]);
      else
        utf16_str[i] = GINT16_FROM_LE (utf16_str[i]);
    }

  if (mod_len > 0)
    {
      padded_len = *bytes_read;
      while (padded_len % mod_len != 0)
        {
          if (! psd_seek (input, 1, G_SEEK_CUR, error))
            {
              psd_set_error (error);
              g_free (utf16_str);
              return NULL;
            }
          (*bytes_read)++;
          padded_len++;
        }
    }

  utf8_str = g_utf16_to_utf8 (utf16_str, len, NULL, &utf8_str_len, NULL);
  *bytes_written = utf8_str_len;
  g_free (utf16_str);

  IFDBG(3) g_debug ("Unicode string: %s, bytes_read: %d, bytes_written: %d",
                    utf8_str, *bytes_read, *bytes_written);

  return utf8_str;
}

gint32
fwrite_unicode_string (const gchar    *src,
                       guint16         mod_len,
                       GOutputStream  *output,
                       GError        **error)
{
  /*
   *  Converts utf-8 string to utf-16 and writes 4 byte length
   *  then string padding to multiple of mod_len.
   */

  gunichar2 *utf16_str;
  gchar      null_str = 0x0;
  gint32     utf16_len = 0;
  gsize      bytes_written = 0;
  gint       i;
  glong      len;

  if (src == NULL)
    {
       /* Write null string as four byte 0 int32 */
      if (! g_output_stream_write_all (output, &utf16_len, 4,
                                       &bytes_written, NULL, error))
        {
          return -1;
        }
      bytes_written += 4;
    }
  else
    {
      utf16_str = g_utf8_to_utf16 (src, -1, NULL, &len, NULL);
      /* Byte swap as required */
      utf16_len = len;
      for (i = 0; i < utf16_len; ++i)
          utf16_str[i] = GINT16_TO_BE (utf16_str[i]);
      utf16_len = GINT32_TO_BE (utf16_len);

      if (! g_output_stream_write_all (output, &utf16_len, 4,
                                       &bytes_written, NULL, error) ||
          ! g_output_stream_write_all (output, &utf16_str, 2 * (utf16_len + 1),
                                       &bytes_written, NULL, error))
        {
          return -1;
        }
      bytes_written += (4 + 2 * utf16_len + 2);
      IFDBG(2) g_debug ("Unicode string: %s, bytes_written: %" G_GSIZE_FORMAT,
                        src, bytes_written);
    }

  /* Pad with nulls */
  if (mod_len > 0)
    {
      while (bytes_written % mod_len != 0)
        {
          if (! g_output_stream_write_all (output, &null_str, 1,
                                           &bytes_written, NULL, error))
            {
              return -1;
            }
          bytes_written++;
        }
    }

  return bytes_written;
}

gint
decode_packbits (const gchar *src,
                 gchar       *dst,
                 guint16      packed_len,
                 guint32      unpacked_len)
{
  /*
   *  Decode a PackBits chunk.
   */

  gint    n;
  gint32  unpack_left = unpacked_len;
  gint32  pack_left = packed_len;
  gint32  error_code = 0;
  gint32  return_val = 0;

  while (unpack_left > 0 && pack_left > 0)
    {
      n = *(const guchar *) src;
      src++;
      pack_left--;

      if (n == 128)     /* nop */
        continue;
      else if (n > 128)
        n -= 256;

      if (n < 0)        /* replicate next gchar |n|+ 1 times */
        {
          n = 1 - n;
          if (pack_left < 1)
            {
              IFDBG(2) g_debug ("Input buffer exhausted in replicate");
              error_code = 1;
              break;
            }
          if (unpack_left < n)
            {
              IFDBG(2) g_debug ("Overrun in packbits replicate of %d chars", n - unpack_left);
              error_code = 2;
              break;
            }
          memset (dst, *src, n);
          src++;
          pack_left--;
          dst         += n;
          unpack_left -= n;
        }
      else              /* copy next n+1 gchars literally */
        {
          n++;
          if (pack_left < n)
            {
              IFDBG(2) g_debug ("Input buffer exhausted in copy");
              error_code = 3;
              break;
            }
          if (unpack_left < n)
            {
              IFDBG(2) g_debug ("Output buffer exhausted in copy");
              error_code = 4;
              break;
            }
          memcpy (dst, src, n);
          src         += n;
          pack_left   -= n;
          dst         += n;
          unpack_left -= n;
        }
    }

  if (unpack_left > 0)
    {
      /* Pad with zeros to end of output buffer */
      memset (dst, 0, unpack_left);
    }

  if (unpack_left)
    {
      IFDBG(2) g_debug ("Packbits decode - unpack left %d", unpack_left);
      return_val -= unpack_left;
    }
  if (pack_left)
    {
      /* Some images seem to have a pad byte at the end of the packed data */
      if (error_code || pack_left != 1)
        {
          IFDBG(2) g_debug ("Packbits decode - pack left %d", pack_left);
          return_val = pack_left;
        }
    }

  IFDBG(2)
    if (error_code)
      g_debug ("Error code %d", error_code);

  return return_val;
}

gchar *
encode_packbits (const gchar *src,
                 guint32      unpacked_len,
                 guint16     *packed_len)
{
  /*
   *  Encode a PackBits chunk.
   */

  GString *dst_str;                      /* destination string */
  gint     curr_char;                    /* current character */
  gchar    char_buff[128];               /* buffer of already read characters */
  guchar   run_len;                      /* number of characters in a run */
  gint32   unpack_left = unpacked_len;

  IFDBG(2) g_debug ("Encode packbits");

  /* Initialise destination string */
  dst_str = g_string_sized_new (unpacked_len);

  /* prime the read loop */
  curr_char = *src;
  src++;
  run_len = 0;

  /* read input until there's nothing left */
  while (unpack_left > 0)
    {
        char_buff[run_len] = (gchar) curr_char;
        IFDBG(2) g_debug ("buff %x, run len %d, curr char %x",
                          char_buff[run_len], run_len, (gchar) curr_char);
        run_len++;

        if (run_len >= MIN_RUN)
          {
            gint i;

            /* check for run  */
            for (i = 2; i <= MIN_RUN; ++i)
              {
                if (curr_char != char_buff[run_len - i])
                  {
                    /* no run */
                    i = 0;
                    break;
                  }
              }

            if (i != 0)
              {
                /* we have a run write out buffer before run*/
                gint next_char;

                if (run_len > MIN_RUN)
                  {
                    /* block size - 1 followed by contents */
                    g_string_append_c (dst_str, (run_len - MIN_RUN - 1));
                    g_string_append_len (dst_str, char_buff, (run_len - MIN_RUN));
                    IFDBG(2) g_debug ("(1) Number of chars: %d, run length tag: %d",
                                      run_len - MIN_RUN, run_len - MIN_RUN - 1);
                  }

                /* determine run length (MIN_RUN so far) */
                run_len = MIN_RUN;

                /* get next character */
                next_char = *src;
                src++;
                unpack_left--;
                while (next_char == curr_char)
                  {
                    run_len++;
                    if (run_len == 128)
                      {
                        /* run is at max length */
                        break;
                      }
                    next_char = *src;
                    src++;
                    unpack_left--;
                  }

                /* write out encoded run length and run symbol */
                g_string_append_c (dst_str, (gchar)(1 - (gint)(run_len)));
                g_string_append_c (dst_str, curr_char);
                IFDBG(2) g_debug ("(2) Number of chars: %d, run length tag: %d",
                                  run_len, (1 - (gint)(run_len)));

                if (unpack_left > 0)
                  {
                    /* make run breaker start of next buffer */
                    char_buff[0] = next_char;
                    run_len = 1;
                  }
                else
                  {
                    /* file ends in a run */
                    run_len = 0;
                  }
              }
          }

        if (run_len == 128)
          {
            /* write out buffer */
            g_string_append_c (dst_str, 127);
            g_string_append_len (dst_str, char_buff, 128);
            IFDBG(2) g_debug ("(3) Number of chars: 128, run length tag: 127");

            /* start a new buffer */
            run_len = 0;
          }

        curr_char = *src;
        src++;
        unpack_left--;
    }

  /* write out last buffer */
  if (run_len != 0)
    {
        if (run_len <= 128)
          {
            /* write out entire copy buffer */
            g_string_append_c (dst_str, run_len - 1);
            g_string_append_len (dst_str, char_buff, run_len);
            IFDBG(2) g_debug ("(4) Number of chars: %d, run length tag: %d",
                              run_len, run_len - 1);
          }
        else
          {
            IFDBG(2) g_debug ("(5) Very bad - should not be here");
          }
    }

  *packed_len = dst_str->len;
  IFDBG(2) g_debug ("Packed len %d, unpacked %d", *packed_len, unpacked_len);

  return g_string_free (dst_str, FALSE);
}

void
psd_to_gimp_blend_mode (PSDlayer      *psd_layer,
                        LayerModeInfo *mode_info)
{
  gint i;

  mode_info->mode            = GIMP_LAYER_MODE_NORMAL;
  /* FIXME: use the image mode to select the correct color spaces.  for now,
   * we use rgb-perceptual blending/compositing unconditionally.
   */
  mode_info->blend_space     = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL;
  mode_info->composite_space = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL;
  if (psd_layer->clipping == 1)
    mode_info->composite_mode  = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP;
  else
    mode_info->composite_mode  = GIMP_LAYER_COMPOSITE_UNION;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_map); i++)
    {
      if (g_ascii_strncasecmp (psd_layer->blend_mode, layer_mode_map[i].psd_mode, 4) == 0)
        {
          if (! layer_mode_map[i].exact && CONVERSION_WARNINGS)
            {
              g_message ("GIMP uses a different equation than Photoshop for "
                         "blend mode: %s. Results will differ.",
                         layer_mode_map[i].name);
            }

          mode_info->mode = layer_mode_map[i].gimp_mode;

          return;
        }
    }

  if (CONVERSION_WARNINGS)
    {
      gchar *mode_name = g_strndup (psd_layer->blend_mode, 4);
      g_message ("Unsupported blend mode: %s. Mode reverts to normal",
                 mode_name);
      g_free (mode_name);
    }
}

const gchar *
gimp_to_psd_blend_mode (const LayerModeInfo *mode_info)
{
  gint i;

  /* FIXME: select the image mode based on the layer mode color spaces.  for
   * now, we assume rgb-perceptual blending/compositing unconditionally.
   */
  if (mode_info->blend_space != GIMP_LAYER_COLOR_SPACE_AUTO &&
      mode_info->blend_space != GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL)
    {
      if (CONVERSION_WARNINGS)
        g_message ("Unsupported blend color space: %s. "
                   "Blend color space reverts to rgb-perceptual",
                   get_enum_value_nick (GIMP_TYPE_LAYER_COLOR_SPACE,
                                        mode_info->blend_space));
    }

  if (mode_info->composite_space != GIMP_LAYER_COLOR_SPACE_AUTO &&
      mode_info->composite_space != GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL)
    {
      if (CONVERSION_WARNINGS)
        g_message ("Unsupported composite color space: %s. "
                   "Composite color space reverts to rgb-perceptual",
                   get_enum_value_nick (GIMP_TYPE_LAYER_COLOR_SPACE,
                                        mode_info->composite_space));
    }

  if (mode_info->composite_mode != GIMP_LAYER_COMPOSITE_AUTO &&
      mode_info->composite_mode != GIMP_LAYER_COMPOSITE_UNION &&
      mode_info->composite_mode != GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP)
    {
      if (CONVERSION_WARNINGS)
        g_message ("Unsupported composite mode: %s. "
                   "Composite mode reverts to union",
                   get_enum_value_nick (GIMP_TYPE_LAYER_COMPOSITE_MODE,
                                        mode_info->composite_mode));
    }

  for (i = 0; i < G_N_ELEMENTS (layer_mode_map); i++)
    {
      if (layer_mode_map[i].gimp_mode == mode_info->mode)
        {
          if (! layer_mode_map[i].exact && CONVERSION_WARNINGS)
            {
              g_message ("GIMP uses a different equation than Photoshop for "
                         "blend mode: %s. Results may differ.",
                         get_enum_value_nick (GIMP_TYPE_LAYER_MODE,
                                              mode_info->mode));
            }

          return layer_mode_map[i].psd_mode;
        }
    }

  if (CONVERSION_WARNINGS)
    g_message ("Unsupported blend mode: %s. Mode reverts to normal",
               get_enum_value_nick (GIMP_TYPE_LAYER_MODE, mode_info->mode));

  return "norm";
}

GimpColorTag
psd_to_gimp_layer_color_tag (guint16 layer_color_tag)
{
  GimpColorTag colorTag;

  switch (layer_color_tag)
    {
    case 1:
      colorTag = GIMP_COLOR_TAG_RED;
      break;

    case 2:
      colorTag = GIMP_COLOR_TAG_ORANGE;
      break;

    case 3:
      colorTag = GIMP_COLOR_TAG_YELLOW;
      break;

    case 4:
      colorTag = GIMP_COLOR_TAG_GREEN;
      break;

    case 5:
      colorTag = GIMP_COLOR_TAG_BLUE;
      break;

    case 6:
      colorTag = GIMP_COLOR_TAG_VIOLET;
      break;

    case 7:
      colorTag = GIMP_COLOR_TAG_GRAY;
      break;

    default:
      if (CONVERSION_WARNINGS)
        g_message ("Unsupported Photoshop layer color tag: %i. GIMP layer color tag set to none.",
                       layer_color_tag);
      colorTag = GIMP_COLOR_TAG_NONE;
    }

  return colorTag;
}

guint16
gimp_to_psd_layer_color_tag (GimpColorTag layer_color_tag)
{
  guint16 color_tag;

  switch (layer_color_tag)
    {
    case GIMP_COLOR_TAG_RED:
        color_tag = 1;
      break;

    case GIMP_COLOR_TAG_ORANGE:
        color_tag = 2;
      break;

    case GIMP_COLOR_TAG_YELLOW:
        color_tag = 3;
      break;

    case GIMP_COLOR_TAG_GREEN:
        color_tag = 4;
      break;

    case GIMP_COLOR_TAG_BLUE:
        color_tag = 5;
      break;

    case GIMP_COLOR_TAG_VIOLET:
        color_tag = 6;
      break;

    case GIMP_COLOR_TAG_GRAY:
        color_tag = 7;
      break;

    default:
      if (CONVERSION_WARNINGS)
        g_message ("Photoshop doesn't support GIMP layer color tag: %i. Photoshop layer color tag set to none.",
                   layer_color_tag);

      color_tag = 0;
    }

  return color_tag;
}

static const gchar *
get_enum_value_nick (GType type,
                     gint  value)
{
  const gchar *nick;

  if (gimp_enum_get_value (type, value, NULL, &nick, NULL, NULL))
    {
      return nick;
    }
  else
    {
      static gchar err_name[32];

      snprintf (err_name, sizeof (err_name), "UNKNOWN (%d)", value);

      return err_name;
    }
}
