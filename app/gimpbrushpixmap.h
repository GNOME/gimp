#ifndef __GIMP_BRUSH_PIXMAP_H__
#define __GIMP_BRUSH_PIXMAP_H__

#include "gimpbrush.h"

typedef struct _GimpBrushPixmap
{
  GimpBrush gbrush;
  TempBuf * pixmap_mask;
} GimpBrushPixmap;

typedef struct _GimpBrushPixmapClass
{
  GimpBrushClass parent_class;
  
   void (* generate)  (GimpBrushPixmap *brush);
} GimpBrushPixmapClass;

/* object stuff */
#define GIMP_TYPE_BRUSH_PIXMAP (gimp_brush_pixmap_get_type ())
#define GIMP_BRUSH_PIXMAP(obj) (GIMP_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PIXMAP, GimpBrushPixmap))
#define GIMP_IS_BRUSH_PIXMAP(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_PIXMAP))

guint gimp_brush_pixmap_get_type (void);

GimpBrushPixmap * gimp_brush_pixmap_new      (char *filename);
GimpBrushPixmap * gimp_brush_pixmap_load      (char *filename);

TempBuf * gimp_brush_pixmap_get_pixmap (GimpBrushPixmap* brush);
#endif  /* __GIMPPIXMAPBRUSH_H__ */
