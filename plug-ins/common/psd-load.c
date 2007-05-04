/*
 * PSD Plugin version 3.0.14
 * This GIMP plug-in is designed to load Adobe Photoshop(tm) files (.PSD)
 *
 * Adam D. Moss <adam@gimp.org> <adam@foxbox.org>
 *
 *     If this plug-in fails to load a file which you think it should,
 *     please file a Bugzilla bug describing what seemed to go wrong,
 *     and anything you know about the image you tried to load.  Attach a
 *     problematic PSD file to the bug report.
 *
 *          Copyright (C) 1997-2004 Adam D. Moss
 *          Copyright (C) 1996      Torsten Martinsen
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
 *  2004-02-12 / v3.0.14 / Adam D. Moss
 *       Fix a twisted utf8-obsessive bug diagnosed by
 *       Piotr Krysiuk <KrysiukP@prokom.pl>
 *
 *  2004-01-06 / v3.0.13 / Adam D. Moss
 *       Disable one of the PanoTools fixes by default, since it causes
 *       regressions in some ordinary PSD file loading.
 *
 *  2004-01-06 / v3.0.12 / Adam D. Moss
 *       Try to avoid 0-sized drawables (including channels) in all
 *       circumstances under GIMP 2.
 *
 *  2004-01-01 / v3.0.11 / Daniel Rogers <dsrogers@phaseveloctiy.org>
 *       GIMP crashes on 0x0 layers, so we skip them.
 *
 *  2003-11-27 / v3.0.10 / Adam D. Moss
 *       GIMP 1.3/2.0 needs its layer/channel names to be UTF8 or it
 *       fails wackily, so convert the strings from the PSD file to
 *       UTF8 instead of using them raw.
 *
 *  2003-10-05 / v3.0.9 / Morten Eriksen
 *       Fixed memory corruption bug: too little memory was allocated
 *       for the bitmap image buffer if (imgwidth % 8 != 0) for
 *       monocolor PSD images.
 *
 *  2003-08-31 / v3.0.8 / applied (modified) patch from Andy Wallis
 *       Fix for handling of layer masks. See bug #68538.
 *
 *  2003-06-16 / v3.0.7 / Adam D. Moss
 *       Avoid memory corruption when things get shot to hell in the
 *       image unpacking phase.  Major version bumped to distinguish
 *       GIMP 1.3 development thread.
 *
 *  2000-08-23 / v2.0.6 / Adam D. Moss
 *       Eliminate more debugging output (don't people have more
 *       substantial problems to report?  I'm poised at my keyboard).
 *
 *  1999-11-14 / v2.0.5 / Adam D. Moss
 *       Applied patch by Andy Hefner to load 1-bit images.
 *
 *  1999-08-13 / v2.0.4 / Adam D. Moss
 *       Allowed NULL layer names again, whee.  (Also fixed the time machine.)
 *
 *  1999-08-20 / v2.0.3 / Adam D. Moss
 *       Ensure that NULL name does not get passed to gimp_layer_new(),
 *       or it will fail to create the layer and cause problems down
 *       the line (only since April 1999).
 *
 *  1999-01-18 / v2.0.2 / Adam D. Moss
 *       Better guess at how PSD files store Guide position precision.
 *
 *  1999-01-10 / v2.0.1 / Adam D. Moss
 *       Greatly reduced memory requirements for layered image loading -
 *       we now do just-in-time channel unpacking.  Some little
 *       cleanups too.
 *
 *  1998-09-04 / v2.0.0 / Adam D. Moss
 *       Now recognises and loads the new Guides extensions written
 *       by Photoshop 4 and 5.
 *
 *  1998-07-31 / v1.9.9.9f / Adam D. Moss
 *       Use GIMP_OVERLAY_MODE if available.
 *
 *  1998-07-31 / v1.9.9.9e / Adam D. Moss
 *       Worked around some buggy PSD savers (suspect PS4 on Mac) - ugh.
 *       Fixed a bug when loading layer masks of certain dimensions.
 *
 *  1998-05-04 / v1.9.9.9b / Adam D. Moss
 *       Changed the Pascal-style string-reading stuff.  That fixed
 *       some file-padding problems.  Made all debugging output
 *       compile-time optional (please leave it enabled for now).
 *       Reduced memory requirements; still much room for improvement.
 *
 *  1998-04-28 / v1.9.9.9 / Adam D. Moss
 *       Fixed the correct channel interlacing of 'raw' flat images.
 *       Thanks to Christian Kirsch and Jay Cox for spotting this.
 *       Changed some of the I/O routines.
 *
 *  1998-04-26 / v1.9.9.8 / Adam D. Moss
 *       Implemented Aux-channels for layered files.  Got rid
 *       of <endian.h> nonsense.  Improved Layer Mask padding.
 *       Enforced num_layers/num_channels limit checks.
 *
 *  1998-04-23 / v1.9.9.5 / Adam D. Moss
 *       Got Layer Masks working, got Aux-channels working
 *       for unlayered files, fixed 'raw' channel loading, fixed
 *       some other mini-bugs, slightly better progress meters.
 *       Thanks to everyone who is helping with the testing!
 *
 *  1998-04-21 / v1.9.9.1 / Adam D. Moss
 *       A little cleanup.  Implemented Layer Masks but disabled
 *       them again - PS masks can be a different size to their
 *       owning layer, unlike those in GIMP.
 *
 *  1998-04-19 / v1.9.9.0 / Adam D. Moss
 *       Much happier now.
 *
 *  1997-03-13 / v1.9.0 / Adam D. Moss
 *       Layers, channels and masks, oh my.
 *       + Bugfixes & rearchitecturing.
 *
 *  1997-01-30 / v1.0.12 / Torsten Martinsen
 *       Flat PSD image loading.
 */

/*
 * TODO:
 *
 *      Crush 16bpp channels *
 *        CMYK -> RGB *
 *      * I don't think these should be done lossily -- wait for
 *        GIMP to be able to support them natively.
 *
 *      Read in the paths.
 *
 */

/*
 * BUGS:
 *
 *      Sometimes creates a superfluous aux channel?  Harmless.
 */



/* *** USER DEFINES *** */

#define LOAD_PROC "file-psd-load"

/* set to TRUE if you want debugging, FALSE otherwise */
#define PSD_DEBUG FALSE

/* the max number of channels that this plugin should let a layer have */
#define MAX_CHANNELS 30

/* set to TRUE to allow a fix for transparency in PSD files
   generated by PanoTools that unfortunately causes regressions
   in some other ordinary files saved by Photoshop. */
#define PANOTOOLS_FIX FALSE

/* *** END OF USER DEFINES *** */



#define IFDBG if (PSD_DEBUG)

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* Local types etc
 */
typedef enum
{
  PSD_UNKNOWN_IMAGE,
  PSD_RGB_IMAGE,
  PSD_RGBA_IMAGE,
  PSD_GRAY_IMAGE,
  PSD_GRAYA_IMAGE,
  PSD_INDEXED_IMAGE,
  PSD_INDEXEDA_IMAGE,
  PSD_BITMAP_IMAGE
} psd_imagetype;


typedef struct PsdChannel
{
  gchar   *name;
  guchar  *data;
  gint     type;

  guint32  compressedsize;

  fpos_t   fpos; /* Remember where the data is in the file, so we can
                    come back to it! */

 /* We can't just assume that the channel's width and height are the
  * same as those of the layer that owns the channel, since this
  * channel may be a layer mask, which Photoshop allows to have a
  * different size from the layer which it applies to.
  */
  guint32  width;
  guint32  height;

} PSDchannel;

typedef struct PsdGuide
{
  gboolean horizontal; /* else vertical */
  gint     position;
} PSDguide;

typedef struct PsdLayer
{
  gint        num_channels;
  PSDchannel *channel;

  gint32      x;
  gint32      y;
  guint32     width;
  guint32     height;

  gchar       blendkey[4];
  guchar      opacity;
  gchar       clipping;

  gboolean    protecttrans;
  gboolean    visible;

  gchar      *name;

  gint32      lm_x;
  gint32      lm_y;
  gint32      lm_width;
  gint32      lm_height;
} PSDlayer;


typedef gint32 Fixed;

typedef struct
{
  Fixed  hRes;
  gint16 hRes_unit;
  gint16 widthUnit;

  Fixed  vRes;
  gint16 vRes_unit;
  gint16 heightUnit;

/* Res_unit :
        1 == Pixels per inch
        2 == Pixels per cm
*/

} PSDresolution;

typedef struct PsdImage
{
  gint           num_layers;
  PSDlayer      *layer;

  gboolean       absolute_alpha;

  gint           type;

  guint          colmaplen;
  guchar        *colmapdata;

  guint          num_aux_channels;
  PSDchannel     aux_channel[MAX_CHANNELS];

  guint          num_guides;
  PSDguide      *guides;

  gchar         *caption;

  guint          active_layer_num;

  guint          resolution_is_set;
  PSDresolution  resolution;
} PSDimage;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static GimpImageType         psd_type_to_gimp_type      (psd_imagetype  psdtype);
static GimpImageBaseType     psd_type_to_gimp_base_type (psd_imagetype  psdtype);
static GimpLayerModeEffects  psd_lmode_to_gimp_lmode    (gchar          modekey[4]);
static GimpUnit              psd_unit_to_gimp_unit      (gint           psdunit);

static gint32                load_image                 (const gchar  *filename);


/* Various local variables...
 */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


static PSDimage psd_image;


static struct
{
  gchar    signature[4];
  guint16  version;
  guchar   reserved[6];
  guint16  channels;
  guint32  rows;
  guint32  columns;
  guint16  bpp;
  guint16  mode;
  guint32  imgreslen;
  guint32  miscsizelen;
  guint16  compression;
  guint16 *rowlength;
  glong    imgdatalen;
} PSDheader;


static gchar * modename[] =
{
  "Bitmap",
  "Grayscale",
  "Indexed Colour",
  "RGB Colour",
  "CMYK Colour",
  "<invalid>",
  "<invalid>",
  "Multichannel",
  "Duotone",
  "Lab Colour",
  "<invalid>"
};


static void   unpack_pb_channel            (FILE    *fd,
                                            guchar  *dst,
                                            gint32   unpackedlen,
                                            guint32 *offset);
static void   decode                       (long     clen,
                                            long     uclen,
                                            guchar  *src,
                                            guchar  *dst,
                                            int      step);
static void   packbitsdecode               (long    *clenp,
                                            long     uclen,
                                            guchar  *src,
                                            guchar  *dst,
                                            int      step);
static void   cmyk2rgb                     (guchar  *src,
                                            guchar  *destp,
                                            long     width,
                                            long     height,
                                            int      alpha);
static void   cmykp2rgb                    (guchar  *src,
                                            guchar  *destp,
                                            long     width,
                                            long     height,
                                            int      alpha);
static void   bitmap2gray                  (guchar  *src,
                                            guchar  *dest,
                                            long     w,
                                            long     h);
static guchar getguchar                    (FILE    *fd,
                                            gchar   *why);
static gint16 getgint16                    (FILE    *fd,
                                            gchar   *why);
static gint32  getgint32                     (FILE    *fd,
                                            gchar   *why);
static void   xfread                       (FILE    *fd,
                                            void    *buf,
                                            long     len,
                                            gchar   *why);
static void   xfread_interlaced            (FILE    *fd,
                                            guchar  *buf,
                                            long     len,
                                            gchar   *why,
                                            gint     step);
static void   read_whole_file              (FILE    *fd);
static void   reshuffle_cmap               (guchar  *map256);
static gchar *getpascalstring              (FILE    *fd,
                                            gchar   *why);
static gchar *getstring                    (size_t   n,
                                            FILE    *fd,
                                            gchar   *why);
static void   throwchunk                   (size_t   n,
                                            FILE    *fd,
                                            gchar   *why);
static void   dumpchunk                    (size_t   n,
                                            FILE    *fd,
                                            gchar   *why);
static void   seek_to_and_unpack_pixeldata (FILE    *fd,
                                            gint     layeri,
                                            gint     channeli);
static void   validate_aux_channel_name    (gint     aux_index);



MAIN ()


static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the Photoshop(tm) PSD file format",
                          "This filter loads files of Adobe Photoshop(tm) "
                          "native PSD format.  These files may be of any "
                          "image type supported by GIMP, with or without "
                          "layers, layer masks, aux channels and guides.",
                          "Adam D. Moss & Torsten Martinsen",
                          "Adam D. Moss & Torsten Martinsen",
                          "1996-1998",
                          "Photoshop image",
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-psd");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "psd",
                                    "",
                                    "0,string,8BPS");
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[2];
  GimpRunMode      run_mode;
  gint32           image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[0].data.d_status = GIMP_PDB_SUCCESS;
          values[1].type          = GIMP_PDB_IMAGE;
          values[1].data.d_image  = image_ID;
        }
      else
        {
          values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
}


static char *
sanitise_string (char *old_name)
{
  if (old_name)
    {
      char *rtn = gimp_any_to_utf8 (old_name, -1,
                                    _("Invalid UTF-8 string in PSD file"));
      g_free(old_name);
      return rtn;
    }

  return NULL;
}

static GimpImageType
psd_type_to_gimp_type (psd_imagetype psdtype)
{
  switch(psdtype)
    {
    case PSD_RGBA_IMAGE:     return GIMP_RGBA_IMAGE;
    case PSD_RGB_IMAGE:      return GIMP_RGB_IMAGE;
    case PSD_GRAYA_IMAGE:    return GIMP_GRAYA_IMAGE;
    case PSD_GRAY_IMAGE:     return GIMP_GRAY_IMAGE;
    case PSD_INDEXEDA_IMAGE: return GIMP_INDEXEDA_IMAGE;
    case PSD_INDEXED_IMAGE:  return GIMP_INDEXED_IMAGE;
    case PSD_BITMAP_IMAGE:   return GIMP_GRAY_IMAGE;
    default:                 return GIMP_RGB_IMAGE;
    }
}

static GimpLayerModeEffects
psd_lmode_to_gimp_lmode (gchar modekey[4])
{
  if (strncmp (modekey, "norm", 4) == 0) return GIMP_NORMAL_MODE;
  if (strncmp (modekey, "dark", 4) == 0) return GIMP_DARKEN_ONLY_MODE;
  if (strncmp (modekey, "lite", 4) == 0) return GIMP_LIGHTEN_ONLY_MODE;
  if (strncmp (modekey, "hue ", 4) == 0) return GIMP_HUE_MODE;
  if (strncmp (modekey, "sat ", 4) == 0) return GIMP_SATURATION_MODE;
  if (strncmp (modekey, "colr", 4) == 0) return GIMP_COLOR_MODE;
  if (strncmp (modekey, "mul ", 4) == 0) return GIMP_MULTIPLY_MODE;
  if (strncmp (modekey, "scrn", 4) == 0) return GIMP_SCREEN_MODE;
  if (strncmp (modekey, "diss", 4) == 0) return GIMP_DISSOLVE_MODE;
  if (strncmp (modekey, "diff", 4) == 0) return GIMP_DIFFERENCE_MODE;
  if (strncmp (modekey, "lum ", 4) == 0) return GIMP_VALUE_MODE;
  if (strncmp (modekey, "hLit", 4) == 0) return GIMP_HARDLIGHT_MODE;
  if (strncmp (modekey, "sLit", 4) == 0) return GIMP_SOFTLIGHT_MODE;
  if (strncmp (modekey, "over", 4) == 0) return GIMP_OVERLAY_MODE;

  g_printerr("PSD: Warning - UNKNOWN layer-blend mode, reverting to 'normal'\n");

  return GIMP_NORMAL_MODE;
}

static GimpImageBaseType
psd_type_to_gimp_base_type (psd_imagetype psdtype)
{
  switch(psdtype)
    {
    case PSD_RGBA_IMAGE:
    case PSD_RGB_IMAGE: return GIMP_RGB;
    case PSD_BITMAP_IMAGE:
    case PSD_GRAYA_IMAGE:
    case PSD_GRAY_IMAGE: return GIMP_GRAY;
    case PSD_INDEXEDA_IMAGE:
    case PSD_INDEXED_IMAGE: return GIMP_INDEXED;
    default:
      /* If I follow the code that calls this function correctly, this
       * should never happen, so a g_assert_not_reached() would
       * probably be better. --tml
       */
      g_message ("Error: Can't convert PSD imagetype to GIMP imagetype");
      gimp_quit();
    }
}

static GimpUnit
psd_unit_to_gimp_unit (int psdunit)
{
  switch (psdunit)
    {
    case 1:
      return GIMP_UNIT_INCH;
    case 2: /* this means cm to PS, but MM is as close as we have */
      return GIMP_UNIT_MM;
    case 3:
      return GIMP_UNIT_POINT;
    case 4:
      return GIMP_UNIT_PICA;
    case 5: /* 5 == Columns, but what the heck is a column? */
    default:
      IFDBG printf("Warning: unable to convert psd unit %d to gimp unit\n", psdunit);
      return GIMP_UNIT_PIXEL;
    }
}

static GimpImageBaseType
psd_mode_to_gimp_base_type (guint16 psdtype)
{
  switch (psdtype)
    {
    case 0:
      g_message (_("Cannot handle bitmap PSD files"));
      gimp_quit();
    case 1:
      return GIMP_GRAY;
    case 2:
      return GIMP_INDEXED;
    case 3:
      return GIMP_RGB;
    case 4:
      g_message (_("Cannot handle PSD files in CMYK color"));
      gimp_quit();
    case 7:
      g_message (_("Cannot handle PSD files in Multichannel color"));
      gimp_quit();
    case 8:
      g_message (_("Cannot handle PSD files in Duotone color"));
      gimp_quit();
    case 9:
      g_message (_("Cannot handle PSD files in Lab color"));
      gimp_quit();
    default:
      g_message (_("Cannot handle the color mode %d of the PSD file"), psdtype);
      gimp_quit();
    }
}

static void
reshuffle_cmap (guchar *map256)
{
  guchar *tmpmap;
  gint    i;

  tmpmap = g_malloc (3 * 256);

  for (i = 0; i < 256; i++)
    {
      tmpmap[i*3  ] = map256[i];
      tmpmap[i*3+1] = map256[i+256];
      tmpmap[i*3+2] = map256[i+512];
    }

  memcpy (map256, tmpmap, 3 * 256);
  g_free (tmpmap);
}

static void
dispatch_resID (guint    ID,
                FILE    *fd,
                guint32 *offset,
                guint32  Size)
{
  if ( (ID <= 0x0bb6) && (ID >= 0x07d0) ) /* 2998 && 2000 */
    {
      IFDBG printf ("\t\tThe psd plugin does not currently support reading path data.\n");
      throwchunk (Size, fd, "dispatch_res path throw");
      (*offset) += Size;
    }
  else
    switch (ID)
      {
      case 0x03ee: /* 1006 */
        {
          gint32 remaining = Size;

          IFDBG printf ("\t\tALPHA CHANNEL NAMES:\n");
          if (Size > 0)
            {
              do
                {
                  guchar slen;

                  slen = getguchar (fd, "alpha channel name length");
                  (*offset)++;
                  remaining--;

                  /* Check for (Mac?) Photoshop (4?) file-writing bug */
                  if (slen > remaining)
                    {
                      IFDBG {printf("\nYay, a file bug.  "
                                    "Yuck.  Photoshop 4/Mac?  "
                                    "I'll work around you.\n");fflush(stdout);}
                      break;
                    }

                  if (slen)
                    {
                      guint32 alpha_name_len;
                      gchar* sname = getstring(slen, fd, "alpha channel name");

                      alpha_name_len = strlen (sname);

                      (*offset) += alpha_name_len;
                      remaining -= alpha_name_len;

                      sname = sanitise_string (sname);

                      psd_image.aux_channel[psd_image.num_aux_channels].name =
                        sname;

                      IFDBG printf("\t\t\tname: \"%s\"\n",
                                   psd_image.aux_channel[psd_image.num_aux_channels].name);
                    }
                  else
                    {
                      psd_image.aux_channel[psd_image.num_aux_channels].name =
                        NULL;
                      IFDBG
                        {printf("\t\t\tNull channel name %d.\n",
                                psd_image.num_aux_channels);fflush(stdout);}
                    }

                  psd_image.num_aux_channels++;

                  if (psd_image.num_aux_channels > MAX_CHANNELS)
                    {
                      g_message (_("Cannot handle PSD file with more than %d channels"),
                                 MAX_CHANNELS);
                      gimp_quit();
                    }
                }
              while (remaining > 0);
            }

          if (remaining)
            {
              IFDBG
                dumpchunk (remaining, fd, "alphaname padding 0 throw");
              else
                throwchunk (remaining, fd, "alphaname padding 0 throw");
              (*offset) += remaining;
              remaining = 0;
            }
        }
        break;
      case 0x03ef: /* 1007 */
        IFDBG printf("\t\tDISPLAYINFO STRUCTURE: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f0: /* 1008 */ /* FIXME: untested */
        {
          psd_image.caption = getpascalstring(fd, "caption string");
          (*offset)++;

          if (psd_image.caption)
            {
              IFDBG printf("\t\t\tcontent: \"%s\"\n",psd_image.caption);
              (*offset) += strlen(psd_image.caption);
            }
        }
        break;
      case 0x03f2: /* 1010 */
        IFDBG printf("\t\tBACKGROUND COLOR: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f4: /* 1012 */
        IFDBG printf("\t\tGREY/MULTICHANNEL HALFTONING INFO: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f5: /* 1013 */
        IFDBG printf("\t\tCOLOUR HALFTONING INFO: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f6: /* 1014 */
        IFDBG printf("\t\tDUOTONE HALFTONING INFO: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f7: /* 1015 */
        IFDBG printf("\t\tGREYSCALE/MULTICHANNEL TRANSFER FUNCTION: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f8: /* 1016 */
        IFDBG printf("\t\tCOLOUR TRANSFER FUNCTION: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03f9: /* 1017 */
        IFDBG printf("\t\tDUOTONE TRANSFER FUNCTION: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03fa: /* 1018 */
        IFDBG printf("\t\tDUOTONE IMAGE INFO: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03fb: /* 1019 */
        IFDBG printf("\t\tEFFECTIVE BLACK/WHITE VALUES: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03fe: /* 1022 */
        IFDBG printf("\t\tQUICK MASK INFO: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x0400: /* 1024 */
        {
          IFDBG printf("\t\tLAYER STATE INFO:\n");
          psd_image.active_layer_num = getgint16(fd, "ID target_layer_num");

          IFDBG printf("\t\t\ttarget: %d\n",(gint)psd_image.active_layer_num);
          (*offset) += 2;
        }
        break;
      case 0x0402: /* 1026 */
        IFDBG printf("\t\tLAYER GROUP INFO: unhandled\n");
        IFDBG printf("\t\t\t(Inferred number of layers: %d)\n",(gint)(Size/2));
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x0405: /* 1029 */
        IFDBG printf ("\t\tIMAGE MODE FOR RAW FORMAT: unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x0408: /* 1032 */
        {
          gint32 remaining = Size;
          int i;
          IFDBG printf ("\t\tGUIDE INFORMATION:\n");

          if (Size > 0)
            {
              gint16 magic1, magic2, magic3, magic4, magic5, magic6;
              gint32 num_guides;
              PSDguide *guide;

              magic1 = getgint16(fd, "guide"); (*offset) += 2;
              magic2 = getgint16(fd, "guide"); (*offset) += 2;
              magic3 = getgint16(fd, "guide"); (*offset) += 2;
              magic4 = getgint16(fd, "guide"); (*offset) += 2;
              magic5 = getgint16(fd, "guide"); (*offset) += 2;
              magic6 = getgint16(fd, "guide"); (*offset) += 2;
              remaining -= 12;

              IFDBG printf("\t\t\tSize: %d\n", Size);
              IFDBG printf("\t\t\tMagic: %d %d %d %d %d %d\n",
                           magic1, magic2, magic3, magic4, magic5, magic6);

              IFDBG printf("\t\t\tMagic: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
                           magic1, magic2, magic3, magic4, magic5, magic6);

              num_guides = getgint32(fd, "guide");
              (*offset) += 4; remaining -= 4;

              if (remaining != num_guides*5)
                {
                  IFDBG printf ("** FUNNY AMOUNT OF GUIDE DATA (%d)\n",
                                remaining);
                  goto funny_amount_of_guide_data;
                }

              IFDBG printf("\t\t\tNumber of guides is %d\n", num_guides);
              psd_image.num_guides = num_guides;
              psd_image.guides = g_new(PSDguide, num_guides);
              guide = psd_image.guides;

              for (i = 0; i < num_guides; i++, guide++)
                {
                  guide->position = getgint32(fd, "guide");
                  IFDBG printf ("Position: %d     %x\n", guide->position,
                                guide->position);
                  guide->horizontal = (1 == getguchar(fd, "guide"));
                  (*offset) += 5; remaining -= 5;

                  if (guide->horizontal)
                    {
                      guide->position =
                        RINT((double)(guide->position * (magic4>>8))
                             /(double)(magic4&255));
                    }
                  else
                    {
                      guide->position =
                        RINT((double)(guide->position * (magic6>>8))
                             /(double)(magic6&255));
                    }

                  IFDBG printf("\t\t\tGuide %d at %d, %s\n", i+1,
                               guide->position,
                               guide->horizontal ? "horizontal" : "vertical");
                }
            }

        funny_amount_of_guide_data:

          if (remaining)
            {
              IFDBG
                {
                  printf ("** GUIDE INFORMATION DROSS: ");
                  dumpchunk(remaining, fd, "dispatch_res");
                }
              else
                {
                  throwchunk (remaining, fd, "dispatch_res");
                }

              (*offset) += remaining;
            }
        }
        break;
      case 0x03ed: /* 1005 */
        {
          IFDBG printf ("\t\tResolution Info:\n");
          psd_image.resolution_is_set = 1;

          psd_image.resolution.hRes = getgint32(fd, "hRes");
          psd_image.resolution.hRes_unit = getgint16(fd, "hRes_unit");
          psd_image.resolution.widthUnit = getgint16(fd, "WidthUnit");
          psd_image.resolution.vRes = getgint32(fd, "vRes");
          psd_image.resolution.vRes_unit = getgint16(fd, "vRes_unit");
          psd_image.resolution.heightUnit = getgint16(fd, "HeightUnit");
          (*offset) += Size;
          IFDBG  printf("\t\t\tres = %f, %f\n",
                        psd_image.resolution.hRes / 65536.0,
                        psd_image.resolution.vRes / 65536.0);
        } break;

      case 0x0409: /* 1033 */
        /* DATA LAYOUT for thumbail resource */
        /* 4 bytes     format             (1 = jfif, 0 = raw) */
        /* 4 bytes     width              width of thumbnail  */
        /* 4 bytes     height             height of thumbnail */
        /* 4 bytes     widthbytes         for validation only?*/
        /* 4 bytes     size               for validation only?*/
        /* 4 bytes     compressed size    for validation only?*/
        /* 2 bytes     bits per pixel     Always 24?          */
        /* 2 bytes     planes             Always 1?           */
        /* size bytes  data               JFIF (or raw) data  */
        IFDBG printf("\t\t<Photoshop 4.0 style thumbnail (BGR)>  unhandled\n");
        /* for resource 0x0409 we have to swap the r and b channels
           after decoding */
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x040C: /* 1036 */
        /* See above */
        IFDBG printf("\t\t<Photoshop 5.0 style thumbnail (RGB)> unhandled\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;
      case 0x03e9: /* 1001 */
      case 0x03f1: /* 1009 */
      case 0x03f3: /* 1011 */
      case 0x03fd: /* 1021 */
      case 0x0401: /* 1025 */
      case 0x0404: /* 1028 */
      case 0x0406: /* 1030 */
      case 0x0bb7: /* 2999 */
      case 0x2710: /* 10000 */
        IFDBG printf ("\t\t<Field is irrelevant to GIMP at this time.>\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;

      case 0x03e8: /* 1000 */
      case 0x03eb: /* 1003 */
        IFDBG printf ("\t\t<Obsolete Photoshop 2.0 field.>\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;

      case 0x03fc: /* 1020 */
      case 0x03ff: /* 1023 */
      case 0x0403: /* 1027 */
        IFDBG printf ("\t\t<Obsolete field.>\n");
        throwchunk (Size, fd, "dispatch_res");
        (*offset) += Size;
        break;

      default:
        IFDBG
          {
            printf ("\t\t<Undocumented field.>\n");
            dumpchunk(Size, fd, "dispatch_res");
          }
        else
          throwchunk (Size, fd, "dispatch_res");

        (*offset) += Size;
        break;
      }
}

static void
do_layer_record (FILE    *fd,
                 guint32 *offset,
                 gint     layernum)
{
  PSDlayer *layer;
  gint32    top, left, bottom, right;
  guint32   extradatasize, layermaskdatasize, layerrangesdatasize;
  guint32   totaloff;
  gchar     sig[4];
  guchar    flags;
  gint      i;

  IFDBG printf("\t\t\tLAYER RECORD (layer %d)\n", (int)layernum);

  layer = psd_image.layer + layernum;

  /* table 11-12 */
  top = getgint32 (fd, "layer top");
  (*offset)+=4;
  left = getgint32 (fd, "layer left");
  (*offset)+=4;
  bottom = getgint32 (fd, "layer bottom");
  (*offset)+=4;
  right = getgint32 (fd, "layer right");
  (*offset)+=4;

  layer->x      = left;
  layer->y      = top;
  layer->width  = right - left;
  layer->height = bottom - top;

  IFDBG printf("\t\t\t\tLayer extents: (%d,%d) -> (%d,%d)\n",
               left,top,right,bottom);

  layer->num_channels = getgint16 (fd, "layer num_channels");
  (*offset)+=2;

  IFDBG printf("\t\t\t\tNumber of channels: %d\n",
               (int)layer->num_channels);

  if (layer->num_channels)
    {
      layer->channel = g_new(PSDchannel, layer->num_channels);

      for (i = 0; i < layer->num_channels; i++)
        {
          PSDchannel *channel = layer->channel + i;

          /* table 11-13 */
          IFDBG printf("\t\t\t\tCHANNEL LENGTH INFO (%d)\n", i);

          channel->type = getgint16(fd, "channel id");
          (*offset)+=2;
          IFDBG printf("\t\t\t\t\tChannel TYPE: %d\n", channel->type);

          channel->compressedsize = getgint32(fd, "channeldatalength");
          (*offset)+=4;
          IFDBG printf("\t\t\t\t\tChannel Data Length: %d\n",
                       channel->compressedsize);
        }
    } else {
      IFDBG printf("\t\t\t\tOo-er, layer has no channels.  Hmm.\n");
    }

  xfread(fd, sig, 4, "layer blend sig");
  (*offset)+=4;

  if (strncmp(sig, "8BIM", 4)!=0)
    {
      IFDBG printf("\t\t\t(layer blend signature '%c%c%c%c' is incorrect: quitting)\n", sig[0], sig[1], sig[2], sig[3]);
      g_message ("Error: layer blend signature is incorrect. :-(");
      gimp_quit();
    }

  xfread(fd, layer->blendkey, 4, "layer blend key");
  (*offset)+=4;

  IFDBG printf("\t\t\t\tBlend type: PSD(\"%c%c%c%c\") = GIMP(%d)\n",
               layer->blendkey[0],
               layer->blendkey[1],
               layer->blendkey[2],
               layer->blendkey[3],
               psd_lmode_to_gimp_lmode(layer->blendkey));

  layer->opacity = getguchar(fd, "layer opacity");
  (*offset)++;

  IFDBG printf("\t\t\t\tLayer Opacity: %d\n", layer->opacity);

  layer->clipping = getguchar(fd, "layer clipping");
  (*offset)++;

  IFDBG printf("\t\t\t\tLayer Clipping: %d (%s)\n",
               layer->clipping,
               layer->clipping == 0 ? "base" : "non-base");

  flags = getguchar(fd, "layer flags");
  (*offset)++;

  IFDBG printf("\t\t\t\tLayer Flags: %d (%s, %s)\n", flags,
         flags&1?"lock alpha":"don't lock alpha",
         flags&2?"visible":"not visible");

  layer->protecttrans = (flags & 1) ? TRUE : FALSE;
  layer->visible = (flags & 2) ? FALSE : TRUE;

  getguchar(fd, "layer record filler");
  (*offset)++;

  extradatasize = getgint32(fd, "layer extra data size");
  (*offset)+=4;
  IFDBG printf("\t\t\t\tEXTRA DATA SIZE: %d\n",extradatasize);

  /* FIXME: should do something with this data */
  /*throwchunk (extradatasize, fd, "layer extradata throw");
  (*offset) += extradatasize;*/

  totaloff = (*offset) + extradatasize;

  /* table 11-14 */
  layermaskdatasize = getgint32(fd, "layer mask data size");
  (*offset)+=4;
  IFDBG printf("\t\t\t\t\tLAYER MASK DATA SIZE: %d\n", layermaskdatasize);

  if (layermaskdatasize)
    {
      guchar color;
      guchar flags;
      int    o = 0;

      top    = getgint32(fd, "lmask top");
      o += 4;
      left   = getgint32(fd, "lmask left");
      o += 4;
      bottom = getgint32(fd, "lmask bottom");
      o += 4;
      right  = getgint32(fd, "lmask right");
      o += 4;

      layer->lm_x = left;
      layer->lm_y = top;
      layer->lm_width = right - left;
      layer->lm_height = bottom - top;

      color = getguchar(fd, "lmask color");
      flags = getguchar(fd, "lmask flags");

      o += 2;

      IFDBG printf ("\t\t\t\t\t\ttop:    %d\n", top);
      IFDBG printf ("\t\t\t\t\t\tleft:   %d\n", left);
      IFDBG printf ("\t\t\t\t\t\tbottom: %d\n", bottom);
      IFDBG printf ("\t\t\t\t\t\tright:  %d\n", right);
      IFDBG printf ("\t\t\t\t\t\tcolor:  %d\n", color);
      IFDBG printf ("\t\t\t\t\t\tflags:  %X\n", flags);
      IFDBG printf ("\t\t\t\t\t\t\trelative: %d\n", flags & 0x1);
      IFDBG printf ("\t\t\t\t\t\t\tvisible:  %d\n", ((flags & 0x2) >> 1));
      IFDBG printf ("\t\t\t\t\t\t\tinvert:   %d\n", ((flags & 0x4) >> 2));

      throwchunk (layermaskdatasize - o, fd, "extra layer mask data");
      (*offset) += layermaskdatasize;
    }

  layerrangesdatasize = getgint32 (fd, "layer ranges data size");
  (*offset) += 4;
  IFDBG printf("\t\t\t\t\t\tLAYER RANGES DATA SIZE: %d\n", layermaskdatasize);

  if (layerrangesdatasize)
    {
      throwchunk (layerrangesdatasize, fd, "layer ranges data throw");
      (*offset) += layerrangesdatasize;
    }

  layer->name = getpascalstring (fd, "layer name");
  (*offset)++;

  if (layer->name)
    {
      (*offset) += strlen (layer->name);
      IFDBG printf("\t\t\t\t\t\tLAYER NAME: '%s'\n", layer->name);
      layer->name = sanitise_string (layer->name);
    }
  else
    {
      IFDBG printf ("\t\t\t\t\t\tNULL LAYER NAME\n");
    }
  /* If no layermask data - set offset and size from layer data */
  if (! layermaskdatasize)
    {
      IFDBG g_printerr ("Setting layer mask data layer\n");

      psd_image.layer[layernum].lm_x      = psd_image.layer[layernum].x;
      psd_image.layer[layernum].lm_y      = psd_image.layer[layernum].y;
      psd_image.layer[layernum].lm_width  = psd_image.layer[layernum].width;
      psd_image.layer[layernum].lm_height = psd_image.layer[layernum].height;
    }

  if (totaloff-(*offset) > 0)
    {
      IFDBG
        {
          printf ("Warning: layer record dross: ");
          dumpchunk (totaloff-(*offset), fd, "layer record dross throw");
        }
      else
        {
          throwchunk (totaloff-(*offset), fd, "layer record dross throw");
        }
      (*offset) = totaloff;
    }
}

static void
do_layer_struct (FILE    *fd,
                 guint32 *offset)
{
  gint i;

  IFDBG printf ("\t\tLAYER STRUCTURE SECTION\n");

  psd_image.num_layers = getgint16 (fd, "layer struct numlayers");
  (*offset)+=2;

  IFDBG printf("\t\t\tCanonical number of layers: %d%s\n",
         psd_image.num_layers>0?
         (int)psd_image.num_layers:abs(psd_image.num_layers),
         psd_image.num_layers>0?"":" (absolute/alpha)");

  if (psd_image.num_layers < 0)
    {
      psd_image.num_layers = -psd_image.num_layers;
      psd_image.absolute_alpha = TRUE;
    }
  else
    {
      psd_image.absolute_alpha = FALSE;
    }

  psd_image.layer = g_new0 (PSDlayer, psd_image.num_layers);

  for (i = 0; i < psd_image.num_layers; i++)
    {
      do_layer_record (fd, offset, i);
    }
}

static void
do_layer_pixeldata (FILE    *fd,
                    guint32 *offset)
{
  gint layeri, channeli;

  for (layeri = 0; layeri < psd_image.num_layers; layeri++)
    {
      PSDlayer *layer = psd_image.layer + layeri;

      for (channeli = 0; channeli < layer->num_channels; channeli++)
        {
          PSDchannel *channel = layer->channel + channeli;

          if (channel->type == -2)
            {
              channel->width = layer->lm_width;
              channel->height = layer->lm_height;
            }
          else
            {
              channel->width = layer->width;
              channel->height = layer->height;
            }

          fgetpos (fd, &channel->fpos);

          throwchunk (channel->compressedsize, fd, "channel data skip");
          (*offset) += channel->compressedsize;
        }
    }
}

static void
seek_to_and_unpack_pixeldata (FILE *fd,
                              gint  layeri,
                              gint  channeli)
{
  int         width, height;
  guchar     *tmpline;
  gint        compression;
  guint32     offset = 0;
  PSDchannel *channel = &psd_image.layer[layeri].channel[channeli];

  fsetpos(fd, &channel->fpos);

  compression = getgint16(fd, "layer channel compression type");
  offset+=2;

  width  = channel->width;
  height = channel->height;

  IFDBG
    {
      printf ("\t\t\tLayer (%d) Channel (%d:%d) Compression: %d (%s)\n",
              layeri,
              channeli,
              channel->type,
              compression,
              compression==0?"raw":(compression==1?"RLE":"*UNKNOWN!*"));

      fflush (stdout);
    }

  channel->data = g_malloc (width * height);

  tmpline = g_malloc (width + 1);

  switch (compression)
    {
    case 0: /* raw data */
      {
        gint linei;

        for (linei = 0; linei < height; linei++)
          {
            xfread (fd, channel->data + linei * width, width,
                    "raw channel line");
            offset += width;
          }

#if 0
        /* Pad raw data to multiple of 2? */
        if ((height * width) & 1)
          {
            getguchar (fd, "raw channel padding");
            offset++;
          }
#endif
      }
      break;
    case 1: /* RLE, one row at a time, padded to an even width */
      {
        gint linei;
        gint blockread;

        /* we throw this away because in theory we can trust the
           data to unpack to the right length... hmm... */
        throwchunk (height * 2, fd, "widthlist");
        offset += height * 2;

        blockread = offset;

        /*IFDBG {printf("\nHere comes the guitar solo...\n");
          fflush(stdout);}*/

        for (linei = 0; linei < height; linei++)
          {
            /*printf(" %d ", *offset);*/
            unpack_pb_channel (fd, tmpline,
                               width
                               /*+ (width&1)*/,
                               &offset);
            memcpy (channel->data + linei * width, tmpline, width);
          }

        IFDBG {printf("\t\t\t\t\tActual compressed size was %d bytes\n",
                      offset-blockread);fflush(stdout);}
      }
      break;

    default: /* *unknown* */
      IFDBG {printf("\nEEP!\n");fflush(stdout);}
      g_message ("*** Unknown compression type in channel.");
      gimp_quit();
    }

  g_free (tmpline);
}

static void
do_layers (FILE    *fd,
           guint32 *offset)
{
  guint32 section_length;

  section_length = getgint32 (fd, "layerinfo sectionlength");
  (*offset)+=4;

  IFDBG printf ("\tLAYER INFO SECTION\n");
  IFDBG printf ("\t\tSECTION LENGTH: %u\n", section_length);

  do_layer_struct (fd, offset);

  do_layer_pixeldata (fd, offset);
}

static void
do_layer_and_mask (FILE *fd)
{
  guint32 offset = 0;
  guint32 Size   = PSDheader.miscsizelen;

  glong   offset_now = ftell (fd);

  IFDBG printf ("LAYER AND MASK INFO\n");
  IFDBG printf ("\tSECTION LENGTH: %u\n", Size);

  if (Size == 0)
    return;

  do_layers (fd, &offset);

  IFDBG {printf("And...?\n");fflush(stdout);}

  if (offset < Size)
    {
      IFDBG
        {
          printf ("PSD: Supposedly there are %d bytes of mask info left.\n",
                  Size-offset);
          if ((Size-offset == 4) || (Size-offset == 24))
            printf("     That sounds good to me.\n");
          else
            printf("     That sounds strange to me.\n");
        }


      /*      if ((getguchar(fd, "mask info throw")!=0) ||
          (getguchar(fd, "mask info throw")!=0) ||
          (getguchar(fd, "mask info throw")!=0) ||
          (getguchar(fd, "mask info throw")!=0))
        {
          printf("*** This mask info block looks pretty bogus.\n");
        }*/
    }
  else
    printf("PSD: Stern warning - no mask info.\n");


  /* If 'offset' wasn't being buggily updated, we wouldn't need this. (!?) */
  fseek(fd, Size+offset_now, SEEK_SET);
}

static void
do_image_resources (FILE *fd)
{
  guint16  ID;
  gchar   *Name;
  guint32  Size;
  guint32  offset = 0;

  IFDBG printf ("IMAGE RESOURCE BLOCK:\n");

  psd_image.resolution_is_set = 0;

  /* FIXME: too trusting that the file isn't corrupt */
  while (offset < PSDheader.imgreslen-1)
    {
      gchar sig[4];

      xfread (fd, sig, 4, "imageresources signature");
      offset += 4;


      /* generic information about a block ID */


      ID = getgint16 (fd, "ID num");
      offset += 2;
      IFDBG printf ("\tID: 0x%04x / ",ID);

      Name = getpascalstring (fd, "ID name");
      offset++;

      if (Name)
        {
          IFDBG printf ("\"%s\" ", Name);
          offset += strlen (Name);

          if (!(strlen (Name) & 1))
            {
              throwchunk (1, fd, "ID name throw");
              offset ++;
            }
          g_free (Name);
        }
      else
        {
          throwchunk (1, fd, "ID name throw2");
          offset++;
        }

      Size = getgint32 (fd, "ID Size");
      offset += 4;
      IFDBG printf ("Size: %d\n", Size);

      if (strncmp (sig, "8BIM", 4) == 0)
        dispatch_resID (ID, fd, &offset, Size);
      else
        {
          printf ("PSD: Warning, unknown resource signature \"%.4s\" at or before offset %d ::: skipping\n", sig, offset - 8);
          throwchunk (Size, fd, "Skipping Unknown Resource");
          offset += Size;
        }

      if (Size&1)
        {
          IFDBG printf ("+1");
          throwchunk (1, fd, "ID content throw");
          offset ++;
        }
    }

  /*  if (offset != PSDheader.imgreslen)
    {
      printf("\tSucking imageres byte...\n");
      throwchunk (1, fd, "imageres suck");
      offset ++;
    }*/

}


#if PANOTOOLS_FIX
/* Convert RGB data to RGBA data */
static guchar *
RGB_to_RGBA (const guchar *rgb_data,
             gint          numpix)
{
  guchar *rtn;
  int     i,j;

  if (!rgb_data)
    {
      printf ("NULL rgb data - eep!");
      return NULL;
    }

  rtn = g_malloc (numpix * 4);

  j = 0;
  for (i = 0; i < numpix; i++)
    {
      rtn[j++] = rgb_data[i*3];
      rtn[j++] = rgb_data[i*3+1];
      rtn[j++] = rgb_data[i*3+2];
      rtn[j++] = 255;
    }

  return rtn;
}
#endif /* PANOTOOLS_FIX */


static guchar *
chans_to_GRAYA (const guchar *grey,
                const guchar *alpha,
                gint          numpix)
{
  guchar *rtn;
  int     i;

  if (!grey || !alpha)
    {
      printf ("NULL channel - eep!");
      return NULL;
    }

  rtn = g_malloc (numpix * 2);

  for (i = 0; i < numpix; i++)
    {
      rtn[i*2  ] = grey[i];
      rtn[i*2+1] = alpha[i];
    }

  return rtn;
}

static guchar *
chans_to_RGB (const guchar *red,
              const guchar *green,
              const guchar *blue,
              gint          numpix)
{
  guchar *rtn;
  int     i;

  if (!red || !green || !blue)
    {
      printf ("NULL channel - eep!");
      return NULL;
    }

  rtn = g_malloc (numpix * 3);

  for (i = 0; i < numpix; i++)
    {
      rtn[i*3  ] = red[i];
      rtn[i*3+1] = green[i];
      rtn[i*3+2] = blue[i];
    }

  return rtn;
}

static guchar *
chans_to_RGBA (const guchar *red,
               const guchar *green,
               const guchar *blue,
               const guchar *alpha,
               gint          numpix)
{
  guchar   *rtn;
  int       i;
  gboolean  careful = FALSE;

  if (!red || !green || !blue || !alpha)
    {
      printf ("chans_to_RGBA : NULL channel - eep!");
      careful = TRUE;
    }

  rtn = g_malloc (numpix * 4);

  if (!careful)
    {
      for (i = 0; i < numpix; i++)
        {
          rtn[i*4  ] = red[i];
          rtn[i*4+1] = green[i];
          rtn[i*4+2] = blue[i];
          rtn[i*4+3] = alpha[i];
        }
    }
  else
    {
      for (i = 0; i < numpix; i++)
        {
          rtn[i*4  ] = (red   == NULL) ? 0 : red[i];
          rtn[i*4+1] = (green == NULL) ? 0 : green[i];
          rtn[i*4+2] = (blue  == NULL) ? 0 : blue[i];
          rtn[i*4+3] = (alpha == NULL) ? 0 : alpha[i];
        }
    }

  return rtn;
}

static gboolean
psd_layer_has_alpha (PSDlayer *layer)
{
  int i;

  for (i = 0; i < layer->num_channels; i++)
    {
      if (layer->channel[i].type == -1)
        {
          return TRUE;
        }
    }

  return FALSE;
}

static void
validate_aux_channel_name (gint aux_index)
{
  if (psd_image.aux_channel[aux_index].name == NULL)
    {
      if (aux_index == 0)
        psd_image.aux_channel[aux_index].name = g_strdup ("Aux channel");
      else
        psd_image.aux_channel[aux_index].name =
          g_strdup_printf ("Aux channel #%d", aux_index);
    }
}

static void
extract_data_and_channels (guchar       *src,
                           gint          gimpstep,
                           gint          psstep,
                           gint32        image_ID,
                           GimpDrawable *drawable,
                           gint          width,
                           gint          height)
{
  guchar       *primary_data;
  guchar       *aux_data;
  GimpPixelRgn  pixel_rgn;

  IFDBG printf ("Extracting primary channel data (%d channels)\n"
                "\tand %d auxiliary channels.\n", gimpstep, psstep-gimpstep);

  /* gimp doesn't like 0 width/height drawables. */
  if ((width == 0) || (height == 0))
    {
      IFDBG printf("(bad channel dimensions -- skipping)");
      return;
    }

  primary_data = g_malloc (width * height * gimpstep);
  {
    int pix, chan;

    for (pix = 0; pix < width * height; pix++)
      {
        for (chan = 0; chan < gimpstep; chan++)
          {
            primary_data[pix * gimpstep + chan] = src[pix * psstep + chan];
          }
      }

    gimp_pixel_rgn_init (&pixel_rgn, drawable,
                         0, 0, drawable->width, drawable->height,
                         TRUE, FALSE);
    gimp_pixel_rgn_set_rect (&pixel_rgn, primary_data,
                             0, 0, drawable->width, drawable->height);

    gimp_drawable_flush (drawable);
    gimp_drawable_detach (drawable);
  }
  g_free (primary_data);

  aux_data = g_malloc (width * height);
  {
    int           pix, chan, aux_index;
    gint32        channel_ID;
    GimpDrawable *chdrawable;
    GimpRGB       colour;

    gimp_rgb_set (&colour, 0.0, 0.0, 0.0);

    for (chan = gimpstep; chan < psstep; chan++)
      {
        for (pix = 0; pix < width * height; pix++)
          {
            aux_data [pix] = src [pix * psstep + chan];
          }

        aux_index = chan - gimpstep;
        validate_aux_channel_name (aux_index);

        channel_ID = gimp_channel_new (image_ID,
                                       psd_image.aux_channel[aux_index].name,
                                       width, height,
                                       100.0, &colour);
        gimp_image_add_channel (image_ID, channel_ID, 0);
        gimp_drawable_set_visible (channel_ID, FALSE);

        chdrawable = gimp_drawable_get (channel_ID);

        gimp_pixel_rgn_init (&pixel_rgn, chdrawable,
                             0, 0, chdrawable->width, chdrawable->height,
                             TRUE, FALSE);
        gimp_pixel_rgn_set_rect (&pixel_rgn, aux_data,
                                 0, 0, chdrawable->width, chdrawable->height);

        gimp_drawable_flush (chdrawable);
        gimp_drawable_detach (chdrawable);
      }
  }
  g_free (aux_data);

  IFDBG printf ("Done with that.\n\n");
}

static void
extract_channels (guchar *src,
                  gint    num_wanted,
                  gint    psstep,
                  gint32  image_ID,
                  gint    width,
                  gint    height)
{
  guchar       *aux_data;
  GimpPixelRgn  pixel_rgn;

  IFDBG printf ("Extracting %d/%d auxiliary channels.\n", num_wanted, psstep);

  /* gimp doesn't like 0 width/height drawables. */
  if ((width == 0) || (height == 0))
    {
      IFDBG printf("(bad channel dimensions -- skipping)");
      return;
    }

  aux_data = g_malloc (width * height);
  {
    int           pix, chan, aux_index;
    gint32        channel_ID;
    GimpDrawable *chdrawable;
    GimpRGB       colour;

    gimp_rgb_set (&colour, 0.0, 0.0, 0.0);

    for (chan = psstep - num_wanted; chan < psstep; chan++)
      {
        for (pix = 0; pix < width * height; pix++)
          {
            aux_data [pix] = src [pix * psstep + chan];
          }

        aux_index = chan - (psstep - num_wanted);
        validate_aux_channel_name (aux_index);

        channel_ID = gimp_channel_new (image_ID,
                                       psd_image.aux_channel[aux_index].name,
                                       width, height,
                                       100.0, &colour);
        gimp_image_add_channel (image_ID, channel_ID, 0);
        gimp_drawable_set_visible (channel_ID, FALSE);

        chdrawable = gimp_drawable_get (channel_ID);

        gimp_pixel_rgn_init (&pixel_rgn, chdrawable,
                             0, 0, chdrawable->width, chdrawable->height,
                             TRUE, FALSE);
        gimp_pixel_rgn_set_rect (&pixel_rgn, aux_data,
                                 0, 0, chdrawable->width, chdrawable->height);

        gimp_drawable_flush (chdrawable);
        gimp_drawable_detach (chdrawable);
      }
  }
  g_free(aux_data);

  IFDBG printf ("Done with that.\n\n");
}

static void
resize_mask (guchar *src,
             guchar *dest,
             gint32  src_x,
             gint32  src_y,
             gint32  src_w,
             gint32  src_h,
             gint32  dest_w,
             gint32  dest_h)
{
  gint x, y;

  IFDBG printf ("--> %p %p : %d %d . %d %d . %d %d\n",
                src, dest,
                src_x, src_y,
                src_w, src_h,
                dest_w, dest_h);

  for (y = 0; y < dest_h; y++)
    {
      for (x = 0; x < dest_w; x++)
        {
          /* Avoid a 1-pixel border top-left */
          if ((x >= src_x) && (x < src_x + src_w) &&
              (y >= src_y) && (y < src_y + src_h))
            {
              dest[dest_w * y + x] =
                src[src_w * (y - src_y) + (x - src_x)];
            }
          else
            {
              dest[dest_w * y + x] = 255;
            }
        }
    }
}

static gint32
load_image (const gchar *name)
{
  FILE          *fd;
  gboolean       want_aux;
  guchar        *cmykbuf;
  guchar        *dest = NULL, *temp;
  long           channels, nguchars;
  psd_imagetype  imagetype;
  gboolean       cmyk = FALSE;
  gint           step = 1;
  gint32         image_ID = -1;
  gint32         layer_ID = -1;
  GimpDrawable  *drawable = NULL;
  GimpPixelRgn   pixel_rgn;
  gint32         iter;
  fpos_t         tmpfpos;
  int            red_chan, grn_chan, blu_chan, alpha_chan, ichan;

  IFDBG printf ("------- %s ---------------------------------\n",name);

  fd = g_fopen (name, "rb");
  if (! fd)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (name), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (name));

  read_whole_file (fd);

  if (psd_image.num_layers > 0) /* PS3-style */
    {
      int               lnum;
      GimpImageBaseType imagetype;

      imagetype = psd_mode_to_gimp_base_type (PSDheader.mode);
      image_ID =
        gimp_image_new (PSDheader.columns, PSDheader.rows, imagetype);
      gimp_image_set_filename (image_ID, name);

      if (psd_image.resolution_is_set)
        {
          if (psd_image.resolution.widthUnit == 2)  /* px/cm */
            {
              gdouble factor = gimp_unit_get_factor (GIMP_UNIT_MM) / 10.0;

              psd_image.resolution.hRes *= factor;
              psd_image.resolution.vRes *= factor;
            }

          gimp_image_set_resolution (image_ID,
                                     psd_image.resolution.hRes / 65536.0,
                                     psd_image.resolution.vRes / 65536.0);

          /* currently can only set one unit for the image so we use the
             horizontal unit from the psd image */
          gimp_image_set_unit (image_ID,
                               psd_unit_to_gimp_unit (psd_image.resolution.widthUnit));
        }

      fgetpos (fd, &tmpfpos);

      for (lnum = 0; lnum < psd_image.num_layers; lnum++)
        {
          gint      numc;
          guchar   *merged_data = NULL;
          PSDlayer *layer       = psd_image.layer + lnum;
          gboolean  empty       = FALSE;

          /*
           * Since PS supports sloppy bounding boxes it is possible to
           * have a 0x0 or Xx0 or 0xY layer.  Gimp doesn't support a
           * 0x0 layer so we insert an empty layer of image size
           * instead.
           */
          if ((layer->width == 0) || (layer->height == 0))
            {
              empty = TRUE;

              layer->x      = 0;
              layer->y      = 0;
              layer->width  = gimp_image_width (image_ID);
              layer->height = gimp_image_height (image_ID);
            }

          numc = layer->num_channels;

          IFDBG printf ("Hey, it's a LAYER with %d channels!\n", numc);

          switch (imagetype)
            {
            case GIMP_GRAY:
              {
                IFDBG printf("It's GRAY.\n");
                if (! empty)
                  {
                    if (!psd_layer_has_alpha (layer))
                      {
                        merged_data = g_malloc (layer->width * layer->height);
                        seek_to_and_unpack_pixeldata (fd, lnum, 0);
                        memcpy (merged_data, layer->channel[0].data,
                                layer->width * layer->height);

                        g_free (layer->channel[0].data);
                      }
                    else
                      {
                        seek_to_and_unpack_pixeldata (fd, lnum, 0);
                        seek_to_and_unpack_pixeldata (fd, lnum, 1);
                        merged_data = chans_to_GRAYA (layer->channel[1].data,
                                                      layer->channel[0].data,
                                                      layer->width *
                                                      layer->height);

                        g_free (layer->channel[0].data);
                        g_free (layer->channel[1].data);
                      }
                  }

                layer_ID = gimp_layer_new (image_ID,
                                           layer->name,
                                           layer->width,
                                           layer->height,
                                           (numc == 1) ?
                                           GIMP_GRAY_IMAGE : GIMP_GRAYA_IMAGE,
                                           (100.0 * layer->opacity) / 255.0,
                                           psd_lmode_to_gimp_lmode (layer->blendkey));
              }
              break;

            case GIMP_RGB:
              {
                IFDBG printf ("It's RGB, %dx%d.\n", layer->width,
                              layer->height);

                if (! empty)
                  {
                    if (!psd_layer_has_alpha (layer))
                      {
                        seek_to_and_unpack_pixeldata (fd, lnum, 0);
                        seek_to_and_unpack_pixeldata (fd, lnum, 1);
                        seek_to_and_unpack_pixeldata (fd, lnum, 2);
                        merged_data = chans_to_RGB (layer->channel[0].data,
                                                    layer->channel[1].data,
                                                    layer->channel[2].data,
                                                    layer->width *
                                                    layer->height);

                        g_free (layer->channel[0].data);
                        g_free (layer->channel[1].data);
                        g_free (layer->channel[2].data);

                        IFDBG g_printerr ("YAH0a\n");
                      }
                    else
                      {
                        seek_to_and_unpack_pixeldata (fd, lnum, 0);
                        seek_to_and_unpack_pixeldata (fd, lnum, 1);
                        seek_to_and_unpack_pixeldata (fd, lnum, 2);
                        seek_to_and_unpack_pixeldata (fd, lnum, 3);

                        /* Fix for unexpected layer data order for files
                         * from PS files created by PanoTools. Rather
                         * than assuming an order, we find the actual order.
                         */

                        red_chan = grn_chan = blu_chan = alpha_chan = -1;

                        for (ichan = 0; ichan < numc; ichan++)
                          {
                            switch (psd_image.layer[lnum].channel[ichan].type)
                              {
                              case 0:    red_chan = ichan; break;
                              case 1:    grn_chan = ichan; break;
                              case 2:    blu_chan = ichan; break;
                              case -1: alpha_chan = ichan; break;
                              }
                          }

                        if ((red_chan < 0) ||
                            (grn_chan < 0) ||
                            (blu_chan < 0) ||
                            (alpha_chan < 0))
                          {
                            g_message ("Error: Cannot identify required RGBA channels");
                            gimp_quit ();
                          }

                        merged_data =
                          chans_to_RGBA (psd_image.layer[lnum].channel[red_chan].data,
                                         psd_image.layer[lnum].channel[grn_chan].data,
                                         psd_image.layer[lnum].channel[blu_chan].data,
                                         psd_image.layer[lnum].channel[alpha_chan].data,
                                         psd_image.layer[lnum].width *
                                         psd_image.layer[lnum].height);

                        g_free (layer->channel[0].data);
                        g_free (layer->channel[1].data);
                        g_free (layer->channel[2].data);
                        g_free (layer->channel[3].data);

                        IFDBG g_printerr ("YAH0b\n");
                      }
                  }

                IFDBG g_printerr ("YAH1\n");

                layer_ID = gimp_layer_new (image_ID,
                                           psd_image.layer[lnum].name,
                                           psd_image.layer[lnum].width,
                                           psd_image.layer[lnum].height,
                                           psd_layer_has_alpha (&psd_image.layer[lnum]) ?
                                           GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                                           (100.0 * psd_image.layer[lnum].opacity) / 255.0,
                                           psd_lmode_to_gimp_lmode(psd_image.layer[lnum].blendkey));

                IFDBG g_printerr ("YAH2\n");
              }
              break;

            default:
              g_message ("Error: Sorry, can't deal with a layered image of this type.\n");
              gimp_quit();
            }

          gimp_image_add_layer (image_ID, layer_ID, 0);

          IFDBG g_printerr ("YAH3\n");

          /* Do a layer mask if it exists */
          for (iter = 0; !empty && iter < layer->num_channels; iter++)
            {
              if (layer->channel[iter].type == -2) /* is mask */
                {
                  gint32  mask_id;
                  guchar *lm_data;

                  IFDBG g_printerr ("Unpacking a layer mask!\n");

                  lm_data = g_malloc (layer->width * layer->height);
                  {
#if PANOTOOLS_FIX
                    guchar *tmp;
#endif

                    seek_to_and_unpack_pixeldata (fd, lnum, iter);
                    /* PS layer masks can be a different size to
                       their owning layer, so we have to resize them. */
                    resize_mask (layer->channel[iter].data,
                                 lm_data,
                                 layer->lm_x - layer->x,
                                 layer->lm_y - layer->y,
                                 layer->lm_width, layer->lm_height,
                                 layer->width, layer->height);

                    /* won't be needing the original data any more */
                    g_free (layer->channel[iter].data);

                    /* give it to GIMP */
                    mask_id = gimp_layer_create_mask (layer_ID, 0);

#if PANOTOOLS_FIX
                     /* Convert the layer RGB data (not the mask) to RGBA */
                    tmp = merged_data;
                     merged_data = RGB_to_RGBA (tmp,
                                               psd_image.layer[lnum].width *
                                               psd_image.layer[lnum].height);
                    g_free (tmp);

                     /* Add alpha - otherwise cannot add layer mask */
                     gimp_layer_add_alpha (layer_ID);

                     /* Add layer mask */
#endif /* PANOTOOLS_FIX */
                    IFDBG printf ("Adding layer mask %d to layer %d\n",
                                  mask_id, layer_ID);

                    gimp_layer_add_mask (layer_ID, mask_id);

                    drawable = gimp_drawable_get (mask_id);

                    gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                                         layer->width, layer->height,
                                         TRUE, FALSE);
                    gimp_pixel_rgn_set_rect (&pixel_rgn, lm_data, 0, 0,
                                             layer->width, layer->height);
                  }
                  g_free (lm_data);

                  gimp_drawable_flush (drawable);
                  gimp_drawable_detach (drawable);
                }
            }

          IFDBG g_printerr ("YAH4\n");

          gimp_layer_translate (layer_ID, layer->x, layer->y);

          gimp_layer_set_lock_alpha (layer_ID, layer->protecttrans);
          gimp_drawable_set_visible (layer_ID, layer->visible);

          drawable = gimp_drawable_get (layer_ID);

          IFDBG g_printerr ("YAH5 - merged_data=%p, drawable=%p, drawdim=%dx%dx%d\n",
                            merged_data,
                            drawable,
                            drawable->width,
                            drawable->height,
                            drawable->bpp);

          if (empty)
            {
              gimp_drawable_fill (layer_ID, GIMP_TRANSPARENT_FILL);
            }
          else
            {
              gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                                   layer->width, layer->height,
                                   TRUE, FALSE);
              gimp_pixel_rgn_set_rect (&pixel_rgn, merged_data, 0, 0,
                                       layer->width, layer->height);

              IFDBG g_printerr ("YAH6\n");
            }

          gimp_drawable_flush (drawable);
          gimp_drawable_detach (drawable);
          drawable = NULL;

          g_free (merged_data);

          gimp_progress_update ((double) (lnum+1.0) /
                                (double) psd_image.num_layers);
        }

      fsetpos (fd, &tmpfpos);
    }

  if ((psd_image.num_aux_channels > 0) &&
      (psd_image.num_layers > 0))
    {
      want_aux = TRUE;
      IFDBG printf ("::::::::::: WANT AUX :::::::::::::::::::::::::::::::::::::::\n");
    }
  else
    {
      want_aux = FALSE;
    }


  if (want_aux || (psd_image.num_layers == 0)) /* Photoshop2-style: NO LAYERS. */
    {
      IFDBG printf ("Image data %ld chars\n", PSDheader.imgdatalen);

      step = PSDheader.channels;

      imagetype = PSD_UNKNOWN_IMAGE;

      switch (PSDheader.mode)
        {
        case 0:                /* Bitmap */
          imagetype = PSD_BITMAP_IMAGE;
          break;
        case 1:                /* Grayscale */
          imagetype = PSD_GRAY_IMAGE;
          break;
        case 2:                /* Indexed Colour */
          imagetype = PSD_INDEXED_IMAGE;
          break;
        case 3:                /* RGB Colour */
          imagetype = PSD_RGB_IMAGE;
          break;
        case 4:                /* CMYK Colour */
          cmyk = TRUE;
          switch (PSDheader.channels)
            {
            case 4:
              imagetype = PSD_RGB_IMAGE;
              break;
            case 5:
              imagetype = PSD_RGBA_IMAGE;
              break;
            default:
              g_message (_("Cannot handle PSD files in CMYK color with more than 5 channels"));
              return -1;
              break;
            }
          break;
        case 7:                /* Multichannel (?) */
        case 8:                /* Duotone */
        case 9:                /* Lab Colour */
        default:
          break;
        }


      if (imagetype == PSD_UNKNOWN_IMAGE)
        {
          g_message (_("Cannot handle image mode %d (%s)"),
                     PSDheader.mode,
                     (PSDheader.mode >= G_N_ELEMENTS (modename)) ?
                     "<out of range>" : modename[PSDheader.mode]);
          return -1;
        }

      if ((PSDheader.bpp != 8) && (PSDheader.bpp != 1))
        {
          g_message (_("Cannot handle %d bits per channel PSD files"), PSDheader.bpp);
          return -1;
        }

      IFDBG printf ("psd:%d gimp:%d gimpbase:%d\n",
                    imagetype,
                    psd_type_to_gimp_type(imagetype),
                    psd_type_to_gimp_base_type(imagetype));


      if (!want_aux)
        {
          /* gimp doesn't like 0 width/height drawables. */
          if ((PSDheader.columns == 0) || (PSDheader.rows == 0))
            {
              IFDBG printf ("(bad psd2-style image dimensions -- skipping)");
              image_ID = -1;
              goto finish_up;
            }

          image_ID = gimp_image_new (PSDheader.columns, PSDheader.rows,
                                     psd_type_to_gimp_base_type(imagetype));
          gimp_image_set_filename (image_ID, name);
          if (psd_type_to_gimp_base_type (imagetype) == GIMP_INDEXED)
            {
              if ((psd_image.colmaplen % 3) != 0)
                printf("PSD: Colourmap looks screwed! Aiee!\n");
              if (psd_image.colmaplen == 0)
                printf("PSD: Indexed image has no colourmap!\n");
              if (psd_image.colmaplen != 768)
                printf("PSD: Warning: Indexed image is %d!=256 colours.\n",
                       psd_image.colmaplen / 3);
              if (psd_image.colmaplen == 768)
                {
                  reshuffle_cmap (psd_image.colmapdata);
                  gimp_image_set_colormap (image_ID,
                                           psd_image.colmapdata,
                                           256);
                }
            }

          layer_ID = gimp_layer_new (image_ID, _("Background"),
                                     PSDheader.columns, PSDheader.rows,
                                     psd_type_to_gimp_type(imagetype),
                                     100, GIMP_NORMAL_MODE);

          gimp_image_add_layer (image_ID, layer_ID, 0);
          drawable = gimp_drawable_get (layer_ID);
        }


      if (want_aux)
        {
          switch (PSDheader.mode)
            {
            case 1:                /* Grayscale */
              channels = 1;
              break;
            case 2:                /* Indexed Colour */
              channels = 1;
              break;
            case 3:                /* RGB Colour */
              channels = 3;
              break;
            default:
              printf ("aux? Aieeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee!!!!!!!!!\n");
              channels = 1;
              break;
            }

          channels = PSDheader.channels - channels;

          if (psd_image.absolute_alpha)
            {
              channels--;
            }
        }
      else
        {
          channels = gimp_drawable_bpp (drawable->drawable_id);
        }


      dest = g_malloc (step * PSDheader.columns * PSDheader.rows);

      if (PSDheader.compression == 1)
        {
          nguchars = PSDheader.columns * PSDheader.rows;
          temp = g_malloc (PSDheader.imgdatalen);
          xfread (fd, temp, PSDheader.imgdatalen, "image data");
          if (!cmyk)
            {
              gimp_progress_update (1.0);

              if (imagetype == PSD_BITMAP_IMAGE) /* convert bitmap to grayscale */
                {
                  guchar *monobuf;

                  monobuf =
                    g_malloc (((PSDheader.columns + 7) >> 3) * PSDheader.rows);

                  decode (PSDheader.imgdatalen,
                          nguchars >> 3, temp,monobuf, step);
                  bitmap2gray (monobuf, dest,
                               PSDheader.columns, PSDheader.rows);

                  g_free (monobuf);
                }
              else
                {
                  decode (PSDheader.imgdatalen, nguchars, temp, dest, step);
                }
            }
          else
            {
              gimp_progress_update (1.0);

              cmykbuf = g_malloc (step * nguchars);
              decode (PSDheader.imgdatalen, nguchars, temp, cmykbuf, step);

              cmyk2rgb (cmykbuf, dest, PSDheader.columns, PSDheader.rows,
                        step > 4);
              g_free (cmykbuf);
            }

          g_free (temp);
        }
      else
        {
          if (!cmyk)
            {
              gimp_progress_update (1.0);

              xfread_interlaced (fd, dest, PSDheader.imgdatalen,
                                 "raw image data", step);
            }
          else
            {
              gimp_progress_update (1.0);

              cmykbuf = g_malloc (PSDheader.imgdatalen);
              xfread_interlaced (fd, cmykbuf, PSDheader.imgdatalen,
                                 "raw cmyk image data", step);

              cmykp2rgb (cmykbuf, dest,
                         PSDheader.columns, PSDheader.rows, step > 4);
              g_free (cmykbuf);
            }
        }


      if (want_aux) /* want_aux */
        {
          extract_channels (dest, channels, step,
                            image_ID,
                            PSDheader.columns, PSDheader.rows);

          goto finish_up; /* Haha!  Look!  A goto! */
        }
      else
        {
          gimp_progress_update (1.0);

          if (channels == step) /* gimp bpp == psd bpp */
            {

              if (psd_type_to_gimp_type (imagetype) == GIMP_INDEXEDA_IMAGE)
                {
                  printf ("@@@@ Didn't know that this could happen...\n");
                  for (iter = 0; iter < drawable->width * drawable->height; iter++)
                    {
                      dest[iter * 2 + 1] = 255;
                    }
                }

              gimp_pixel_rgn_init (&pixel_rgn, drawable,
                                   0, 0, drawable->width, drawable->height,
                                   TRUE, FALSE);
              gimp_pixel_rgn_set_rect (&pixel_rgn, dest,
                                       0, 0, drawable->width, drawable->height);

              gimp_drawable_flush (drawable);
              gimp_drawable_detach (drawable);
            }
          else
            {
              IFDBG printf ("Uhhh... uhm... extra channels... heavy...\n");

              extract_data_and_channels (dest, channels, step,
                                         image_ID, drawable,
                                         drawable->width, drawable->height);
            }
        }


    finish_up:

      g_free (dest);

      if (psd_image.colmaplen > 0)
        g_free(psd_image.colmapdata);
    }

  if (psd_image.num_guides > 0)
    {
      PSDguide *guide = psd_image.guides;
      int       i;

      IFDBG printf ("--- Adding %d Guides\n", psd_image.num_guides);

      for (i = 0; i < psd_image.num_guides; i++, guide++)
        {
          if (guide->horizontal)
            gimp_image_add_hguide (image_ID, guide->position);
          else
            gimp_image_add_vguide (image_ID, guide->position);
        }
    }

  gimp_displays_flush ();

  IFDBG printf ("--- %d layers : pos %ld : a-alph %d ---\n",
                psd_image.num_layers, ftell (fd),
                psd_image.absolute_alpha);

  return image_ID;
}

static void
decode (long    clen,
        long    uclen,
        guchar *src,
        guchar *dst,
        int     step)
{
  gint     i, j;
  gint32   l;
  guint16 *w;

  l = clen;
  for (i = 0; i < PSDheader.rows*PSDheader.channels; ++i)
    {
      l -= PSDheader.rowlength[i];
    }
  if (l)
    {
      g_warning("decode: %d should be zero\n", l);
    }

  w = PSDheader.rowlength;

  packbitsdecode (&clen, uclen, src, dst++, step);

  for (j = 0; j < step-1; ++j)
    {
      for (i = 0; i < PSDheader.rows; ++i)
        {
          src += *w++;
        }

      packbitsdecode (&clen, uclen, src, dst++, step);
    }

  IFDBG printf ("clen %ld\n", clen);
}

/*
 * Decode a PackBits data stream.
 */
static void
packbitsdecode (long   *clenp,
                long    uclen,
                guchar *src,
                guchar *dst,
                int     step)
{
  gint   n, b;
  gint32 clen = *clenp;

  while ((clen > 0) && (uclen > 0))
    {
      n = (int) *src++;

      if (n >= 128)
        n -= 256;
      if (n < 0)
        {
          /* replicate next guchar -n+1 times */

          clen -= 2;
          if (n == -128)        /* nop */
            continue;
          n = -n + 1;
          uclen -= n;
          for (b = *src++; n > 0; --n)
            {
              *dst = b;
              dst += step;
            }
        }
      else
        {
          /* copy next n+1 guchars literally */

          for (b = ++n; b > 0; --b)
            {
              *dst = *src++;
              dst += step;
            }

          uclen -= n;
          clen -= n+1;
        }
    }

  if (uclen > 0)
    {
      printf ("PSD: unexpected EOF while reading image data\n");
      gimp_quit ();
    }

  *clenp = clen;
}

/*
 * Decode a PackBits channel from file.
 */
static void
unpack_pb_channel (FILE    *fd,
                   guchar  *dst,
                   gint32   unpackedlen,
                   guint32 *offset)
{
  int    n, b;
  gint32 upremain = unpackedlen;

  while (upremain > 0)
    {
      n = (int) getguchar(fd, "packbits1");
      (*offset)++;

      if (n >= 128)
        n -= 256;
      if (n < 0)
        {
          /* replicate next guchar -n+1 times */

          if (n == -128)        /* nop */
            continue;
          n = -n + 1;
          /* upremain -= n;*/

          b = getguchar(fd, "packbits2");
          (*offset)++;
          for (; n > 0; --n)
            {
              if (upremain >= 0)
                {
                  *dst = b;
                  dst ++;
                }

              upremain--;
            }
        }
      else
        {
          /* copy next n+1 guchars literally */

          for (b = ++n; b > 0; --b)
            {
              const guchar c = getguchar (fd, "packbits3");

              if (upremain >= 0)
                {
                  *dst = c;
                  dst ++;
                }

              (*offset)++;
              upremain--;
            }
          /* upremain -= n;*/
        }
    }

  if (upremain < 0)
    {
      printf("*** Unpacking overshot destination (%d) buffer by %d bytes!\n",
             unpackedlen,
             -upremain);
    }
}

static void
cmyk2rgb (unsigned char *src,
          unsigned char *dst,
          long           width,
          long           height,
          int            alpha)
{
  int r, g, b, k;
  int i, j;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          r = *src++;
          g = *src++;
          b = *src++;
          k = *src++;

          gimp_cmyk_to_rgb_int (&r, &g, &b, &k);

          *dst++ = r;
          *dst++ = g;
          *dst++ = b;

          if (alpha)
            *dst++ = *src++;
        }

      if ((i % 5) == 0)
        gimp_progress_update ((2.0 * height) / ((double) height + (double) i));
    }
}

/*
 * Decode planar CMYK(A) to RGB(A).
 */
static void
cmykp2rgb (unsigned char *src,
           unsigned char *dst,
           long           width,
           long           height,
           int            alpha)
{
    int     r, g, b, k;
    int     i, j;
    long    n;
    guchar *rp, *gp, *bp, *kp, *ap;

    n = width * height;
    rp = src;
    gp = rp + n;
    bp = gp + n;
    kp = bp + n;
    ap = kp + n;

    for (i = 0; i < height; i++)
      {
        for (j = 0; j < width; j++)
          {
            r = *rp++;
            g = *gp++;
            b = *bp++;
            k = *kp++;

            gimp_cmyk_to_rgb_int (&r, &g, &b, &k);

            *dst++ = r;
            *dst++ = g;
            *dst++ = b;

            if (alpha)
              *dst++ = *ap++;
          }

        if ((i % 5) == 0)
          gimp_progress_update ((2.0 * height) / ((double) height + (double) i));
      }
}

static void
bitmap2gray (guchar *src,
             guchar *dest,
             long    w,
             long    h)
{
  int i,j;

  for(i = 0; i < h; i++)
    {
      int mask = 0x80;

      for(j = 0; j < w; j++)
        {
          *dest++ = (*src&mask) ? 0 : 255;
          mask >>= 1;

           if(!mask)
            {
              src++;
              mask = 0x80;
            }
        }

      if (mask != 0x80) src++;
    }
}

static void
dumpchunk (size_t  n,
           FILE   *fd,
           gchar  *why)
{
  guint32 i;

  printf("\n");

  for (i = 0; i < n; i++)
    {
      printf ("%02x ", (int) getguchar (fd, why));
    }

  printf ("\n");
}

static void
throwchunk (size_t  n,
            FILE   *fd,
            gchar  *why)
{
#if 0
  guchar *tmpchunk;

  if (n == 0)
    return;

  tmpchunk = g_malloc (n);
  xfread (fd, tmpchunk, n, why);
  g_free (tmpchunk);
#else
  if (n == 0)
    return;

  if (fseek (fd, (glong) n, SEEK_CUR) != 0)
    {
      printf ("PSD: unable to seek forward while reading '%s' chunk\n", why);
      gimp_quit();
    }
#endif
}

static gchar *
getstring (size_t  n,
           FILE   *fd,
           gchar  *why)
{
  gchar *tmpchunk;

  tmpchunk = g_malloc (n + 1);
  xfread (fd, tmpchunk, n, why);
  tmpchunk[n] = 0;

  return tmpchunk;  /* caller should free memory */
}

static gchar *
getpascalstring (FILE  *fd,
                 gchar *why)
{
  guchar *tmpchunk;
  guchar  len;

  xfread (fd, &len, 1, why);

  if (len == 0)
    return NULL;

  tmpchunk = g_malloc(len+1);

  xfread(fd, tmpchunk, len, why);
  tmpchunk[len]=0;

  return (gchar *) tmpchunk; /* caller should free memory */
}

static guchar
getguchar (FILE  *fd,
           gchar *why)
{
  gint tmp;

  tmp = fgetc (fd);

  if (tmp == EOF)
    {
      printf ("PSD: unexpected EOF while reading '%s' chunk\n", why);
      gimp_quit ();
    }

  return tmp;
}

static gint16
getgint16 (FILE  *fd,
           gchar *why)
{
  guchar b1, b2;

  b1 = getguchar (fd, why);
  b2 = getguchar (fd, why);

  return (gint16) ((b1 * 256) + b2);
}

static gint32
getgint32 (FILE  *fd,
          gchar *why)
{
  guchar s1, s2, s3, s4;

  s1 = getguchar (fd, why);
  s2 = getguchar (fd, why);
  s3 = getguchar (fd, why);
  s4 = getguchar (fd, why);

  return (gint32) ((s1 * 256 * 256 * 256) +
                  (s2 * 256 * 256) +
                  (s3 * 256) +
                   s4);
}

static void
xfread (FILE  *fd,
        void  *buf,
        long   len,
        gchar *why)
{
  if (fread (buf, len, 1, fd) == 0)
    {
      printf ("PSD: unexpected EOF while reading '%s' chunk\n", why);
      gimp_quit();
    }
}

static void
xfread_interlaced (FILE   *fd,
                   guchar *buf,
                   long    len,
                   gchar  *why,
                   gint    step)
{
  guchar *dest;
  gint    pix, pos, bpplane;

  bpplane = len / step;

  if (len % step != 0)
    {
      printf ("PSD: Stern warning: data size is not a factor of step size!\n");
    }

  for (pix = 0; pix < step; pix++)
    {
      dest = buf + pix;

      for (pos = 0; pos < bpplane; pos++)
        {
          *dest = getguchar (fd, why);
          dest += step;
        }
    }
}

static void
read_whole_file (FILE *fd)
{
    guint16 w;
    glong   pos;
    gchar   dummy[6];
    gint    i;

    xfread (fd, &PSDheader.signature, 4, "signature");
    PSDheader.version  = getgint16 (fd, "version");
    xfread (fd, &dummy, 6, "reserved");
    PSDheader.channels = getgint16 (fd, "channels");
    PSDheader.rows     = getgint32 (fd, "rows");
    PSDheader.columns  = getgint32 (fd, "columns");
    PSDheader.bpp      = getgint16 (fd, "depth");
    PSDheader.mode     = getgint16 (fd, "mode");


    psd_image.num_layers       = 0;
    psd_image.type             = PSDheader.mode;
    psd_image.colmaplen        = 0;
    psd_image.num_aux_channels = 0;
    psd_image.num_guides       = 0;


    psd_image.colmaplen = getgint32 (fd, "color data length");

    if (psd_image.colmaplen > 0)
      {
        psd_image.colmapdata = g_malloc (psd_image.colmaplen);
        xfread (fd, psd_image.colmapdata, psd_image.colmaplen, "colormap");
      }


    PSDheader.imgreslen = getgint32 (fd, "image resource length");
    if (PSDheader.imgreslen > 0)
      {
        do_image_resources (fd);
      }


    PSDheader.miscsizelen = getgint32 (fd, "misc size data length");
    if (PSDheader.miscsizelen > 0)
      {
        do_layer_and_mask (fd);
      }


    PSDheader.compression = getgint16 (fd, "compression");
    IFDBG printf("<<compr:%d>>", (int) PSDheader.compression);
    if (PSDheader.compression == 1) /* RLE */
      {
        PSDheader.rowlength = g_malloc (PSDheader.rows *
                                        PSDheader.channels * sizeof(guint16));

        for (i = 0; i < PSDheader.rows * PSDheader.channels; ++i)
          PSDheader.rowlength[i] = getgint16 (fd, "x");
      }
    pos = ftell (fd);
    fseek (fd, 0, SEEK_END);
    PSDheader.imgdatalen = ftell (fd) - pos;
    fseek (fd, pos, SEEK_SET);


    if (strncmp (PSDheader.signature, "8BPS", 4) != 0)
      {
        g_message (_("This is not an Adobe Photoshop PSD file"));
        gimp_quit ();
      }
    if (PSDheader.version != 1)
      {
        g_message (_("The PSD file has bad version number '%d', not 1"), PSDheader.version);
        gimp_quit ();
      }
    w = PSDheader.mode;
    IFDBG printf ("HEAD:\n"
                  "\tChannels %d\n\tRows %d\n\tColumns %d\n\tDepth %d\n\tMode %d (%s)\n"
                  "\tColour data %d guchars\n",
                  PSDheader.channels, PSDheader.rows,
                  PSDheader.columns, PSDheader.bpp,
                  w, modename[w < 10 ? w : 10],
                  psd_image.colmaplen);
    /*    printf("\tImage resource length: %lu\n", PSDheader.imgreslen);*/
    IFDBG printf ("\tLayer/Mask Data length: %u\n", PSDheader.miscsizelen);
    w = PSDheader.compression;
    IFDBG printf ("\tCompression %d (%s)\n", w, w ? "RLE" : "raw");
}
