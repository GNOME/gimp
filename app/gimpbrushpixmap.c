#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "patterns.h"
#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "paint_core.h"
#include "gimprc.h"
#include "libgimp/gimpintl.h"

static void
gimp_brush_pixmap_generate(GimpBrushPixmap *brush);

static GimpObjectClass* parent_class;


static void
gimp_brush_pixmap_destroy(GtkObject *object)
{
  GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
gimp_brush_pixmap_class_init (GimpBrushPixmapClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (gimp_brush_get_type());
  object_class->destroy =  gimp_brush_pixmap_destroy;
}

void
gimp_brush_pixmap_init(GimpBrushPixmap *brush)
{
  brush->pixmap_mask      =   NULL;
}

GtkType gimp_brush_pixmap_get_type(void)
{
  static GtkType type=0;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushPixmap",
      sizeof(GimpBrushPixmap),
      sizeof(GimpBrushPixmapClass),
      (GtkClassInitFunc)gimp_brush_pixmap_class_init,
      (GtkObjectInitFunc)gimp_brush_pixmap_init,
     /* reserved_1 */ NULL,
     /* reserver_2 */ NULL,
    (GtkClassInitFunc) NULL};
    type=gtk_type_unique(GIMP_TYPE_BRUSH, &info);
  }
  return type;
}

TempBuf *
gimp_brush_pixmap_get_pixmap (GimpBrushPixmap* brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_PIXMAP(brush), NULL);
  return brush->pixmap_mask;
}

GimpBrushPixmap *
gimp_brush_pixmap_load (char *filename)
{
  GimpBrushPixmap *brush;
  GPatternP pattern;
  FILE *fp;

  brush = GIMP_BRUSH_PIXMAP(gimp_type_new(gimp_brush_pixmap_get_type()));
  GIMP_BRUSH(brush)->filename = g_strdup (filename);

  pattern = (GPatternP) g_malloc (sizeof (GPattern));
  pattern->filename = g_strdup (filename);
  pattern->name = NULL;
  pattern->mask = NULL;

  if ((fp =fopen(filename, "rb")) == NULL)
    return NULL;


  /* we read in the brush mask first */

  if(!gimp_brush_load_brush(GIMP_BRUSH(brush),fp,filename))
    {
      g_message (_("failed to load a brush mask in the pixmap brush"));
      return NULL;
    }
  
  if(!load_pattern_pattern(pattern, fp, filename))
    {
      g_message (_("failed to load a section of pixmap mask in the pixmap brush"));
      return NULL;
    }
  
  brush->pixmap_mask = pattern->mask;

  /*  Clean up  */
  fclose (fp);

  g_free(pattern);
  return brush;
}
