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

/* Shamelesssly stolen from IO.xs.  See perlguts, this is only for
 * 5.004 compatibility.
 */
#ifndef newCONSTSUB
static void
newCONSTSUB(stash,name,sv)
    HV *stash;
    char *name;
    SV *sv;
{
#ifdef dTHR
    dTHR;
#endif
    U32 oldhints = hints;
    HV *old_cop_stash = curcop->cop_stash;
    HV *old_curstash = curstash;
    line_t oldline = curcop->cop_line;
    curcop->cop_line = copline;

    hints &= ~HINT_BLOCK_SCOPE;
    if(stash)
	curstash = curcop->cop_stash = stash;

    newSUB(
	MY_start_subparse(FALSE, 0),
	newSVOP(OP_CONST, 0, newSVpv(name,0)),
	newSVOP(OP_CONST, 0, &sv_no),	/* SvPV(&sv_no) == "" -- GMB */
	newSTATEOP(0, Nullch, newSVOP(OP_CONST, 0, sv))
    );

    hints = oldhints;
    curcop->cop_stash = old_cop_stash;
    curstash = old_curstash;
    curcop->cop_line = oldline;
}
#endif

MODULE = Gimp	PACKAGE = Gimp

PROTOTYPES: ENABLE

BOOT:
{
   HV *stash = gv_stashpvn("Gimp", 4, TRUE);
   
   newCONSTSUB(stash,"ADDITION_MODE",newSViv(ADDITION_MODE));
   newCONSTSUB(stash,"ALPHA_MASK",newSViv(ALPHA_MASK));
   newCONSTSUB(stash,"APPLY",newSViv(APPLY));
   newCONSTSUB(stash,"BEHIND_MODE",newSViv(BEHIND_MODE));
   newCONSTSUB(stash,"BG_BUCKET_FILL",newSViv(BG_BUCKET_FILL));
   newCONSTSUB(stash,"BG_IMAGE_FILL",newSViv(BG_IMAGE_FILL));
   newCONSTSUB(stash,"BILINEAR",newSViv(BILINEAR));
   newCONSTSUB(stash,"BLACK_MASK",newSViv(BLACK_MASK));
   newCONSTSUB(stash,"BLUE_CHANNEL",newSViv(BLUE_CHANNEL));
   newCONSTSUB(stash,"BLUR",newSViv(BLUR));
   newCONSTSUB(stash,"CLIP_TO_BOTTOM_LAYER",newSViv(CLIP_TO_BOTTOM_LAYER));
   newCONSTSUB(stash,"CLIP_TO_IMAGE",newSViv(CLIP_TO_IMAGE));
   newCONSTSUB(stash,"COLOR_MODE",newSViv(COLOR_MODE));
   newCONSTSUB(stash,"CONICAL_ASYMMETRIC",newSViv(CONICAL_ASYMMETRIC));
   newCONSTSUB(stash,"CONICAL_SYMMETRIC",newSViv(CONICAL_SYMMETRIC));
   newCONSTSUB(stash,"CUSTOM",newSViv(CUSTOM));
   newCONSTSUB(stash,"DARKEN_ONLY_MODE",newSViv(DARKEN_ONLY_MODE));
   newCONSTSUB(stash,"DIFFERENCE_MODE",newSViv(DIFFERENCE_MODE));
   newCONSTSUB(stash,"DISCARD",newSViv(DISCARD));
   newCONSTSUB(stash,"DISSOLVE_MODE",newSViv(DISSOLVE_MODE));
   newCONSTSUB(stash,"EXPAND_AS_NECESSARY",newSViv(EXPAND_AS_NECESSARY));
   newCONSTSUB(stash,"FG_BG_HSV",newSViv(FG_BG_HSV));
   newCONSTSUB(stash,"FG_BG_RGB",newSViv(FG_BG_RGB));
   newCONSTSUB(stash,"FG_BUCKET_FILL",newSViv(FG_BUCKET_FILL));
   newCONSTSUB(stash,"FG_TRANS",newSViv(FG_TRANS));
   newCONSTSUB(stash,"GRAY",newSViv(GRAY));
   newCONSTSUB(stash,"GRAYA_IMAGE",newSViv(GRAYA_IMAGE));
   newCONSTSUB(stash,"GRAY_CHANNEL",newSViv(GRAY_CHANNEL));
   newCONSTSUB(stash,"GRAY_IMAGE",newSViv(GRAY_IMAGE));
   newCONSTSUB(stash,"GREEN_CHANNEL",newSViv(GREEN_CHANNEL));
   newCONSTSUB(stash,"HUE_MODE",newSViv(HUE_MODE));
   newCONSTSUB(stash,"IMAGE_CLONE",newSViv(IMAGE_CLONE));
   newCONSTSUB(stash,"INDEXED",newSViv(INDEXED));
   newCONSTSUB(stash,"INDEXEDA_IMAGE",newSViv(INDEXEDA_IMAGE));
   newCONSTSUB(stash,"INDEXED_CHANNEL",newSViv(INDEXED_CHANNEL));
   newCONSTSUB(stash,"INDEXED_IMAGE",newSViv(INDEXED_IMAGE));
   newCONSTSUB(stash,"LIGHTEN_ONLY_MODE",newSViv(LIGHTEN_ONLY_MODE));
   newCONSTSUB(stash,"LINEAR",newSViv(LINEAR));
   newCONSTSUB(stash,"MULTIPLY_MODE",newSViv(MULTIPLY_MODE));
   newCONSTSUB(stash,"NORMAL_MODE",newSViv(NORMAL_MODE));
   newCONSTSUB(stash,"OVERLAY_MODE",newSViv(OVERLAY_MODE));
   newCONSTSUB(stash,"PARAM_BOUNDARY",newSViv(PARAM_BOUNDARY));
   newCONSTSUB(stash,"PARAM_CHANNEL",newSViv(PARAM_CHANNEL));
   newCONSTSUB(stash,"PARAM_COLOR",newSViv(PARAM_COLOR));
   newCONSTSUB(stash,"PARAM_DISPLAY",newSViv(PARAM_DISPLAY));
   newCONSTSUB(stash,"PARAM_DRAWABLE",newSViv(PARAM_DRAWABLE));
   newCONSTSUB(stash,"PARAM_END",newSViv(PARAM_END));
   newCONSTSUB(stash,"PARAM_FLOAT",newSViv(PARAM_FLOAT));
   newCONSTSUB(stash,"PARAM_FLOATARRAY",newSViv(PARAM_FLOATARRAY));
   newCONSTSUB(stash,"PARAM_IMAGE",newSViv(PARAM_IMAGE));
   newCONSTSUB(stash,"PARAM_INT16",newSViv(PARAM_INT16));
   newCONSTSUB(stash,"PARAM_INT16ARRAY",newSViv(PARAM_INT16ARRAY));
   newCONSTSUB(stash,"PARAM_INT32",newSViv(PARAM_INT32));
   newCONSTSUB(stash,"PARAM_INT32ARRAY",newSViv(PARAM_INT32ARRAY));
   newCONSTSUB(stash,"PARAM_INT8",newSViv(PARAM_INT8));
   newCONSTSUB(stash,"PARAM_INT8ARRAY",newSViv(PARAM_INT8ARRAY));
   newCONSTSUB(stash,"PARAM_LAYER",newSViv(PARAM_LAYER));
   newCONSTSUB(stash,"PARAM_PATH",newSViv(PARAM_PATH));
   newCONSTSUB(stash,"PARAM_REGION",newSViv(PARAM_REGION));
   newCONSTSUB(stash,"PARAM_SELECTION",newSViv(PARAM_SELECTION));
   newCONSTSUB(stash,"PARAM_STATUS",newSViv(PARAM_STATUS));
   newCONSTSUB(stash,"PARAM_STRING",newSViv(PARAM_STRING));
   newCONSTSUB(stash,"PARAM_STRINGARRAY",newSViv(PARAM_STRINGARRAY));
   newCONSTSUB(stash,"PATTERN_BUCKET_FILL",newSViv(PATTERN_BUCKET_FILL));
   newCONSTSUB(stash,"PATTERN_CLONE",newSViv(PATTERN_CLONE));
   newCONSTSUB(stash,"PIXELS",newSViv(PIXELS));
   newCONSTSUB(stash,"POINTS",newSViv(POINTS));
   newCONSTSUB(stash,"PROC_EXTENSION",newSViv(PROC_EXTENSION));
   newCONSTSUB(stash,"PROC_PLUG_IN",newSViv(PROC_PLUG_IN));
   newCONSTSUB(stash,"PROC_TEMPORARY",newSViv(PROC_TEMPORARY));
   newCONSTSUB(stash,"RADIAL",newSViv(RADIAL));
   newCONSTSUB(stash,"RED_CHANNEL",newSViv(RED_CHANNEL));
   newCONSTSUB(stash,"REPEAT_NONE",newSViv(REPEAT_NONE));
   newCONSTSUB(stash,"REPEAT_SAWTOOTH",newSViv(REPEAT_SAWTOOTH));
   newCONSTSUB(stash,"REPEAT_TRIANGULAR",newSViv(REPEAT_TRIANGULAR));
   newCONSTSUB(stash,"RGB",newSViv(RGB));
   newCONSTSUB(stash,"RGBA_IMAGE",newSViv(RGBA_IMAGE));
   newCONSTSUB(stash,"RGB_IMAGE",newSViv(RGB_IMAGE));
   newCONSTSUB(stash,"RUN_INTERACTIVE",newSViv(RUN_INTERACTIVE));
   newCONSTSUB(stash,"RUN_NONINTERACTIVE",newSViv(RUN_NONINTERACTIVE));
   newCONSTSUB(stash,"RUN_WITH_LAST_VALS",newSViv(RUN_WITH_LAST_VALS));
   newCONSTSUB(stash,"SATURATION_MODE",newSViv(SATURATION_MODE));
   newCONSTSUB(stash,"SCREEN_MODE",newSViv(SCREEN_MODE));
   newCONSTSUB(stash,"SELECTION_ADD",newSViv(SELECTION_ADD));
   newCONSTSUB(stash,"SELECTION_INTERSECT",newSViv(SELECTION_INTERSECT));
   newCONSTSUB(stash,"SELECTION_REPLACE",newSViv(SELECTION_REPLACE));
   newCONSTSUB(stash,"SELECTION_SUB",newSViv(SELECTION_SUB));
   newCONSTSUB(stash,"SHAPEBURST_ANGULAR",newSViv(SHAPEBURST_ANGULAR));
   newCONSTSUB(stash,"SHAPEBURST_DIMPLED",newSViv(SHAPEBURST_DIMPLED));
   newCONSTSUB(stash,"SHAPEBURST_SPHERICAL",newSViv(SHAPEBURST_SPHERICAL));
   newCONSTSUB(stash,"SHARPEN",newSViv(SHARPEN));
   newCONSTSUB(stash,"SQUARE",newSViv(SQUARE));
   newCONSTSUB(stash,"STATUS_CALLING_ERROR",newSViv(STATUS_CALLING_ERROR));
   newCONSTSUB(stash,"STATUS_EXECUTION_ERROR",newSViv(STATUS_EXECUTION_ERROR));
   newCONSTSUB(stash,"STATUS_PASS_THROUGH",newSViv(STATUS_PASS_THROUGH));
   newCONSTSUB(stash,"STATUS_SUCCESS",newSViv(STATUS_SUCCESS));
   newCONSTSUB(stash,"SUBTRACT_MODE",newSViv(SUBTRACT_MODE));
   newCONSTSUB(stash,"TRANS_IMAGE_FILL",newSViv(TRANS_IMAGE_FILL));
   newCONSTSUB(stash,"TRACE_NONE",newSViv(TRACE_NONE));
   newCONSTSUB(stash,"TRACE_CALL",newSViv(TRACE_CALL));
   newCONSTSUB(stash,"TRACE_TYPE",newSViv(TRACE_TYPE));
   newCONSTSUB(stash,"TRACE_NAME",newSViv(TRACE_NAME));
   newCONSTSUB(stash,"TRACE_DESC",newSViv(TRACE_DESC));
   newCONSTSUB(stash,"TRACE_ALL",newSViv(TRACE_ALL));
   newCONSTSUB(stash,"VALUE_MODE",newSViv(VALUE_MODE));
   newCONSTSUB(stash,"WHITE_IMAGE_FILL",newSViv(WHITE_IMAGE_FILL));
   newCONSTSUB(stash,"WHITE_MASK",newSViv(WHITE_MASK));
#if HAVE_DIVIDE_MODE || IN_GIMP
   newCONSTSUB(stash,"DIVIDE_MODE",newSViv(DIVIDE_MODE));
#endif
#if GIMP11
   newCONSTSUB(stash,"FG_IMAGE_FILL",newSViv(FG_IMAGE_FILL));
   newCONSTSUB(stash,"NO_IMAGE_FILL",newSViv(NO_IMAGE_FILL));
#endif
#if GIMP_PARASITE
   newCONSTSUB(stash,"PARAM_PARASITE",newSViv(PARAM_PARASITE));
   newCONSTSUB(stash,"PARASITE_PERSISTENT",newSViv(PARASITE_PERSISTENT));
#endif
   newCONSTSUB(stash,"ALL_HUES",	newSViv(0));
   newCONSTSUB(stash,"RED_HUES",	newSViv(1));
   newCONSTSUB(stash,"YELLOW_HUES",	newSViv(2));
   newCONSTSUB(stash,"GREEN_HUES",	newSViv(3));
   newCONSTSUB(stash,"CYAN_HUES",	newSViv(4));
   newCONSTSUB(stash,"BLUE_HUES",	newSViv(5));
   newCONSTSUB(stash,"MAGENTA_HUES",	newSViv(6));

   newCONSTSUB(stash,"MESSAGE_BOX",	newSViv(0));
   newCONSTSUB(stash,"CONSOLE",		newSViv(1));

   newCONSTSUB(stash,"SHADOWS",		newSViv(0));
   newCONSTSUB(stash,"MIDTONES",	newSViv(1));
   newCONSTSUB(stash,"HIGHLIGHTS",	newSViv(2));

   newCONSTSUB(stash,"HORIZONTAL",	newSViv(0));
   newCONSTSUB(stash,"VERTICAL",	newSViv(1));
}

