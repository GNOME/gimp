/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string>

#include <lcms2.h>

/*  These libgimp includes are not needed here at all, but this is a
 *  convenient place to make sure the public libgimp headers are
 *  C++-clean. The C++ compiler will choke on stuff like naming
 *  a struct member or parameter "private".
 */
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#if defined(__MINGW32__)
#ifndef FLT_EPSILON
#define FLT_EPSILON  __FLT_EPSILON__
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON  __DBL_EPSILON__
#endif
#ifndef LDBL_EPSILON
#define LDBL_EPSILON __LDBL_EPSILON__
#endif
#endif

/* ignore deprecated warnings from OpenEXR headers */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfRgbaYca.h>
#include <ImfStandardAttributes.h>
#pragma GCC diagnostic pop

#include "exr-attribute-blob.h"
#include "openexr-wrapper.h"

using namespace Imf;
using namespace Imf::RgbaYca;
using namespace Imath;

static bool XYZ_equal(cmsCIEXYZ *a, cmsCIEXYZ *b)
{
  static const double epsilon = 0.0001;
  // Y is encoding the luminance, we normalize that for comparison
  return fabs ((a->X / a->Y * b->Y) - b->X) < epsilon &&
         fabs ((a->Y / a->Y * b->Y) - b->Y) < epsilon &&
         fabs ((a->Z / a->Y * b->Y) - b->Z) < epsilon;
}

struct _EXRLoader
{
  _EXRLoader(const char* filename) :
    refcount_(1),
    file_(filename),
    data_window_(file_.header().dataWindow()),
    channels_(file_.header().channels())
  {
    const Channel* chan;

    if (channels_.findChannel("R") ||
        channels_.findChannel("G") ||
        channels_.findChannel("B"))
      {
        format_string_ = "RGB";
        image_type_ = IMAGE_TYPE_RGB;

        if ((chan = channels_.findChannel("R")))
          pt_ = chan->type;
        else if ((chan = channels_.findChannel("G")))
          pt_ = chan->type;
        else
          pt_ = channels_.findChannel("B")->type;
      }
    else if (channels_.findChannel("Y") &&
             (channels_.findChannel("RY") ||
              channels_.findChannel("BY")))
      {
        format_string_ = "Y'CbCr";
        image_type_ = IMAGE_TYPE_YUV;

        /* TODO: Use RGBA interface to incorporate
         * RY/BY chroma channels */
        pt_ = channels_.findChannel("Y")->type;
      }
    else if (channels_.findChannel("Y"))
      {
        format_string_ = "Y";
        image_type_ = IMAGE_TYPE_GRAY;

        pt_ = channels_.findChannel("Y")->type;
      }
    else
      {
        int         channel_count = 0;
        const char *channel_name  = NULL;

        for (ChannelList::ConstIterator i = channels_.begin();
             i != channels_.end(); ++i)
          {
            channel_count++;

            pt_ = i.channel().type;
            channel_name = i.name();
          }

       /* Assume single channel images are grayscale,
        * no matter what the channel name is. */
        if (channel_count == 1)
          {
            format_string_ = channel_name;
            image_type_ = IMAGE_TYPE_UNKNOWN_1_CHANNEL;
            unknown_channel_name_ = channel_name;

            /* TODO: Pass this information back so it can be displayed
             * in the UI. */
            printf ("OpenEXR Warning: Single channel image with unknown "
                    "channel %s, loading as grayscale\n", channel_name);
          }
        else
          {
            throw;
          }
      }

    if (channels_.findChannel("A"))
      {
        format_string_.append("A");
        has_alpha_ = true;
      }
    else
      {
        has_alpha_ = false;
      }

    switch (pt_)
      {
      case UINT:
        format_string_.append(" u32");
        bpc_ = 4;
        break;
      case HALF:
        format_string_.append(" half");
        bpc_ = 2;
        break;
      case FLOAT:
      default:
        format_string_.append(" float");
        bpc_ = 4;
      }
  }

  int readPixelRow(char *pixels,
                   int   bpp,
                   int   row)
  {
    const int actual_row = data_window_.min.y + row;
    FrameBuffer fb;
    // This is necessary because OpenEXR expects the buffer to begin at
    // (0, 0). Though it probably results in some unmapped address,
    // hopefully OpenEXR will not make use of it. :/
    char* base = pixels - (data_window_.min.x * bpp);

    switch (image_type_)
      {
      case IMAGE_TYPE_UNKNOWN_1_CHANNEL:
        fb.insert(unknown_channel_name_, Slice(pt_, base, bpp, 0, 1, 1, 0.5));
        break;

      case IMAGE_TYPE_YUV:
      case IMAGE_TYPE_GRAY:
        fb.insert("Y", Slice(pt_, base, bpp, 0, 1, 1, 0.5));
        if (hasAlpha())
          {
            fb.insert("A", Slice(pt_, base + bpc_, bpp, 0, 1, 1, 1.0));
          }
        break;

      case IMAGE_TYPE_RGB:
      default:
        fb.insert("R", Slice(pt_, base + (bpc_ * 0), bpp, 0, 1, 1, 0.0));
        fb.insert("G", Slice(pt_, base + (bpc_ * 1), bpp, 0, 1, 1, 0.0));
        fb.insert("B", Slice(pt_, base + (bpc_ * 2), bpp, 0, 1, 1, 0.0));
        if (hasAlpha())
          {
            fb.insert("A", Slice(pt_, base + (bpc_ * 3), bpp, 0, 1, 1, 1.0));
          }
      }

    file_.setFrameBuffer(fb);
    file_.readPixels(actual_row);

    return 0;
  }

  int getWidth() const {
    return data_window_.max.x - data_window_.min.x + 1;
  }

  int getHeight() const {
    return data_window_.max.y - data_window_.min.y + 1;
  }

  EXRPrecision getPrecision() const {
    EXRPrecision prec;

    switch (pt_)
      {
      case UINT:
        prec = PREC_UINT;
        break;
      case HALF:
        prec = PREC_HALF;
        break;
      case FLOAT:
      default:
        prec = PREC_FLOAT;
      }

    return prec;
  }

  EXRImageType getImageType() const {
    return image_type_;
  }

  int hasAlpha() const {
    return has_alpha_ ? 1 : 0;
  }

  GimpColorProfile *getProfile() const {
    Chromaticities chromaticities;
    float whiteLuminance = 1.0;

    GimpColorProfile *linear_srgb_profile;
    cmsHPROFILE linear_srgb_lcms;

    GimpColorProfile *profile;
    cmsHPROFILE lcms_profile;

    cmsCIEXYZ *gimp_r_XYZ, *gimp_g_XYZ, *gimp_b_XYZ, *gimp_w_XYZ;
    cmsCIEXYZ exr_r_XYZ, exr_g_XYZ, exr_b_XYZ, exr_w_XYZ;

    // get the color information from the EXR
    if (hasChromaticities (file_.header ()))
      chromaticities = Imf::chromaticities (file_.header ());
    else
      return NULL;

    if (Imf::hasWhiteLuminance (file_.header ()))
      whiteLuminance = Imf::whiteLuminance (file_.header ());
    else
      return NULL;

#if 0
    std::cout << "hasChromaticities: "
              << hasChromaticities (file_.header ())
              << std::endl;
    std::cout << "hasWhiteLuminance: "
              << hasWhiteLuminance (file_.header ())
              << std::endl;
    std::cout << whiteLuminance << std::endl;
    std::cout << chromaticities.red << std::endl;
    std::cout << chromaticities.green << std::endl;
    std::cout << chromaticities.blue << std::endl;
    std::cout << chromaticities.white << std::endl;
    std::cout << std::endl;
#endif

    cmsCIExyY whitePoint = { chromaticities.white.x,
                             chromaticities.white.y,
                             whiteLuminance };
    cmsCIExyYTRIPLE CameraPrimaries = { { chromaticities.red.x,
                                          chromaticities.red.y,
                                          whiteLuminance },
                                        { chromaticities.green.x,
                                          chromaticities.green.y,
                                          whiteLuminance },
                                        { chromaticities.blue.x,
                                          chromaticities.blue.y,
                                          whiteLuminance } };

    // get the primaries + wp from GIMP's internal linear sRGB profile
    linear_srgb_profile = gimp_color_profile_new_rgb_srgb_linear ();
    linear_srgb_lcms = gimp_color_profile_get_lcms_profile (linear_srgb_profile);

    gimp_r_XYZ = (cmsCIEXYZ *) cmsReadTag (linear_srgb_lcms, cmsSigRedColorantTag);
    gimp_g_XYZ = (cmsCIEXYZ *) cmsReadTag (linear_srgb_lcms, cmsSigGreenColorantTag);
    gimp_b_XYZ = (cmsCIEXYZ *) cmsReadTag (linear_srgb_lcms, cmsSigBlueColorantTag);
    gimp_w_XYZ = (cmsCIEXYZ *) cmsReadTag (linear_srgb_lcms, cmsSigMediaWhitePointTag);

    cmsxyY2XYZ(&exr_r_XYZ, &CameraPrimaries.Red);
    cmsxyY2XYZ(&exr_g_XYZ, &CameraPrimaries.Green);
    cmsxyY2XYZ(&exr_b_XYZ, &CameraPrimaries.Blue);
    cmsxyY2XYZ(&exr_w_XYZ, &whitePoint);

    // ... and check if the data stored in the EXR matches GIMP's internal profile
    bool exr_is_linear_srgb = XYZ_equal (&exr_r_XYZ, gimp_r_XYZ) &&
                              XYZ_equal (&exr_g_XYZ, gimp_g_XYZ) &&
                              XYZ_equal (&exr_b_XYZ, gimp_b_XYZ) &&
                              XYZ_equal (&exr_w_XYZ, gimp_w_XYZ);

    // using GIMP's linear sRGB profile allows to skip the conversion popup
    if (exr_is_linear_srgb)
      return linear_srgb_profile;

    // nope, it's something else. Clean up and build a new profile
    g_object_unref (linear_srgb_profile);

    // TODO: maybe factor this out into libgimpcolor/gimpcolorprofile.h ?
    double Parameters[2] = { 1.0, 0.0 };
    cmsToneCurve *Gamma[3];
    Gamma[0] = Gamma[1] = Gamma[2] = cmsBuildParametricToneCurve(0,
                                                                 1,
                                                                 Parameters);
    lcms_profile = cmsCreateRGBProfile (&whitePoint, &CameraPrimaries, Gamma);
    cmsFreeToneCurve (Gamma[0]);
    if (lcms_profile == NULL) return NULL;

//     cmsSetProfileVersion (lcms_profile, 2.1);
    cmsMLU *mlu0 = cmsMLUalloc (NULL, 1);
    cmsMLUsetASCII (mlu0, "en", "US", "(GIMP internal)");
    cmsMLU *mlu1 = cmsMLUalloc(NULL, 1);
    cmsMLUsetASCII (mlu1, "en", "US", "color profile from EXR chromaticities");
    cmsMLU *mlu2 = cmsMLUalloc(NULL, 1);
    cmsMLUsetASCII (mlu2, "en", "US", "color profile from EXR chromaticities");
    cmsWriteTag (lcms_profile, cmsSigDeviceMfgDescTag, mlu0);
    cmsWriteTag (lcms_profile, cmsSigDeviceModelDescTag, mlu1);
    cmsWriteTag (lcms_profile, cmsSigProfileDescriptionTag, mlu2);
    cmsMLUfree (mlu0);
    cmsMLUfree (mlu1);
    cmsMLUfree (mlu2);

    profile = gimp_color_profile_new_from_lcms_profile (lcms_profile,
                                                        NULL);
    cmsCloseProfile (lcms_profile);

    return profile;
  }

  gchar *getComment() const {
    char *result = NULL;
    const Imf::StringAttribute *comment = file_.header().findTypedAttribute<Imf::StringAttribute>("comment");
    if (comment)
      result = g_strdup (comment->value().c_str());
    return result;
  }

  guchar *getExif(guint *size) const {
    guchar jpeg_exif[] = "Exif\0\0";
    guchar *exif_data = NULL;
    *size = 0;

    const Imf::BlobAttribute *exif = file_.header().findTypedAttribute<Imf::BlobAttribute>("exif");

    if (exif)
      {
        exif_data = (guchar *)(exif->value().data.get());
        *size = exif->value().size;
        /* darktable 4.0.0 and earlier appended a jpg-compatible exif00 string,
         * so get rid of that again. We explicitly reduce the size by 1 since
         * the compiler adds an extra \0 to the end of jpeg_exif. */
        if ( ! memcmp (jpeg_exif, exif_data, sizeof(jpeg_exif)-1))
          {
            *size -= 6;
            exif_data += 6;
          }
      }

    return (guchar *)g_memdup2 (exif_data, *size);
  }

  guchar *getXmp(guint *size) const {
    guchar *result = NULL;
    *size = 0;
    const Imf::StringAttribute *xmp = file_.header().findTypedAttribute<Imf::StringAttribute>("xmp");
    if (xmp)
      {
        *size = xmp->value().size();
        result = (guchar *) g_memdup2 (xmp->value().data(), *size);
      }
    return result;
  }

  size_t refcount_;
  InputFile file_;
  const Box2i data_window_;
  const ChannelList& channels_;
  PixelType pt_;
  int bpc_;
  EXRImageType image_type_;
  bool has_alpha_;
  std::string format_string_;
  std::string unknown_channel_name_;
};

EXRLoader*
exr_loader_new (const char *filename)
{
  EXRLoader* file;

  // Don't let any exceptions propagate to the C layer.
  try
    {
      Imf::BlobAttribute::registerAttributeType();
      file = new EXRLoader(filename);
    }
  catch (...)
    {
      file = NULL;
    }

  return file;
}

EXRLoader*
exr_loader_ref (EXRLoader *loader)
{
  ++loader->refcount_;
  return loader;
}

void
exr_loader_unref (EXRLoader *loader)
{
  if (--loader->refcount_ == 0)
    {
      delete loader;
    }
}

int
exr_loader_get_width (EXRLoader *loader)
{
  int width;
  // Don't let any exceptions propagate to the C layer.
  try
    {
      width = loader->getWidth();
    }
  catch (...)
    {
      width = -1;
    }

  return width;
}

int
exr_loader_get_height (EXRLoader *loader)
{
  int height;
  // Don't let any exceptions propagate to the C layer.
  try
    {
      height = loader->getHeight();
    }
  catch (...)
    {
      height = -1;
    }

  return height;
}

EXRImageType
exr_loader_get_image_type (EXRLoader *loader)
{
  // This does not throw.
  return loader->getImageType();
}

EXRPrecision
exr_loader_get_precision (EXRLoader *loader)
{
  // This does not throw.
  return loader->getPrecision();
}

int
exr_loader_has_alpha (EXRLoader *loader)
{
  // This does not throw.
  return loader->hasAlpha();
}

GimpColorProfile *
exr_loader_get_profile (EXRLoader *loader)
{
  return loader->getProfile ();
}

gchar *
exr_loader_get_comment (EXRLoader *loader)
{
  return loader->getComment ();
}

guchar *
exr_loader_get_exif (EXRLoader *loader,
                     guint *size)
{
  return loader->getExif (size);
}

guchar *
exr_loader_get_xmp (EXRLoader *loader,
                    guint *size)
{
  return loader->getXmp (size);
}

int
exr_loader_read_pixel_row (EXRLoader *loader,
                           char *pixels,
                           int bpp,
                           int row)
{
  int retval = -1;
  // Don't let any exceptions propagate to the C layer.
  try
    {
      retval = loader->readPixelRow(pixels, bpp, row);
    }
  catch (...)
    {
      retval = -1;
    }

  return retval;
}
