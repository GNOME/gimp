#include "config.h"

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
#include "ppport.h"

#include <libgimp/gimpmodule.h>

MODULE = Gimp::Module	PACKAGE = Gimp::Module

VERSIONCHECK: DISABLE
PROTOTYPES: ENABLE

BOOT:
{
   HV *stash = gv_stashpvn("Gimp::Module", 12, TRUE);
   
   newCONSTSUB(stash,"GIMP_MODULE_OK",newSViv(GIMP_MODULE_OK));
   newCONSTSUB(stash,"GIMP_MODULE_UNLOAD",newSViv(GIMP_MODULE_UNLOAD));
}

