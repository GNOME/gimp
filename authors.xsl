<?xml version="1.0" encoding="UTF-8"?>

<!--  simple XSL transformation to create a text version from authors.xml  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dc="http://purl.org/dc/elements/1.1/">

  <xsl:output method="text" />

  <xsl:template match="/dc:gimp-authors">
    <xsl:text>This file is generated from authors.xml, do not edit it directly.
    </xsl:text>
    <xsl:apply-templates />
  </xsl:template>

</xsl:stylesheet>
