#include <EXTERN.h>
#include <perl.h> 

#include <stdio.h>

#include <libgimp/gimpmodule.h>

#include "../gppport.h"

static GimpModuleInfo info = {
    NULL,
    "Generic Embedded Perl Interpreter",
    "Marc Lehmann <pcg@goof.com>",
    "v0.00",
    "(c) 1999, released under the GPL",
    "1999-04-14"
};

void ERR(char *msg)
{
  STRLEN dc;
  dTHR;

  fprintf (stderr, "(Perl module error, please report!) %s: %s\n", msg, SvPV(ERRSV,dc));
}

static PerlInterpreter *interp;

static int perl_init(void)
{
  if (!interp)
    {
      extern void xs_init();
      char *embedding[] = { "", "-e", "0" };
      SV *res;

      interp = perl_alloc();
      if (interp)
        {
	  perl_construct(interp);
          {
            dTHR; /* NOT earlier! */

            perl_parse(interp, xs_init, 3, embedding, NULL);
            perl_eval_pv ("require Gimp::Module", FALSE);

            if (SvTRUE (ERRSV))
              {
                ERR ("error during require Gimp::Module, perl NOT initialized!");
                return GIMP_MODULE_UNLOAD;
              }

            res = perl_eval_pv ("Gimp::Module::_init()", FALSE);
            if (SvTRUE (ERRSV))
              {
                ERR ("error during require Gimp::Module::_init(), perl NOT initialized!");
                return GIMP_MODULE_UNLOAD;
              }

            if (res && SvIOK (res))
              return SvIV (res);
          }
        }

      return GIMP_MODULE_UNLOAD;
    }

  return GIMP_MODULE_OK;
}

static void perl_deinit(void)
{
  dTHR;

  if (interp)
    {
      perl_run(interp);
      perl_eval_pv ("Gimp::Module::_deinit()", FALSE);

      if (SvTRUE (ERRSV))
        ERR ("error during require Gimp::Module::_init()");

      PL_perl_destruct_level = 0;
      perl_destruct(interp);
      perl_free(interp);

      interp = 0;
    }
}

G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
  GimpModuleStatus s;

  *inforet = &info;
  
  s = perl_init ();

  if (s != GIMP_MODULE_OK)
    perl_deinit ();

  return s;
}

G_MODULE_EXPORT void
module_unload (void *shutdown_data,
               void (*completed_cb)(void *),
               void *completed_data)
{
  perl_deinit ();
  /* perl is unloadable (atexit & friends), *sigh* */
  /* completed_cb (completed_data); */
}
