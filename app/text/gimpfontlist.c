/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontlist.c
 * Copyright (C) 2003-2004  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
 *                          Manish Singh <yosh@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <gegl.h>

#include "text-types.h"

#include "gimpfont.h"
#include "gimpfontlist.h"

#include "gimp-intl.h"


/* Use fontconfig directly for speed. We can use the pango stuff when/if
 * fontconfig/pango get more efficient.
 */
#define USE_FONTCONFIG_DIRECTLY

#ifdef USE_FONTCONFIG_DIRECTLY
#include <fontconfig/fontconfig.h>
#endif


static void   gimp_font_list_add_font   (GimpFontList         *list,
                                         PangoContext         *context,
                                         PangoFontDescription *desc);

static void   gimp_font_list_load_names (GimpFontList         *list,
                                         PangoFontMap         *fontmap,
                                         PangoContext         *context);


G_DEFINE_TYPE (GimpFontList, gimp_font_list, GIMP_TYPE_LIST)


static void
gimp_font_list_class_init (GimpFontListClass *klass)
{
}

static void
gimp_font_list_init (GimpFontList *list)
{
}

GimpContainer *
gimp_font_list_new (gdouble xresolution,
                    gdouble yresolution)
{
  GimpFontList *list;

  g_return_val_if_fail (xresolution > 0.0, NULL);
  g_return_val_if_fail (yresolution > 0.0, NULL);

  list = g_object_new (GIMP_TYPE_FONT_LIST,
                       "children-type", GIMP_TYPE_FONT,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       NULL);

  list->xresolution = xresolution;
  list->yresolution = yresolution;

  return GIMP_CONTAINER (list);
}

void
gimp_font_list_restore (GimpFontList *list)
{
  PangoFontMap *fontmap;
  PangoContext *context;

  g_return_if_fail (GIMP_IS_FONT_LIST (list));

  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
  if (! fontmap)
    g_error ("You are using a Pango that has been built against a cairo "
             "that lacks the Freetype font backend");

  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap),
                                       list->yresolution);
  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  gimp_container_freeze (GIMP_CONTAINER (list));

  gimp_font_list_load_names (list, PANGO_FONT_MAP (fontmap), context);
  g_object_unref (context);

  gimp_list_sort_by_name (GIMP_LIST (list));

  gimp_container_thaw (GIMP_CONTAINER (list));
}

static void
gimp_font_list_add_font (GimpFontList         *list,
                         PangoContext         *context,
                         PangoFontDescription *desc)
{
  gchar *name;

  if (! desc)
    return;

  name = pango_font_description_to_string (desc);

  /* It doesn't look like pango_font_description_to_string() could ever
   * return NULL. But just to be double sure and avoid a segfault, I
   * check before validating the string.
   */
  if (name && strlen (name) > 0 &&
      g_utf8_validate (name, -1, NULL))
    {
      GimpFont *font;

      font = g_object_new (GIMP_TYPE_FONT,
                           "name",          name,
                           "pango-context", context,
                           NULL);

      gimp_container_add (GIMP_CONTAINER (list), GIMP_OBJECT (font));
      g_object_unref (font);
    }

  g_free (name);
}

#ifdef USE_FONTCONFIG_DIRECTLY
/* We're really chummy here with the implementation. Oh well. */

/* This is copied straight from make_alias_description in pango, plus
 * the gimp_font_list_add_font bits.
 */
static void
gimp_font_list_make_alias (GimpFontList *list,
                           PangoContext *context,
                           const gchar  *family,
                           gboolean      bold,
                           gboolean      italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, family);
  pango_font_description_set_style (desc,
                                    italic ?
                                    PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc,
                                     bold ?
                                     PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  gimp_font_list_add_font (list, context, desc);

  pango_font_description_free (desc);
}

static void
gimp_font_list_load_aliases (GimpFontList *list,
                             PangoContext *context)
{
  const gchar *families[] = { "Sans-serif", "Serif", "Monospace" };
  gint         i;

  for (i = 0; i < 3; i++)
    {
      gimp_font_list_make_alias (list, context, families[i], FALSE, FALSE);
      gimp_font_list_make_alias (list, context, families[i], TRUE,  FALSE);
      gimp_font_list_make_alias (list, context, families[i], FALSE, TRUE);
      gimp_font_list_make_alias (list, context, families[i], TRUE,  TRUE);
    }
}

static void
gimp_font_list_load_names (GimpFontList *list,
                           PangoFontMap *fontmap,
                           PangoContext *context)
{
  FcObjectSet *os;
  FcPattern   *pat;
  FcFontSet   *fontset;
  gint         i;

  g_return_if_fail (GIMP_IS_FONT_LIST (list));
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  os = FcObjectSetBuild (FC_FAMILY, FC_STYLE,
                         FC_SLANT, FC_WEIGHT, FC_WIDTH,
                         NULL);
  g_return_if_fail (os);

  pat = FcPatternCreate ();
  if (! pat)
    {
      FcObjectSetDestroy (os);
      g_critical ("%s: FcPatternCreate() returned NULL.", G_STRFUNC);
      return;
    }

  fontset = FcFontList (NULL, pat, os);

  FcPatternDestroy (pat);
  FcObjectSetDestroy (os);

  g_return_if_fail (fontset);

  for (i = 0; i < fontset->nfont; i++)
    {
      PangoFontDescription *desc;

      desc = pango_fc_font_description_from_pattern (fontset->fonts[i], FALSE);
      gimp_font_list_add_font (list, context, desc);
      pango_font_description_free (desc);
    }

  /*  only create aliases if there is at least one font available  */
  if (fontset->nfont > 0)
    gimp_font_list_load_aliases (list, context);

  FcFontSetDestroy (fontset);
}

#else  /* ! USE_FONTCONFIG_DIRECTLY */

static void
gimp_font_list_load_names (GimpFontList *list,
                           PangoFontMap *fontmap,
                           PangoContext *context)
{
  PangoFontFamily **families;
  PangoFontFace   **faces;
  gint              n_families;
  gint              n_faces;
  gint              i, j;

  pango_font_map_list_families (fontmap, &families, &n_families);

  for (i = 0; i < n_families; i++)
    {
      pango_font_family_list_faces (families[i], &faces, &n_faces);

      for (j = 0; j < n_faces; j++)
        {
          PangoFontDescription *desc;

          desc = pango_font_face_describe (faces[j]);
          gimp_font_list_add_font (list, context, desc);
          pango_font_description_free (desc);
        }
    }

  g_free (families);
}

#endif /* USE_FONTCONFIG_DIRECTLY */
