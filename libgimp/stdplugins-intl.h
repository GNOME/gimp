#ifndef __STDPLUGINS_INTL_H__
#define __STDPLUGINS_INTL_H__

#include <locale.h>

#include "libgimp/gimpintl.h"

#ifdef HAVE_LC_MESSAGES
#define INIT_I18N()	G_STMT_START{			\
  setlocale(LC_MESSAGES, ""); 				\
  bindtextdomain("gimp-std-plugins", LOCALEDIR);	\
  textdomain("gimp-std-plugins");			\
  			}G_STMT_END
#else
#define INIT_I18N()	G_STMT_START{			\
  bindtextdomain("gimp-std-plugins", LOCALEDIR);	\
  textdomain("gimp-std-plugins");			\
  			}G_STMT_END
#endif

#define INIT_I18N_UI()	G_STMT_START{	\
  gtk_set_locale();			\
  setlocale (LC_NUMERIC, "C");		\
  INIT_I18N();				\
			}G_STMT_END

#endif /* __STDPLUGINS_INTL_H__ */
