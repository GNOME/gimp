#include "config.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "libgimp/gserialize.h"

#ifndef  WAIT_ANY
#define  WAIT_ANY -1
#endif   /*  WAIT_ANY  */

#include "libgimp/gimpfeatures.h"

#include "appenv.h"
#include "app_procs.h"
#include "errors.h"
#include "install.h"

#include "libgimp/gimpintl.h"
