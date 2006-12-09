/* re.c */
/* Henry Spencer's implementation of Regular Expressions,
   used for TinyScheme */
/* Refurbished by Stephen Gildea */
#include "regex.h"
#include "tinyscheme/scheme-private.h"

#if defined(_WIN32)
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif

/* Since not exported */
#define T_STRING 1

pointer     foreign_re_match(scheme *sc, pointer args);
EXPORT void init_re(scheme *sc);


static void set_vector_elem(pointer vec, int ielem, pointer newel) {
 int n=ielem/2;
 if(ielem%2==0) {
     vec[1+n]._object._cons._car=newel;
 } else {
     vec[1+n]._object._cons._cdr=newel;
 }
}

pointer foreign_re_match(scheme *sc, pointer args) {
  pointer retval=sc->F;
  int retcode;
  regex_t rt;
  pointer first_arg, second_arg;
  pointer third_arg=sc->NIL;
  char *string;
  char *pattern;
  int num=0;

  if(!((args != sc->NIL) && sc->vptr->is_string((first_arg = sc->vptr->pair_car(args)))
       && (args=sc->vptr->pair_cdr(args))
       && sc->vptr->is_pair(args) && sc->vptr->is_string((second_arg = sc->vptr->pair_car(args))))) {
    return sc->F;
  }
  pattern = sc->vptr->string_value(first_arg);
  string = sc->vptr->string_value(second_arg);

  args=sc->vptr->pair_cdr(args);
  if(args!=sc->NIL) {
    if(!(sc->vptr->is_pair(args) && sc->vptr->is_vector((third_arg = sc->vptr->pair_car(args))))) {
      return sc->F;
    } else {
      num=third_arg->_object._number.value.ivalue;
    }
  }


  if(regcomp(&rt,pattern,REG_EXTENDED)!=0) {
    return sc->F;
  }

  if(num==0) {
    retcode=regexec(&rt,string,0,0,0);
  } else {
    regmatch_t *pmatch=malloc((num+1)*sizeof(regmatch_t));
    if(pmatch!=0) {
      retcode=regexec(&rt,string,num+1,pmatch,0);
      if(retcode==0) {
       int i;
       for(i=0; i<num; i++) {
#undef cons
         set_vector_elem(third_arg, i,
                         sc->vptr->cons(sc, sc->vptr->mk_integer(sc, pmatch[i].rm_so),
                                             sc->vptr->mk_integer(sc, pmatch[i].rm_eo)));
        }
      }
      free(pmatch);
    } else {
      sc->no_memory=1;
      retcode=-1;
    }
  }

  if(retcode==0) {
    retval=sc->T;
  }

  regfree(&rt);

  return(retval);
}

#if 0
static char* utilities=";; return the substring of STRING matched in MATCH-VECTOR, \n"
";; the Nth subexpression match (default 0).\n"
"(define (re-match-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string (car (vector-ref match-vector n))\n"
"                    (cdr (vector-ref match-vector n)))))\n"
"(define (re-before-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string 0 (car (vector-ref match-vector n)))))\n"
"(define (re-after-nth string match-vector . n)\n"
"  (let ((n (if (pair? n) (car n) 0)))\n"
"    (substring string (cdr (vector-ref match-vector n))\n"
"             (string-length string))))\n";
#endif

void init_re(scheme *sc) {
  sc->vptr->scheme_define(sc,sc->global_env,sc->vptr->mk_symbol(sc,"re-match"),sc->vptr->mk_foreign_func(sc, foreign_re_match));
  /*    sc->vptr->load_string(sc,utilities);*/
}
