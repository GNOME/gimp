#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "gimpbrush.h"
#include "gimpbrushpixmap.h"
#include "gimpbrushlist.h"

#include "paint_core.h"
#include "gimprc.h"
#include "gimpbrushhose.h"

static void
gimp_brush_hose_generate(GimpBrushHose *brush);

static GimpObjectClass* parent_class;

static void
gimp_brush_hose_destroy(GtkObject *object)
{
  GTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
gimp_brush_hose_class_init (GimpBrushHoseClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS(klass);
  
  parent_class = gtk_type_class (GIMP_TYPE_BRUSH_PIXMAP);
  object_class->destroy = gimp_brush_hose_destroy;
}

void
gimp_brush_hose_init(GimpBrushHose *brush)
{
  brush->name = NULL;
  brush->filename = NULL;
  brush->brush_list = gimp_brush_list_new();
}

GtkType gimp_brush_hose_get_type(void)
{
  static GtkType type=0;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushHose",
      sizeof(GimpBrushHose),
      sizeof(GimpBrushHoseClass),
      (GtkClassInitFunc)gimp_brush_hose_class_init,
      (GtkObjectInitFunc)gimp_brush_hose_init,
      /* reserved_1 */ NULL,
      /* reserver_2 */ NULL,
    (GtkClassInitFunc) NULL};
    type=gtk_type_unique(GIMP_TYPE_BRUSH_PIXMAP, &info);
  }
  return type;
}


GimpBrushHose *
gimp_brush_hose_load (char *file_name)
{
  GimpBrushHose *hose;
  GimpBrushPixmap *brush;
  GimpBrushList *list;
  GimpObject o;
  FILE *fp;
  unsigned char buf[sz_BrushHeader];
  gchar buf2[1024];
  BrushHeader header;
  int bn_size;
  unsigned int *hp;
  int i;
  int num_of_brushes;
  int brush_count=0;

  hose = GIMP_BRUSH_HOSE(gimp_type_new(gimp_brush_hose_get_type()));
  GIMP_BRUSH_HOSE(hose)->filename = g_strdup(file_name);

  brush = GIMP_BRUSH_PIXMAP (hose);

  list = gimp_brush_list_new();

  printf("opening hose: %s\n", file_name);

  if ((fp = fopen(file_name, "rb")) == NULL)
    return NULL;
  
  /* the file format starts with a painfully simple text header
     and we use a painfully simple way to read it  */
  if(fgets (buf2, 1024, fp) == NULL)
    return NULL;
  buf2[strlen(buf2) - 1] = '\0';
  hose->name = g_strdup(buf2);

  if(fgets (buf2, 1024, fp) == NULL)
    return NULL;
  //buf2[strlen(buf2) - 1] = '\0';
  num_of_brushes = strtol(buf2,NULL,10);

  /* get the number of brushes */

  printf("filename: %s \n name: %s  number: %i \n",hose->filename,  hose->name, num_of_brushes);


  /* FIXME

     Think i need to flesh out all the bits if this thing is
     going to stop segfaulting

  */

  while(brush_count < num_of_brushes)
    {

      /* we read in the brush mask first */
      
      /*  Read in the header size  */
      if ((fread (buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader)
	{
	  fclose (fp);
	  gimp_object_destroy (hose);
	  gimp_object_destroy (brush);
	  return NULL;
	}
      
      
      if (brush_count > 0)
	brush = GIMP_BRUSH_PIXMAP(gimp_type_new(gimp_brush_pixmap_get_type()));
      GIMP_BRUSH(brush)->filename = g_strdup(file_name);

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
	      gimp_object_destroy (hose);
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
	      gimp_object_destroy (hose);
	      gimp_object_destroy (brush);
	      return NULL;
	    }
	}
  
    
      switch(header.version)
	{
      
	case 1:
	case 2:
	  /*  Get a new brush mask  */
	  GIMP_BRUSH(brush)->mask = temp_buf_new (header.width,
						  header.height,
						  header.bytes,
						  0, 0, NULL);
	  /*  Read the brush mask data  */
	  
	  if ((fread (temp_buf_data (GIMP_BRUSH(brush)->mask),
		      1,header.width * header.height,
		      fp)) <	 header.width * header.height)
	    g_message ("GIMP brush file appears to be truncated.");
  
	  break;
	default:
	  g_message ("Unknown brush format version #%d in \"%s\"\n",
		     header.version, file_name);
  
	  fclose (fp);
	  gimp_object_destroy (hose);
	  gimp_object_destroy (brush);
	  return NULL;
	}
     
      /* PATTERN STUFF HERE */
      /*  Read in the header size  */
      if ((fread (buf, 1, sz_PatternHeader, fp)) < sz_PatternHeader)
	{
	  fclose (fp);
	  gimp_object_destroy (hose);
	  gimp_object_destroy (brush);
	  return NULL;
	}
  

      hp = (unsigned int *) &header;
      for (i = 0; i < (sz_PatternHeader / 4); i++)
	hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
	  (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);
     
  
      brush->pixmap_mask = temp_buf_new (header.width,
					 header.height,
					 header.bytes,
					 0, 0, NULL);
  
     
      /*  Skip the pattern name  */
      if ((bn_size = (header.header_size - sz_PatternHeader)))
	if ((fseek (fp, bn_size, SEEK_CUR)) < 0)
	  {
	    g_message ("Error in GIMP pattern file...aborting.");
	    fclose (fp);
	    gimp_object_destroy (hose);
	    gimp_object_destroy (brush);
	    return NULL;
	  }

      if ((fread (temp_buf_data (brush->pixmap_mask), 1,
		  header.width * header.height * header.bytes, fp))
	  < header.width * header.height * header.bytes)
	g_message ("GIMP pattern file appears to be truncated.");


      gimp_brush_list_add(list,GIMP_BRUSH(brush));
      
      printf("got here brush_count: %i\n", brush_count);
      printf("brush_list_count: %i  \n", gimp_brush_list_length(list));
      
     
      brush_count++;
      printf("brush index is: %i \n", gimp_brush_list_get_brush_index(list, GIMP_BRUSH(brush)));
    }
  
  fclose (fp);

  if (!GIMP_IS_BRUSH_HOSE(hose))
    g_print ("Is not BRUSH_HOSE???\n");
#if 0
  brush = GIMP_BRUSH_PIXMAP((gimp_brush_list_get_brush_by_index(list,0)));
  hose->pixmap_brush = *brush;
#endif
  hose->brush_list = list;
  /* random test code */

  printf("got here, return hose;\n");
  return hose;

}
