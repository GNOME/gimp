#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "paint_core.h"
#include "gimprc.h"

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
gimp_brush_pixmap_load (char *file_name)
{
  GimpBrushPixmap *brush;
  FILE *fp;
  char string[256];
  float fl;
  float version;
  unsigned char buf[sz_BrushHeader];
  BrushHeader header;
  int bn_size;
  unsigned int * hp;
  char * nothing;
  int i;
  

  brush = GIMP_BRUSH_PIXMAP(gimp_type_new(gimp_brush_pixmap_get_type()));
  GIMP_BRUSH(brush)->filename = g_strdup (file_name);

  if ((fp =fopen(file_name, "rb")) == NULL)
    return NULL;


  /* we read in the brush mask first */

  /*  Read in the header size  */
  if ((fread (buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader)
    {
      fclose (fp);
      gimp_object_destroy (brush);
      return NULL;
    }
  

   /*  rearrange the bytes in each unsigned int  */
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
      (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);
  

  /*  Check for correct file format */
  if (header.magic_number != GBRUSH_MAGIC)
    {
      /*  One thing that can save this error is if the brush is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  gimp_object_destroy (brush);
	  return NULL;
	}
    }

  if (header.version == 1)
  {
     /*  If this is a version 1 brush, set the fp back 8 bytes  */
     fseek (fp, -8, SEEK_CUR);
     header.header_size += 8;
     /*  spacing is not defined in version 1  */
     header.spacing = 25;
  }
  
  
  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sz_BrushHeader)))
    {
      GIMP_BRUSH(brush)->name = (char *) g_malloc (sizeof (char) * bn_size);
      if ((fread (GIMP_BRUSH(brush)->name, 1, bn_size, fp)) < bn_size)
	{
	  g_message ("Error in GIMP brush file...aborting.");
	  fclose (fp);
	  gimp_object_destroy (brush);
	  return NULL;
	}
    }

  
  
  
   switch(header.version)
     {
  
     case 1:
     case 2:
       /*  Get a new brush mask  */
       GIMP_BRUSH(brush)->mask = temp_buf_new (header.width, header.height, header.bytes,
					       0, 0, NULL);
           GIMP_BRUSH(brush)->spacing = header.spacing;
     /* set up spacing axis */
	   GIMP_BRUSH(brush)->x_axis.x = header.width  / 2.0;
	   GIMP_BRUSH(brush)->x_axis.y = 0.0;
	   GIMP_BRUSH(brush)->y_axis.x = 0.0;
	   GIMP_BRUSH(brush)->y_axis.y = header.height / 2.0;
	   /*  Read the brush mask data  */
       if ((fread (temp_buf_data (GIMP_BRUSH(brush)->mask), 1, header.width * header.height,
		   fp)) <	 header.width * header.height)
       g_message ("GIMP brush file appears to be truncated.");
  
       break;
     default:
       g_message ("Unknown brush format version #%d in \"%s\"\n",
		  header.version, file_name);
  
       fclose (fp);
       gimp_object_destroy (brush);
       return NULL;
     }
     
     /* PATTERN STUFF HERE */
     /*  Read in the header size  */
     if ((fread (buf, 1, sz_PatternHeader, fp)) < sz_PatternHeader)
       {
	 fclose (fp);
	 return NULL;
       }
  

     hp = (unsigned int *) &header;
     for (i = 0; i < (sz_PatternHeader / 4); i++)
       hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
	 (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);
     
  
     brush->pixmap_mask = temp_buf_new (header.width, header.height, header.bytes, 0, 0, NULL);
  
     
      /*  Read in the pattern name  */
  if ((bn_size = (header.header_size - sz_PatternHeader)))
    {
  
      nothing = (char *) g_malloc (sizeof (char) * bn_size);
      if ((fread (nothing, 1, bn_size, fp)) < bn_size)
	{
	  g_message ("Error in GIMP pattern file...aborting.");
	  fclose (fp);
	
	  return NULL;
	}
        
    }
  else
    {
        
      nothing = g_strdup ("Unnamed");
    }


  if ((fread (temp_buf_data (brush->pixmap_mask), 1,
	      header.width * header.height * header.bytes, fp))
      < header.width * header.height * header.bytes)
    g_message ("GIMP pattern file appears to be truncated.");

  /*  Clean up  */
  fclose (fp);

  return brush;
}
