#ifndef __GIMP_BRUSH_HOSE_H__
#define __GIMP_BRUSH_HOSE_H__

#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "gimpbrushlistP.h"

typedef struct _GimpBrushHose
{
  GimpBrushPixmap pixmap_brush;
  GimpBrushList *brush_list;
  char * name;
  char * filename;
} GimpBrushHose;

typedef struct _GimpBrushHoseClass
{
  GimpBrushPixmapClass parent_class;
  void (* generate) (GimpBrushHose *brush);
} GimpBrushHoseClass;

/* object stuff */
#define GIMP_TYPE_BRUSH_HOSE (gimp_brush_hose_get_type ())
#define GIMP_BRUSH_HOSE(obj) (GIMP_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_HOSE, GimpBrushHose))
#define GIMP_IS_BRUSH_HOSE(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_HOSE))

GtkType gimp_brush_hose_get_type (void);

GimpBrushHose * gimp_brush_hose_new  (char *file_name);
GimpBrushHose * gimp_brush_hose_load (char *file_name);

#endif /* __GIMPBRUSHHOSE_H__ */


  
