## Find the install dirs for the python installation.
##  By James Henstridge

# serial 1

AC_DEFUN(AM_PATH_PYTHON,
  [AC_CHECK_PROGS(PYTHON, python python1.5 python1.4 python1.3,no)
  if test "$PYTHON" != no; then
    AC_MSG_CHECKING([where .py files should go])
changequote(, )dnl
    pythondir=`$PYTHON -c '
import sys
if sys.version[0] > "1" or sys.version[2] > "4":
  print "%s/lib/python%s/site-packages" % (sys.prefix, sys.version[:3])
else:
  print "%s/lib/python%s" % (sys.prefix, sys.version[:3])'`
changequote([, ])dnl
    AC_MSG_RESULT($pythondir)
    AC_MSG_CHECKING([where python extensions should go])
changequote(, )dnl
    pyexecdir=`$PYTHON -c '
import sys
if sys.version[0] > "1" or sys.version[2] > "4":
  print "%s/lib/python%s/site-packages" % (sys.exec_prefix, sys.version[:3])
else:
  print "%s/lib/python%s/sharedmodules" % (sys.exec_prefix, sys.version[:3])'`
changequote([, ])dnl
    AC_MSG_RESULT($pyexecdir)
  else
    # these defaults are version independent ...
    AC_MSG_CHECKING([where .py files should go])
    pythondir='$(prefix)/lib/site-python'
    AC_MSG_RESULT($pythondir)
    AC_MSG_CHECKING([where python extensions should go])
    pyexecdir='$(exec_prefix)/lib/site-python'
    AC_MSG_RESULT($pyexecdir)
  fi
  AC_SUBST(pythondir)
  AC_SUBST(pyexecdir)])


## this one is commonly used with AM_PATH_PYTHONDIR ...
dnl AM_CHECK_PYMOD(MODNAME [,SYMBOL [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Check if a module containing a given symbol is visible to python.
AC_DEFUN(AM_CHECK_PYMOD,
[AC_REQUIRE([AM_PATH_PYTHON])
py_mod_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING(for ifelse([$2],[],,[$2 in ])python module $1)
AC_CACHE_VAL(py_cv_mod_$py_mod_var, [
ifelse([$2],[], [prog="
import sys
try:
	import $1
except ImportError:
	sys.exit(1)
except:
	sys.exit(0)
sys.exit(0)"], [prog="
import $1
$1.$2"])
if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
  then
    eval "py_cv_mod_$py_mod_var=yes"
  else
    eval "py_cv_mod_$py_mod_var=no"
  fi
])
py_val=`eval "echo \`echo '$py_cv_mod_'$py_mod_var\`"`
if test "x$py_val" != xno; then
  AC_MSG_RESULT(yes)
  ifelse([$3], [],, [$3
])dnl
else
  AC_MSG_RESULT(no)
  ifelse([$4], [],, [$4
])dnl
fi
])



# serial 1

dnl finds information needed for compilation of shared library style python
dnl extensions.  AM_PATH_PYTHON should be called before hand.
AC_DEFUN(AM_INIT_PYEXEC_MOD,
  [AC_REQUIRE([AM_PATH_PYTHON])
  AC_MSG_CHECKING([for python headers])
  AC_CACHE_VAL(am_cv_python_includes,
    [changequote(,)dnl
    am_cv_python_includes="`$PYTHON -c '
import sys
includepy = \"%s/include/python%s\" % (sys.prefix, sys.version[:3])
if sys.version[0] > \"1\" or sys.version[2] > \"4\":
  libpl = \"%s/include/python%s\" % (sys.exec_prefix, sys.version[:3])
else:
  libpl = \"%s/lib/python%s/config\" % (sys.exec_prefix, sys.version[:3])
print \"-I%s -I%s\" % (includepy, libpl)'`"
    changequote([, ])])
  PYTHON_INCLUDES="$am_cv_python_includes"
  AC_MSG_RESULT(found)
  AC_SUBST(PYTHON_INCLUDES)

  AC_MSG_CHECKING([definitions from Python makefile])
  AC_CACHE_VAL(am_cv_python_makefile,
    [changequote(,)dnl
    py_makefile="`$PYTHON -c '
import sys
print \"%s/lib/python%s/config/Makefile\"%(sys.exec_prefix, sys.version[:3])'`"
    if test ! -f "$py_makefile"; then
      AC_MSG_ERROR([*** Couldn't find the python config makefile.  Maybe you are
*** missing the development portion of the python installation])
    fi
    eval `sed -n \
-e "s/^CC=[ 	]*\(.*\)/am_cv_python_CC='\1'/p" \
-e "s/^OPT=[ 	]*\(.*\)/am_cv_python_OPT='\1'/p" \
-e "s/^CCSHARED=[ 	]*\(.*\)/am_cv_python_CCSHARED='\1'/p" \
-e "s/^LDSHARED=[ 	]*\(.*\)/am_cv_python_LDSHARED='\1'/p" \
-e "s/^SO=[ 	]*\(.*\)/am_cv_python_SO='\1'/p" \
    $py_makefile`
    am_cv_python_makefile=found
    changequote([, ])])
  AC_MSG_RESULT(done)
  CC="$am_cv_python_CC"
  OPT="$am_cv_python_OPT"
  SO="$am_cv_python_SO"
  PYTHON_CFLAGS="$am_cv_python_CCSHARED \$(OPT)"
  PYTHON_LINK="$am_cv_python_LDSHARED -o \[$]@"

  AC_SUBST(CC)dnl
  AC_SUBST(OPT)dnl
  AC_SUBST(SO)dnl
  AC_SUBST(PYTHON_CFLAGS)dnl
  AC_SUBST(PYTHON_LINK)])

