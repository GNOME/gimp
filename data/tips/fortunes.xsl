<?xml version="1.0" encoding="utf-8"?>

<!-- This simple XSL transformation creates a text version from
     gimp-tips.xml.in which can then be used to seed
     https://wiki.gimp.org/gimp/FortuneCookies in the GIMP Wiki. -->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- Pass this stylesheet a lang parameter in order to select a language. -->
  <xsl:param name="lang" />

  <xsl:output method="text" />

  <xsl:template match="/">
    <xsl:apply-templates select="//thetip[lang($lang)]" />
  </xsl:template>

  <xsl:template match="thetip">
 * <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="tt">
    <xsl:text>{{{</xsl:text>
    <xsl:apply-templates />
    <xsl:text>}}}</xsl:text>
  </xsl:template>

  <!-- This sucks, but I don't seem to get xsl:strip-space to work. -->
  <xsl:template match="text()">
    <xsl:call-template name="search-and-replace">
      <xsl:with-param name="input" select="." />
      <xsl:with-param name="search-string"  select="'&#xa;    '" />
      <xsl:with-param name="replace-string" select="' '" />
    </xsl:call-template>
  </xsl:template>   

  <xsl:template name="search-and-replace">
    <xsl:param name="input" />
    <xsl:param name="search-string" />
    <xsl:param name="replace-string" />
    <xsl:choose>
      <xsl:when test="$search-string and contains($input, $search-string)">
	<xsl:value-of select="substring-before($input, $search-string)" />
	<xsl:value-of select="$replace-string" />
	<xsl:call-template name="search-and-replace">
	  <xsl:with-param name="input"
			  select="substring-after($input, $search-string)" />
	  <xsl:with-param name="search-string" select="$search-string" />
	  <xsl:with-param name="replace-string" select="$replace-string" />
	</xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
	<xsl:value-of select="$input" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
