PDB-compatability

# Programmers Reference:  PDB procedures for compatabililty

## About

The audience is GIMP maintainers.

This discusses procedures for compatability in the PDB and ScriptFu.
A procedure for compatability is:

  * deprecated
  * OR a wrapper for a plugin now in GEGL

## What

### Deprecated

A deprecated procedure is one provided for backward compatibility.
Older plugins calling a deprecated procedure still work.
Plugin authors should not use deprecated procedures in new plugin code.
In the future, gimp.org may obsolete i.e. delete the procedures.
Thus breaking any existing plugins still using the procedures.

A deprecated procedure can for:

  * a renaming, to correct a naming mistake
  and thereby conform to naming conventions

  * an adaptor, having a signature different from a new signature.
  For example, to accomodate the "multi-layer select" feature,
  a new signature takes a container of layers,
  and a deprecated signature takes a single layer and adapts.

### Wrappers for GEGL

These are wrappers/adaptors.
In early GIMP 2, the procedures were C plugins in GIMP.
Now (since about GIMP 2.8) they are C plugins in GEGL
and a wrapper/adaptor delegates to GEGL.

These are NOT planned for obsolescence.

Most GEGL plugins are wrapped.
A few are not.
Occasionally, new GEGL plugins are mistakenly not wrapped in GIMP.

A plugin in an introspected language can avoid a wrapper
by creating a GEGL node and calling GEGL directly.
(That is the recourse when there is no wrapper for a GEGL plugin.)

## Categories of deprecation and wrapping

1.  Aliases for renamed procedures:
these are not separate procedures,
just alternate names pointing to another procedure

2.  True deprecated procedures:
these are separately defined procedures

3.  ScriptFu deprecated procedures:
separately defined procedures or aliases, in the Scheme language

4.  GEGL wrappers:
separately defined procedures that wrap GEGL plugin filters

## GUI: how the PDB Browser shows deprecated and wrapped PDB procedures

This describes how a plugin author may know what procedures are deprecated or wrappers.

The PDB Browser inconsistently shows this information.

1.  Aliases for renamed procedures:
the PDB browser does not reveal this at all.

2.  True deprecated procedures:
the PDB browser indicates they are deprecated in the "blurb" field.

3.  ScriptFu deprecated procedures:
the PDB browser doesn't show these,
since they are Scheme procedures not PDB procedures.

4.  GEGL wrappers:
the PDB browser indicates they are GEGL wrappers in the "author" field.

## Implementation

This describes how deprecation and wrappers are implemented.

Read this if you are a maintainer and you want to deprecate a procedure
or obsolete a deprecated procedure.

### Aliased deprecated procedures

Declared in app/pdb/gimp-pdb-compat.c.
The code enters an alias name in the hash table of PDB procedure names, pointing to the same procedure as another name.
The name usually starts with "gimp-".

### True deprecated procedures

Defined in in pdb/groups/foo.pdb using the "std_pdb_deprecated" tag.
The name usually starts with "gimp-".

These also generate deprecated functions in libgimp.

### ScriptFu deprecated

ScriptFu provides some compatibility on top of "true deprecated procedures"

1.  Defined in the latter half of plug-ins/script-fu/scripts/script-fu-compat.init.
Adaptors for Scheme functions present in the SIOD dialect of Scheme.
That dialect was used in early GIMP ScriptFu.
This provides backward compatibility for some older ScriptFu scripts.

2.  Defined in plug-ins/script-fu/scripts/plug-in-compat.init.
Adaptors for PDB procedures present in earlier GIMP 2.
For example, at one time "plug-in-color-map"
and "gimp-image-get-active-layer" were defined in ScriptFu
(but not elsewhere) for compatibility.

The names can start with "gimp-" or "plug-in" or just be from some Scheme language dialect.

### GEGL wrappers

Defined in pdb/groups/plug_in_compat.pdb.

!!! These do not generate functions in libgimp.
A C plugin or plugin in an introspected language
must call GEGL directly,
or call the wrapping PDB procedure.

The names start with "plug-in-".

## Process for deprecation, and status

At a major release,
gimp.org might obsolete formerly deprecated procedures.
This breaks API (for the PDB and libgimp.)

API is not usually broken for a minor release.
Deprecated procedures are sometimes added, but not removed,
for a minor release.

As of the time of this writing (GIMP 3.0) there are:

  * no aliased procedures
  * no true deprecated procedures
  * no ScriptFu deprecated procedures for SIOD dialect or older PDB

The mechanisms for deprecation (the implementation as described above)
is still in place.


