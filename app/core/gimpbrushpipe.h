#ifndef __GIMP_BRUSH_PIPE_H__
#define __GIMP_BRUSH_PIPE_H__

#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "gimpbrushlistP.h"

typedef struct _GimpBrushPipe
{
  GimpBrushPixmap pixmap_brush;
  GimpBrushList *brush_list;
  char * name;
  char * filename;
} GimpBrushPipe;

typedef struct _GimpBrushPipeClass
{
  GimpBrushPixmapClass parent_class;
  void (* generate) (GimpBrushPipe *brush);
} GimpBrushPipeClass;

/* object stuff */
#define GIMP_TYPE_BRUSH_PIPE (gimp_brush_pipe_get_type ())
#define GIMP_BRUSH_PIPE(obj) (GIMP_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipe))
#define GIMP_IS_BRUSH_PIPE(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_PIPE))

GtkType gimp_brush_pipe_get_type (void);

GimpBrushPipe * gimp_brush_pipe_new  (char *file_name);
GimpBrushPipe * gimp_brush_pipe_load (char *file_name);

#endif /* __GIMPBRUSHPIPE_H__ */


  
