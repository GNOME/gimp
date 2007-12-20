/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

#include "psd.h"
#include "psd-util.h"

#include "libgimp/stdplugins-intl.h"

/*  Local constants */
#define MIN_RUN     3

/*  Local function prototypes  */


/* Utility function */
gchar *
fread_pascal_string (gint32        *bytes_read,
                     gint32        *bytes_written,
                     const guint16  pad_len,
                     FILE          *f)
{
  /* Reads a pascal string padded to a multiple of pad_len and converts to utf-8 */

  gchar        *str,
               *utf8_str;
  guchar        len;
  gint32        padded_len;

  *bytes_read = 0;
  *bytes_written = -1;

  if (fread (&len, 1, 1, f) < 1)
    {
      g_message (_("Error reading pascal string length"));
      return NULL;
    }
  (*bytes_read)++;
  IFDBG(3) g_debug ("Pascal string length %d", len);

  if (len == 0)
    {
      if (fseek (f, pad_len - 1, SEEK_CUR) < 0)
        {
          g_message (_("Error setting file position"));
          return NULL;
        }
      *bytes_read += (pad_len - 1);
      *bytes_written = 0;
      return NULL;
    }

  str = g_malloc (len);
  if (fread (str, len, 1, f) < 1)
    {
      g_message (_("Error reading pascal string"));
      return NULL;
    }
  *bytes_read += len;

  if (pad_len > 0)
    {
      padded_len = len + 1;
      while (padded_len % pad_len != 0)
        {
          if (fseek (f, 1, SEEK_CUR) < 0)
            {
              g_message (_("Error setting file position"));
              return NULL;
            }
          (*bytes_read)++;
          padded_len++;
        }
    }

  utf8_str = gimp_any_to_utf8 (str, len, _("Invalid UTF-8 string in PSD file"));
  *bytes_written = strlen (utf8_str);
  g_free (str);

  IFDBG(2) g_debug ("Pascal string: %s, bytes_read: %d, bytes_written: %d",
                    utf8_str, *bytes_read, *bytes_written);

  return utf8_str;
}

gint32
fwrite_pascal_string (const gchar   *src,
                      const guint16  pad_len,
                      FILE          *f)
{
  /* Converts utf-8 string to current locale and writes as pascal string with
     padding to pad width */

  gchar        *str,
               *pascal_str,
                null_str = 0x0;
  guchar        pascal_len;
  gint32        bytes_written = 0;
  gsize         len;

  if (src == NULL)
    {
       /* Write null string as two null bytes (0x0) */
      if (fwrite (&null_str, 1, 1, f) < 1
          || fwrite (&null_str, 1, 1, f) < 1)
        {
          g_message (_("Error writing pascal string"));
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
      if (fwrite (&pascal_len, 1, 1, f) < 1
          || fwrite (pascal_str, pascal_len, 1, f) < 1)
        {
          g_message (_("Error writing pascal string"));
          return -1;
        }
      bytes_written++;
      bytes_written += pascal_len;
      IFDBG(2) g_debug ("Pascal string: %s, bytes_written: %d",
                        pascal_str, bytes_written);
    }

  /* Pad with nulls */
  if (pad_len > 0)
    {
      while (bytes_written % pad_len != 0)
        {
          if (fwrite (&null_str, 1, 1, f) < 1)
            {
              g_message (_("Error writing pascal string"));
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
                      const guint16  pad_len,
                      FILE          *f)
{
  /* Reads a utf-16 string padded to a multiple of pad_len and converts to utf-8 */

  gchar        *utf8_str;
  gunichar2    *utf16_str;
  gint32        len,
                i,
                padded_len;
  glong         utf8_str_len;

  *bytes_read = 0;
  *bytes_written = -1;

  if (fread (&len, 4, 1, f) < 1)
    {
      g_message (_("Error reading unicode string length"));
      return NULL;
    }
  *bytes_read += 4;
  len = GINT32_FROM_BE (len);
  IFDBG(3) g_debug ("Unicode string length %d", len);

  if (len == 0)
    {
      if (fseek (f, pad_len - 1, SEEK_CUR) < 0)
        {
          g_message (_("Error setting file position"));
          return NULL;
        }
      *bytes_read += (pad_len - 1);
      *bytes_written = 0;
      return NULL;
    }

  utf16_str = g_malloc (len * 2);
  for (i = 0; i < len; ++i)
    {
      if (fread (&utf16_str[i], 2, 1, f) < 1)
        {
          g_message (_("Error reading unicode string"));
          return NULL;
        }
      *bytes_read += 2;
      utf16_str[i] = GINT16_FROM_BE (utf16_str[i]);
    }

  if (pad_len > 0)
    {
      padded_len = len + 1;
      while (padded_len % pad_len != 0)
        {
          if (fseek (f, 1, SEEK_CUR) < 0)
            {
              g_message (_("Error setting file position"));
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
fwrite_unicode_string (const gchar   *src,
                       const guint16  pad_len,
                       FILE          *f)
{
  /* Converts utf-8 string to utf-16 and writes 4 byte length then string
     padding to pad width */

  gunichar2    *utf16_str;
  gchar         null_str = 0x0;
  gint32        utf16_len = 0,
                bytes_written = 0;
  gint          i;
  glong         len;

  if (src == NULL)
    {
       /* Write null string as four byte 0 int32 */
      if (fwrite (&utf16_len, 4, 1, f) < 1)
        {
          g_message (_("Error writing unicode string"));
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

      if (fwrite (&utf16_len, 4, 1, f) < 1
          || fwrite (utf16_str, 2, utf16_len + 1, f) < utf16_len + 1)
        {
          g_message (_("Error writing unicode string"));
          return -1;
        }
      bytes_written += (4 + 2 * utf16_len + 2);
      IFDBG(2) g_debug ("Unicode string: %s, bytes_written: %d",
                        src, bytes_written);
    }

  /* Pad with nulls */
  if (pad_len > 0)
    {
      while (bytes_written % pad_len != 0)
        {
          if (fwrite (&null_str, 1, 1, f) < 1)
            {
              g_message (_("Error writing unicode string"));
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
  int    n;
  gchar  dat;
  gint32 unpack_left = unpacked_len,
         pack_left = packed_len,
         error_code = 0,
         return_val = 0;

  IFDBG(3) g_debug ("Decode packbits");
  IFDBG(3) g_debug ("Packed len %d, unpacked %d",packed_len, unpacked_len);

  while (unpack_left > 0 && pack_left > 0)
    {
      n = *src;
      src++;
      pack_left--;

      if (n == 128)     /* nop */
        continue;
      else if (n > 128)
        n -= 256;

      if (n < 0)        /* replicate next gchar |n|+ 1 times */
        {
          n  = 1 - n;
          if (! pack_left)
            {
              IFDBG(2) g_debug ("Input buffer exhausted in replicate");
              error_code = 1;
              break;
            }
          if (n > unpack_left)
            {
              IFDBG(2) g_debug ("Overrun in packbits replicate of %d chars", n - unpack_left);
              error_code = 2;
            }
          dat = *src;
          for (; n > 0; --n)
            {
              if (! unpack_left)
                break;
              *dst = dat;
              dst++;
              unpack_left--;
            }
          if (unpack_left)
            {
              src++;
              pack_left--;
            }
        }
      else              /* copy next n+1 gchars literally */
        {
          n++;
          for (; n > 0; --n)
            {
              if (! pack_left)
                {
                  IFDBG(2) g_debug ("Input buffer exhausted in copy");
                  error_code = 3;
                  break;
                }
              if (! unpack_left)
                {
                  IFDBG(2) g_debug ("Output buffer exhausted in copy");
                  error_code = 4;
                  break;
                }
              *dst = *src;
              dst++;
              unpack_left--;
              src++;
              pack_left--;
            }
        }
    }

  if (unpack_left > 0)
    {
      /* Pad with zeros to end of output buffer */
      for (n = 0; n < pack_left; ++n)
        {
          *dst = 0;
          dst++;
        }
    }

  IFDBG(3) g_debug ("Pack left %d, unpack left %d", pack_left, unpack_left);
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
encode_packbits (const gchar   *src,
                 const guint32  unpacked_len,
                 guint16       *packed_len)
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

GimpLayerModeEffects
psd_to_gimp_blend_mode (const gchar *psd_mode)
{
  if (g_ascii_strncasecmp (psd_mode, "norm", 4) == 0)           /* Normal (ps3) */
    return GIMP_NORMAL_MODE;
  if (g_ascii_strncasecmp (psd_mode, "dark", 4) == 0)           /* Darken (ps3) */
    return GIMP_DARKEN_ONLY_MODE;
  if (g_ascii_strncasecmp (psd_mode, "lite", 4) == 0)           /* Lighten (ps3) */
      return GIMP_LIGHTEN_ONLY_MODE;
  if (g_ascii_strncasecmp (psd_mode, "hue ", 4) == 0)           /* Hue (ps3) */
    return GIMP_HUE_MODE;
  if (g_ascii_strncasecmp (psd_mode, "sat ", 4) == 0)           /* Saturation (ps3) */
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Gimp uses a different equation to photoshop for the "
                     "saturation blend mode. Results will differ."));
      return GIMP_SATURATION_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "colr", 4) == 0)           /* Color (ps3) */
    return GIMP_COLOR_MODE;
  if (g_ascii_strncasecmp (psd_mode, "lum ", 4) == 0)           /* Luminosity (ps3) */
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Gimp uses a different equation to photoshop for the "
                     "value (luminosity) blend mode. Results will differ."));
      return GIMP_VALUE_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "mul ", 4) == 0)           /* Multiply (ps3) */
    return GIMP_MULTIPLY_MODE;
  if (g_ascii_strncasecmp (psd_mode, "scrn", 4) == 0)           /* Screen (ps3) */
    return GIMP_SCREEN_MODE;
  if (g_ascii_strncasecmp (psd_mode, "diss", 4) == 0)           /* Dissolve (ps3) */
    return GIMP_DISSOLVE_MODE;
  if (g_ascii_strncasecmp (psd_mode, "over", 4) == 0)           /* Overlay (ps3) */
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Gimp uses a different equation to photoshop for the "
                     "overlay blend mode. Results will differ."));
      return GIMP_OVERLAY_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "hLit", 4) == 0)           /* Hard light (ps3) */
    return GIMP_HARDLIGHT_MODE;
  if (g_ascii_strncasecmp (psd_mode, "sLit", 4) == 0)           /* Soft light (ps3) */
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Gimp uses a different equation to photoshop for the "
                     "soft light blend mode. Results will differ."));
    return GIMP_SOFTLIGHT_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "diff", 4) == 0)           /* Difference (ps3) */
    return GIMP_DIFFERENCE_MODE;
  if (g_ascii_strncasecmp (psd_mode, "smud", 4) == 0)           /* Exclusion (ps6) */
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Exclusion blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "div ", 4) == 0)           /* Color dodge (ps6) */
      return GIMP_DODGE_MODE;
  if (g_ascii_strncasecmp (psd_mode, "idiv", 4) == 0)           /* Color burn (ps6) */
      return GIMP_BURN_MODE;
  if (g_ascii_strncasecmp (psd_mode, "lbrn", 4) == 0)           /* Linear burn (ps7)*/
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Linear burn blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "lddg", 4) == 0)           /* Linear dodge (ps7)*/
    return GIMP_ADDITION_MODE;
  if (g_ascii_strncasecmp (psd_mode, "lLit", 4) == 0)           /* Linear light (ps7)*/
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Linear light blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "pLit", 4) == 0)           /* Pin light (ps7)*/
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Pin light blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "vLit", 4) == 0)           /* Vivid light (ps7)*/
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Vivid light blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }
  if (g_ascii_strncasecmp (psd_mode, "hMix", 4) == 0)           /* Hard Mix (CS)*/
    {
      if (CONVERSION_WARNINGS)
        g_message (_("Hard mix blend mode not supported by GIMP. "
                     "Blend mode reverts to normal."));
      return GIMP_NORMAL_MODE;
    }

  if (CONVERSION_WARNINGS)
    g_message (_("Unknown blend mode %.4s. Blend mode reverts to normal."), psd_mode);
  return GIMP_NORMAL_MODE;
}

gchar *
gimp_to_psd_blend_mode (const GimpLayerModeEffects gimp_layer_mode)
{
  gchar        *psd_mode;

  switch (gimp_layer_mode)
    {
      case GIMP_NORMAL_MODE:
        psd_mode = g_strndup ("norm", 4);                       /* Normal (ps3) */
        break;
      case GIMP_DISSOLVE_MODE:
        psd_mode = g_strndup ("diss", 4);                       /* Dissolve (ps3) */
        break;
      case GIMP_BEHIND_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Behind blend mode not supported in PSD file. "
                       "Blend mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;
      case GIMP_MULTIPLY_MODE:
        psd_mode = g_strndup ("mul ", 4);                       /* Multiply (ps3) */
        break;
      case GIMP_SCREEN_MODE:
        psd_mode = g_strndup ("scrn", 4);                       /* Screen (ps3) */
        break;
      case GIMP_OVERLAY_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Gimp uses a different equation to photoshop for the "
                       "overlay blend mode. Results will differ."));
        psd_mode = g_strndup ("over", 4);                       /* Overlay (ps3) */
        break;
      case GIMP_DIFFERENCE_MODE:
        psd_mode = g_strndup ("diff", 4);                       /* Difference (ps3) */
        break;
      case GIMP_ADDITION_MODE:
        psd_mode = g_strndup ("lddg", 4);                       /* Linear dodge (ps7)*/
        break;
      case GIMP_SUBTRACT_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Photoshop does not support the subtract "
                       "blend mode. Layer mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;
      case GIMP_DARKEN_ONLY_MODE:
        psd_mode = g_strndup ("dark", 4);                       /* Darken (ps3) */
        break;
      case GIMP_LIGHTEN_ONLY_MODE:
        psd_mode = g_strndup ("lite", 4);                       /* Lighten (ps3) */
        break;
      case GIMP_HUE_MODE:
        psd_mode = g_strndup ("hue ", 4);                       /* Hue (ps3) */
        break;
      case GIMP_SATURATION_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Gimp uses a different equation to photoshop for the "
                       "saturation blend mode. Results will differ."));
        psd_mode = g_strndup ("sat ", 4);                       /* Saturation (ps3) */
        break;
      case GIMP_COLOR_MODE:
        psd_mode = g_strndup ("colr", 4);                       /* Color (ps3) */
        break;
      case GIMP_VALUE_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Gimp uses a different equation to photoshop for the "
                       "value (luminosity) blend mode. Results will differ."));
        psd_mode = g_strndup ("lum ", 4);                       /* Luminosity (ps3) */
        break;
      case GIMP_DIVIDE_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Photoshop does not support the divide "
                       "blend mode. Layer mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;
      case GIMP_DODGE_MODE:
        psd_mode = g_strndup ("div ", 4);                       /* Color Dodge (ps6) */
        break;
      case GIMP_BURN_MODE:
        psd_mode = g_strndup ("idiv", 4);                       /* Color Burn (ps6) */
        break;
      case GIMP_HARDLIGHT_MODE:
        psd_mode = g_strndup ("hLit", 4);                       /* Hard Light (ps3) */
        break;
      case GIMP_SOFTLIGHT_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Gimp uses a different equation to photoshop for the "
                       "soft light blend mode. Results will differ."));
        psd_mode = g_strndup ("sLit", 4);                       /* Soft Light (ps3) */
        break;
      case GIMP_GRAIN_EXTRACT_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Photoshop does not support the grain extract "
                       "blend mode. Layer mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;
      case GIMP_GRAIN_MERGE_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Photoshop does not support the grain merge "
                       "blend mode. Layer mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;
      case GIMP_COLOR_ERASE_MODE:
        if (CONVERSION_WARNINGS)
          g_message (_("Photoshop does not support the color erase "
                       "blend mode. Layer mode reverts to normal."));
        psd_mode = g_strndup ("norm", 4);
        break;

      default:
        if (CONVERSION_WARNINGS)
          g_message (_("Blend mode %d not supported. Blend mode reverts to normal.")
                     , gimp_layer_mode);
        psd_mode = g_strndup ("norm", 4);
    }

  return psd_mode;
}
