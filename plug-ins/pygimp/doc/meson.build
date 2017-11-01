
pygimp_ps = custom_target('pygimp.ps',
  input : [ 'pygimp.sgml' ],
  output: [ 'pygimp.ps' ],
  command: [
    'db2ps',
    '@INPUT@'
  ],
)
pygimp_pdf = custom_target('pygimp.pdf',
  input : [ 'pygimp.sgml' ],
  output: [ 'pygimp.pdf' ],
  command: [
    'db2pdf',
    '@INPUT@'
  ],
)
pygimp_html = custom_target('pygimp.html',
  input : [ 'pygimp.sgml' ],
  output: [ 'pygimp.html' ],
  command: [
    'db2html', '--nochunks',
    '@INPUT@'
  ],
)
