/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontlist.c
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

#include <string.h>

#include <glib-object.h>
#include <pango/pangoft2.h>

#include "text-types.h"

#include "gimpfont.h"
#include "gimpfontlist.h"

#include "gimp-intl.h"


static void   gimp_font_list_class_init        (GimpFontListClass *klass);
static void   gimp_font_list_init              (GimpFontList      *list);

static gint   gimp_font_list_font_compare_func (gconstpointer      first,
                                                gconstpointer      second);


static GimpListClass *parent_class = NULL;


GType
gimp_font_list_get_type (void)
{
  static GType list_type = 0;

  if (! list_type)
    {
      static const GTypeInfo list_info =
      {
        sizeof (GimpFontListClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_font_list_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_font     */
	sizeof (GimpFontList),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_font_list_init,
      };

      list_type = g_type_register_static (GIMP_TYPE_LIST,
					  "GimpFontList", 
					  &list_info, 0);
    }

  return list_type;
}

static void
gimp_font_list_class_init (GimpFontListClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);
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
                       "children_type", GIMP_TYPE_FONT,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       NULL);

  list->xresolution = xresolution;
  list->yresolution = yresolution;

  return GIMP_CONTAINER (list);
}

void
gimp_font_list_restore (GimpFontList *list)
{
  PangoFontMap     *fontmap;
  PangoContext     *context;
  PangoFontFamily **families;
  PangoFontFace   **faces;
  gint              n_families;
  gint              n_faces;
  gint              i, j;

  g_return_if_fail (GIMP_IS_FONT_LIST (list));

  fontmap = pango_ft2_font_map_new (); 
  pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap),
                                     list->xresolution, list->yresolution);

  context = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (fontmap));

  pango_font_map_list_families (fontmap, &families, &n_families);
  g_object_unref (fontmap);

  gimp_container_freeze (GIMP_CONTAINER (list));

  for (i = 0; i < n_families; i++)
    {
      pango_font_family_list_faces (families[i], &faces, &n_faces);
      
      for (j = 0; j < n_faces; j++)
        {
          PangoFontDescription *desc;
          GimpFont             *font;
          gchar                *name;

	  desc = pango_font_face_describe (faces[j]);
          name = pango_font_description_to_string (desc);
          pango_font_description_free (desc);

          font = g_object_new (GIMP_TYPE_FONT,
                               "name",          name,
                               "pango-context", context,
                               NULL);
          g_free (name);

          gimp_container_add (GIMP_CONTAINER (list), GIMP_OBJECT (font));
          g_object_unref (font);
        }
    }

  g_free (families);
  g_object_unref (context);

  gimp_list_sort (GIMP_LIST (list), gimp_font_list_font_compare_func);

  gimp_container_thaw (GIMP_CONTAINER (list));
}

static gint
gimp_font_list_font_compare_func (gconstpointer first,
				  gconstpointer second)
{
  return strcmp (((const GimpObject *) first)->name,
		 ((const GimpObject *) second)->name);
}
