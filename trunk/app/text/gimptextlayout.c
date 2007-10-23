/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "libgimpmath/gimpmath.h"

#include "core/gimpimage.h"
#include "core/gimpunit.h"

#include "gimptext.h"
#include "gimptext-private.h"
#include "gimptextlayout.h"


static void   gimp_text_layout_finalize           (GObject        *object);

static void   gimp_text_layout_position           (GimpTextLayout *layout);

static PangoContext * gimp_text_get_pango_context (GimpText       *text,
                                                   gdouble         xres,
                                                   gdouble         yres);

static gint   gimp_text_layout_pixel_size         (Gimp           *gimp,
                                                   gdouble         value,
                                                   GimpUnit        unit,
                                                   gdouble         res);
static gint   gimp_text_layout_point_size         (Gimp           *gimp,
                                                   gdouble         value,
                                                   GimpUnit        unit,
                                                   gdouble         res);


G_DEFINE_TYPE (GimpTextLayout, gimp_text_layout, G_TYPE_OBJECT)

#define parent_class gimp_text_layout_parent_class


static void
gimp_text_layout_class_init (GimpTextLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_text_layout_finalize;
}

static void
gimp_text_layout_init (GimpTextLayout *layout)
{
  layout->text   = NULL;
  layout->layout = NULL;
}

static void
gimp_text_layout_finalize (GObject *object)
{
  GimpTextLayout *layout = GIMP_TEXT_LAYOUT (object);

  if (layout->text)
    {
      g_object_unref (layout->text);
      layout->text = NULL;
    }
  if (layout->layout)
    {
      g_object_unref (layout->layout);
      layout->layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


GimpTextLayout *
gimp_text_layout_new (GimpText  *text,
                      GimpImage *image)
{
  GimpTextLayout       *layout;
  PangoContext         *context;
  PangoFontDescription *font_desc;
  PangoAlignment        alignment = PANGO_ALIGN_LEFT;
  gdouble               xres, yres;
  gint                  size;

  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  font_desc = pango_font_description_from_string (text->font);
  g_return_val_if_fail (font_desc != NULL, NULL);

  gimp_image_get_resolution (image, &xres, &yres);

  size = gimp_text_layout_point_size (image->gimp,
                                      text->font_size,
                                      text->unit,
                                      yres);

  pango_font_description_set_size (font_desc, MAX (1, size));

  context = gimp_text_get_pango_context (text, xres, yres);

  layout = g_object_new (GIMP_TYPE_TEXT_LAYOUT, NULL);
  layout->text   = g_object_ref (text);
  layout->layout = pango_layout_new (context);
  layout->xres = xres;
  layout->yres = yres;

  g_object_unref (context);

  pango_layout_set_font_description (layout->layout, font_desc);
  pango_font_description_free (font_desc);

  if (text->text)
    pango_layout_set_text (layout->layout, text->text, -1);
  else
    pango_layout_set_text (layout->layout, NULL, 0);

  switch (text->justify)
    {
    case GIMP_TEXT_JUSTIFY_LEFT:
      alignment = PANGO_ALIGN_LEFT;
      break;
    case GIMP_TEXT_JUSTIFY_RIGHT:
      alignment = PANGO_ALIGN_RIGHT;
      break;
    case GIMP_TEXT_JUSTIFY_CENTER:
      alignment = PANGO_ALIGN_CENTER;
      break;
    case GIMP_TEXT_JUSTIFY_FILL:
      /* FIXME: This doesn't work since the implementation is missing
         at the Pango level.
       */
      alignment = PANGO_ALIGN_LEFT;
      pango_layout_set_justify (layout->layout, TRUE);
      break;
    }

  pango_layout_set_alignment (layout->layout, alignment);

  switch (text->box_mode)
    {
    case GIMP_TEXT_BOX_DYNAMIC:
      break;
    case GIMP_TEXT_BOX_FIXED:
      pango_layout_set_width (layout->layout,
                              gimp_text_layout_pixel_size (image->gimp,
                                                           text->box_width,
                                                           text->box_unit,
                                                           xres));
      break;
    }

  pango_layout_set_indent (layout->layout,
                           gimp_text_layout_pixel_size (image->gimp,
                                                        text->indent,
                                                        text->unit,
                                                        xres));
  pango_layout_set_spacing (layout->layout,
                            gimp_text_layout_pixel_size (image->gimp,
                                                         text->line_spacing,
                                                         text->unit,
                                                         yres));
  if (fabs (text->letter_spacing) > 0.1)
    {
      PangoAttrList  *attrs = pango_attr_list_new ();
      PangoAttribute *attr;

      attr = pango_attr_letter_spacing_new (text->letter_spacing * PANGO_SCALE);

      attr->start_index = 0;
      attr->end_index   = -1;

      pango_attr_list_insert (attrs, attr);

      pango_layout_set_attributes (layout->layout, attrs);
      pango_attr_list_unref (attrs);
    }

  gimp_text_layout_position (layout);

  switch (text->box_mode)
    {
    case GIMP_TEXT_BOX_DYNAMIC:
      break;
    case GIMP_TEXT_BOX_FIXED:
      layout->extents.height =
        PANGO_PIXELS (gimp_text_layout_pixel_size (image->gimp,
                                                   text->box_height,
                                                   text->box_unit,
                                                   yres));
      break;
    }

  return layout;
}

gboolean
gimp_text_layout_get_size (GimpTextLayout *layout,
                           gint           *width,
                           gint           *height)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), FALSE);

  if (width)
    *width = layout->extents.width;
  if (height)
    *height = layout->extents.height;

  return (layout->extents.width > 0 && layout->extents.height > 0);
}

void
gimp_text_layout_get_offsets (GimpTextLayout *layout,
                              gint           *x,
                              gint           *y)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x)
    *x = layout->extents.x;
  if (y)
    *y = layout->extents.y;
}

static void
gimp_text_layout_position (GimpTextLayout *layout)
{
  PangoRectangle  ink;
  PangoRectangle  logical;
  gint            x1, y1;
  gint            x2, y2;

  layout->extents.x      = 0;
  layout->extents.x      = 0;
  layout->extents.width  = 0;
  layout->extents.height = 0;

  pango_layout_get_pixel_extents (layout->layout, &ink, &logical);

  ink.width = ceil ((gdouble) ink.width * layout->xres / layout->yres);
  logical.width = ceil ((gdouble) logical.width * layout->xres / layout->yres);

#ifdef VERBOSE
  g_print ("ink rect: %d x %d @ %d, %d\n",
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n",
           logical.width, logical.height, logical.x, logical.y);
#endif

  if (ink.width < 1 || ink.height < 1)
    return;

  x1 = MIN (ink.x, logical.x);
  y1 = MIN (ink.y, logical.y);
  x2 = MAX (ink.x + ink.width,  logical.x + logical.width);
  y2 = MAX (ink.y + ink.height, logical.y + logical.height);

  layout->extents.x      = - x1;
  layout->extents.y      = - y1;
  layout->extents.width  = x2 - x1;
  layout->extents.height = y2 - y1;

  if (layout->text->border > 0)
    {
      gint border = layout->text->border;

      layout->extents.x      += border;
      layout->extents.y      += border;
      layout->extents.width  += 2 * border;
      layout->extents.height += 2 * border;
    }

#ifdef VERBOSE
  g_print ("layout extents: %d x %d @ %d, %d\n",
           layout->extents.width, layout->extents.height,
           layout->extents.x, layout->extents.y);
#endif
}


static void
gimp_text_ft2_subst_func (FcPattern *pattern,
                          gpointer   data)
{
  GimpText *text = GIMP_TEXT (data);

  FcPatternAddBool (pattern, FC_HINTING,   text->hinting);
  FcPatternAddBool (pattern, FC_AUTOHINT,  text->autohint);
  FcPatternAddBool (pattern, FC_ANTIALIAS, text->antialias);
}

static PangoContext *
gimp_text_get_pango_context (GimpText *text,
                             gdouble   xres,
                             gdouble   yres)
{
  PangoContext    *context;
  PangoFT2FontMap *fontmap;

  fontmap = PANGO_FT2_FONT_MAP (pango_ft2_font_map_new ());

  pango_ft2_font_map_set_resolution (fontmap, xres, yres);

  pango_ft2_font_map_set_default_substitute (fontmap,
                                             gimp_text_ft2_subst_func,
                                             g_object_ref (text),
                                             (GDestroyNotify) g_object_unref);

  context = pango_ft2_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  /*  Workaround for bug #143542 (PangoFT2Fontmap leak),
   *  see also bug #148997 (Text layer rendering leaks font file descriptor):
   *
   *  Calling pango_ft2_font_map_substitute_changed() causes the
   *  font_map cache to be flushed, thereby removing the circular
   *  reference that causes the leak.
   */
  g_object_weak_ref (G_OBJECT (context),
                     (GWeakNotify) pango_ft2_font_map_substitute_changed,
                     fontmap);

  if (text->language)
    pango_context_set_language (context,
                                pango_language_from_string (text->language));

  switch (text->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      break;
    case GIMP_TEXT_DIRECTION_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_RTL);
      break;
    }

  return context;
}

static gint
gimp_text_layout_pixel_size (Gimp     *gimp,
                             gdouble   value,
                             GimpUnit  unit,
                             gdouble   res)
{
  gdouble factor;

  switch (unit)
    {
    case GIMP_UNIT_PIXEL:
      return PANGO_SCALE * value;

    default:
      factor = _gimp_unit_get_factor (gimp, unit);
      g_return_val_if_fail (factor > 0.0, 0);

      return PANGO_SCALE * value * res / factor;
    }
}

static gint
gimp_text_layout_point_size (Gimp     *gimp,
                             gdouble   value,
                             GimpUnit  unit,
                             gdouble   res)
{
  gdouble factor;

  switch (unit)
    {
    case GIMP_UNIT_POINT:
      return PANGO_SCALE * value;

    case GIMP_UNIT_PIXEL:
      g_return_val_if_fail (res > 0.0, 0);
      return (PANGO_SCALE * value *
              _gimp_unit_get_factor (gimp, GIMP_UNIT_POINT) / res);

    default:
      factor = _gimp_unit_get_factor (gimp, unit);
      g_return_val_if_fail (factor > 0.0, 0);

      return (PANGO_SCALE * value *
              _gimp_unit_get_factor (gimp, GIMP_UNIT_POINT) / factor);
    }
}
