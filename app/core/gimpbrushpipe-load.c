#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "patterns.h"
#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "gimpbrushlist.h"

#include "paint_core.h"
#include "gimprc.h"
#include "gimpbrushpipe.h"
#include "libgimp/gimpintl.h"

static void
gimp_brush_pipe_generate(GimpBrushPipe *brush);

static GimpObjectClass* parent_class;

static void
gimp_brush_pipe_destroy(GtkObject *object)
{
  GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
gimp_brush_pipe_class_init (GimpBrushPipeClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS(klass);
  
  parent_class = gtk_type_class (GIMP_TYPE_BRUSH_PIXMAP);
  object_class->destroy = gimp_brush_pipe_destroy;
}

void
gimp_brush_pipe_init(GimpBrushPipe *brush)
{
  brush->name = NULL;
  brush->filename = NULL;
  brush->brush_list = gimp_brush_list_new();
}

GtkType gimp_brush_pipe_get_type(void)
{
  static GtkType type=0;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushPipe",
      sizeof(GimpBrushPipe),
      sizeof(GimpBrushPipeClass),
      (GtkClassInitFunc)gimp_brush_pipe_class_init,
      (GtkObjectInitFunc)gimp_brush_pipe_init,
      /* reserved_1 */ NULL,
      /* reserver_2 */ NULL,
    (GtkClassInitFunc) NULL};
    type=gtk_type_unique(GIMP_TYPE_BRUSH_PIXMAP, &info);
  }
  return type;
}


GimpBrushPipe *
gimp_brush_pipe_load (char *file_name)
{
  GimpBrushPipe *pipe;
  GimpBrushPixmap *brush;
  GimpBrushList *list;
  GPatternP pattern;
  FILE *fp;
  gchar buf2[1024];
  int num_of_brushes;
  int brush_count=0;

  pipe = GIMP_BRUSH_PIPE(gimp_type_new(gimp_brush_pipe_get_type()));
  GIMP_BRUSH_PIPE(pipe)->filename = g_strdup(file_name);

  pattern = (GPatternP) g_malloc (sizeof (GPattern));
  pattern->filename = g_strdup (file_name);
  pattern->name = NULL;
  pattern->mask = NULL;

  brush = GIMP_BRUSH_PIXMAP (pipe);

  list = gimp_brush_list_new();

  if ((fp = fopen(file_name, "rb")) == NULL)
    return NULL;
  
  /* the file format starts with a painfully simple text header
     and we use a painfully simple way to read it  */
  if(fgets (buf2, 1024, fp) == NULL)
    return NULL;
  buf2[strlen(buf2) - 1] = '\0';
  pipe->name = g_strdup(buf2);

  /* get the number of brushes */
  if(fgets (buf2, 1024, fp) == NULL)
    return NULL;
  num_of_brushes = strtol(buf2,NULL,10);


  while(brush_count < num_of_brushes)
    {

     
      if (brush_count > 0)
	brush = GIMP_BRUSH_PIXMAP(gimp_type_new(gimp_brush_pixmap_get_type()));
      GIMP_BRUSH(brush)->filename = g_strdup(file_name);


      /* load the brush */
      if(!gimp_brush_load_brush(GIMP_BRUSH(brush),fp,file_name))
	{
	  g_message (_("failed to load a brush mask in the pipe"));
	  return NULL;
	}

      /* load the pattern data*/
      if(!load_pattern_pattern(pattern, fp, file_name))
       {
	  g_message (_("failed to load a section of pixmap mask in the pipe"));
	  return NULL;
       }
     brush->pixmap_mask = pattern->mask;
     
     gimp_brush_list_add(list,GIMP_BRUSH(brush));
     
     
      brush_count++;
    }
  
  fclose (fp);

  if (!GIMP_IS_BRUSH_PIPE(pipe))
    g_print ("Is not BRUSH_PIPE???\n");

  pipe->brush_list = list;

  return pipe;

}
