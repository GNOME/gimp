#include "config.h"

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
#define NEED_newCONSTSUB
#include "gppport.h"

/* FIXME */
/* dirty is used in gimp.h.  */
#ifdef dirty
# undef dirty
#endif
#include "extradefs.h"

#if GIMP_MAJOR_VERSION>1 || (GIMP_MAJOR_VERSION==1 && GIMP_MINOR_VERSION>=1)
# define GIMP11 1
# define GIMP_PARASITE 1
#endif

#ifndef HAVE_EXIT
/* expect iso-c here.  */
# include <signal.h>
#endif

MODULE = Gimp	PACKAGE = Gimp

PROTOTYPES: ENABLE

void
_exit()
	CODE:
#ifdef HAVE__EXIT
	_exit(0);
#elif defined(SIGKILL)
	raise(SIGKILL);
#else
	raise(9);
#endif
	abort();

void
xs_exit(status)
	int	status
	CODE:
	exit (status);

BOOT:
{
   HV *stash = gv_stashpvn("Gimp", 4, TRUE);
   
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
#if GIMP_PARASITE
   newCONSTSUB(stash,"PARAM_PARASITE",newSViv(PARAM_PARASITE));

   newCONSTSUB(stash,"PARASITE_PERSISTENT",newSViv(PARASITE_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_UNDOABLE",newSViv(PARASITE_UNDOABLE));

   newCONSTSUB(stash,"PARASITE_ATTACH_PARENT",newSViv(PARASITE_ATTACH_PARENT));
   newCONSTSUB(stash,"PARASITE_PARENT_PERSISTENT",newSViv(PARASITE_PARENT_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_PARENT_UNDOABLE",newSViv(PARASITE_PARENT_UNDOABLE));
   
   newCONSTSUB(stash,"PARASITE_ATTACH_GRANDPARENT",newSViv(PARASITE_ATTACH_GRANDPARENT));
   newCONSTSUB(stash,"PARASITE_GRANDPARENT_PERSISTENT",newSViv(PARASITE_GRANDPARENT_PERSISTENT));
   newCONSTSUB(stash,"PARASITE_GRANDPARENT_UNDOABLE",newSViv(PARASITE_GRANDPARENT_UNDOABLE));
#endif
   
   newCONSTSUB(stash,"PROC_EXTENSION",newSViv(PROC_EXTENSION));
   newCONSTSUB(stash,"PROC_PLUG_IN",newSViv(PROC_PLUG_IN));
   newCONSTSUB(stash,"PROC_TEMPORARY",newSViv(PROC_TEMPORARY));
   
   newCONSTSUB(stash,"STATUS_CALLING_ERROR",newSViv(STATUS_CALLING_ERROR));
   newCONSTSUB(stash,"STATUS_EXECUTION_ERROR",newSViv(STATUS_EXECUTION_ERROR));
   newCONSTSUB(stash,"STATUS_PASS_THROUGH",newSViv(STATUS_PASS_THROUGH));
   newCONSTSUB(stash,"STATUS_SUCCESS",newSViv(STATUS_SUCCESS));
   
   newCONSTSUB(stash,"TRACE_NONE",newSViv(TRACE_NONE));
   newCONSTSUB(stash,"TRACE_CALL",newSViv(TRACE_CALL));
   newCONSTSUB(stash,"TRACE_TYPE",newSViv(TRACE_TYPE));
   newCONSTSUB(stash,"TRACE_NAME",newSViv(TRACE_NAME));
   newCONSTSUB(stash,"TRACE_DESC",newSViv(TRACE_DESC));
   newCONSTSUB(stash,"TRACE_ALL" ,newSViv(TRACE_ALL ));
   /**/
#if HAVE_DIVIDE_MODE || IN_GIMP
   /*newCONSTSUB(stash,"DIVIDE_MODE",newSViv(DIVIDE_MODE));*/
#endif
}

