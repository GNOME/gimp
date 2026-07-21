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

/* Adobe uses fixed point ints which consist of four bytes: a 16-bit number and
 * 16-bit fraction */
#define FIXED_TO_FLOAT(num,fract) (gfloat) num + fract / 65535.0f

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

typedef  struct
{
    gint        lvl;
    gint        ar_lvl;
    guint32     bufpos;
    guint32     cmdpos;
    guint32     val_len;
    guint32     val_pos;
    guint32     value_start_pos;
    gchar      *last_cmd;
    gboolean    reading_command;
    gboolean    reading_value;
    gboolean    reading_binary;
} ParserData;

/*  Local function prototypes  */
static const gchar * get_enum_value_nick (GType          type,
                                          gint           value);

static gchar  *      load_key            (GInputStream   *input,
                                          GError        **error);

static gint          load_type           (GInputStream   *input,
                                          gboolean        ibm_pc_format,
                                          const gchar    *class_id,
                                          const gchar    *key,
                                          const gchar    *type,
                                          JsonNode	   **node,
                                          GError        **error);

static gint          parse_text_info     (guint32         len,
                                          gchar          *buf,
                                          JsonNode      **tdta_root,
                                          GError        **error);

static gint          parse_text_loop     (guint32         len,
                                          gchar          *buf,
                                          JsonNode      **node,
                                          ParserData     *data,
                                          GError        **error);

static JsonNode * add_descriptor_float   (const gchar    *key,
                                          const gchar    *type,
                                          const gchar    *float_type,
                                          gfloat          value);

static JsonNode * add_descriptor_double  (const gchar    *key,
                                          const gchar    *type,
                                          gdouble         value);

static JsonNode * add_descriptor_bool    (const gchar    *key,
                                          const gchar    *type,
                                          gboolean        value);

static JsonNode * add_descriptor_comp    (const gchar    *key,
                                          const gchar    *type,
                                          gint64          value);

static JsonNode * add_descriptor_long    (const gchar    *key,
                                          const gchar    *type,
                                          gint32          value);

static JsonNode * add_descriptor_enum    (const gchar    *key,
                                          const gchar    *type,
                                          const gchar    *name,
                                          const gchar    *value);

static JsonNode * add_descriptor_text    (const gchar    *key,
                                          const gchar    *type,
                                          const gchar    *name,
                                          const gchar    *value);

static JsonNode * add_descriptor_objc    (const gchar    *key,
                                          const gchar    *type);

static JsonNode * add_descriptor_vlls    (const gchar    *key,
                                          const gchar    *type);

static JsonNode * add_descriptor_float_list
                                         (const gchar    *key,
                                          const gchar    *type,
                                          const gchar    *float_type);

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

/* Names of layer modes in descriptors */
static const LayerModeMapping descriptor_mode_map[] =
/*  Name             PSD     GIMP                                   Exact?  */
{
  /* From SDK, apparently less possible values than regular blend mode?
   * typeBlendMode 'BlnM' // enumNormal, enumDissolve, enumBehind, enumClear,
                             enumMultiply, enumScreen, enumOverlay, enumSoftLight,
                             enumHardLight, enumDarken, enumLighten, enumDifference,
                             enumHue, enumSaturation, enumColor, enumLuminosity,
                             enumExclusion, enumColorDodge, enumColorBurn.
   */
  { "Normal",        "Nrml", GIMP_LAYER_MODE_NORMAL,                TRUE },
  { "Dissolve",      "Dslv", GIMP_LAYER_MODE_DISSOLVE,              TRUE },
  { "Behind",        "Bhnd", GIMP_LAYER_MODE_BEHIND,                TRUE },
  /* Cl(e)ar sounds like color erase, although according to Adobe it's not
   * available as a regular layer mode:
   * https://helpx.adobe.com/photoshop/using/blending-modes.html
   */
  { "Color Erase",   "Clar", GIMP_LAYER_MODE_COLOR_ERASE,           TRUE },
  { "Multiply",      "Mltp", GIMP_LAYER_MODE_MULTIPLY,              TRUE },
  { "Screen",        "Scrn", GIMP_LAYER_MODE_SCREEN,                TRUE },
  { "Overlay",       "Ovrl", GIMP_LAYER_MODE_OVERLAY,               TRUE },
  { "Soft Light",    "SftL", GIMP_LAYER_MODE_SOFTLIGHT,             FALSE },
  { "Hard Light",    "HrdL", GIMP_LAYER_MODE_HARDLIGHT,             TRUE },
  { "Darken",        "Drkn", GIMP_LAYER_MODE_DARKEN_ONLY,           TRUE },
  { "Difference",    "Dfrn", GIMP_LAYER_MODE_DIFFERENCE,            TRUE },
  { "Hue",           "H   ", GIMP_LAYER_MODE_LCH_HUE,               FALSE },
  { "Saturation",    "Strt", GIMP_LAYER_MODE_LCH_CHROMA,            FALSE },
  { "Color",         "Clr ", GIMP_LAYER_MODE_LCH_COLOR,             FALSE },
  { "Luminosity",    "Lmns", GIMP_LAYER_MODE_LCH_LIGHTNESS,         FALSE },
  { "Exclusion",     "Xclu", GIMP_LAYER_MODE_EXCLUSION,             TRUE  },
  { "Color Dodge",   "CDdg", GIMP_LAYER_MODE_DODGE,                 FALSE },
  { "Color Burn",    "CBrn", GIMP_LAYER_MODE_BURN,                  FALSE },
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
          gpointer       data,
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
psd_read_chars (GInputStream  *input,
                gchar         *data,
                gint           count,
                gboolean       ibm_pc_format,
                GError       **error)
{
  if (psd_read (input, data, count, error) < count)
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    {
      for (gint i = 0; i < count / 2; i++)
        {
          gchar tmp = data[i];
          data[i] = data[count - 1 - i];
          data[count - 1 - i] = tmp;
        }
    }

  return TRUE;
}

gboolean
psd_read_int16 (GInputStream  *input,
                gint16        *data,
                gboolean       ibm_pc_format,
                GError       **error)
{
  if (psd_read (input, data, sizeof (gint16), error) < sizeof (gint16))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GINT16_FROM_LE (*data);
  else
    *data = GINT16_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_uint16 (GInputStream  *input,
                 guint16        *data,
                 gboolean       ibm_pc_format,
                 GError       **error)
{
  if (psd_read (input, data, sizeof (guint16), error) < sizeof (guint16))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GUINT16_FROM_LE (*data);
  else
    *data = GUINT16_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_int32 (GInputStream  *input,
                gint32        *data,
                gboolean       ibm_pc_format,
                GError       **error)
{
  if (psd_read (input, data, sizeof (gint32), error) < sizeof (gint32))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GINT32_FROM_LE (*data);
  else
    *data = GINT32_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_uint32 (GInputStream  *input,
                 guint32        *data,
                 gboolean       ibm_pc_format,
                 GError       **error)
{
  if (psd_read (input, data, sizeof (guint32), error) < sizeof (guint32))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GUINT32_FROM_LE (*data);
  else
    *data = GUINT32_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_int64 (GInputStream  *input,
                gint64        *data,
                gboolean       ibm_pc_format,
                GError       **error)
{
  if (psd_read (input, data, sizeof (gint64), error) < sizeof (gint64))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GINT64_FROM_LE (*data);
  else
    *data = GINT64_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_uint64 (GInputStream  *input,
                 guint64        *data,
                 gboolean       ibm_pc_format,
                 GError       **error)
{
  if (psd_read (input, data, sizeof (guint64), error) < sizeof (guint64))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    *data = GUINT64_FROM_LE (*data);
  else
    *data = GUINT64_FROM_BE (*data);

  return TRUE;
}

gboolean
psd_read_fixed_float (GInputStream  *input,
                      gfloat        *data,
                      gboolean       ibm_pc_format,
                      GError       **error)
{
  guint16 temp[2];
  if (! psd_read_uint16 (input, &temp[0], ibm_pc_format, error) ||
      ! psd_read_uint16 (input, &temp[1], ibm_pc_format, error))
    {
      psd_set_error (error);
      return FALSE;
    }

  *data = FIXED_TO_FLOAT (temp[0], temp[1]);

  return TRUE;
}

gboolean
psd_read_len (GInputStream  *input,
              guint64       *data,
              gint           psd_version,
              gboolean       ibm_pc_format,
              GError       **error)
{
  gint block_len_size = (psd_version == 1 ? 4 : 8);

  if (psd_read (input, data, block_len_size, error) < block_len_size)
    {
      psd_set_error (error);
      return FALSE;
    }

  if (psd_version == 1)
    {
      if (! ibm_pc_format)
        *data = GUINT32_FROM_BE (*data);
      else
        *data = GUINT32_FROM_LE (*data);
    }
  else
    {
      if (! ibm_pc_format)
        *data = GUINT64_FROM_BE (*data);
      else
        *data = GUINT64_FROM_LE (*data);
    }

  return TRUE;
}

gboolean
psd_read_double (GInputStream  *input,
                 gdouble       *data,
                 gboolean       ibm_pc_format,
                 GError       **error)
{
  union conv_double {
    guint64 u64_tmp;
    gdouble double_tmp;
  } conv_double = {};

  if (psd_read (input, &conv_double, sizeof (guint64), error) < sizeof (guint64))
    {
      psd_set_error (error);
      return FALSE;
    }

  if (ibm_pc_format)
    conv_double.u64_tmp = GUINT64_FROM_LE (conv_double.u64_tmp);
  else
    conv_double.u64_tmp = GUINT64_FROM_BE (conv_double.u64_tmp);

  *data = conv_double.double_tmp;

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
  IFDBG(4) g_debug ("Pascal string length %d", len);

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

  str = g_malloc (len + 1);
  str[len] = '\0';
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

  IFDBG(4) g_debug ("Pascal string: %s, bytes_read: %d, bytes_written: %d",
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

  IFDBG(4) g_debug ("Unicode string length %d", len);

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

  utf16_str = g_try_malloc (len * 2);
  if (! utf16_str)
    {
      psd_set_error (error);
      return NULL;
    }

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

  IFDBG(4) g_debug ("Unicode string: %s, bytes_read: %d, bytes_written: %d",
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

gint
parse_descriptor (GInputStream  *input,
                  gboolean       ibm_pc_format,
                  JsonNode     **root_node,
                  GError       **error)
{
  JsonObject *obj = NULL;
  gint        res = 0;

  *root_node = json_node_new (JSON_NODE_OBJECT);
  if (! *root_node)
    {
      /* FIXME: What error class would be most suitable here? */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error creating json node."));
      return -1;
    }
  obj = json_object_new ();
  json_node_init_object (*root_node, obj);

  /* FIXME: Should we add a version number to the root in case we
   *        need to make changes in the future?
   */

  res = load_descriptor (input, ibm_pc_format, root_node, error);
  if (res < 0)
    {
      json_node_free (*root_node);
      *root_node = NULL;
    }
  else
    {
      /* Set immutable for performance when reading the data. */
      json_node_seal (*root_node);
    }

  return res;
}

gint
load_descriptor (GInputStream  *input,
                 gboolean       ibm_pc_format,
                 JsonNode     **base_node,
                 GError       **error)
{
  gchar      *uniqueID       = NULL;
  gchar      *classID_string = NULL;
  guint32     num_items      = 0;
  gint32      bread, bwritten;
  gint        i;
  JsonObject *obj            = NULL;
  JsonArray  *arr            = NULL;
  JsonNode   *root           = NULL;

  IFDBG(3) g_debug ("start load_descriptor - Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  /* Read unicode string */
  uniqueID = fread_unicode_string (&bread, &bwritten, 1, ibm_pc_format,
                                   input, error);
  /* uniqueID seems to always be empty at the root */

  if (! uniqueID)
    {
      g_warning ("uniqueID is NULL! (or an error occurred)");
      psd_set_error (error);
      return -1;
    }

  IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  classID_string = load_key (input, error);
  /* root class ID seems to always be 'null' */
  IFDBG(3) g_debug ("Unique ID: %s, Class ID: %s", uniqueID, classID_string);
  g_free (uniqueID);

  /* Initialize our json objects used to store the collected information. */
  if (base_node == NULL || *base_node == NULL)
    {
      JsonNode *local = NULL;

      local = json_node_new (JSON_NODE_OBJECT);
      obj = json_object_new ();
      json_node_init_object (local, obj);
      if (base_node)
        *base_node = local;
    }
  else
    {
      root = *base_node;
      obj = json_node_get_object (root);
    }

  json_object_set_string_member (obj, "classID", classID_string);

  IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));
  if (psd_read (input, &num_items, 4, error) < 4)
    {
      psd_set_error (error);
      g_free (classID_string);
      return -1;
    }
  num_items = (ibm_pc_format ? GUINT32_FROM_LE (num_items) : GUINT32_FROM_BE (num_items));
  IFDBG(3) g_debug ("Number of items: %u", num_items);
  json_object_set_int_member (obj, "count", num_items);

  arr = json_array_new ();

  json_object_set_array_member (obj, "descriptor", arr);

  for (i = 0; i < num_items; i++)
    {
      gchar    *key;
      gchar     type[4];
      gint      res;
      JsonNode *node;

      IFDBG(4) g_debug ("Offset: %" G_GOFFSET_FORMAT, PSD_TELL(input));

      key = load_key (input, error);
      if (! key)
        {
          g_free (classID_string);
          return -1;
        }
      else if (psd_read (input, &type, 4, error) < 4)
        {
          psd_set_error (error);
          g_free (classID_string);
          return -1;
        }
      IFDBG(3) g_debug ("Item: %d, key: %s type: %.4s", i, key, type);

      node = NULL;
      res = load_type (input, ibm_pc_format, classID_string, key, type, &node, error);
      if (node)
        json_array_add_element (arr, node);
      else
        g_warning ("Missing node for key %s!", key);

      g_free (key);
      if (res < 0)
        return res;
    }

  IFDBG(4)
    if (root)
      g_debug ("Text data Json:\n%s\n", json_to_string (root, TRUE));

  g_free (classID_string);

  return 0;
}

static gchar *
load_key (GInputStream  *input,
          GError       **error)
{
  guint32  len = 0;
  gchar   *key;

  if (psd_read (input, &len, 4, error) < 4)
    {
      psd_set_error (error);
      return NULL;
    }
  len = GUINT32_FROM_BE (len);

  if (len == 0)
    {
      /* pre-defined key is always 4 bytes */
      len = 4;
    }

  /* We can't use fread_pascal_string since that expects a 1-byte length
     instead of 4-bytes. */
  key = g_malloc0 (len+1);
  if (! key)
    return NULL;

  if (psd_read (input, key, len, error) < len)
    {
      psd_set_error (error);
      g_free (key);
      return NULL;
    }
  key [len] = '\0';

  return key;
}

static JsonNode*
add_descriptor_float (const gchar  *key,
                      const gchar  *type,
                      const gchar  *float_type,
                      gfloat        value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  /* These values are not NULL terminated */
  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);
  tmp = g_strdup_printf ("%.4s", float_type);
  json_object_set_string_member (obj, "float_name", tmp);
  g_free (tmp);

  json_object_set_double_member (obj, "value", value);

  return local;
}

gint
parse_tdta (guint32    len,
            gchar     *buf,
            JsonNode **tdta_root,
            GError   **error)
{
  JsonObject *obj      = NULL;
  JsonObject *desc_obj = NULL;
  JsonNode   *root     = NULL;
  JsonNode   *desc     = NULL;
  gint        res;

  /* Initialize our json objects used to store the collected information. */
  if (tdta_root == NULL || *tdta_root == NULL)
    {
      JsonNode *local = json_node_new (JSON_NODE_OBJECT);

      obj = json_object_new ();
      json_node_init_object (local, obj);
      if (tdta_root)
        *tdta_root = local;
    }
  else
    {
      root = *tdta_root;
      obj  = json_node_get_object (root);
    }

  desc_obj = json_object_new ();
  desc     = json_node_new (JSON_NODE_OBJECT);
  json_node_init_object (desc, desc_obj);
  json_object_set_object_member (obj, "descriptor", desc_obj);

  res = parse_text_info (len, buf, &desc, error);
  if (res < 0)
    json_node_free (desc);

  return res;
}

static gint
parse_text_info (guint32    len,
                 gchar     *buf,
                 JsonNode **tdta_root,
                 GError   **error)
{
  ParserData  data     = {};
  JsonObject *obj      = NULL;
  JsonObject *desc_obj = NULL;
  JsonNode   *root     = NULL;
  JsonNode   *desc     = NULL;

  if (tdta_root == NULL || *tdta_root == NULL)
    {
      g_critical (_("Invalid root node!"));
      return -1;
    }
  root = *tdta_root;

  obj  = json_node_get_object (root);
  json_object_set_string_member (obj, "classID", "textdata");

  desc_obj = json_object_new ();
  desc     = json_node_new (JSON_NODE_OBJECT);
  json_node_init_object (desc, desc_obj);
  json_object_set_object_member (obj, "descriptor", desc_obj);

  IFDBG(3) g_debug ("Parsing text info...");

  data.reading_binary = FALSE;
  data.bufpos         = 0;

  parse_text_loop (len, buf, &desc, &data, error);

  g_free (data.last_cmd);

  IFDBG(4)
    if (root)
      g_debug ("Text data Json:\n%s\n", json_to_string (root, TRUE));

  return data.bufpos;
}

static gint
parse_text_loop (guint32      len,
                 gchar       *buf,
                 JsonNode   **node,
                 ParserData  *data,
                 GError     **error)
{
  JsonObject *desc_obj    = NULL;
  JsonArray  *arr         = NULL;
  JsonNode   *arr_node    = NULL;

  JsonNode   *member_node = NULL;
  JsonObject *member_obj  = NULL;

  gboolean    in_array    = FALSE;

#define COMMAND_SIZE 128 /* No idea what the real Photoshop maximum is. */
  gchar       command[COMMAND_SIZE + 1];
  gchar       simple_value[COMMAND_SIZE + 1];

  if (! JSON_NODE_HOLDS_OBJECT (*node))
    {
      g_critical ("Object node expected!");
      return -1;
    }

  desc_obj  = json_node_get_object (*node);

  while (data->bufpos < len)
    {
      gchar token;

      token = buf[data->bufpos];

      if (data->reading_binary)
        {
          gchar     nexttoken = ' ';
          gboolean  endbinary = FALSE;

          /* reading 16-bit values unless token == ) */
          if (token == ')' && data->bufpos + 1 < len)
            {
              nexttoken = buf[data->bufpos+1];
              if (nexttoken == ' '  ||
                  nexttoken == 0x09 ||
                  nexttoken == 0x0a)
                endbinary = TRUE;
            }

          if (! endbinary)
            {
              data->bufpos++;
              continue;
            }
        }

      switch (token)
        {
        case '(': /* start of binary value */
          data->reading_binary  = TRUE;
          data->value_start_pos = data->bufpos + 1;
          IFDBG(4) g_debug ("Binary value start");
          break;

        case ')': /* end of binary value */
          {
            gchar  *name_utf16;
            gchar  *name_utf8;
            GError *error = NULL;
            gsize   bw, br;

            data->reading_binary = FALSE;
            data->val_len = data->bufpos - data->value_start_pos;

            /* Get the name as UTF-8 from UTF-16 data. */
            name_utf16 = g_malloc0 (data->val_len + 2);
            memcpy (name_utf16, &buf[data->value_start_pos+2], data->val_len-2);
            name_utf8 = g_convert (name_utf16, data->val_len, "utf-8", "utf-16BE", &br, &bw, &error);

            if (error)
              {
                g_warning ("Error converting utf-16 to utf-8: %s", error->message);
                g_clear_error (&error);
              }
            IFDBG(3) g_debug ("Text value: '%s'", name_utf8);

            json_object_set_string_member (member_obj, "type", "string");
            json_object_set_string_member (member_obj, "value", name_utf8);

            g_free (name_utf16);
            g_free (name_utf8);

            IFDBG(4) g_debug ("Binary value end, value length: %u bytes", data->val_len);
          }
          break;

        case '/': /* start of command */
          data->reading_command = TRUE;
          data->cmdpos          = 0;
          break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case '.':
          if (data->reading_command)
            {
              if (token == '-' || token == '.')
                {
                  IFDBG(3) g_debug ("Unexpected token %c in command!", token);
                }
              else if (data->cmdpos < COMMAND_SIZE)
                {
                  command[data->cmdpos++] = token;
                }
              else
                {
                  g_critical ("Text command token too long!");
                }
            }
          else if (! data->reading_binary)
            {
              if (! data->reading_value)
                {
                  data->reading_value = TRUE;
                  data->val_pos       = 0;
                }
              if (data->val_pos < COMMAND_SIZE)
                {
                  simple_value[data->val_pos++] = token;
                }
              else
                {
                  g_critical ("Text value token too long!");
                }
            }
          break;

        case ' ':
        case 0x09: /* TAB character */
        case 0x0a: /* newline character */
          if (data->reading_command)
            {
              command[data->cmdpos] = '\0';
              data->cmdpos          = 0;
              data->reading_command = FALSE;
              g_free (data->last_cmd);
              data->last_cmd = g_strdup ((const gchar *) &command);

              member_node = json_node_new (JSON_NODE_OBJECT);
              member_obj  = json_object_new ();
              json_node_init_object (member_node, member_obj);
              json_object_set_object_member (desc_obj, command, member_obj);

              IFDBG(3) g_debug ("Command: %s at level %d", command, data->lvl);

              json_object_set_string_member (member_obj, "key", command);
              IFDBG(4)
                json_object_set_int_member (member_obj, "level", data->lvl);
            }
          else if (data->reading_value)
            {
              simple_value[data->val_pos] = '\0';
              data->val_pos = 0;
              data->reading_value = FALSE;
              IFDBG(3) g_debug ("Value: %s", simple_value);

              if (! in_array)
                {
                  if (member_obj == NULL)
                    g_critical ("Cannot add value: member object is NULL!");

                  json_object_set_string_member (member_obj, "type", "string");
                  json_object_set_string_member (member_obj, "value", simple_value);
                }
              else
                {
                  json_array_add_string_element (arr, simple_value);
                }
            }
          break;

        case '<': /* Increase level */
          if (data->bufpos + 1 < len && buf[data->bufpos + 1] == '<')
            {
              JsonNode   *new_node   = NULL;
              JsonObject *new_obj    = NULL;
              JsonObject *parent_obj = NULL;

              if (member_obj)
                {
                  parent_obj = desc_obj;
                  desc_obj   = member_obj;
                  member_obj = NULL;
                }

              if (desc_obj == NULL)
                g_critical ("Increasing level: descriptor object is NULL!");

              json_object_set_string_member (desc_obj, "type", "Objc");

              new_obj  = json_object_new ();
              new_node = json_node_new (JSON_NODE_OBJECT);
              json_node_init_object (new_node, new_obj);

              if (! in_array)
                json_object_set_object_member (desc_obj, "descriptor", new_obj);
              else
                json_array_add_object_element (arr, new_obj);

              data->lvl++;
              data->bufpos++;
              if (data->bufpos < len)
                {
                  data->bufpos++;

                  parse_text_loop (len, buf, &new_node, data, error);
                }
              if (parent_obj)
                desc_obj = parent_obj;
            }
          break;

        case '>': /* Decrease level */
          if (data->bufpos + 1 < len && buf[data->bufpos+1] == '>')
            {
              data->lvl--;
              data->bufpos++;

              return data->bufpos;
            }
          break;

        case '[': /* begin array */
          data->ar_lvl++;
          IFDBG(3) g_debug ("Array start, level %d", data->ar_lvl);
          json_object_set_string_member (member_obj, "type", "array");

          arr      = json_array_new ();
          arr_node = json_node_new (JSON_NODE_ARRAY);
          json_node_init_array (arr_node, arr);
          json_object_set_array_member (member_obj, "values", arr);
          in_array = TRUE;
          break;

        case ']': /* end array */
          data->ar_lvl--;
          IFDBG(3) g_debug ("Array end, level %d", data->ar_lvl);
          in_array = FALSE;
          break;

        case (gchar) 0xfe: /* feff is UTF-16 BOM */
          if (data->reading_command)
            {
              if (data->bufpos + 1 < len && buf[data->bufpos+1] == (gchar) 0xff)
                {
                  data->bufpos++;
                  IFDBG(3) g_debug ("Value is using UTF-16");
                }
            }
          break;

        default:
          if (data->reading_command)
            {
              command[data->cmdpos] = '\0';
              data->cmdpos = 0;
              g_free (data->last_cmd);
              data->last_cmd = g_strdup ((const gchar *) &command);
              data->reading_command = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Command: %s at level %d", command, data->lvl);
              g_warning ("Unexpected token. Command: %s", command);
            }
          else if (data->reading_value)
            {
              simple_value[data->val_pos] = '\0';
              data->val_pos = 0;
              data->reading_value = FALSE;
              IFDBG(2) g_debug ("--> Error: unexpected token! Value: %s", simple_value);
              g_warning ("Unexpected token value.");
            }
          break;
        }
      data->bufpos++;
    }

#undef COMMAND_SIZE

  /* Check here for conditions that could indicate a broken image or
   * an issue with our parser. */
  if (in_array)
    g_warning ("Unclosed array: loading may be incorrect!");
  if (data->reading_command)
    g_warning ("Incomplete command: loading may be incorrect!");
  if (data->reading_value)
    g_warning ("Incomplete value: loading may be incorrect!");

  return data->bufpos;
}

static JsonNode*
add_descriptor_double (const gchar  *key,
                       const gchar  *type,
                       gdouble       value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  /* These values are not NULL terminated */
  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_double_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_bool (const gchar  *key,
                     const gchar  *type,
                     gboolean      value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_boolean_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_comp (const gchar  *key,
                     const gchar  *type,
                     gint64        value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_int_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_long (const gchar  *key,
                     const gchar  *type,
                     gint32        value)
{
  return add_descriptor_comp (key, type, value);
}

static JsonNode*
add_descriptor_enum (const gchar  *key,
                     const gchar  *type,
                     const gchar  *name,
                     const gchar  *value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_string_member (obj, "enum_name",   name);

  json_object_set_string_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_text (const gchar  *key,
                     const gchar  *type,
                     const gchar  *name,
                     const gchar  *value)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  json_object_set_string_member (obj, "text_name", name);

  json_object_set_string_member (obj, "value", value);

  return local;
}

static JsonNode*
add_descriptor_objc (const gchar  *key,
                     const gchar  *type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static JsonNode*
add_descriptor_vlls (const gchar  *key,
                     const gchar  *type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static JsonNode*
add_descriptor_float_list (const gchar  *key,
                           const gchar  *type,
                           const gchar  *float_type)
{
  JsonNode   *local;
  JsonObject *obj;
  gchar      *tmp;

  local = json_node_new (JSON_NODE_OBJECT);
  obj = json_object_new ();
  json_node_init_object (local, obj);

  json_object_set_string_member (obj, "key",   key);

  tmp = g_strdup_printf ("%.4s", type);
  json_object_set_string_member (obj, "type",  tmp);
  g_free (tmp);

  return local;
}

static gint
load_type (GInputStream  *input,
           gboolean       ibm_pc_format,
           const gchar   *class_id,
           const gchar   *key,
           const gchar   *type,
           JsonNode	    **node,
           GError       **error)
{
  IFDBG(4) g_debug ("Loading type %.4s for key %s", type, key);

  if (memcmp (type, "obj ", 4) == 0)
    {
      /* Reference structure*/
      IFDBG(3) g_debug ("ob 'reference structure'");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "Objc", 4) == 0)
    {
      /* Descriptor structure*/
      gint      res;
      JsonNode *desc_node = NULL;

      IFDBG(3) g_debug ("Objc Descriptor begin");
      desc_node = add_descriptor_objc (key, type);
      res = load_descriptor (input, ibm_pc_format, &desc_node, error);
      *node = desc_node;
      IFDBG(3) g_debug ("Objc Descriptor end");
      if (res < 0)
        return res;
    }
  else if (memcmp (type, "VlLs", 4) == 0)
    {
      /* List */
      gint32       list_items = 0;
      gint32       li;
      JsonArray   *arr;

      if (psd_read (input, &list_items, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      list_items = (ibm_pc_format ? GUINT32_FROM_LE (list_items) : GUINT32_FROM_BE (list_items));
      IFDBG(3) g_debug ("Number of items in list: %i", list_items);

      *node = add_descriptor_vlls (key, type);
      arr = json_array_new ();
      json_object_set_array_member (json_node_get_object (*node), "list", arr);

      for (li = 0; li < list_items; li++)
        {
          gchar     type[4];
          gint      res;
          JsonNode *list_node;

          if (psd_read (input, &type, 4, error) < 4)
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("Item: %d, type: %.4s", li, type);

          list_node = NULL;
          res = load_type (input, ibm_pc_format, class_id, key, type, &list_node, error);
          if (res < 0)
            return res;

          if (list_node)
            json_array_add_element (arr, list_node);
          else
            g_debug ("Unable to add array element!");
        }
    }
  else if (memcmp (type, "doub", 4) == 0)
    {
      gdouble data;

      if (! psd_read_double (input, &data, ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("double value: %f", data);
      *node = add_descriptor_double (key, type, data);
    }
  else if (memcmp (type, "UntF", 4) == 0)
    {
      /* Unit float */
      gchar   floatkey[4] = "";
      gdouble data;

      if (psd_read (input, &floatkey, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
        if (! psd_read_double (input, &data, ibm_pc_format, error))
          {
            psd_set_error (error);
            return -1;
          }

          IFDBG(3) g_debug ("Float: %.4s, value: %f", floatkey, data);
      *node = add_descriptor_float (key, type, floatkey, data);
    }
  else if (memcmp (type, "UnFl ", 4) == 0)
    {
      /* Undocumented: Unit Floats */
      gchar      floatkey[4] = "";
      guint32    count       = 0;
      gint       i;
      JsonArray *arr;

      if (psd_read (input, &floatkey, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      if (psd_read (input, &count, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      count = (ibm_pc_format ? GUINT32_FROM_LE (count) : GUINT32_FROM_BE (count));
      IFDBG(3) g_debug ("Array[%u] of float of type: %.4s", count, floatkey);

      *node = add_descriptor_float_list (key, type, floatkey);
      arr = json_array_new ();
      json_object_set_array_member (json_node_get_object (*node), "list", arr);

      for (i = 0; i < count; i++)
        {
          gdouble   data;
          JsonNode *list_node;

          if (! psd_read_double (input, &data, ibm_pc_format, error))
            {
              psd_set_error (error);
              return -1;
            }
          IFDBG(3) g_debug ("[%i] - value: %f", i, data);

          list_node = add_descriptor_double (key, "doub", data);
          if (list_node)
            json_array_add_element (arr, list_node);
          else
            g_debug ("Unable to add array element!");
        }
    }
  else if (memcmp (type, "TEXT", 4) == 0)
    {
      /* String */
      gchar  *str;
      gint32  bread, bwritten;

      str = fread_unicode_string (&bread, &bwritten, 1, ibm_pc_format,
                                  input, error);
      if (! str)
        {
          psd_set_error (error);
          return -1;
        }
      IFDBG(3) g_debug ("string value: %s (class id: '%s', key: '%s')", str, class_id, key);
      *node = add_descriptor_text (key, type, class_id, str);

      g_free (str);
    }
  else if (memcmp (type, "enum", 4) == 0)
    {
      /* Enumerated descriptor */
      gchar  *nameID, *valueID;

      nameID  = load_key (input, error);
      valueID = load_key (input, error);
      /* FIXME: Error handling */

      IFDBG(3) g_debug ("Enum name: %s, value: %s",
                        nameID, valueID);
      *node = add_descriptor_enum (key, type, nameID, valueID);

      g_free (nameID);
      g_free (valueID);
    }
  else if (memcmp (type, "long", 4) == 0)
    {
      /* Integer */
      gint32 psdlong = 0;

      if (psd_read (input, &psdlong, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      psdlong = (ibm_pc_format ? GUINT32_FROM_LE (psdlong) : GUINT32_FROM_BE (psdlong));
      IFDBG(3) g_debug ("long value: %d", psdlong);
      *node = add_descriptor_long (key, type, psdlong);
    }
  else if (memcmp (type, "comp", 4) == 0)
    {
      /* Large Integer */
      gint64 psdlarge = 0;

      if (psd_read (input, &psdlarge, 8, error) < 8)
        {
          psd_set_error (error);
          return -1;
        }
      psdlarge = (ibm_pc_format ? GUINT32_FROM_LE (psdlarge) : GUINT32_FROM_BE (psdlarge));
      IFDBG(3) g_debug ("large value: %" G_GINT64_FORMAT, psdlarge);
      *node = add_descriptor_comp (key, type, psdlarge);
    }
  else if (memcmp (type, "bool", 4) == 0)
    {
      /* Boolean */
      gboolean val = FALSE;

      if (psd_read (input, &val, 1, error) < 1)
        {
          psd_set_error (error);
          return -1;
        }
      IFDBG(3) g_debug ("Boolean value: %u", (gboolean) val);
      *node = add_descriptor_bool (key, type, val);
    }
  else if (memcmp (type, "GlbO", 4) == 0)
    {
      /* GlobalObject same as Descriptor */
      IFDBG(3) g_debug ("GlbO GlobalObject same as Descriptor");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "type", 4) == 0)
    {
      /* Class */
      IFDBG(3) g_debug ("type (Class)");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "GlbC", 4) == 0)
    {
      /* Class */
      IFDBG(3) g_debug ("GlbC (global? Class)");
      g_message ("FIXME Type: %.4s", type);
    }
  else if (memcmp (type, "alis", 4) == 0)
    {
      /* Alias */
      guint32  size = 0;
      goffset  ofs;

      /* Used in placedLayer.psd in 'lnkE' resource */
      /* According to the specs:
       * FSSpec data structure for Macintosh, or a handle to a string to the
       * full path on Windows.
       * At this time it doesn't look like we will need this data.
       */
      IFDBG(3) g_debug ("alis (Alias)");

      if (psd_read (input, &size, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      size = (ibm_pc_format ? GUINT32_FROM_LE (size) : GUINT32_FROM_BE (size));
      ofs  = PSD_TELL(input) + size;

      IFDBG(4) g_debug ("Size of alis: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);

      if (! psd_seek (input, ofs, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
    }
  else if (memcmp (type, "tdta", 4) == 0)
    {
      /* Raw Data */
      guint32   size      = 0;
      goffset   ofs;
      gint      res;
      gchar    *buf       = NULL;
      JsonNode *tdta_node = NULL;

      if (psd_read (input, &size, 4, error) < 4)
        {
          psd_set_error (error);
          return -1;
        }
      size = (ibm_pc_format ? GUINT32_FROM_LE (size) : GUINT32_FROM_BE (size));
      ofs  = PSD_TELL(input) + size;

      IFDBG(3) g_debug ("Size of tdta: %u, ending offset: %" G_GOFFSET_FORMAT, size, ofs);
      buf = g_malloc (size);
      if (psd_read (input, buf, size, error) < size)
        {
          g_warning (_("Could not read enough bytes!"));
          psd_set_error (error);
          return -1;
        }

      IFDBG(3) g_debug ("tdta begin");
      tdta_node = add_descriptor_objc (key, type);

      res = parse_tdta (size, buf, &tdta_node, error);
      if (res < 0)
        return -1;
      else
        *node = tdta_node;
      IFDBG(3) g_debug ("tdta end");

      g_free (buf);
      if (! psd_seek (input, ofs, G_SEEK_SET, error))
        {
          psd_set_error (error);
          return -1;
        }
    }
  else if (memcmp (type, "ObAr", 4) == 0)
    {
      /* Undocumented: Object Array */
      gint32    res;
      guint32   desc_ver  = 0;
      JsonNode *desc_node = NULL;

      if (! psd_read_uint32 (input, &desc_ver, ibm_pc_format, error))
        {
          psd_set_error (error);
          return -1;
        }


      IFDBG(4) g_debug ("Descriptor version: %u", desc_ver);

      /* Not sure what the difference is with Objc, except the extra
       * descriptor version. It doesn't seem like there is an array of
       * descriptors.
       */
      IFDBG(3) g_debug ("Descriptor array begin");
      desc_node = add_descriptor_objc (key, type);
      res = load_descriptor (input, ibm_pc_format, &desc_node, error);
      *node = desc_node;
      IFDBG(3) g_debug ("Descriptor array end");
      if (res < 0)
        return res;
    }
  else if (memcmp (type, "Pth ", 4) == 0)
    {
      /* Undocumented: Path */
      IFDBG(3) g_debug ("Pth (Path, undocumented)");
      g_message ("FIXME Type: %.4s", type);
    }
  else
    {
      /* Unknown/undocumented Photoshop descriptor type */
      IFDBG(3) g_debug ("Unknown or undocumented type %s", type);
      g_warning ("Unknown or undocumented type %s", type);
      g_message ("FIXME Type: %.4s", type);
      return -2;
    }

  return 0;
}

void
psd_to_gimp_blend_mode (PSDlayer      *psd_layer,
                        LayerModeInfo *mode_info)
{
  mode_info->mode  = GIMP_LAYER_MODE_NORMAL;
  /* FIXME: use the image mode to select the correct color spaces.  for now,
   * we use rgb-perceptual blending/compositing unconditionally.
   */
  mode_info->blend_space     = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL;
  mode_info->composite_space = GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL;
  if (psd_layer->clipping == 1)
    mode_info->composite_mode  = GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP;
  else
    mode_info->composite_mode  = GIMP_LAYER_COMPOSITE_UNION;

  if (! convert_psd_mode (psd_layer->blend_mode, &mode_info->mode))
    {
      if (CONVERSION_WARNINGS)
        {
          gchar *mode_name = g_strndup (psd_layer->blend_mode, 4);
          g_message ("Unsupported blend mode: %s. Mode reverts to normal",
                     mode_name);
          g_free (mode_name);
        }
    }
}

gboolean
convert_psd_mode (const gchar   *psd_mode,
                  GimpLayerMode *mode)
{
  *mode = GIMP_LAYER_MODE_NORMAL;

  for (gint i = 0; i < G_N_ELEMENTS (layer_mode_map); i++)
    {
      if (g_ascii_strncasecmp (psd_mode, layer_mode_map[i].psd_mode, 4) == 0)
        {
          if (! layer_mode_map[i].exact && CONVERSION_WARNINGS)
            {
              g_message ("GIMP uses a different equation than Photoshop for "
                         "blend mode: %s. Results will differ.",
                         layer_mode_map[i].name);
            }

          *mode = layer_mode_map[i].gimp_mode;
          return TRUE;
        }
    }

  return FALSE;
}

gboolean
convert_psd_effect_mode (const gchar   *psd_mode,
                         GimpLayerMode *mode)
{
  *mode = GIMP_LAYER_MODE_REPLACE;

  for (gint i = 0; i < G_N_ELEMENTS (descriptor_mode_map); i++)
    {
      if (g_ascii_strncasecmp (psd_mode, descriptor_mode_map[i].psd_mode, 4) == 0)
        {
          if (! descriptor_mode_map[i].exact && CONVERSION_WARNINGS)
            {
              g_message ("GIMP uses a different equation than Photoshop for "
                         "blend mode: %s. Results will differ.",
                         descriptor_mode_map[i].name);
            }

          *mode = descriptor_mode_map[i].gimp_mode;
          return TRUE;
        }
    }

  return FALSE;
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
