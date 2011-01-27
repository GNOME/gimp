<?xml version="1.0" encoding="UTF-8"?>

<!--  simple XSL transformation to create a text version from authors.xml  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dc="http://purl.org/dc/elements/1.1/">

  <xsl:output method="text" />

  <xsl:template name="contributor">
    <xsl:param name="role" />
    <xsl:apply-templates select="dc:contributor[contains(@role, $role)]" />
  </xsl:template>

  <xsl:template match="/dc:gimp-authors">
    <xsl:text> -- This file is generated from authors.xml, do not edit it directly. --

GIMP was originally written by:

</xsl:text>
    <xsl:apply-templates select="dc:creator" />
    <xsl:text>

The current maintainers are:

</xsl:text>
    <xsl:apply-templates select="dc:maintainer" />
    <xsl:text>

The following people have contributed code to GIMP:

</xsl:text>
    <xsl:call-template name="contributor">
      <xsl:with-param name="role" select="'author'"/>
    </xsl:call-template>
    <xsl:text>

The following people have contributed art to GIMP:

</xsl:text>
    <xsl:call-template name="contributor">
      <xsl:with-param name="role" select="'artist'"/>
    </xsl:call-template>
    <xsl:text>

The following people have helped to document GIMP:

</xsl:text>
    <xsl:call-template name="contributor">
      <xsl:with-param name="role" select="'documenter'"/>
    </xsl:call-template>

  </xsl:template>

  <xsl:template match="dc:creator">
    <xsl:text> </xsl:text><xsl:apply-templates /><xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="dc:maintainer">
    <xsl:text> </xsl:text><xsl:apply-templates /><xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="dc:contributor">
    <xsl:text> </xsl:text><xsl:apply-templates /><xsl:text>
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
