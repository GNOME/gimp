<?xml version="1.0" encoding="UTF-8"?>

<!--  simple XSL transformation to create a text version from authors.xml  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="text" />

  <xsl:template match="/gimp-authors">
    <xsl:text>This file is generated from authors.xml, do not edit it directly.
    </xsl:text>
    <xsl:apply-templates />
  </xsl:template>

</xsl:stylesheet>
