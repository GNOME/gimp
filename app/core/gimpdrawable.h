/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__


#include "gimpobject.h"


#define GIMP_TYPE_DRAWABLE            (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_IS_DRAWABLE(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DRAWABLE))
#define GIMP_DRAWABLE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE, GimpDrawableClass))
#define GIMP_IS_DRAWABLE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE))


typedef struct _GimpDrawableClass GimpDrawableClass;

struct _GimpDrawable
{
  GimpObject data;

  gchar         *name;                  /* name of drawable               */
  TileManager   *tiles;                 /* tiles for drawable data        */
  gboolean       visible;               /* controls visibility            */
  gint           width, height;		/* size of drawable               */
  gint           offset_x, offset_y;	/* offset of layer in image       */

  gint           bytes;			/* bytes per pixel                */
  gint           ID;			/* provides a unique ID           */
  guint32        tattoo;		/* provides a perminant ID        */
  GimpImage     *gimage;		/* gimage owner                   */
  GimpImageType  type;			/* type of drawable               */
  gboolean       has_alpha;		/* drawable has alpha             */

  ParasiteList  *parasites;             /* Plug-in parasite data          */

  /*  Preview variables  */
  GSList        *preview_cache;	       	/* preview caches of the channel  */
  gboolean       preview_valid;		/* is the preview valid?          */
};

struct _GimpDrawableClass
{
  GimpObjectClass parent_class;

  void (* invalidate_preview) (GimpDrawable *drawable);
};


/*  drawable access functions  */

GtkType         gimp_drawable_get_type           (void);

void            gimp_drawable_merge_shadow       (GimpDrawable       *drawable,
						  gint                undo);
void            gimp_drawable_fill               (GimpDrawable       *drawable,
						  guchar              r,
						  guchar              g,
						  guchar              b,
						  guchar              a);

gboolean        gimp_drawable_mask_bounds        (GimpDrawable       *drawable,
						  gint               *x1,
						  gint               *y1,
						  gint               *x2,
						  gint               *y2);

void            gimp_drawable_invalidate_preview (GimpDrawable       *drawable, 
						  gboolean            emit_signal);
gboolean        gimp_drawable_has_alpha          (const GimpDrawable *drawable);
GimpImageType   gimp_drawable_type               (const GimpDrawable *drawable);
GimpImageType   gimp_drawable_type_with_alpha    (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_rgb             (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_gray            (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_indexed         (const GimpDrawable *drawable);
TileManager *   gimp_drawable_data               (const GimpDrawable *drawable);
TileManager *   gimp_drawable_shadow             (GimpDrawable *);
gint            gimp_drawable_bytes              (GimpDrawable *);
gint            gimp_drawable_width              (GimpDrawable *);
gint            gimp_drawable_height             (GimpDrawable *);
gboolean	gimp_drawable_visible	         (const GimpDrawable *drawable);
void            gimp_drawable_offsets            (GimpDrawable *, 
						  gint *, gint *);

guchar        * gimp_drawable_cmap               (const GimpDrawable *drawable);
const gchar   *	gimp_drawable_get_name	         (const GimpDrawable *drawable);
void 		gimp_drawable_set_name	         (GimpDrawable       *drawable,
						  const gchar        *name);

guchar        * gimp_drawable_get_color_at       (GimpDrawable       *drawable,
						  gint                x,
						  gint                y);

void            gimp_drawable_parasite_attach    (GimpDrawable       *drawable,
						  GimpParasite       *parasite);
void            gimp_drawable_parasite_detach    (GimpDrawable       *drawable,
						  const gchar        *parasite);
GimpParasite  * gimp_drawable_parasite_find      (const GimpDrawable *drawable,
						  const gchar        *name);
gchar        ** gimp_drawable_parasite_list      (const GimpDrawable *drawable,
						  gint               *count);
Tattoo          gimp_drawable_get_tattoo         (const GimpDrawable *drawable);
void            gimp_drawable_set_tattoo         (GimpDrawable       *drawable,
						  Tattoo              tattoo);

GimpDrawable *  gimp_drawable_get_ID             (gint);
void		gimp_drawable_deallocate         (GimpDrawable *);
GimpImage *     gimp_drawable_gimage             (const GimpDrawable *drawable);
void            gimp_drawable_set_gimage         (GimpDrawable       *drawable,
						  GimpImage          *gimage);

void            gimp_drawable_configure          (GimpDrawable       *drawable,
						  GimpImage          *gimage,
						  gint                width,
						  gint                height,
						  GimpImageType       type,
						  const gchar        *name);

#endif /* __GIMP_DRAWABLE_H__ */
