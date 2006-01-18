/* The GIMP -- an image manipulation program
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
#include <pango/pangoft2.h>

#include "text-types.h"

#include "base/temp-buf.h"

#include "gimpfont.h"

#include "gimp-intl.h"


/* This is a so-called pangram; it's supposed to
   contain all characters found in the alphabet. */
#define GIMP_TEXT_PANGRAM     N_("Pack my box with\nfive dozen liquor jugs.")

#define GIMP_FONT_POPUP_SIZE  (PANGO_SCALE * 30)


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
                                             gint           width,
                                             gint           height);


G_DEFINE_TYPE (GimpFont, gimp_font, GIMP_TYPE_VIEWABLE);

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

  if (! font->popup_layout        ||
      font->popup_width  != width ||
      font->popup_height != height)
    {
      PangoFontDescription *font_desc;
      const gchar          *name;

      name = gimp_object_get_name (GIMP_OBJECT (font));

      font_desc = pango_font_description_from_string (name);
      g_return_val_if_fail (font_desc != NULL, NULL);

      pango_font_description_set_size (font_desc,
                                       PANGO_SCALE * height * 2.0 / 3.0);

      layout = pango_layout_new (font->pango_context);
      pango_layout_set_font_description (layout, font_desc);
      pango_font_description_free (font_desc);

      pango_layout_set_text (layout, "Aa", -1);
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
