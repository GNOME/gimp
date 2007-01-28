<?xml version="1.0" encoding="UTF-8"?>

<!--  simple XSL transformation to create a header file from authors.xml  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dc="http://purl.org/dc/elements/1.1/">

  <xsl:output method="text" />
  <xsl:template match="/dc:gimp-authors">
<xsl:text>
/* NOTE: This file is auto-generated from authors.xml, do not edit it. */

static const gchar * const authors[] =
{
</xsl:text>
  <xsl:apply-templates select="dc:creator" />
  <xsl:apply-templates select="dc:contributor[contains(@role, 'author')]" />
<xsl:text>  NULL
};
</xsl:text>

<xsl:text>
static const gchar * const artists[] =
{
</xsl:text>
  <xsl:apply-templates select="dc:contributor[contains(@role, 'artist')]" />
<xsl:text>  NULL
};
</xsl:text>

<xsl:text>
static const gchar * const documenters[] =
{
</xsl:text>
  <xsl:apply-templates select="dc:contributor[contains(@role, 'documenter')]" />
<xsl:text>  NULL
};
</xsl:text>
  </xsl:template>

  <xsl:template match="dc:creator">  "<xsl:apply-templates />",
</xsl:template>
  <xsl:template match="dc:contributor">  "<xsl:apply-templates />",
</xsl:template>

</xsl:stylesheet>
