#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libgimp/gimp.h>

/* FIXME */
/* sys/param.h is redefining these! */
#undef MIN
#undef MAX

/* dunno where this comes from */
#undef VOIDUSED

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
}
#endif

/* FIXME */
/* dirty is used in gimp.h.  */
#undef dirty
#include "extradefs.h"

#if GIMP_MAJOR_VERSION>1 || (GIMP_MAJOR_VERSION==1 && GIMP_MINOR_VERSION>=1)
#define GIMP11 1
#define GIMP_PARASITE 1
#endif

static double
constant(name)
char *name;
{
    errno = 0;
    switch (*name) {
    case 'A':
	if (strEQ(name, "ADDITION_MODE"))
	    return ADDITION_MODE;
	if (strEQ(name, "ALPHA_MASK"))
	    return ALPHA_MASK;
	if (strEQ(name, "APPLY"))
	    return APPLY;
	break;
    case 'B':
	if (strEQ(name, "BEHIND_MODE"))
	    return BEHIND_MODE;
	if (strEQ(name, "BG_BUCKET_FILL"))
	    return BG_BUCKET_FILL;
	if (strEQ(name, "BG_IMAGE_FILL"))
	    return BG_IMAGE_FILL;
	if (strEQ(name, "BILINEAR"))
	    return BILINEAR;
	if (strEQ(name, "BLACK_MASK"))
	    return BLACK_MASK;
	if (strEQ(name, "BLUE_CHANNEL"))
	    return BLUE_CHANNEL;
	if (strEQ(name, "BLUR"))
	    return BLUR;
	break;
    case 'C':
	if (strEQ(name, "CLIP_TO_BOTTOM_LAYER"))
	    return CLIP_TO_BOTTOM_LAYER;
	if (strEQ(name, "CLIP_TO_IMAGE"))
	    return CLIP_TO_IMAGE;
	if (strEQ(name, "COLOR_MODE"))
	    return COLOR_MODE;
	if (strEQ(name, "CONICAL_ASYMMETRIC"))
	    return CONICAL_ASYMMETRIC;
	if (strEQ(name, "CONICAL_SYMMETRIC"))
	    return CONICAL_SYMMETRIC;
	if (strEQ(name, "CUSTOM"))
	    return CUSTOM;
	break;
    case 'D':
	if (strEQ(name, "DARKEN_ONLY_MODE"))
	    return DARKEN_ONLY_MODE;
	if (strEQ(name, "DIFFERENCE_MODE"))
	    return DIFFERENCE_MODE;
	if (strEQ(name, "DISCARD"))
	    return DISCARD;
	if (strEQ(name, "DISSOLVE_MODE"))
	    return DISSOLVE_MODE;
#if HAVE_DIVIDE_MODE || IN_GIMP
	if (strEQ(name, "DIVIDE_MODE"))
	    return DIVIDE_MODE;
#endif
	break;
    case 'E':
	if (strEQ(name, "EXPAND_AS_NECESSARY"))
	    return EXPAND_AS_NECESSARY;
	break;
    case 'F':
#if GIMP11
	if (strEQ(name, "FG_IMAGE_FILL"))
	    return FG_IMAGE_FILL;
#endif
	if (strEQ(name, "FG_BG_HSV"))
	    return FG_BG_HSV;
	if (strEQ(name, "FG_BG_RGB"))
	    return FG_BG_RGB;
	if (strEQ(name, "FG_BUCKET_FILL"))
	    return FG_BUCKET_FILL;
	if (strEQ(name, "FG_TRANS"))
	    return FG_TRANS;
	break;
    case 'G':
	if (strEQ(name, "GRAY"))
	    return GRAY;
	if (strEQ(name, "GRAYA_IMAGE"))
	    return GRAYA_IMAGE;
	if (strEQ(name, "GRAY_CHANNEL"))
	    return GRAY_CHANNEL;
	if (strEQ(name, "GRAY_IMAGE"))
	    return GRAY_IMAGE;
	if (strEQ(name, "GREEN_CHANNEL"))
	    return GREEN_CHANNEL;
	break;
    case 'H':
	if (strEQ(name, "HUE_MODE"))
	    return HUE_MODE;
	break;
    case 'I':
	if (strEQ(name, "IMAGE_CLONE"))
	    return IMAGE_CLONE;
	if (strEQ(name, "INDEXED"))
	    return INDEXED;
	if (strEQ(name, "INDEXEDA_IMAGE"))
	    return INDEXEDA_IMAGE;
	if (strEQ(name, "INDEXED_CHANNEL"))
	    return INDEXED_CHANNEL;
	if (strEQ(name, "INDEXED_IMAGE"))
	    return INDEXED_IMAGE;
	break;
    case 'L':
	if (strEQ(name, "LIGHTEN_ONLY_MODE"))
	    return LIGHTEN_ONLY_MODE;
	if (strEQ(name, "LINEAR"))
	    return LINEAR;
	break;
    case 'M':
	if (strEQ(name, "MULTIPLY_MODE"))
	    return MULTIPLY_MODE;
	break;
    case 'N':
	if (strEQ(name, "NORMAL_MODE"))
	    return NORMAL_MODE;
#if GIMP11
	if (strEQ(name, "NO_IMAGE_FILL"))
	    return NO_IMAGE_FILL;
#endif
	break;
    case 'O':
	if (strEQ(name, "OVERLAY_MODE"))
	    return OVERLAY_MODE;
	break;
    case 'P':
#if GIMP_PARASITE
	if (strEQ(name, "PARAM_PARASITE"))
	    return PARAM_PARASITE;
	if (strEQ(name, "PARASITE_PERSISTANT"))
	    return PARASITE_PERSISTANT;
#endif
	if (strEQ(name, "PARAM_BOUNDARY"))
	    return PARAM_BOUNDARY;
	if (strEQ(name, "PARAM_CHANNEL"))
	    return PARAM_CHANNEL;
	if (strEQ(name, "PARAM_COLOR"))
	    return PARAM_COLOR;
	if (strEQ(name, "PARAM_DISPLAY"))
	    return PARAM_DISPLAY;
	if (strEQ(name, "PARAM_DRAWABLE"))
	    return PARAM_DRAWABLE;
	if (strEQ(name, "PARAM_END"))
	    return PARAM_END;
	if (strEQ(name, "PARAM_FLOAT"))
	    return PARAM_FLOAT;
	if (strEQ(name, "PARAM_FLOATARRAY"))
	    return PARAM_FLOATARRAY;
	if (strEQ(name, "PARAM_IMAGE"))
	    return PARAM_IMAGE;
	if (strEQ(name, "PARAM_INT16"))
	    return PARAM_INT16;
	if (strEQ(name, "PARAM_INT16ARRAY"))
	    return PARAM_INT16ARRAY;
	if (strEQ(name, "PARAM_INT32"))
	    return PARAM_INT32;
	if (strEQ(name, "PARAM_INT32ARRAY"))
	    return PARAM_INT32ARRAY;
	if (strEQ(name, "PARAM_INT8"))
	    return PARAM_INT8;
	if (strEQ(name, "PARAM_INT8ARRAY"))
	    return PARAM_INT8ARRAY;
	if (strEQ(name, "PARAM_LAYER"))
	    return PARAM_LAYER;
	if (strEQ(name, "PARAM_PATH"))
	    return PARAM_PATH;
	if (strEQ(name, "PARAM_REGION"))
	    return PARAM_REGION;
	if (strEQ(name, "PARAM_SELECTION"))
	    return PARAM_SELECTION;
	if (strEQ(name, "PARAM_STATUS"))
	    return PARAM_STATUS;
	if (strEQ(name, "PARAM_STRING"))
	    return PARAM_STRING;
	if (strEQ(name, "PARAM_STRINGARRAY"))
	    return PARAM_STRINGARRAY;
	if (strEQ(name, "PATTERN_BUCKET_FILL"))
	    return PATTERN_BUCKET_FILL;
	if (strEQ(name, "PATTERN_CLONE"))
	    return PATTERN_CLONE;
	if (strEQ(name, "PIXELS"))
	    return PIXELS;
	if (strEQ(name, "POINTS"))
	    return POINTS;
	if (strEQ(name, "PROC_EXTENSION"))
	    return PROC_EXTENSION;
	if (strEQ(name, "PROC_PLUG_IN"))
	    return PROC_PLUG_IN;
	if (strEQ(name, "PROC_TEMPORARY"))
	    return PROC_TEMPORARY;
	break;
    case 'R':
	if (strEQ(name, "RADIAL"))
	    return RADIAL;
	if (strEQ(name, "RED_CHANNEL"))
	    return RED_CHANNEL;
	if (strEQ(name, "REPEAT_NONE"))
	    return REPEAT_NONE;
	if (strEQ(name, "REPEAT_SAWTOOTH"))
	    return REPEAT_SAWTOOTH;
	if (strEQ(name, "REPEAT_TRIANGULAR"))
	    return REPEAT_TRIANGULAR;
	if (strEQ(name, "RGB"))
	    return RGB;
	if (strEQ(name, "RGBA_IMAGE"))
	    return RGBA_IMAGE;
	if (strEQ(name, "RGB_IMAGE"))
	    return RGB_IMAGE;
	if (strEQ(name, "RUN_INTERACTIVE"))
	    return RUN_INTERACTIVE;
	if (strEQ(name, "RUN_NONINTERACTIVE"))
	    return RUN_NONINTERACTIVE;
	if (strEQ(name, "RUN_WITH_LAST_VALS"))
	    return RUN_WITH_LAST_VALS;
	break;
    case 'S':
	if (strEQ(name, "SATURATION_MODE"))
	    return SATURATION_MODE;
	if (strEQ(name, "SCREEN_MODE"))
	    return SCREEN_MODE;
	if (strEQ(name, "SELECTION_ADD"))
	    return SELECTION_ADD;
	if (strEQ(name, "SELECTION_INTERSECT"))
	    return SELECTION_INTERSECT;
	if (strEQ(name, "SELECTION_REPLACE"))
	    return SELECTION_REPLACE;
	if (strEQ(name, "SELECTION_SUB"))
	    return SELECTION_SUB;
	if (strEQ(name, "SHAPEBURST_ANGULAR"))
	    return SHAPEBURST_ANGULAR;
	if (strEQ(name, "SHAPEBURST_DIMPLED"))
	    return SHAPEBURST_DIMPLED;
	if (strEQ(name, "SHAPEBURST_SPHERICAL"))
	    return SHAPEBURST_SPHERICAL;
	if (strEQ(name, "SHARPEN"))
	    return SHARPEN;
	if (strEQ(name, "SQUARE"))
	    return SQUARE;
	if (strEQ(name, "STATUS_CALLING_ERROR"))
	    return STATUS_CALLING_ERROR;
	if (strEQ(name, "STATUS_EXECUTION_ERROR"))
	    return STATUS_EXECUTION_ERROR;
	if (strEQ(name, "STATUS_PASS_THROUGH"))
	    return STATUS_PASS_THROUGH;
	if (strEQ(name, "STATUS_SUCCESS"))
	    return STATUS_SUCCESS;
	if (strEQ(name, "SUBTRACT_MODE"))
	    return SUBTRACT_MODE;
	break;
    case 'T':
	if (strEQ(name, "TRANS_IMAGE_FILL"))
	    return TRANS_IMAGE_FILL;
	if (strEQ(name, "TRACE_NONE")) return TRACE_NONE;
	if (strEQ(name, "TRACE_CALL")) return TRACE_CALL;
	if (strEQ(name, "TRACE_TYPE")) return TRACE_TYPE;
	if (strEQ(name, "TRACE_NAME")) return TRACE_NAME;
	if (strEQ(name, "TRACE_DESC")) return TRACE_DESC;
	if (strEQ(name, "TRACE_ALL"))  return TRACE_ALL;
	break;
    case 'V':
	if (strEQ(name, "VALUE_MODE"))
	    return VALUE_MODE;
	break;
    case 'W':
	if (strEQ(name, "WHITE_IMAGE_FILL"))
	    return WHITE_IMAGE_FILL;
	if (strEQ(name, "WHITE_MASK"))
	    return WHITE_MASK;
	break;
    }
    errno = EINVAL;
    return 0;
}

MODULE = Gimp	PACKAGE = Gimp

PROTOTYPES: ENABLE

double
constant(name)
	char *		name

