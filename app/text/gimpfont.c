/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfont.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#define PANGO_ENABLE_BACKEND 1   /* Argh */
#include <pango/pangoft2.h>

#define PANGO_ENABLE_ENGINE  1   /* Argh */
#include <pango/pango-ot.h>
#include <freetype/tttables.h>

#include "text-types.h"

#include "base/temp-buf.h"

#include "gimpfont.h"

#include "gimp-intl.h"


/* This is a so-called pangram; it's supposed to
   contain all characters found in the alphabet. */
#define GIMP_TEXT_PANGRAM     N_("Pack my box with\nfive dozen liquor jugs.")

#define GIMP_FONT_POPUP_SIZE  (PANGO_SCALE * 30)

#define DEBUGPRINT(x) /* g_print x */

enum
{
  PROP_0,
  PROP_PANGO_CONTEXT
};


struct _GimpFont
{
  GimpViewable  parent_instance;

  PangoContext *pango_context;

  PangoLayout  *popup_layout;
  gint          popup_width;
  gint          popup_height;
};

struct _GimpFontClass
{
  GimpViewableClass   parent_class;
};


static void      gimp_font_finalize         (GObject       *object);
static void      gimp_font_set_property     (GObject       *object,
                                             guint          property_id,
                                             const GValue  *value,
                                             GParamSpec    *pspec);

static void      gimp_font_get_preview_size (GimpViewable  *viewable,
                                             gint           size,
                                             gboolean       popup,
                                             gboolean       dot_for_dot,
                                             gint          *width,
                                             gint          *height);
static gboolean  gimp_font_get_popup_size   (GimpViewable  *viewable,
                                             gint           width,
                                             gint           height,
                                             gboolean       dot_for_dot,
                                             gint          *popup_width,
                                             gint          *popup_height);
static TempBuf * gimp_font_get_new_preview  (GimpViewable  *viewable,
                                             GimpContext   *context,
                                             gint           width,
                                             gint           height);

static const gchar * gimp_font_get_sample_string (PangoContext         *context,
                                                  PangoFontDescription *font_desc);


G_DEFINE_TYPE (GimpFont, gimp_font, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_font_parent_class


static void
gimp_font_class_init (GimpFontClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize     = gimp_font_finalize;
  object_class->set_property = gimp_font_set_property;

  viewable_class->get_preview_size = gimp_font_get_preview_size;
  viewable_class->get_popup_size   = gimp_font_get_popup_size;
  viewable_class->get_new_preview  = gimp_font_get_new_preview;

  viewable_class->default_stock_id = "gtk-select-font";

  g_object_class_install_property (object_class, PROP_PANGO_CONTEXT,
                                   g_param_spec_object ("pango-context",
                                                        NULL, NULL,
                                                        PANGO_TYPE_CONTEXT,
                                                        GIMP_PARAM_WRITABLE));
}

static void
gimp_font_init (GimpFont *font)
{
  font->pango_context = NULL;
}

static void
gimp_font_finalize (GObject *object)
{
  GimpFont *font = GIMP_FONT (object);

  if (font->pango_context)
    {
      g_object_unref (font->pango_context);
      font->pango_context = NULL;
    }

  if (font->popup_layout)
    {
      g_object_unref (font->popup_layout);
      font->popup_layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_font_set_property (GObject       *object,
                        guint          property_id,
                        const GValue  *value,
                        GParamSpec    *pspec)
{
  GimpFont *font = GIMP_FONT (object);

  switch (property_id)
    {
    case PROP_PANGO_CONTEXT:
      if (font->pango_context)
        g_object_unref (font->pango_context);
      font->pango_context = (PangoContext *) g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_font_get_preview_size (GimpViewable *viewable,
                            gint          size,
                            gboolean      popup,
                            gboolean      dot_for_dot,
                            gint         *width,
                            gint         *height)
{
  *width  = size;
  *height = size;
}

static gboolean
gimp_font_get_popup_size (GimpViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  GimpFont             *font = GIMP_FONT (viewable);
  PangoFontDescription *font_desc;
  PangoRectangle        ink;
  PangoRectangle        logical;
  const gchar          *name;

  if (! font->pango_context)
    return FALSE;

  name = gimp_object_get_name (GIMP_OBJECT (font));

  font_desc = pango_font_description_from_string (name);
  g_return_val_if_fail (font_desc != NULL, FALSE);

  pango_font_description_set_size (font_desc, GIMP_FONT_POPUP_SIZE);

  if (font->popup_layout)
    g_object_unref (font->popup_layout);

  font->popup_layout = pango_layout_new (font->pango_context);
  pango_layout_set_font_description (font->popup_layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (font->popup_layout, gettext (GIMP_TEXT_PANGRAM), -1);
  pango_layout_get_pixel_extents (font->popup_layout, &ink, &logical);

  *popup_width  = MAX (ink.width,  logical.width)  + 6;
  *popup_height = MAX (ink.height, logical.height) + 6;

  font->popup_width  = *popup_width;
  font->popup_height = *popup_height;

  return TRUE;
}

static TempBuf *
gimp_font_get_new_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpFont       *font = GIMP_FONT (viewable);
  PangoLayout    *layout;
  PangoRectangle  ink;
  PangoRectangle  logical;
  gint            layout_width;
  gint            layout_height;
  gint            layout_x;
  gint            layout_y;
  TempBuf        *temp_buf;
  FT_Bitmap       bitmap;
  guchar         *p;
  guchar          black = 0;

  if (! font->pango_context)
    return NULL;

  if (! font->popup_layout ||
      font->popup_width != width || font->popup_height != height)
    {
      PangoFontDescription *font_desc;
      const gchar          *name;

      name = gimp_object_get_name (GIMP_OBJECT (font));

      DEBUGPRINT (("%s: ", name));

      font_desc = pango_font_description_from_string (name);
      g_return_val_if_fail (font_desc != NULL, NULL);

      pango_font_description_set_size (font_desc,
                                       PANGO_SCALE * height * 2.0 / 3.0);

      layout = pango_layout_new (font->pango_context);

      pango_layout_set_font_description (layout, font_desc);
      pango_layout_set_text (layout,
                             gimp_font_get_sample_string (font->pango_context,
                                                          font_desc),
                             -1);

      pango_font_description_free (font_desc);
    }
  else
    {
      layout = g_object_ref (font->popup_layout);
    }

  temp_buf = temp_buf_new (width, height, 1, 0, 0, &black);

  bitmap.width  = temp_buf->width;
  bitmap.rows   = temp_buf->height;
  bitmap.pitch  = temp_buf->width;
  bitmap.buffer = temp_buf_data (temp_buf);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  layout_width  = MAX (ink.width,  logical.width);
  layout_height = MAX (ink.height, logical.height);

  layout_x = (bitmap.width - layout_width)  / 2;
  layout_y = (bitmap.rows  - layout_height) / 2;

  if (ink.x < logical.x)
    layout_x += logical.x - ink.x;

  if (ink.y < logical.y)
    layout_y += logical.y - ink.y;

  pango_ft2_render_layout (&bitmap, layout, layout_x, layout_y);

  g_object_unref (layout);

  p = temp_buf_data (temp_buf);

  for (height = temp_buf->height; height; height--)
    for (width = temp_buf->width; width; width--, p++)
      *p = 255 - *p;

  return temp_buf;
}

GimpFont *
gimp_font_get_standard (void)
{
  static GimpFont *standard_font = NULL;

  if (! standard_font)
    standard_font = g_object_new (GIMP_TYPE_FONT,
                                  "name", "Sans",
                                  NULL);

  return standard_font;
}


static inline gboolean
gimp_font_covers_string (PangoFcFont *font,
                         const gchar *sample)
{
  const gchar *p;

  for (p = sample; *p; p = g_utf8_next_char (p))
    {
      if (! pango_fc_font_has_char (font, g_utf8_get_char (p)))
        return FALSE;
    }

  return TRUE;
}

/* Guess a suitable short sample string for the font. */
static const gchar *
gimp_font_get_sample_string (PangoContext         *context,
                             PangoFontDescription *font_desc)
{
  PangoFont        *font;
  PangoOTInfo      *ot_info;
  FT_Face           face;
  TT_OS2           *os2;
  PangoOTTableType  tt;
  gint              i;

  /* This is a table of scripts and corresponding short sample strings
   * to be used instead of the Latin sample string Aa. The script
   * codes are as in ISO15924 (see
   * http://www.unicode.org/iso15924/iso15924-codes.html), but in
   * lower case. The Unicode subrange bit numbers, as used in TrueType
   * so-called OS/2 tables, are from
   * http://www.microsoft.com/typography/otspec/os2.htm#ur .
   *
   * The table is mostly ordered by Unicode order. But as there are
   * fonts that support several of these scripts, the ordering is
   * be modified so that the script which such a font is more likely
   * to be actually designed for comes first and matches.
   *
   * These sample strings are mostly just guesswork as for their
   * usefulness. Usually they contain what I assume is the first
   * letter in the corresponding alphabet, or two first letters if the
   * first one happens to look too "trivial" to be recognizable by
   * itself.
   *
   * This table is used to determine the primary script a font has
   * been designed for.
   *
   * Very useful link: http://www.travelphrases.info/fonts.html
   */
  static const struct
  {
    const gchar  script[4];
    gint         bit;
    const gchar *sample;
  } scripts[] = {
    /* Han is first because fonts that support it presumably are primarily
     * designed for it.
     */
    {
      "hani",                   /* Han Ideographic */
      59,
      "\346\260\270"            /* U+6C38 "forever". Ed Trager says
                                 * this is a "pan-stroke" often used
                                 * in teaching Chinese calligraphy. It
                                 * contains the eight basic Chinese
                                 * stroke forms.
                                 */
    },
    {
      "copt",                   /* Coptic */
      7,
      "\316\221\316\261"        /* U+0410 GREEK CAPITAL LETTER ALPHA
                                   U+0430 GREEK SMALL LETTER ALPHA
                                 */
    },
    {
      "grek",                   /* Greek */
      7,
      "\316\221\316\261"        /* U+0410 GREEK CAPITAL LETTER ALPHA
                                   U+0430 GREEK SMALL LETTER ALPHA
                                 */
    },
    {
      "cyrl",                   /* Cyrillic */
      9,
      "\320\220\325\260"        /* U+0410 CYRILLIC CAPITAL LETTER A
                                   U+0430 CYRILLIC SMALL LETTER A
                                 */
    },
    {
      "armn",                   /* Armenian */
      10,
      "\324\261\325\241"        /* U+0531 ARMENIAN CAPITAL LETTER AYB
                                   U+0561 ARMENIAN SMALL LETTER AYB
                                 */
    },
    {
      "hebr",                   /* Hebrew */
      11,
      "\327\220"                /* U+05D0 HEBREW LETTER ALEF */
    },
    {
      "arab",                   /* Arabic */
      13,
      "\330\247\330\250"        /* U+0627 ARABIC LETTER ALEF
                                 * U+0628 ARABIC LETTER BEH
                                 */
    },
    {
      "syrc",                   /* Syriac */
      71,
      "\334\220\334\222"        /* U+0710 SYRIAC LETTER ALAPH
                                 * U+0712 SYRIAC LETTER BETH
                                 */
    },
    {
      "thaa",                   /* Thaana */
      72,
      "\336\200\336\201"        /* U+0780 THAANA LETTER HAA
                                 * U+0781 THAANA LETTER SHAVIYANI
                                 */
    },
    /* Should really use some better sample strings for the complex
     * scripts. Something that shows how well the font handles the
     * complex processing for each script.
     */
    {
      "deva",                   /* Devanagari */
      15,
      "\340\244\205"            /* U+0905 DEVANAGARI LETTER A*/
    },
    {
      "beng",                   /* Bengali */
      16,
      "\340\246\205"            /* U+0985 BENGALI LETTER A */
    },
    {
      "guru",                   /* Gurmukhi */
      17,
      "\340\250\205"            /* U+0A05 GURMUKHI LETTER A */
    },
    {
      "gujr",                   /* Gujarati */
      18,
      "\340\252\205"            /* U+0A85 GUJARATI LETTER A */
    },
    {
      "orya",                   /* Oriya */
      19,
      "\340\254\205"            /* U+0B05 ORIYA LETTER A */
    },
    {
      "taml",                   /* Tamil */
      20,
      "\340\256\205"            /* U+0B85 TAMIL LETTER A */
    },
    {
      "telu",                   /* Telugu */
      21,
      "\340\260\205"            /* U+0C05 TELUGU LETTER A */
    },
    {
      "knda",                   /* Kannada */
      22,
      "\340\262\205"            /* U+0C85 KANNADA LETTER A */
    },
    {
      "mylm",                   /* Malayalam */
      23,
      "\340\264\205"            /* U+0D05 MALAYALAM LETTER A */
    },
    {
      "sinh",                   /* Sinhala */
      73,
      "\340\266\205"            /* U+0D85 SINHALA LETTER AYANNA */
    },
    {
      "thai",                   /* Thai */
      24,
      "\340\270\201\340\270\264"/* U+0E01 THAI CHARACTER KO KAI
                                 * U+0E34 THAI CHARACTER SARA I
                                 */
    },
    {
      "laoo",                   /* Lao */
      25,
      "\340\272\201\340\272\264"/* U+0E81 LAO LETTER KO
                                 * U+0EB4 LAO VOWEL SIGN I
                                 */
    },
    {
      "tibt",                   /* Tibetan */
      70,
      "\340\274\200"            /* U+0F00 TIBETAN SYLLABLE OM */
    },
    {
      "mymr",                   /* Myanmar */
      74,
      "\341\200\200"            /* U+1000 MYANMAR LETTER KA */
    },
    {
      "geor",                   /* Georgian */
      26,
      "\341\202\240\341\203\200" /* U+10A0 GEORGIAN CAPITAL LETTER AN
                                  * U+10D0 GEORGIAN LETTER AN
                                  */
    },
    {
      "hang",                   /* Hangul */
      28,
      "\341\204\200\341\204\201"/* U+1100 HANGUL CHOSEONG KIYEOK
                                 * U+1101 HANGUL CHOSEONG SSANGKIYEOK
                                 */
    },
    {
      "ethi",                   /* Ethiopic */
      75,
      "\341\210\200"            /* U+1200 ETHIOPIC SYLLABLE HA */
    },
    {
      "cher",                   /* Cherokee */
      76,
      "\341\216\243"            /* U+13A3 CHEROKEE LETTER O */
    },
    {
      "cans",                   /* Unified Canadian Aboriginal Syllabics */
      77,
      "\341\220\201"            /* U+1401 CANADIAN SYLLABICS E */
    },
    {
      "ogam",                   /* Ogham */
      78,
      "\341\232\201"            /* U+1681 OGHAM LETTER BEITH */
    },
    {
      "runr",                   /* Runic */
      79,
      "\341\232\240"            /* U+16A0 RUNIC LETTER FEHU FEOH FE F */
    },
    {
      "tglg",                   /* Tagalog */
      84,
      "\341\234\200"            /* U+1700 TAGALOG LETTER A */
    },
    {
      "hano",                   /* Hanunoo */
      -1,
      "\341\234\240"            /* U+1720 HANUNOO LETTER A */
    },
    {
      "buhd",                   /* Buhid */
      -1,
      "\341\235\200"            /* U+1740 BUHID LETTER A */
    },
    {
      "tagb",                   /* Tagbanwa */
      -1,
      "\341\235\240"            /* U+1760 TAGBANWA LETTER A */
    },
    {
      "khmr",                   /* Khmer */
      80,
      "\341\236\201\341\237\222\341\236\211\341\236\273\341\237\206"
                                /* U+1781 KHMER LETTER KHA
                                 * U+17D2 KHMER LETTER SIGN COENG
                                 * U+1789 KHMER LETTER NYO
                                 * U+17BB KHMER VOWEL SIGN U
                                 * U+17C6 KHMER SIGN NIKAHIT
                                 * A common word meaning "I" that contains
                                 * lots of complex processing.
                                 */
    },
    {
      "mong",                   /* Mongolian */
      81,
      "\341\240\240"            /* U+1820 MONGOLIAN LETTER A */
    },
    {
      "limb",                   /* Limbu */
      -1,
      "\341\244\201"            /* U+1901 LIMBU LETTER KA */
    },
    {
      "tale",                   /* Tai Le */
      -1,
      "\341\245\220"            /* U+1950 TAI LE LETTER KA */
    },
    {
      "latn",                   /* Latin */
      0,
      "Aa"
    }
  };

  gint ot_alts[4];
  gint n_ot_alts = 0;
  gint sr_alts[20];
  gint n_sr_alts = 0;

  font = pango_context_load_font (context, font_desc);

  g_return_val_if_fail (PANGO_IS_FC_FONT (font), "Aa");

  face = pango_fc_font_lock_face (PANGO_FC_FONT (font));
  ot_info = pango_ot_info_get (face);

  /* First check what script(s), if any, the font has GSUB or GPOS
   * OpenType layout tables for.
   */
  for (tt = PANGO_OT_TABLE_GSUB;
       n_ot_alts < G_N_ELEMENTS (ot_alts) && tt <= PANGO_OT_TABLE_GPOS;
       tt++)
    {
      PangoOTTag *slist = pango_ot_info_list_scripts (ot_info, tt);

      if (slist)
        {
          for (i = 0;
               n_ot_alts < G_N_ELEMENTS (ot_alts) && i < G_N_ELEMENTS (scripts);
               i++)
            {
              gint j, k;

              for (k = 0; k < n_ot_alts; k++)
                if (ot_alts[k] == i)
                  break;

              if (k == n_ot_alts)
                for (j = 0;
                     n_ot_alts < G_N_ELEMENTS (ot_alts) && slist[j];
                     j++)
                  {
#define TAG(s) FT_MAKE_TAG (s[0], s[1], s[2], s[3])

                    if (slist[j] == TAG (scripts[i].script) &&
                        gimp_font_covers_string (PANGO_FC_FONT (font),
                                                 scripts[i].sample))
                      {
                        ot_alts[n_ot_alts++] = i;
                        DEBUGPRINT (("%.4s ", scripts[i].script));
                      }
#undef TAG
                  }
            }

          g_free (slist);
        }
    }

  DEBUGPRINT (("; OS/2: "));

  /* Next check the OS/2 table for Unicode ranges the font claims
   * to cover.
   */

  os2 = FT_Get_Sfnt_Table (face, ft_sfnt_os2);

  if (os2)
    {
      for (i = 0;
           n_sr_alts < G_N_ELEMENTS (sr_alts) && i < G_N_ELEMENTS (scripts);
           i++)
        {
          if (scripts[i].bit >= 0 &&
              (&os2->ulUnicodeRange1)[scripts[i].bit/32] & (1 << (scripts[i].bit % 32)) &&
              gimp_font_covers_string (PANGO_FC_FONT (font),
                                       scripts[i].sample))
            {
              sr_alts[n_sr_alts++] = i;
              DEBUGPRINT (("%.4s ", scripts[i].script));
            }
        }
    }

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));

  g_object_unref (font);

  if (n_ot_alts > 2)
    {
      /* The font has OpenType tables for several scripts. If it
       * support Basic Latin as well, use Aa.
       */
      gint i;

      for (i = 0; i < n_sr_alts; i++)
        if (scripts[sr_alts[i]].bit == 0)
          {
            DEBUGPRINT (("=> several OT, also latin, use Aa\n"));
            return "Aa";
          }
    }

  if (n_ot_alts > 0 && n_sr_alts >= n_ot_alts + 3)
    {
      /* At least one script with an OpenType table, but many more
       * subranges than such scripts. If it supports Basic Latin,
       * use Aa, else the highest priority subrange.
       */
      gint i;

      for (i = 0; i < n_sr_alts; i++)
        if (scripts[sr_alts[i]].bit == 0)
          {
            DEBUGPRINT (("=> several SR, also latin, use Aa\n"));
            return "Aa";
          }

      DEBUGPRINT (("=> several SR, use %.4s\n", scripts[sr_alts[0]].script));
      return scripts[sr_alts[0]].sample;
    }

  if (n_ot_alts > 0)
    {
      /* OpenType tables for at least one script, use the
       * highest priority one
       */
      DEBUGPRINT (("=> at least one OT, use %.4s\n",
                   scripts[sr_alts[0]].script));
      return scripts[ot_alts[0]].sample;
    }

  if (n_sr_alts > 0)
    {
      /* Use the highest priority subrange.  This means that a
       * font that supports Greek, Cyrillic and Latin (quite
       * common), will get the Greek sample string. That is
       * capital and lowercase alpha, which looks like capital A
       * and lowercase alpha, so it's actually quite nice, and
       * doesn't give a too strong impression that the font would
       * be for Greek only.
       */
      DEBUGPRINT (("=> at least one SR, use %.4s\n",
                   scripts[sr_alts[0]].script));
     return scripts[sr_alts[0]].sample;
    }

  /* Final fallback */
  DEBUGPRINT (("=> fallback, use Aa\n"));
  return "Aa";
}
