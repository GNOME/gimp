/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfont.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include <pango/pangocairo.h>

#include <hb.h>
#include <hb-ot.h>
#include <hb-ft.h>

#define PANGO_ENABLE_ENGINE  1   /* Argh */
#include <pango/pango-ot.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "core/gimptempbuf.h"

#include "core/gimp-memsize.h"
#include "core/gimpcontainer.h"

#include "gimpfont.h"
#include "gimpfontfactory.h"

#include "gimp-intl.h"


/* This is a so-called pangram; it's supposed to
   contain all characters found in the alphabet. */
#define GIMP_TEXT_PANGRAM     N_("Pack my box with\nfive dozen liquor jugs.")

#define GIMP_FONT_POPUP_SIZE  (PANGO_SCALE * 30)

#define DEBUGPRINT(x) /* g_print x */

enum
{
  GIMP_FONT_SYMBOL_FONTHASH,
  GIMP_FONT_SYMBOL_FULLNAME,
  GIMP_FONT_SYMBOL_FAMILY,
  GIMP_FONT_SYMBOL_STYLE,
  GIMP_FONT_SYMBOL_PSNAME,
  GIMP_FONT_SYMBOL_INDEX,
  GIMP_FONT_SYMBOL_WEIGHT,
  GIMP_FONT_SYMBOL_SLANT,
  GIMP_FONT_SYMBOL_WIDTH,
  GIMP_FONT_SYMBOL_FONTVERSION
};

enum
{
  PROP_0,
  PROP_PANGO_CONTEXT
};


struct _GimpFont
{
  GimpData      parent_instance;

  PangoContext *pango_context;

  PangoLayout  *popup_layout;
  gint          popup_width;
  gint          popup_height;
  gchar        *lookup_name;

  /*properties for serialization*/
  gchar        *hash;
  gchar        *fullname;
  gchar        *family;
  gchar        *style;
  gchar        *psname;
  gint          weight;
  gint          width;
  gint          index;
  gint          slant;
  gint          fontversion;

  /*for backward compatibility*/
  gchar        *desc;
  gchar        *file_path;
};

struct _GimpFontClass
{
  GimpDataClass   parent_class;

  GimpContainer  *fonts_container;
};


static void          gimp_font_finalize           (GObject               *object);
static void          gimp_font_set_property       (GObject               *object,
                                                   guint                  property_id,
                                                   const GValue          *value,
                                                   GParamSpec            *pspec);

static void          gimp_font_get_preview_size   (GimpViewable          *viewable,
                                                   gint                   size,
                                                   gboolean               popup,
                                                   gboolean               dot_for_dot,
                                                   gint                  *width,
                                                   gint                  *height);
static gboolean      gimp_font_get_popup_size     (GimpViewable          *viewable,
                                                   gint                   width,
                                                   gint                   height,
                                                   gboolean               dot_for_dot,
                                                   gint                  *popup_width,
                                                   gint                  *popup_height);
static GimpTempBuf * gimp_font_get_new_preview    (GimpViewable          *viewable,
                                                   GimpContext           *context,
                                                   gint                   width,
                                                   gint                   height);

static void          gimp_font_config_iface_init  (GimpConfigInterface  *iface);
static gboolean      gimp_font_serialize          (GimpConfig           *config,
                                                   GimpConfigWriter     *writer,
                                                   gpointer              data);
static GimpConfig  * gimp_font_deserialize_create (GType                 type,
                                                   GScanner             *scanner,
                                                   gint                  nest_level,
                                                   gpointer              data);

static gint64        gimp_font_get_memsize        (GimpObject           *object,
                                                   gint64               *gui_size);

static inline gboolean gimp_font_covers_string    (PangoFont            *font,
                                                   const gchar          *sample);
static hb_tag_t      get_hb_table_type            (PangoOTTableType      table_type);
static const gchar * gimp_font_get_sample_string  (PangoContext         *context,
                                                   PangoFontDescription *font_desc);

static const gchar * gimp_font_get_hash           (GimpFont             *font);


G_DEFINE_TYPE_WITH_CODE (GimpFont, gimp_font, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                         gimp_font_config_iface_init))


#define parent_class gimp_font_parent_class

static void
gimp_font_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize          = gimp_font_serialize;
  iface->deserialize_create = gimp_font_deserialize_create;
}

static gboolean
gimp_font_serialize (GimpConfig       *config,
                     GimpConfigWriter *writer,
                     gpointer          data)
{
  GimpFont *font;

  g_return_val_if_fail (GIMP_IS_FONT (config), FALSE);

  font = GIMP_FONT (config);

  if (font == GIMP_FONT (gimp_font_get_standard ()))
    return TRUE;

  gimp_config_writer_open   (writer, "fonthash");
  gimp_config_writer_string (writer, gimp_font_get_hash (font));
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "fullname");
  gimp_config_writer_string (writer, font->fullname);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "family");
  gimp_config_writer_string (writer, font->family);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "style");
  gimp_config_writer_string (writer, font->style);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "psname");
  gimp_config_writer_string (writer, font->psname);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "index");
  gimp_config_writer_printf (writer, "%d", font->index);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "weight");
  gimp_config_writer_printf (writer, "%d", font->weight);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "slant");
  gimp_config_writer_printf (writer, "%d", font->slant);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "width");
  gimp_config_writer_printf (writer, "%d", font->width);
  gimp_config_writer_close  (writer);

  gimp_config_writer_open   (writer, "fontversion");
  gimp_config_writer_printf (writer, "%d", font->fontversion);
  gimp_config_writer_close  (writer);

  return TRUE;
}

static GimpConfig *
gimp_font_deserialize_create (GType     type,
                              GScanner *scanner,
                              gint      nest_level,
                              gpointer  data)
{
  GimpFont      *font                    = NULL;
  GimpContainer *fonts_container         = GIMP_FONT_CLASS (g_type_class_peek (GIMP_TYPE_FONT))->fonts_container;
  gint           most_similar_font_index = -1;
  gint           font_count              = gimp_container_get_n_children (fonts_container);
  gint           largest_similarity      = 0;
  GList         *similar_fonts           = NULL;
  GList         *iter;
  gint           i;
  gchar         *fonthash                = NULL;
  gchar         *fullname                = NULL;
  gchar         *family                  = NULL;
  gchar         *psname                  = NULL;
  gchar         *style                   = NULL;
  gint           index                   = -1;
  gint           weight                  = -1;
  gint           slant                   = -1;
  gint           width                   = -1;
  gint           fontversion             = -1;
  guint          scope_id;
  guint          old_scope_id;

  /* This is for backward compatibility with older xcf files.
   * The font used to be serialized as a string containing
   * its name.
   */
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
    {
      PangoFontDescription *pfd        = NULL;
      PangoFontMap         *fontmap    = NULL;
      PangoContext         *context    = NULL;
      PangoFcFont          *fc_font    = NULL;
      FcPattern            *fc_pattern = NULL;
      gchar                *font_name  = NULL;
      gchar                *fullname   = NULL;
      gchar                *file_path  = NULL;

      gimp_scanner_parse_string (scanner, &font_name);

      fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
      context = pango_font_map_create_context (fontmap);
      pfd = pango_font_description_from_string (font_name);
      fc_font = PANGO_FC_FONT (pango_context_load_font (context, pfd));
      fc_pattern = pango_fc_font_get_pattern (fc_font);
      FcPatternGetString (fc_pattern, FC_FULLNAME, 0, (FcChar8 **) &fullname);
      FcPatternGetString (fc_pattern, FC_FILE,     0, (FcChar8 **) &file_path);

      for (i = 0; i < font_count; i++)
        {
          font = GIMP_FONT (gimp_container_get_child_by_index (fonts_container, i));

          if (!g_strcmp0 (font->desc, font_name) || !g_strcmp0 (font->file_path, file_path))
            break;

          font = NULL;
        }

      if (font == NULL)
        font = GIMP_FONT (gimp_font_get_standard ());

      g_object_ref (font);

      g_object_unref (fontmap);
      g_object_unref (context);
      g_object_unref (fc_font);
      g_free (font_name);

      return GIMP_CONFIG (font);
    }

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_RIGHT_PAREN)
    return GIMP_CONFIG (GIMP_FONT (gimp_font_get_standard ()));

  scope_id = g_type_qname (type);
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id, "fonthash",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_FONTHASH));
  g_scanner_scope_add_symbol (scanner, scope_id, "fullname",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_FULLNAME));
  g_scanner_scope_add_symbol (scanner, scope_id, "family",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_FAMILY));
  g_scanner_scope_add_symbol (scanner, scope_id, "style",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_STYLE));
  g_scanner_scope_add_symbol (scanner, scope_id, "psname",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_PSNAME));
  g_scanner_scope_add_symbol (scanner, scope_id, "index",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_INDEX));
  g_scanner_scope_add_symbol (scanner, scope_id, "weight",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_WEIGHT));
  g_scanner_scope_add_symbol (scanner, scope_id, "slant",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_SLANT));
  g_scanner_scope_add_symbol (scanner, scope_id, "width",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_WIDTH));
  g_scanner_scope_add_symbol (scanner, scope_id, "fontversion",
                              GINT_TO_POINTER (GIMP_FONT_SYMBOL_FONTVERSION));

  while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
    {
      GTokenType token;

      g_scanner_get_next_token  (scanner); /* ( */

      token = g_scanner_get_next_token (scanner);

      if (token != G_TOKEN_SYMBOL)
        {
          /* When encountering an unknown symbol, we simply ignore its contents
           * (up to the next closing parenthese). This would allow us to load
           * future XCF files in case we add new fields to recognize fonts
           * without having to bump the XCF version (just ignoring unknown
           * fields). We still output a small message on stderr.
           */
          if (token == G_TOKEN_IDENTIFIER)
            g_printerr ("%s: ignoring unknown symbol '%s'.\n", G_STRFUNC, scanner->value.v_string);
          else
            g_printerr ("%s: ignoring unknown token %d.\n", G_STRFUNC, token);

          while ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_EOF)
            {
              if (token == G_TOKEN_RIGHT_PAREN)
                break;
            }

          continue;
        }

      switch (GPOINTER_TO_INT (scanner->value.v_symbol))
        {
        case GIMP_FONT_SYMBOL_FONTHASH:
          gimp_scanner_parse_string (scanner, &fonthash);
          break;
        case GIMP_FONT_SYMBOL_FULLNAME:
          gimp_scanner_parse_string (scanner, &fullname);
          break;
        case GIMP_FONT_SYMBOL_FAMILY:
          gimp_scanner_parse_string (scanner, &family);
          break;
        case GIMP_FONT_SYMBOL_STYLE:
          gimp_scanner_parse_string (scanner, &style);
          break;
        case GIMP_FONT_SYMBOL_PSNAME:
          gimp_scanner_parse_string (scanner, &psname);
          break;
        case GIMP_FONT_SYMBOL_INDEX:
          gimp_scanner_parse_int (scanner, &index);
          break;
        case GIMP_FONT_SYMBOL_WEIGHT:
          gimp_scanner_parse_int (scanner, &weight);
          break;
        case GIMP_FONT_SYMBOL_SLANT:
          gimp_scanner_parse_int (scanner, &slant);
          break;
        case GIMP_FONT_SYMBOL_WIDTH:
          gimp_scanner_parse_int (scanner, &width);
          break;
        case GIMP_FONT_SYMBOL_FONTVERSION:
          gimp_scanner_parse_int (scanner, &fontversion);
          break;
        default:
          break;
        }

      if (g_scanner_get_next_token (scanner) != G_TOKEN_RIGHT_PAREN)
        break;
    }

  g_scanner_scope_remove_symbol (scanner, scope_id, "fonthash");
  g_scanner_scope_remove_symbol (scanner, scope_id, "fullname");
  g_scanner_scope_remove_symbol (scanner, scope_id, "family");
  g_scanner_scope_remove_symbol (scanner, scope_id, "style");
  g_scanner_scope_remove_symbol (scanner, scope_id, "psname");
  g_scanner_scope_remove_symbol (scanner, scope_id, "index");
  g_scanner_scope_remove_symbol (scanner, scope_id, "weight");
  g_scanner_scope_remove_symbol (scanner, scope_id, "slant");
  g_scanner_scope_remove_symbol (scanner, scope_id, "width");
  g_scanner_scope_remove_symbol (scanner, scope_id, "fontversion");
  g_scanner_set_scope (scanner, old_scope_id);

  for (i = 0; i < font_count; i++)
    {
      gint current_font_similarity = 0;

      font = GIMP_FONT (gimp_container_get_child_by_index (fonts_container, i));

      /* Some attrs are more identifying than others,
       * hence their higher importance in measuring similarity.
       */

      if (fullname != NULL && !g_strcmp0 (font->fullname, fullname))
        current_font_similarity += 5;

      if (family != NULL && !g_strcmp0 (font->family, family))
        current_font_similarity += 5;

      if (psname != NULL && font->psname != NULL && !g_strcmp0 (font->psname, psname))
        current_font_similarity += 5;

      if (current_font_similarity < 5)
        continue;

      if (style != NULL && font->style != NULL && !g_strcmp0 (font->style, style))
        current_font_similarity++;

      if (font->weight != -1 && font->weight == weight)
        current_font_similarity++;

      if (font->width != -1 && font->width == width)
        current_font_similarity++;

      if (font->slant != -1 && font->slant == slant)
        current_font_similarity++;

      if (font->index != -1 && font->index == index)
        current_font_similarity++;

      if (font->fontversion != -1 && font->fontversion == fontversion)
        current_font_similarity++;

      if (current_font_similarity > largest_similarity)
        {
          largest_similarity      = current_font_similarity;
          most_similar_font_index = i;
          g_clear_pointer (&similar_fonts, g_list_free);
          similar_fonts = g_list_prepend (similar_fonts, GINT_TO_POINTER (i));
        }
      else if (current_font_similarity == largest_similarity)
        {
          similar_fonts = g_list_prepend (similar_fonts, GINT_TO_POINTER (i));
        }
    }

  /* In case there are multiple font with identical info,
   * the font file hash should be used for comparison.
   */
  if (g_list_length (similar_fonts) == 1)
    g_clear_pointer (&similar_fonts, g_list_free);
  for (iter = similar_fonts; iter; iter = iter->next)
    {
      i    = GPOINTER_TO_INT (iter->data);
      font = GIMP_FONT (gimp_container_get_child_by_index (fonts_container, i));

      if (g_strcmp0 (gimp_font_get_hash (font), fonthash) == 0)
        {
          most_similar_font_index = i;
          break;
        }
    }

  if (most_similar_font_index > -1)
    font = GIMP_FONT (gimp_container_get_child_by_index (fonts_container, most_similar_font_index));
  else
    font = GIMP_FONT (gimp_font_get_standard ());

  g_object_ref (font);

  g_list_free (similar_fonts);
  g_free (fonthash);
  g_free (fullname);
  g_free (family);
  g_free (psname);
  g_free (style);

  return GIMP_CONFIG (font);
}

void
gimp_font_class_set_font_factory (GimpFontFactory *factory)
{
  GimpFontClass *klass = GIMP_FONT_CLASS (g_type_class_peek (GIMP_TYPE_FONT));

  g_return_if_fail (GIMP_IS_FONT_FACTORY (factory));

  klass->fonts_container = gimp_data_factory_get_container (GIMP_DATA_FACTORY (factory));
}

void
gimp_font_set_font_info (GimpFont *font,
                         gpointer  font_info[])
{
  font->fullname    =  g_strdup ((gchar*)font_info[PROP_FULLNAME]);
  font->family      =  g_strdup ((gchar*)font_info[PROP_FAMILY]);
  font->style       =  g_strdup ((gchar*)font_info[PROP_STYLE]);
  font->psname      =  g_strdup ((gchar*)font_info[PROP_PSNAME]);
  font->desc        =  g_strdup ((gchar*)font_info[PROP_DESC]);
  font->weight      = *(gint*)font_info[PROP_WEIGHT];
  font->width       = *(gint*)font_info[PROP_WIDTH];
  font->index       = *(gint*)font_info[PROP_INDEX];
  font->slant       = *(gint*)font_info[PROP_SLANT];
  font->fontversion = *(gint*)font_info[PROP_FONTVERSION];
  font->file_path   =  g_strdup ((gchar*)font_info[PROP_FILE]);
}

void
gimp_font_set_lookup_name (GimpFont    *font,
                           gchar       *name)
{
  font->lookup_name = name;
}

gboolean
gimp_font_match_by_lookup_name (GimpFont    *font,
                                const gchar *name)
{
  return !g_strcmp0 (font->lookup_name, name);
}

gboolean
gimp_font_match_by_description (GimpFont    *font,
                                const gchar *desc)
{
  return !g_strcmp0 (font->desc, desc);
}

const gchar*
gimp_font_get_lookup_name (GimpFont *font)
{
  return font->lookup_name;
}

static void
gimp_font_class_init (GimpFontClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize            = gimp_font_finalize;
  object_class->set_property        = gimp_font_set_property;
  gimp_object_class->get_memsize    = gimp_font_get_memsize;

  viewable_class->get_preview_size  = gimp_font_get_preview_size;
  viewable_class->get_popup_size    = gimp_font_get_popup_size;
  viewable_class->get_new_preview   = gimp_font_get_new_preview;

  viewable_class->default_icon_name = "gtk-select-font";

  g_object_class_install_property (object_class, PROP_PANGO_CONTEXT,
                                   g_param_spec_object ("pango-context",
                                                        NULL, NULL,
                                                        PANGO_TYPE_CONTEXT,
                                                        GIMP_PARAM_WRITABLE));
}

static void
gimp_font_init (GimpFont *font)
{
}

static void
gimp_font_finalize (GObject *object)
{
  GimpFont *font = GIMP_FONT (object);

  g_clear_object (&font->pango_context);
  g_clear_object (&font->popup_layout);
  g_free (font->lookup_name);
  g_free (font->hash);
  g_free (font->fullname);
  g_free (font->family);
  g_free (font->style);
  g_free (font->psname);
  g_free (font->desc);
  g_free (font->file_path);

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
      font->pango_context = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_font_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpFont *font    = GIMP_FONT (object);
  gint64    memsize = 0;

  memsize += gimp_string_get_memsize (font->lookup_name);
  memsize += gimp_string_get_memsize (font->hash);
  memsize += gimp_string_get_memsize (font->fullname);
  memsize += gimp_string_get_memsize (font->family);
  memsize += gimp_string_get_memsize (font->style);
  memsize += gimp_string_get_memsize (font->psname);
  memsize += gimp_string_get_memsize (font->desc);
  memsize += gimp_string_get_memsize (font->file_path);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
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

  name = font->lookup_name;

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

  *popup_width = cairo_format_stride_for_width (CAIRO_FORMAT_A8, *popup_width);

  font->popup_width  = *popup_width;
  font->popup_height = *popup_height;

  return TRUE;
}

static GimpTempBuf *
gimp_font_get_new_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpFont        *font = GIMP_FONT (viewable);
  PangoLayout     *layout;
  PangoRectangle   ink;
  PangoRectangle   logical;
  gint             layout_width;
  gint             layout_height;
  gint             layout_x;
  gint             layout_y;
  GimpTempBuf     *temp_buf;
  cairo_t         *cr;
  cairo_surface_t *surface;

  if (! font->pango_context)
    return NULL;

  if (! font->popup_layout ||
      font->popup_width != width || font->popup_height != height)
    {
      PangoFontDescription *font_desc;
      const gchar          *name;

      name = font->lookup_name;

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

  width = cairo_format_stride_for_width (CAIRO_FORMAT_A8, width);

  temp_buf = gimp_temp_buf_new (width, height, babl_format ("Y' u8"));
  memset (gimp_temp_buf_get_data (temp_buf), 255, width * height);

  surface = cairo_image_surface_create_for_data (gimp_temp_buf_get_data (temp_buf),
                                                 CAIRO_FORMAT_A8,
                                                 width, height, width);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  layout_width  = MAX (ink.width,  logical.width);
  layout_height = MAX (ink.height, logical.height);

  layout_x = (width - layout_width)  / 2;
  layout_y = (height - layout_height) / 2;

  if (ink.x < logical.x)
    layout_x += logical.x - ink.x;

  if (ink.y < logical.y)
    layout_y += logical.y - ink.y;

  cr = cairo_create (surface);

  cairo_translate (cr, layout_x, layout_y);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  pango_cairo_show_layout (cr, layout);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  g_object_unref (layout);

  return temp_buf;
}

GimpData *
gimp_font_get_standard (void)
{
  static GimpData *standard_font = NULL;

  if (! standard_font)
    {
      g_set_weak_pointer (&standard_font,
                          g_object_new (GIMP_TYPE_FONT,
                                        "name", "",
                                        NULL));;

     /* pango_font_description_from_string doesn't accept NULL */
      GIMP_FONT (standard_font)->lookup_name = g_strdup ("");
      gimp_data_clean (standard_font);
      gimp_data_make_internal (standard_font, "gimp-font-standard");
    }

  return standard_font;
}


/* Private functions */

static inline gboolean
gimp_font_covers_string (PangoFont   *font,
                         const gchar *sample)
{
  const gchar *p;

  for (p = sample; *p; p = g_utf8_next_char (p))
    {
      if (! pango_font_has_char (font, g_utf8_get_char (p)))
        return FALSE;
    }

  return TRUE;
}

/* This function was picked up from Pango's pango-ot-info.c. Until there
 * is a better way to get the tag, we use this.
 */
static hb_tag_t
get_hb_table_type (PangoOTTableType table_type)
{
  switch (table_type)
    {
    case PANGO_OT_TABLE_GSUB:
      return HB_OT_TAG_GSUB;
    case PANGO_OT_TABLE_GPOS:
      return HB_OT_TAG_GPOS;
    default:
      return HB_TAG_NONE;
    }
}

/* Guess a suitable short sample string for the font. */
static const gchar *
gimp_font_get_sample_string (PangoContext         *context,
                             PangoFontDescription *font_desc)
{
  PangoFont        *font;
  hb_face_t        *hb_face;
  hb_font_t        *hb_font;
  FT_Face           face;
  TT_OS2           *os2;
  PangoOTTableType  tt;
  gint              i;

  /* This is a table of scripts and corresponding short sample strings
   * to be used instead of the Latin sample string Aa. The script
   * codes are as in ISO15924 (see
   * https://www.unicode.org/iso15924/iso15924-codes.html), but in
   * lower case. The Unicode subrange bit numbers, as used in TrueType
   * so-called OS/2 tables, are from
   * https://www.microsoft.com/typography/otspec/os2.htm#ur .
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
   * Very useful link: https://www.wazu.jp/index.html
   */
  static const struct
  {
    const gchar  script[4];
    gint         bit;
    const gchar *sample;
  } scripts[] = {
    /* CJK are first because fonts that support these characters
     * presumably are primarily designed for it.
     *
     * Though not a universal rules, most fonts targeting Korean will
     * also have the ideographs as they were used historically (and
     * still in some cases). As for Japanese, ideographs are still
     * definitely used daily. After all the block is called "*CJK*
     * (Chinese-Japanese-Korean) Unified Ideographs". So let's give the
     * Chinese representation less priority towards finding
     * Korean/Japanese targeting.
     * Then we prioritize Korean because many fonts for Korean also
     * design Hiragana/Katakana blocks. On the other hand, I could see
     * very few (none so far) Japanese fonts designing also Hangul
     * characters. So if we see both Hiragana and Hangul, we can assume
     * this is a font for Korean market, whereas Hiragana only is for
     * Japanese.
     * XXX: of course we could probably come with a better algorithm to
     * determine the target language. Probably looking at more than the
     * existence of a single block would help (since some languages are
     * spread on several blocks).
     */
    {
      "hang",                   /* Korean alphabet (Hangul)   */
      56,                       /* Hangul Syllables block     */
      "\355\225\234"            /* U+D55C HANGUL SYLLABLE HAN */
    },
    {
      "japa",                   /* Japanese                   */
      49,                       /* Hiragana block             */
      "\343\201\202"            /* U+3042 HIRAGANA LETTER A   */
    },
    {
      "hani",                   /* Han Ideographic            */
      59,                       /* CJK Unified Ideographs     */
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

  hb_font = pango_font_get_hb_font (font);
  g_return_val_if_fail (hb_font != NULL, "Aa");

  /* These are needed to set hb_font to the right internal format, which
   * can only work if the font is not immutable. Hence we make a copy.
   */
  hb_font = hb_font_create_sub_font (hb_font);
  hb_ft_font_set_funcs (hb_font);
  /* TODO: use hb_ft_font_lock_face/hb_ft_font_unlock_face() when we
   * bump to harfbuzz >= 2.6.5.
   */
  face = hb_ft_font_get_face (hb_font);

  /* Are there actual cases where this function could return NULL while
   * it's not a bug in the code?
   * For instance if the font file is broken, we don't want to return a
   * CRITICAL, but properly address the issue (removing the font with a
   * warning or whatnot).
   * See #5922.
   */
  g_return_val_if_fail (face != NULL, "Aa");

  hb_face = hb_ft_face_create (face, NULL);

  /* First check what script(s), if any, the font has GSUB or GPOS
   * OpenType layout tables for.
   */
  for (tt = PANGO_OT_TABLE_GSUB;
       n_ot_alts < G_N_ELEMENTS (ot_alts) && tt <= PANGO_OT_TABLE_GPOS;
       tt++)
    {
      hb_tag_t tag;
      unsigned int count;
      PangoOTTag *slist;

      tag = get_hb_table_type (tt);
      count = hb_ot_layout_table_get_script_tags (hb_face, tag, 0, NULL, NULL);
      slist = g_new (PangoOTTag, count + 1);
      hb_ot_layout_table_get_script_tags (hb_face, tag, 0, &count, slist);
      slist[count] = 0;

      for (i = 0;
           n_ot_alts < G_N_ELEMENTS (ot_alts) && i < G_N_ELEMENTS (scripts);
           i++)
        {
          gint j, k;

          for (k = 0; k < n_ot_alts; k++)
            if (ot_alts[k] == i)
              break;

          if (k == n_ot_alts)
            {
              for (j = 0;
                   n_ot_alts < G_N_ELEMENTS (ot_alts) && slist[j];
                   j++)
                {
#define TAG(s) FT_MAKE_TAG (s[0], s[1], s[2], s[3])

                  if (slist[j] == TAG (scripts[i].script) &&
                      gimp_font_covers_string (font, scripts[i].sample))
                    {
                      ot_alts[n_ot_alts++] = i;
                      DEBUGPRINT (("%.4s ", scripts[i].script));
                    }
#undef TAG
                }
            }
        }

      g_free (slist);
    }

  hb_face_destroy (hb_face);

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
              gimp_font_covers_string (font, scripts[i].sample))
            {
              sr_alts[n_sr_alts++] = i;
              DEBUGPRINT (("%.4s ", scripts[i].script));
            }
        }
    }

  hb_font_destroy (hb_font);
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

static const gchar *
gimp_font_get_hash (GimpFont *font)
{
  /* Computing the hash is expensive, so it's only done in a few case, such as
   * when serializing, when a similar fonts at deserialization looks like it
   * could be a good identity candidate.
   */
  if (font->hash == NULL)
    {
      PangoFontDescription *pfd        = pango_font_description_from_string (font->lookup_name);
      PangoFcFont          *pango_font = PANGO_FC_FONT (pango_context_load_font (font->pango_context, pfd));
      GChecksum            *checksum   = g_checksum_new (G_CHECKSUM_SHA256);
      gchar                *file;
      hb_blob_t            *hb_blob;
      guint                 length;
      const char           *hb_data;

      FcPatternGetString (pango_fc_font_get_pattern (pango_font), FC_FILE, 0, (FcChar8 **) &file);

      hb_blob  = hb_blob_create_from_file (file);
      hb_data  = hb_blob_get_data (hb_blob, &length);
      g_checksum_update (checksum, (const guchar*) hb_data, length);
      font->hash = g_strdup (g_checksum_get_string (checksum));

      pango_font_description_free (pfd);
      g_object_unref (pango_font);
      hb_blob_destroy (hb_blob);
      g_checksum_free (checksum);
    }

  return font->hash;
}
