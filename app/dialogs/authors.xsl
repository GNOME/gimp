<?xml version="1.0" encoding="UTF-8"?>

<!--  XSL transformation to create a header file from authors.xml  -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dc="http://purl.org/dc/elements/1.1/">

  <xsl:output method="text" />

  <xsl:template name="recent-contributor">
    <xsl:param name="role" />
    <xsl:apply-templates select="dc:contributor[contains(@role, $role) and number(@last-active) >= 2.8]" />
  </xsl:template>

  <xsl:template match="/dc:gimp-authors">
<xsl:text>
/* NOTE: This file is auto-generated from authors.xml, do not edit it. */

static const gchar * const creators[] =
{
</xsl:text>
  <xsl:apply-templates select="dc:creator" />
<xsl:text>  NULL
};
</xsl:text>

<xsl:text>
static const gchar * const authors[] =
{
</xsl:text>
  <xsl:apply-templates select="dc:creator" />
  <xsl:call-template name="recent-contributor">
    <xsl:with-param name="role" select="'author'"/>
  </xsl:call-template>
<xsl:text>  NULL
};
</xsl:text>

<xsl:text>
static const gchar * const artists[] =
{
</xsl:text>
  <xsl:call-template name="recent-contributor">
    <xsl:with-param name="role" select="'artist'"/>
  </xsl:call-template>
<xsl:text>  NULL
};
</xsl:text>

<xsl:text>
static const gchar * const documenters[] =
{
</xsl:text>
  <xsl:call-template name="recent-contributor">
    <xsl:with-param name="role" select="'documenter'"/>
  </xsl:call-template>
<xsl:text>  NULL
};
</xsl:text>
  </xsl:template>

  <xsl:template match="dc:creator">  "<xsl:apply-templates />",
</xsl:template>
  <xsl:template match="dc:contributor">  "<xsl:apply-templates />",
</xsl:template>

</xsl:stylesheet>
