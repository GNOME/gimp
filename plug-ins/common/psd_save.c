/*
 * PSD Save Plugin version 1.0 (BETA)
 * This GIMP plug-in is designed to save Adobe Photoshop(tm) files (.PSD)
 *
 * Monigotes
 *
 *     If this plug-in fails to save a file which you think it should,
 *     please tell me what seemed to go wrong, and anything you know
 *     about the image you tried to save.  Please don't send big PSD
 *     files to me without asking first.
 *
 *          Copyright (C) 2000 Monigotes
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Adobe and Adobe Photoshop are trademarks of Adobe Systems
 * Incorporated that may be registered in certain jurisdictions.
 */

/*
 * Revision history:
 *
 *  2000.02 / v1.0 / Monigotes
 *       First version.
 *
 */

/*
 * TODO:
 */

/*
 * BUGS:
 */


/* *** DEFINES *** */

/* set to TRUE if you want debugging, FALSE otherwise */
#define DEBUG FALSE

/* 1: Normal debuggin, 2: Deep debuggin */
#define DEBUG_LEVEL 1

#define IFDBG if (DEBUG)
#define IF_DEEP_DBG if (DEBUG && DEBUG_LEVEL==2)

/* *** END OF DEFINES *** */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include "libgimp/gimp.h"


/* Local types etc
 */


typedef struct PsdLayerDimension
{
  gint left;
  gint top;
  gint32 width;
  gint32 height;
} PSD_Layer_Dimension;


typedef struct PsdImageData
{
  gint compression;         /* 0, if there's no compresion; 1, if there is */

  gint32 image_height;     
  gint32 image_width;      

  gint baseType;

  gint nChannels;           /* Numero de canales de usuario de la imagen */
  gint32 *lChannels;        /* Canales de usuario de la imagen */

  gint nLayers;            		/* Num de capas de la imagen */
  gint32 *lLayers;         		/* Identificador de cada capa */
  PSD_Layer_Dimension* layersDim;	/* Dimensiones de cada capa */

} PSD_Image_Data;

static PSD_Image_Data PSDImageData;



/* Declare some local functions.
 */

static void   query      (void);
static void   run        (gchar    *name,
                          int      nparams,
                          GimpParam  *param,
                          int     *nreturn_vals,
                          GimpParam **return_vals);
static void* xmalloc(size_t n);
static void psd_lmode_layer(gint32 idLayer, gchar* psdMode);
static void reshuffle_cmap_write(guchar *mapGimp);
static void save_header (FILE *fd, gint32 image_id);
static void save_color_mode_data (FILE *fd, gint32 image_id);
static void save_resources (FILE *fd, gint32 image_id);
static void save_layerAndMask (FILE *fd, gint32 image_id);
static void save_data (FILE *fd, gint32 image_id);
static int save_image (gchar *filename, gint32 image_id);
static void xfwrite(FILE *fd, void *buf, long len, gchar *why);
static void write_pascalstring (FILE *fd, char *val, gint paded, gchar *why);
static void write_string (FILE *fd, char *val, gchar *why);
static void write_gchar(FILE *fd, unsigned char val, gchar *why);
static void write_gshort(FILE *fd, gshort val, gchar *why);
static void write_glong(FILE *fd, glong val, gchar *why);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static const gchar *prog_name = "PSD";

MAIN()


static void
query ()
{
  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32, "compression", "Compression type: { NONE (0), LZW (1), PACKBITS (2)" },
    { GIMP_PDB_INT32, "fillorder", "Fill Order: { MSB to LSB (0), LSB to MSB (1)" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_psd_save",
                          "saves files in the Photoshop(tm) PSD file format",
                          "This filter saves files of Adobe Photoshop(tm) native PSD format.  These files may be of any image type supported by GIMP, with or without layers, layer masks, aux channels and guides.",
                          "Monigotes",
                          "Monigotes",
                          "2000",
                          "<Save>/PSD",
			  "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);
  gimp_register_save_handler ("file_psd_save", "psd", "");
}



static void
run (gchar    *name,
     int      nparams,
     GimpParam  *param,
     int     *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunMode      run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  if (strcmp (name, "file_psd_save") == 0)
  {
    IFDBG printf("\n---------------- %s ----------------\n", param[3].data.d_string);

    if ( save_image (param[3].data.d_string,          /* Nombre del fichero */
                     param[1].data.d_image) )         /* Identificador de la imagen */
    {
      values[0].data.d_status = GIMP_PDB_SUCCESS;
    }
    else
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
  }
}



static void *
xmalloc(size_t n)
{
  void *p;

  if (n == 0)
    {
      IFDBG printf("PSD: WARNING: %s: xmalloc asked for zero-sized chunk\n", prog_name);

      return (NULL);
    }

  if ((p = g_malloc(n)) != NULL)
    return p;

  IFDBG printf("%s: out of memory\n", prog_name);
  gimp_quit();
  return NULL;
}

static void psd_lmode_layer(gint32 idLayer, gchar* psdMode)
{
  switch ( gimp_layer_get_mode (idLayer) )
  {
    case GIMP_NORMAL_MODE:
      strcpy (psdMode, "norm");
      break;
    case GIMP_DARKEN_ONLY_MODE:
      strcpy (psdMode, "dark");
      break;
    case GIMP_LIGHTEN_ONLY_MODE:
      strcpy (psdMode, "lite");
      break;
    case GIMP_HUE_MODE:
      strcpy (psdMode, "hue ");
      break;
    case GIMP_SATURATION_MODE:
      strcpy (psdMode, "sat ");
      break;
    case GIMP_COLOR_MODE:
      strcpy (psdMode, "colr");
      break;
    case GIMP_MULTIPLY_MODE:
      strcpy (psdMode, "mul ");
      break;
    case GIMP_SCREEN_MODE:
      strcpy (psdMode, "scrn");
      break;
    case GIMP_DISSOLVE_MODE:
      strcpy (psdMode, "diss");
      break;
    case GIMP_DIFFERENCE_MODE:
      strcpy (psdMode, "diff");
      break;
    case GIMP_VALUE_MODE:                  /* ? */
      strcpy (psdMode, "lum ");
      break;
    case GIMP_OVERLAY_MODE:                /* ? */
      strcpy (psdMode, "over");
      break;
/*    case GIMP_BEHIND_MODE:                 Estos son de GIMP 1.1.14*/
/*    case GIMP_DIVIDE_MODE:                 Estos son de GIMP 1.1.14*/
    case GIMP_ADDITION_MODE:
    case GIMP_SUBTRACT_MODE:
      IFDBG printf("PSD: Warning - unsupported layer-blend mode: %c, using 'norm' mode\n",
                   gimp_layer_get_mode (idLayer));
      strcpy (psdMode, "norm");
      break;
    default:
      IFDBG printf("PSD: Warning - UNKNOWN layer-blend mode, reverting to 'norm'\n");
      strcpy (psdMode, "norm");
      break;
  }
}


static void write_string (FILE *fd, char *val, gchar *why)
{
  write_gchar(fd, strlen(val), why);
  xfwrite(fd, val, strlen(val), why);
}


static void write_pascalstring (FILE *fd, char *val, gint paded, gchar *why)
{
  unsigned char valLength;
  gint i;

  /* Calcula la longitud del string a grabar y lo limita a 255 */

  valLength = ( strlen(val) > 255 ) ? 255 : (unsigned char) strlen(val);

  /* Se realiza la grabacion */

  if ( valLength !=  0 )
  {
    write_gchar(fd, valLength, why);
    xfwrite(fd, val, valLength, why);
  }
  else
    write_gshort(fd, 0, why);

  /* Si la longitud total (longitud + contenido) no es multiplo del valor 'paded',
     se añaden los ceros necesarios para rellenar */

  valLength ++;						/* Le añado el campo longitud */

  if ( (valLength % paded) == 0 )
    return;

  for ( i=0; i < (paded - (valLength % paded)); i++)
    write_gchar(fd, 0, why);
}


static void xfwrite(FILE * fd, void * buf, long len, gchar *why)
{
  if (fwrite(buf, len, 1, fd) == 0)
  {
    IFDBG printf(" Funcion: xfwrite: Error mientras grababa '%s'\n", why);
    gimp_quit();
  }
}




static void write_gchar(FILE *fd, unsigned char val, gchar *why)
{
  unsigned char b[2];
  gint32 pos;

  b[0] = val;
  b[1] = 0;

  pos = ftell(fd);
  if (fwrite(&b, 1, 2, fd) == 0)
  {
    IFDBG printf(" Funcion: write_gchar: Error mientras grababa '%s'\n", why);
    gimp_quit();
  }
  fseek(fd, pos+1, SEEK_SET);
}




static void write_gshort(FILE *fd, gshort val, gchar *why)
{
  unsigned char b[2];
  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;*/

  b[1] = val & 255;
  b[0] = (val >> 8) & 255;

  if (fwrite(&b, 1, 2, fd) == 0)
  {
    IFDBG printf(" Funcion: write_gshort: Error mientras grababa '%s'\n", why);
    gimp_quit();
  }
}




static void write_glong(FILE *fd, glong val, gchar *why)
{
  unsigned char b[4];
  /*  b[0] = val & 255;
      b[1] = (val >> 8) & 255;
      b[2] = (val >> 16) & 255;
      b[3] = (val >> 24) & 255;*/

  b[3] = val & 255;
  b[2] = (val >> 8) & 255;
  b[1] = (val >> 16) & 255;
  b[0] = (val >> 24) & 255;

  if (fwrite(&b, 1, 4, fd) == 0)
  {
    IFDBG printf(" Funcion: write_glong: Error mientras grababa '%s'\n", why);
    gimp_quit();
  }
}


static void pack_pb_line ( guchar* ini, guchar* fin,
                           guchar* datosRes, gshort* longitud )
{
  int i,j;
  gint32 restantes;

  restantes = fin - ini;
  *longitud = 0;

  while ( restantes > 0 )
  {
    /* Busca caracteres iguales a la muestra */

    i = 0;
    while ( ( i < 128 ) &&
            ( ini + i < fin ) &&
            ( (*ini) == *(ini + i) ) )
      i++;

    if ( i > 1 )		/* Ha encontrado iguales */
    {
      IF_DEEP_DBG printf("Repeticion: '%d', %d veces ------------> ", *ini, i);
      IF_DEEP_DBG printf("Grabo: '%d' %d\n", -(i - 1), *ini);

      *(datosRes++) = -(i - 1);
      *(datosRes++) = *ini;

      ini += i;
      restantes -= i;
      *longitud += 2;
    }
    else		/* Busca caracteres distintos al anterior */
    {
      i = 0;
      while ( ( i < 128 ) &&
              ( ini + i + 1 <= fin ) &&
              ( *(ini + i) != *(ini + i + 1) ) )
        i++;

      /* Si solo quedaba 1 en la linea, el while anterior no lo coje  */

      if ( restantes == 1 )
      {
        IF_DEEP_DBG printf("1 Restante:\t");
        i = 1;
      }

      if ( i > 0 )		/* Ha encontrado distintos */
      {
        IF_DEEP_DBG printf("%d distintos              ------------> Grabo: '%d' ", i, i - 1);

        *(datosRes++) = i - 1;
        for (j=0; j<i; j++)
        {
          IF_DEEP_DBG printf ("%d ", *(ini + j));
          *(datosRes++) = *(ini + j);
        }

        IF_DEEP_DBG printf ("\n");

        ini += i;
        restantes -= i;
        *longitud += ( i + 1 );
      }

    } /* else */

    IF_DEEP_DBG printf("Restantes: %d\n", restantes);
  } /* while principal */

  IF_DEEP_DBG printf("\nLongitud total: %d\n", *longitud);

/*  if ( *longitud & 1 )		// longitud impar, se añade un nop (128)
  {
    *longitud += 1;
    *(datosRes++) = 128;

    IF_DEEP_DBG printf("Longitud total modificada: %d\n", *longitud);
  } */
}


static void GRAYA_to_chans ( guchar* greyA, gint numpix,
                             guchar** grey, guchar** alpha )
{
  int i;
  gint nPix;

  if ( greyA == NULL )
  {
    IFDBG printf("greyA es un canal nulo");
    *grey = NULL;
    *alpha = NULL;
    return;
  }

  nPix = numpix/2;
  *grey = xmalloc(nPix);
  *alpha = xmalloc(nPix);

  for (i=0; i<nPix; i++)
  {
    (*grey)[i] = greyA[i*2];
    (*alpha)[i] = greyA[i*2+1];
  }
}


static void RGB_to_chans ( guchar* rgb, gint numpix,
                           guchar** red, guchar** green, guchar** blue )
{
  int i;
  gint nPix;

  if ( rgb == NULL )
  {
    IFDBG printf("rgb es un canal nulo");
    *red = NULL;
    *green = NULL;
    *blue = NULL;
    return;
  }

  nPix = numpix/3;
  *red = xmalloc(nPix);
  *green = xmalloc(nPix);
  *blue = xmalloc(nPix);

  for (i=0; i<nPix; i++)
  {
    (*red)[i] = rgb[i*3];
    (*green)[i] = rgb[i*3+1];
    (*blue)[i] = rgb[i*3+2];
  }
}


static void RGBA_to_chans ( guchar* rgbA, gint numpix,
                            guchar** red, guchar** green, guchar** blue,
                            guchar** alpha )
{
  int i;
  gint nPix;

  if ( rgbA == NULL )
  {
    IFDBG printf("rgb es un canal nulo");
    *red = NULL;
    *green = NULL;
    *blue = NULL;
    *alpha = NULL;
    return;
  }

  nPix = numpix/4;
  *red = xmalloc(nPix);
  *green = xmalloc(nPix);
  *blue = xmalloc(nPix);
  *alpha = xmalloc(nPix);

  for (i=0; i<nPix; i++)
  {
    (*red)[i] = rgbA[i*4];
    (*green)[i] = rgbA[i*4+1];
    (*blue)[i] = rgbA[i*4+2];
    (*alpha)[i] = rgbA[i*4+3];
  }
}


static gint gimpBaseTypeToPsdMode (gint gimpBaseType)
{
  switch (gimpBaseType)
  {
    case GIMP_RGB_IMAGE: return(3);                     /* RGB */
    case GIMP_GRAY_IMAGE: return(1);                    /* Grayscale */
    case GIMP_INDEXED_IMAGE: return(2);                 /* Indexed */
    default:
      g_message ("PSD: Error: Can't convert GIMP base imagetype to PSD mode\n");
      IFDBG printf ("PSD Save: el gimpBaseType vale %d, no puede convertirlo a PSD mode", gimpBaseType);
      gimp_quit();
      return(3);                        /* Retorno RGB por defecto */
  }
}


static gint nCanalesLayer (gint gimpBaseType, gint hasAlpha)
{
  int incAlpha = 0;

  incAlpha = ( hasAlpha == 0 ) ? 0 : 1;

  switch (gimpBaseType)
  {
    case GIMP_RGB_IMAGE: return(3+incAlpha);                /* R,G,B y Alpha (Si existe) */
    case GIMP_GRAY_IMAGE: return(1+incAlpha);               /* G y Alpha (Si existe) */
    case GIMP_INDEXED_IMAGE: return(1+incAlpha);            /* I y Alpha (Si existe) */
    default:
      return(0);                        /* Retorno 0 cananles por defecto */
  }
}


static void  reshuffle_cmap_write(guchar *mapGimp)
{
  guchar *mapPSD;
  int i;

  mapPSD = xmalloc(768);

  for (i=0;i<256;i++)
  {
    mapPSD[i] = mapGimp[i*3];
    mapPSD[i+256] = mapGimp[i*3+1];
    mapPSD[i+512] = mapGimp[i*3+2];
  }

  for (i=0;i<768;i++)
  {
    mapGimp[i] = mapPSD[i];
  }

  g_free(mapPSD);
}


static void save_header (FILE *fd, gint32 image_id)
{
  IFDBG printf (" Funcion: save_header\n");
  IFDBG printf ("      Filas: %d\n", PSDImageData.image_height);
  IFDBG printf ("      Columnas: %d\n", PSDImageData.image_width);
  IFDBG printf ("      Tipo base: %d\n", PSDImageData.baseType);
  IFDBG printf ("      Numero de canales: %d\n", PSDImageData.nChannels);

  xfwrite(fd, "8BPS", 4, "signature");
  write_gshort(fd, 1, "version");
  write_glong(fd, 0, "reserved 1");       /* 6 bytes para el campo 'reservado'. 4 bytes de un long */
  write_gshort(fd, 0, "reserved 1");      /* y 2 bytes de un short */
  write_gshort(fd, PSDImageData.nChannels +
                   nCanalesLayer (PSDImageData.baseType, 0),
               "channels");
  write_glong(fd, PSDImageData.image_height, "rows");
  write_glong(fd, PSDImageData.image_width, "columns");
  write_gshort(fd, 8, "depth");        /* Parece que GIMP solo soporta las
                                           imagenes PSD de 8 bits de profundidad */
  write_gshort(fd, gimpBaseTypeToPsdMode(PSDImageData.baseType), "mode");
}



static void save_color_mode_data (FILE *fd, gint32 image_id)
{
  guchar *cmap;
  guchar *cmap_modificado;
  int i;
  gint32 nColors;

  IFDBG printf (" Funcion: save_color_mode_data\n");

  switch (PSDImageData.baseType)
  {
    case GIMP_INDEXED_IMAGE:
      IFDBG printf ("      Tipo de la imagen: INDEXED\n");

      cmap = gimp_image_get_cmap (image_id, &nColors);
      IFDBG printf ("      Longitud del colormap devuelto por gimp_image_get_cmap: %d\n", nColors);

      if ( nColors == 0 )
      {
        IFDBG printf("      La imagen indexada no tiene colormap\n");
        write_glong(fd, 0, "color data length");
      }
      else if ( nColors != 256 )
      {
        IFDBG printf("      La imagen indexada tiene %d!=256 colores\n", nColors );
        IFDBG printf("      Relleno hasta llegar a 256 con ceros\n" );
        write_glong(fd, 768, "color data length");                   /* Para este tipo, longitud siempre 768 */

        cmap_modificado = xmalloc(768);
        for (i=0; i<nColors*3; i++)
          cmap_modificado[i] = cmap[i];

        for (i=nColors*3; i<768; i++)
          cmap_modificado[i] = 0;

        reshuffle_cmap_write(cmap_modificado);
        xfwrite(fd, cmap_modificado, 768, "colormap");                          /* Graba el mapa de colores reajustado */

        g_free(cmap_modificado);
      }
      else         /* nColors es igual a 256 */
      {
        write_glong(fd, 768, "color data length");                   /* Para este tipo, longitud siempre 768 */
        reshuffle_cmap_write(cmap);
        xfwrite(fd, cmap, 768, "colormap");                          /* Graba el mapa de colores reajustado */
      }
      break;

    default:
      IFDBG printf ("      Tipo de la imagen: No INDEXED\n");
      write_glong(fd, 0, "color data length");
  }
}



static void save_resources (FILE *fd, gint32 image_id)
{
  int i;
  char **chName=NULL;           /* Nombres de los canales */
  char *fileName;               /* Nombre del fichero de la imagen */
  gint32 idActLayer;            /* Id de la capa activa */
  guint nActiveLayer=0;         /* Num de la capa activa */
  gboolean hayActiveLayer;      /* TRUE si hay alguna capa activa */

  gint32 posActual;		/* Posicion: Fin de fichero */
  gint32 posTotal;		/* Posicion: Longitud de la seccion recursos */
  gint32 posNombres;		/* Posicion: Longitud de Nombres de canales */


  /* Los unicos recursos relevantes en GIMP son: 0x03EE, 0x03F0 y 0x0400 */
  /* Para la version 4.0 de Adobe Photoshop, estudiar: 0x0408, 0x040A y 0x040B */

  IFDBG printf (" Funcion: save_resources\n");

  /* Obtiene los nombres de los canales */

  if ( PSDImageData.nChannels > 0 )
    chName = (char **) xmalloc( sizeof(char *) * PSDImageData.nChannels );

  for (i=0; i<PSDImageData.nChannels; i++)
  {
    chName[i] = gimp_channel_get_name(PSDImageData.lChannels[i]);
    IFDBG printf ("      Nombre del canal %d: %s\n", i, chName[i]);
  }

  /* Obtiene el titulo de la imagen, su nombre de fichero */

  fileName = gimp_image_get_filename(image_id);
  IFDBG printf ("      Titulo de la imagen: %s\n", fileName);

  /* Obtiene el numero de la capa activa */

  idActLayer = gimp_image_get_active_layer(image_id);
  IFDBG printf ("      Identificador de capa activa es: %d\n", idActLayer);

  hayActiveLayer = FALSE;
  for (i=0; i<PSDImageData.nLayers; i++)
       if ( idActLayer == PSDImageData.lLayers[i] )
       {
            nActiveLayer = i;
            hayActiveLayer = TRUE;
       }
  
  if (hayActiveLayer)
  {
       IFDBG printf ("      El numero de capa activa es: %d\n", nActiveLayer);
  }
  else
  {
       IFDBG printf ("      No hay capa activa\n");
  }
  

  /* Aqui empiezo a grabar toda la historia */

  posTotal = ftell(fd);
  write_glong(fd, 0, "image resources length");


  /* --------------- Voy a grabar: Nombres de los canales --------------- */

  if ( PSDImageData.nChannels > 0 )
  {
    xfwrite(fd, "8BIM", 4, "imageresources signature");
    write_gshort(fd, 0x03EE, "0x03EE Id");
      /* write_pascalstring(fd, Name, "Id name"); */
    write_gshort(fd, 0, "Id name");             /* Pongo un string nulo, dos ceros */

    /* Marco la posicion actual del fichero */

    posNombres = ftell(fd);
    write_glong(fd, 0, "0x03EE resource size");

    /* Grabo todos los strings */

    for (i=PSDImageData.nChannels-1; i>=0; i--)
/*      write_pascalstring(fd, chName[i], 2, "chanel name"); */
      write_string(fd, chName[i], "channel name");

    /* Calculo y grabo la longitud real de este recurso */

    posActual = ftell(fd);		/* Posicion del fin de fichero */

    fseek(fd, posNombres, SEEK_SET);
    write_glong(fd, posActual - posNombres - sizeof(glong), "0x03EE resource size");
    IFDBG printf ("\n      Longitud total de 0x03EE resource: %d\n",
                  (int) (posActual - posNombres - sizeof(glong)) );

    /* Vuelve al final de fichero para continuar con la grabacion */

    fseek(fd, posActual, SEEK_SET);

    /* Si la longitud es impar, añado un cero */

    if ( ( posActual - posNombres - sizeof(glong) ) & 1 )
      write_gchar(fd, 0, "pad byte");
  }


  /* --------------- Voy a grabar: Numero de capa activa --------------- */

  if ( hayActiveLayer )
  {
    xfwrite(fd, "8BIM", 4, "imageresources signature");
    write_gshort(fd, 0x0400, "0x0400 Id");
      /* write_pascalstring(fd, Name, "Id name"); */
    write_gshort(fd, 0, "Id name");      /* Pongo un string nulo, que son dos ceros */
    write_glong(fd, sizeof(gshort), "0x0400 resource size");

    /* Grabo el titulo como gshort, tendra siempre una longitud par */

    write_gshort(fd, nActiveLayer, "active layer");

    IFDBG printf ("      Longitud total de 0x0400 resource: %d\n", (int) sizeof(gshort) );
  }


  /* --------------- Voy a grabar: Longitud total de la seccion --------------- */

  posActual = ftell(fd);		/* Posicion del fin de fichero */

  fseek(fd, posTotal, SEEK_SET);
  write_glong(fd, posActual - posTotal - sizeof(glong), "image resources length");
  IFDBG printf ("      Longitud total de la seccion de recursos: %d\n",
                (int) (posActual - posTotal - sizeof(glong)) );

  /* Vuelve al final de fichero para continuar con la grabacion */

  fseek(fd, posActual, SEEK_SET);

  g_free(chName);
}


static void get_compress_channel_data (guchar* channel_data,
                                       gint32 channel_cols,
                                       gint32 channel_rows,
                                       gshort** tablaLongitudes,
                                       guchar** datosRes,
                                       glong* longTotalComprimido )
{
  gint i;
  gint32 longitud;		/* Longitud de datos comprimidos */
  gshort longAux;		/* Auxiliar */
  guchar* ini;			/* Posicion de inicio de una fila en channel_data */
  guchar* fin;			/* Posicion de fin de una fila en channel_data */
  gint32 channel_length;	/* Longitud total del canal */


  channel_length = channel_cols * channel_rows;
  *datosRes = g_new (guchar, channel_length * 2);
  *tablaLongitudes = g_new (gshort, channel_rows);

  /* Para cada una de las filas del canal */

  longitud = 0;
  for (i=0; i<channel_rows; i++)
  {
    ini = channel_data + ( i * channel_cols );
    fin = ini + channel_cols;

    /* Creando los datos comprimidos de la linea */
    pack_pb_line ( ini, fin, (*datosRes) + longitud, &longAux );
    (*tablaLongitudes)[i] = longAux;
    longitud += longAux;
  }

  *longTotalComprimido = (longitud + channel_rows * sizeof(gshort)) + sizeof(gshort);
}



























static void save_channel_data (FILE *fd, guchar* channel_data,
                               gint32 channel_cols,
                               gint32 channel_rows,
                               gint32 posLong, gchar *why )
{
  gint i;
  gint32 longitud;		/* Longitud de datos comprimidos */
  glong longTotalCrudo;		/* Longitud total de los datos en crudo */
  glong longTotalComprimido;	/* Longitud total de los datos coprimidos */
  gshort* tablaLongitudes;	/* Longitudes de cada linea comprimida */
  gshort longAux;		/* Auxiliar */
  guchar* datosRes;		/* Datos comprimidos de una linea */
  guchar* ini;			/* Posicion de inicio de una fila en channel_data */
  guchar* fin;			/* Posicion de fin de una fila en channel_data */
  gint32 channel_length;	/* Longitud total del canal */


  channel_length = channel_cols * channel_rows;
  datosRes = g_new (guchar, channel_length * 2);
  tablaLongitudes = g_new (gshort, channel_rows);

  /* Para cada una de las filas del canal */

  longitud = 0;
  for (i=0; i<channel_rows; i++)
  {
    ini = channel_data + ( i * channel_cols );
    fin = ini + channel_cols;

    /* Creando los datos comprimidos de la linea */
    pack_pb_line ( ini, fin, datosRes + longitud, &longAux );
    tablaLongitudes[i] = longAux;
    longitud += longAux;
  }

  /* Calcula las longitudes totales de los dos tipos */

  longTotalCrudo = (channel_rows*channel_cols) + sizeof(gshort);
  longTotalComprimido = (longitud + channel_rows * sizeof(gshort)) + sizeof(gshort);

/*  IFDBG printf("\nLongitud comprimida: %ld\n", longTotalComprimido );
   IFDBG printf("\nLongitud cruda: %ld\n", longTotalCrudo ); */

  if ( longTotalComprimido < longTotalCrudo )
  {
    IFDBG printf("        Grabo datos (RLE): %ld\n", longTotalComprimido );

    write_gshort(fd, 1, "Compression");		/* Graba el tipo de compresion */

    /* Graba la tabla de longitudes de linea comprimida */

    for (i=0; i<channel_rows; i++)
      write_gshort(fd, tablaLongitudes[i], "RLE length");

    xfwrite(fd, datosRes, longitud, why);	/* Graba los datos comprimidos */

    /* Actualiza el valor correspondiente a la longitud de los datos del canal */

    fseek(fd, posLong, SEEK_SET);
    write_glong(fd, longTotalComprimido, "channel data length");
    fseek(fd, 0, SEEK_END);
  }
  else
  {
    IFDBG printf("        Grabo datos (raw): %ld\n", longTotalCrudo);

    write_gshort(fd, 0, "Compression");		/* Graba el tipo de compresion */

    xfwrite(fd, channel_data, channel_length, why);	/* Graba los datos en crudo */
  }
}


static void save_layerAndMask (FILE *fd, gint32 image_id)
{
  int i,j;
  int idCanal;
  gint offset_x;		/* Offset x de cada capa */
  gint offset_y;                /* Offset y de cada capa*/
  gint32 layerHeight;		/* Altura de cada capa */
  gint32 layerWidth;            /* Anchura de cada capa*/
  gchar blendMode[5];			/* Blend mode de la capa */
  unsigned char layerOpacity;		/* Opacidad de la capa */
  unsigned char flags;			/* Banderas de la capa */
  gint nChannelsLayer;			/* Numero de canales de una capa */
  gint32 longCanal;			/* Longitud de los datos de un canal */
  char *layerName;			/* Nombre de la capa */

  gint32 posActual;			/* Posicion: Fin de fichero */
  gint32 posExtraData;			/* Posicion: Longitud de Extra data */
  gint32 posLayerMask;			/* Posicion: Longitud Layer & Mask section */
  gint32 posLayerInfo;			/* Posicion: Longitud Layer info section */
  gint32** posLongChannel;		/* Posicion: Longitud de un canal */


  IFDBG printf (" Funcion: save_layer&mask\n");

  /* Creo la primera dimension del array (capas, canales) */

  posLongChannel = g_new (gint32*, PSDImageData.nLayers);

  /* Seccion Layer and mask information  */

  posLayerMask = ftell(fd);
  write_glong(fd, 0, "layers & mask information length");

  /* Seccion Layer info */

  posLayerInfo = ftell(fd);
  write_glong(fd, 0, "layers info section length");

  /* Seccion Layer structure */

  write_gshort(fd, PSDImageData.nLayers, "Layer structure count");

  /* Seccion Layer records */
  /* La ultima capa que tiene GIMP debe grabarse la primera */	

  for (i=PSDImageData.nLayers-1; i>=0; i--)
  {
    gimp_drawable_offsets( PSDImageData.lLayers[i], &offset_x, &offset_y );
    layerHeight = gimp_drawable_height( PSDImageData.lLayers[i] );
    layerWidth = gimp_drawable_width( PSDImageData.lLayers[i] );

    PSDImageData.layersDim[i].left = offset_x;
    PSDImageData.layersDim[i].top = offset_y;
    PSDImageData.layersDim[i].height = layerHeight;
    PSDImageData.layersDim[i].width = layerWidth;

    IFDBG printf ("      Capa numero: %d\n", i);
    IFDBG printf ("         Offset x: %d\n", PSDImageData.layersDim[i].left);
    IFDBG printf ("         Offset y: %d\n", PSDImageData.layersDim[i].top);
    IFDBG printf ("         Height: %d\n", PSDImageData.layersDim[i].height);
    IFDBG printf ("         Width: %d\n", PSDImageData.layersDim[i].width);

    write_glong(fd, PSDImageData.layersDim[i].top, "Layer top");
    write_glong(fd, PSDImageData.layersDim[i].left, "Layer left");
    write_glong(fd, PSDImageData.layersDim[i].height +
                    PSDImageData.layersDim[i].top, "Layer top");
    write_glong(fd, PSDImageData.layersDim[i].width +
                    PSDImageData.layersDim[i].left, "Layer right");

    nChannelsLayer = nCanalesLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha(PSDImageData.lLayers[i]));


    write_gshort(fd, nChannelsLayer, "Number channels in the layer");
    IFDBG printf ("         Numero de canales: %d\n", nChannelsLayer);

    /* Creo la segunda dimension del array (capas, canales) */

    posLongChannel[i] = g_new (gint32, nChannelsLayer);

/* Probar con gimp_drawable_bytes() */

    for (j=0; j<nChannelsLayer; j++)
    {
      if ( gimp_drawable_has_alpha(PSDImageData.lLayers[i]) )
        idCanal = j - 1;
      else
        idCanal = j;

      write_gshort(fd, idCanal, "Channel ID");
      IFDBG printf ("           - Identificador: %d\n", idCanal);

      /* Grabo la longitud como si no hubiese compression. Si en realidad
         la hay, lo modifico mas tarde, cuando grabe los datos */

      posLongChannel[i][j] = ftell(fd);
      longCanal = sizeof(gshort) + ( PSDImageData.layersDim[i].width *
                                     PSDImageData.layersDim[i].height );

      write_glong(fd, longCanal, "Channel ID");
      IFDBG printf ("             Longitud: %d\n", longCanal );
    }

    xfwrite(fd, "8BIM", 4, "blend mode signature");

    psd_lmode_layer(PSDImageData.lLayers[i], blendMode);
    IFDBG printf ("         Blend mode: %s\n", blendMode);
    xfwrite(fd, blendMode, 4, "blend mode key");

    layerOpacity = ( gimp_layer_get_opacity(PSDImageData.lLayers[i]) * 255.0 ) / 100.0;
    IFDBG printf ("         Opacity: %u\n", layerOpacity);
    write_gchar(fd, layerOpacity, "Opacity");

    /* Parece que el siguiente campo no se usa en GIMP */
    write_gchar(fd, 0, "Clipping");

    flags = 0;
    if ( gimp_layer_get_preserve_transparency(PSDImageData.lLayers[i]) ) flags = flags | 1;
    if ( ! gimp_layer_get_visible(PSDImageData.lLayers[i]) ) flags = flags | 2;
    IFDBG printf ("         Flags: %u\n", flags);
    write_gchar(fd, flags, "Flags");

    /* Byte de relleno para parear */
    write_gchar(fd, 0, "Filler");

    posExtraData = ftell(fd);		             /* Posicion de Extra Data size */
    write_glong(fd, 0, "Extra data size");

    /* OJO Pongo vacia Layer mask / adjustment layer data */
    write_glong(fd, 0, "Layer mask size");
    IFDBG printf ("\n         Layer mask size: %d\n", 0);

    /* OJO Pongo vacia Layer blending ranges data */
    write_glong(fd, 0, "Layer blending size");
    IFDBG printf ("\n         Layer blending size: %d\n", 0);

    layerName = gimp_layer_get_name(PSDImageData.lLayers[i]);
    write_pascalstring(fd, layerName, 4, "layer name");
    IFDBG printf ("\n         Layer name: %s\n", layerName);

    /* Graba la longitud real de: Extra data */

    posActual = ftell(fd);		/* Posicion del fin de fichero */

    fseek(fd, posExtraData, SEEK_SET);
    write_glong(fd, posActual - posExtraData - sizeof(glong), "Extra data size");
    IFDBG printf ("      Longitud total de ExtraData: %d\n",
                  (int) (posActual - posExtraData - sizeof(glong)) );

    /* Vuelve al final de fichero para continuar con la grabacion */

    fseek(fd, posActual, SEEK_SET);
  }


  /* Seccion channel image data */	
  /* La ultima capa que tiene GIMP debe grabarse la primera */	

  for (i=PSDImageData.nLayers-1; i>=0; i--)
  {
    int nCanal;
    GimpDrawable* drawable;
    GimpPixelRgn region;		/* Region de la imagen */
    guchar* data;		/* Bytes que componen un layer con todos sus canales */
    guchar* red;		/* Bytes que componen un canal R */
    guchar* green;		/* Bytes que componen un canal G */
    guchar* blue;		/* Bytes que componen un canal B */
    guchar* gray;		/* Bytes que componen un canal G */
    guchar* alpha;		/* Bytes que componen un canal Alpha */
    gint32 longCanal;		/* Longitud de los datos de un canal */

    IFDBG printf ("\n     Chanels image data. Layer: %d\n", i);

    longCanal = PSDImageData.layersDim[i].width *
                PSDImageData.layersDim[i].height;
    nChannelsLayer = nCanalesLayer (PSDImageData.baseType,
                                    gimp_drawable_has_alpha(PSDImageData.lLayers[i]));
    data = g_new (guchar, longCanal * nChannelsLayer);

    drawable = gimp_drawable_get(PSDImageData.lLayers[i]);

    gimp_pixel_rgn_init(&region, drawable, 0, 0,
                            PSDImageData.layersDim[i].width,
                            PSDImageData.layersDim[i].height, FALSE, FALSE );

    gimp_pixel_rgn_get_rect(&region, data, 0, 0,
                            PSDImageData.layersDim[i].width,
                            PSDImageData.layersDim[i].height );

    IFDBG printf ("        Longitud canal: %d\n", longCanal);

    nCanal = 0;
    switch ( PSDImageData.baseType )
    {
      case GIMP_RGB_IMAGE:

        if ( gimp_drawable_has_alpha(PSDImageData.lLayers[i]) )
        {
          RGBA_to_chans (data, longCanal * nChannelsLayer, &red, &green, &blue, &alpha);
          IFDBG printf ("        Grabando canal alpha...\n");

          save_channel_data (fd, alpha, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             posLongChannel[i][nCanal++],
                             "alpha channel" );
        }
        else
          RGB_to_chans ( data, longCanal * nChannelsLayer, &red, &green, &blue);

        IFDBG printf ("        Grabando canal red...\n");
        save_channel_data (fd, red,PSDImageData.layersDim[i].width,
                           PSDImageData.layersDim[i].height,
                           posLongChannel[i][nCanal++],
                           "red channel" );

        IFDBG printf ("        Grabando canal green...\n");
        save_channel_data (fd, green, PSDImageData.layersDim[i].width,
                           PSDImageData.layersDim[i].height,
                           posLongChannel[i][nCanal++],
                           "green channel" );

        IFDBG printf ("        Grabando canal blue...\n");
        save_channel_data (fd, blue, PSDImageData.layersDim[i].width,
                           PSDImageData.layersDim[i].height,
                           posLongChannel[i][nCanal++],
                           "blue channel" );
        break;

      case GIMP_GRAY_IMAGE:

        if ( gimp_drawable_has_alpha(PSDImageData.lLayers[i]) )
        {
          GRAYA_to_chans ( data, longCanal * nChannelsLayer, &gray, &alpha );

          IFDBG printf ("        Grabando canal alpha...\n");
          save_channel_data (fd, alpha, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             posLongChannel[i][nCanal++],
                             "alpha channel" );

          IFDBG printf ("        Grabando canal gray...\n");
          save_channel_data (fd, gray, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             posLongChannel[i][nCanal++],
                             "gray channel" );
        }
        else
        {
          IFDBG printf ("        Grabando canal gray...\n");
          save_channel_data (fd, data, PSDImageData.layersDim[i].width,
                             PSDImageData.layersDim[i].height,
                             posLongChannel[i][nCanal++],
                             "gray channel" );
        }

        break;

      case GIMP_INDEXED_IMAGE:
        IFDBG printf ("        Grabando canal indexed...\n");
        save_channel_data (fd, data, PSDImageData.layersDim[i].width,
                           PSDImageData.layersDim[i].height,
                           posLongChannel[i][nCanal++],
                           "indexed channel" );
        break;
    }


  }


  posActual = ftell(fd);		/* Posicion del fin de fichero */

  /* Graba la longitud real de: Seccion Layer info */

  fseek(fd, posLayerInfo, SEEK_SET);
  write_glong(fd, posActual - posLayerInfo - sizeof(glong), "layers info section length");
  IFDBG printf ("\n      Longitud total de layers info section: %d\n",
                (int) (posActual - posLayerInfo - sizeof(glong)));

  /* Graba la longitud real de: Seccion Layer and mask information  */

  fseek(fd, posLayerMask, SEEK_SET);
  write_glong(fd, posActual - posLayerMask - sizeof(glong), "layers & mask information length");
  IFDBG printf ("      Longitud total de layers & mask information: %d\n",
                (int) (posActual - posLayerMask - sizeof(glong)));

  /* Vuelve al final de fichero para continuar con la grabacion */

  fseek(fd, posActual, SEEK_SET);
}



static void save_data (FILE *fd, gint32 image_id)
{
  int nCanales;
  int i, j;
  int nCanal;
  gint offset_x;		/* Offset x de cada capa */
  gint offset_y;                /* Offset y de cada capa*/
  gint32 layerHeight;		/* Altura de cada capa */
  gint32 layerWidth;            /* Anchura de cada capa*/
  GimpDrawable* drawable;
  GimpPixelRgn region;		/* Region de la imagen */
  guchar* data;			/* Bytes que componen un layer con todos sus canales */
  guchar* red;			/* Bytes que componen un canal R */
  guchar* green;		/* Bytes que componen un canal G */
  guchar* blue;			/* Bytes que componen un canal B */
  guchar* gray_indexed = NULL;	/* Bytes que componen un canal B */
  gint32 longCanal;		/* Longitud de los datos de un canal */
  gint nChannelsLayer;		/* Numero de canales de una capa */

  gshort** TLdataCompress;
  guchar** dataCompress;
  glong* longDataCompress;
  glong longTotal;


  IFDBG printf ("\n Funcion: save_data\n");

  nCanales = PSDImageData.nChannels +
             nCanalesLayer (PSDImageData.baseType, 0);
  TLdataCompress = g_new (gshort*, nCanales);
  dataCompress = g_new (guchar*, nCanales);
  longDataCompress = g_new (glong, nCanales);

  i = PSDImageData.nLayers - 1;		/* Canal a grabar */
  IFDBG printf ("     Procesando capa %d\n", i);

  gimp_drawable_offsets( PSDImageData.lLayers[i], &offset_x, &offset_y );
  layerHeight = gimp_drawable_height( PSDImageData.lLayers[i] );
  layerWidth = gimp_drawable_width( PSDImageData.lLayers[i] );

  longCanal = layerWidth * layerHeight;
  nChannelsLayer = nCanalesLayer (PSDImageData.baseType,
                                  gimp_drawable_has_alpha(PSDImageData.lLayers[i]));
  data = g_new (guchar, longCanal * nChannelsLayer);

  drawable = gimp_drawable_get(PSDImageData.lLayers[i]);
  gimp_pixel_rgn_init(&region, drawable, 0, 0, layerWidth, layerHeight, FALSE, FALSE );
  gimp_pixel_rgn_get_rect(&region, data, 0, 0, layerWidth, layerHeight);

  nCanal = 0;
  switch ( PSDImageData.baseType )
  {
    case GIMP_RGB_IMAGE:
      RGB_to_chans ( data, longCanal * nChannelsLayer, &red, &green, &blue);

      get_compress_channel_data ( red, layerWidth, layerHeight,
                                  &(TLdataCompress[nCanal]), &(dataCompress[nCanal]),
                                  &(longDataCompress[nCanal]) );
      IFDBG printf ("        Longitud comprimida canal rojo: %ld\n",
                    longDataCompress[nCanal]);
      nCanal++;

      get_compress_channel_data ( green, layerWidth, layerHeight,
                                  &(TLdataCompress[nCanal]), &(dataCompress[nCanal]),
                                  &(longDataCompress[nCanal]) );
      IFDBG printf ("        Longitud comprimida canal verde: %ld\n",
                    longDataCompress[nCanal]);
      nCanal++;

      get_compress_channel_data ( blue, layerWidth, layerHeight,
                                  &(TLdataCompress[nCanal]), &(dataCompress[nCanal]),
                                  &(longDataCompress[nCanal]) );
      IFDBG printf ("        Longitud comprimida canal azul: %ld\n",
                    longDataCompress[nCanal]);
      nCanal++;
      break;

    case GIMP_GRAY_IMAGE:
    case GIMP_INDEXED_IMAGE:
      gray_indexed = data;
      get_compress_channel_data ( gray_indexed, layerWidth, layerHeight,
                                  &(TLdataCompress[nCanal]), &(dataCompress[nCanal]),
                                  &(longDataCompress[nCanal]) );
      IFDBG printf ("        Longitud comprimida canal gray o indexed: %ld\n",
                    longDataCompress[nCanal]);
      nCanal++;
      break;
  }


  for (i=PSDImageData.nChannels-1; i>=0; i--)
  {
    longCanal = PSDImageData.image_width *
                PSDImageData.image_height;

    drawable = gimp_drawable_get(PSDImageData.lChannels[i]);

    gimp_pixel_rgn_init(&region, drawable, 0, 0,
                        PSDImageData.image_width,
                        PSDImageData.image_height, FALSE, FALSE );

    gimp_pixel_rgn_get_rect(&region, data, 0, 0,
                            PSDImageData.image_width,
                            PSDImageData.image_height );

    get_compress_channel_data ( data, layerWidth, layerHeight,
                                &(TLdataCompress[nCanal]), &(dataCompress[nCanal]),
                                &(longDataCompress[nCanal]) );
    IFDBG printf ("     Longitud comprimida canal usuario: %ld\n",
                  longDataCompress[nCanal]);
    nCanal++;
  }

  /* Calculo la longitud de todos los canales comprimidos */

  longTotal = 0;
  for (i=0; i<nCanales; i++)
    longTotal += longDataCompress[i];

  IFDBG printf ("\n     Longitud total de datos comprimidos: %ld\n", longTotal);
  IFDBG printf ("     Longitud total de datos en crudo: %d\n", nCanales*longCanal);

  /* Decido si grabo los datos comprimidos o crudos */

  if ( longTotal < nCanales*longCanal )		/* Grabo comprimidos */
  {
    IFDBG printf ("\n     Grabando los datos comprimidos\n");
    write_gshort(fd, 1, "RLE compression");

    /* Se graban primero todas las longitudes de linea */

    for (i=0; i<nCanales; i++)
      for (j=0; j<layerHeight; j++)
        write_gshort(fd, TLdataCompress[i][j], "line lengths");

    /* Y ahora los datos comprimidos */

    for (i=0; i<nCanales; i++)
    {
      longTotal = 0;
      for (j=0; j<layerHeight; j++)
        longTotal += TLdataCompress[i][j];

      xfwrite(fd, dataCompress[i], longTotal, "channel data");
    }
  }
  else			/* Grabo crudos */
  {
    IFDBG printf ("\n     Grabando los datos en crudo\n");
    write_gshort(fd, 0, "RLE compression");

    /* Los canales de la capa a grabar ya los tengo de antes */

    switch ( PSDImageData.baseType )
    {
      case GIMP_RGB_IMAGE:
        xfwrite(fd, red, longCanal, "red channel data");
        xfwrite(fd, green, longCanal, "green channel data");
        xfwrite(fd, blue, longCanal, "blue channel data");
        break;

      case GIMP_GRAY_IMAGE:
      case GIMP_INDEXED_IMAGE:
        xfwrite(fd, gray_indexed, longCanal, "gray or indexed channel data");
        break;
    }

    /* Y ahora voy a por los canales de usuario */

    for (i=PSDImageData.nChannels-1; i>=0; i--)
    {
      drawable = gimp_drawable_get(PSDImageData.lChannels[i]);

      gimp_pixel_rgn_init(&region, drawable, 0, 0,
                          PSDImageData.image_width,
                          PSDImageData.image_height, FALSE, FALSE );

      gimp_pixel_rgn_get_rect(&region, data, 0, 0,
                              PSDImageData.image_width,
                              PSDImageData.image_height );

      xfwrite(fd, data, longCanal, "channel data");
    }

    /* ¡¡¡ y... YATAAAAA !!!  (¿Como?,¿Yataaa?) */
  }
}



static void get_image_data (FILE *fd, gint32 image_id)
{
  IFDBG printf (" Funcion: get_image_data\n");

  PSDImageData.compression = 0;		/* No hay compresion */

  PSDImageData.image_height = gimp_image_height(image_id);
  IFDBG printf ("      Obtenido numero de filas: %d\n", PSDImageData.image_height);

  PSDImageData.image_width = gimp_image_width(image_id);
  IFDBG printf ("      Obtenido numero de columnas: %d\n", PSDImageData.image_width);

  PSDImageData.baseType = gimp_image_base_type(image_id);
  IFDBG printf ("      Obtenido tipo base: %d\n", PSDImageData.baseType);

  /* El formato PSD no permite imagenes indexadas con capas */

  if ( PSDImageData.baseType == GIMP_INDEXED_IMAGE )
  {
    IFDBG printf ("      Realizo el flatten de la imagen indexada\n");
    gimp_image_flatten(image_id);
  }

  PSDImageData.lChannels = gimp_image_get_channels(image_id, &PSDImageData.nChannels);
  IFDBG printf ("      Obtenido numero de canales: %d\n", PSDImageData.nChannels);

  PSDImageData.lLayers = gimp_image_get_layers(image_id, &PSDImageData.nLayers);
  IFDBG printf ("      Obtenido numero de capas: %d\n", PSDImageData.nLayers);

  PSDImageData.layersDim = g_new (PSD_Layer_Dimension, PSDImageData.nLayers);
}



int save_image (gchar *filename, gint32 image_id)
{
  FILE *fd;
  gchar *name_buf;

  IFDBG printf (" Funcion: save_image\n");

  name_buf = g_malloc (64+strlen(filename));
  IFDBG printf (name_buf, "Saving %s:", filename);
  gimp_progress_init(name_buf);
  g_free(name_buf);

  IFDBG printf ("      Se ha puesto el mensaje en GIMP\n");

  fd = fopen(filename, "wb");
  if (fd==NULL)
  {
    IFDBG printf("PSD Save: no pudo abrir \"%s\"\n", filename);
    return(-1);
  }

  IFDBG printf ("      Se ha abierto el fichero \"%s\"\n", filename);

  get_image_data (fd, image_id);
  save_header (fd, image_id);
  save_color_mode_data (fd, image_id);
  save_resources (fd, image_id);

  /* El formato PSD no permite imagenes indexadas con capas */

  if ( PSDImageData.baseType == GIMP_INDEXED_IMAGE )
    write_glong(fd, 0, "layers info section length");
  else
    save_layerAndMask (fd, image_id);

  /* Si es una indexada, aqui irá la información de la capa y los canales */

  save_data (fd, image_id);

  IFDBG printf ("\n\n");

  fclose(fd);
  return TRUE;
}


