use ExtUtils::MakeMaker;

$topdir="..";
do '../config.pl';

WriteMakefile(
    'NAME'	=> 'Gimp::Lib',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$CPPFLAGS -I.. $CFLAGS $GIMP_INC_NOUI $DEFS".($IN_GIMP ? " -DIN_GIMP" : ""),
    'LDLOADLIBS'=> "$LDFLAGS $LIBS $GIMP_LIBS_NOUI",
    'DEFINE'	=> '',
);
