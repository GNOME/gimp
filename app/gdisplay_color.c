/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include "gdisplay_color.h"
#include "gimpimageP.h"

typedef struct _ColorDisplayInfo ColorDisplayInfo;

struct _ColorDisplayInfo {
  char                    *name;
  GimpColorDisplayMethods  methods;
  GSList                  *refs;
};

static GHashTable *color_display_table = NULL;

gboolean
gimp_color_display_register (const char              *name,
    			     GimpColorDisplayMethods *methods)
{
  ColorDisplayInfo *info;
  
  info = g_new (ColorDisplayInfo, 1);
  
  info->name = g_strdup (name);
  info->methods = *methods;
  info->refs = NULL;
  
  if (!color_display_table)
    color_display_table = g_hash_table_new (g_str_hash, g_str_equal);
  
  if (g_hash_table_lookup (color_display_table, name))
    return FALSE;

  if (!methods->convert)
    return FALSE;

  g_hash_table_insert (color_display_table, info->name, info);
  
  return TRUE;
}

gboolean
gimp_color_display_unregister (const char *name)
{
  ColorDisplayInfo *info;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      g_hash_table_remove (color_display_table, name);
      
      /* FIXME: Check refs here */

      g_free (info->name);
      g_free (info);
    }
  
  return TRUE;
}

void
gdisplay_color_attach (GDisplay   *gdisp,
    		       const char *name)
{
  ColorDisplayInfo *info;
  
  gdisplay_color_detach (gdisp);

  gdisp->cd_name = g_strdup (name);
  gdisp->cd_ID = NULL;
  gdisp->cd_convert = NULL;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      if (!info->refs && info->methods.init)
	info->methods.init ();

      info->refs = g_slist_append (info->refs, gdisp);

      if (info->methods.new)
	gdisp->cd_ID = info->methods.new (gdisp->gimage->base_type);

      gdisp->cd_convert = info->methods.convert;
    }
}

void
gdisplay_color_detach (GDisplay *gdisp)
{
  ColorDisplayInfo *info;
  
  if (gdisp->cd_name)
    {
      if ((info = g_hash_table_lookup (color_display_table, gdisp->cd_name)))
	{
	  if (info->methods.destroy)
	    info->methods.destroy (gdisp->cd_ID);

	  info->refs = g_slist_remove (info->refs, gdisp);
      
	  if (!info->refs && info->methods.finalize)
	    info->methods.finalize ();
	}

      g_free (gdisp->cd_name);
      gdisp->cd_name = NULL;
    }  
}

typedef struct _GammaContext GammaContext;

struct _GammaContext
{
  double gamma;
  guchar *lookup;
};

static
gpointer gamma_new (int type)
{
  int i;
  GammaContext *context = NULL;

  context = g_new (GammaContext, 1);
  context->gamma = 1.0;
  context->lookup = g_new (guchar, 256);

  for (i = 0; i < 256; i++)
    context->lookup[i] = i;

  return context;
}

static
void gamma_destroy (gpointer cd_ID)
{
  GammaContext *context = (GammaContext *) cd_ID;

  g_free (context->lookup);
  g_free (context);
}

static
void gamma_convert (gpointer  cd_ID,
    		    guchar   *buf,
		    int       width,
		    int       height,
		    int       bpp)
{
  int i;
  guchar *lookup = ((GammaContext *) cd_ID)->lookup;

  for (i = 0; i < width * height * bpp; i++)
    *buf++ = lookup[*buf];
}

void
gdisplay_color_init (void)
{
}
