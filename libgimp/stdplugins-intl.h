#include "libgimp/gimpintl.h"

#ifdef HAVE_LC_MESSAGES
#define INIT_I18N() \
  setlocale(LC_MESSAGES, ""); \
  bindtextdomain("gimp-std-plugins", LOCALEDIR); \
  textdomain("gimp-std-plugins")

#else
#define INIT_I18N() \
  bindtextdomain("gimp-std-plugins", LOCALEDIR); \
  textdomain("gimp-std-plugins")
#endif
