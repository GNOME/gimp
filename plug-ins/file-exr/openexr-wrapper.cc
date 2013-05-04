#include "config.h"

#include "openexr-wrapper.h"

#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfRgbaYca.h>
#include <ImfStandardAttributes.h>

#include <string>

using namespace Imf;
using namespace Imf::RgbaYca;
using namespace Imath;

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
        format_string_ = "RGB";
        image_type_ = IMAGE_TYPE_RGB;

        pt_ = channels_.findChannel("Y")->type;

        // FIXME: no chroma handling for now.
        throw;
      }
    else if (channels_.findChannel("Y"))
      {
        format_string_ = "Y";
        image_type_ = IMAGE_TYPE_GRAY;

        pt_ = channels_.findChannel("Y")->type;
      }
    else
      {
        throw;
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

  int readPixelRow(char* pixels,
                   int bpp,
                   int row)
  {
    const int actual_row = data_window_.min.y + row;
    FrameBuffer fb;
    // This is necessary because OpenEXR expects the buffer to begin at
    // (0, 0). Though it probably results in some unmapped address,
    // hopefully OpenEXR will not make use of it. :/
    char* base = pixels - (data_window_.min.x * bpp);

    switch (image_type_)
      {
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

  size_t refcount_;
  InputFile file_;
  const Box2i data_window_;
  const ChannelList& channels_;
  PixelType pt_;
  int bpc_;
  EXRImageType image_type_;
  bool has_alpha_;
  std::string format_string_;
};

EXRLoader*
exr_loader_new (const char *filename)
{
  EXRLoader* file;

  // Don't let any exceptions propagate to the C layer.
  try
    {
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
