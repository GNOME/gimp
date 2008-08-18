<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		version='1.0'>
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>
<xsl:include href="common.xsl"/>
<xsl:include href="html.xsl"/>
<xsl:include href="devhelp.xsl"/>

  <!-- ========================================================= -->
  <!-- template to create the index.sgml anchor index -->

  <xsl:template name="generate.index">
    <xsl:call-template name="write.text.chunk">
      <xsl:with-param name="filename" select="'index.sgml'"/>
      <xsl:with-param name="content">
        <!-- check all anchor and refentry elements -->
        <xsl:apply-templates select="//anchor|//refentry|//refsect1|//refsect2|//book"
                             mode="generate.index.mode"/>
      </xsl:with-param>
      <xsl:with-param name="encoding" select="'utf-8'"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="*" mode="generate.index.mode">
    <xsl:if test="not(@href)">
      <xsl:if test="@id">
        <xsl:text>&lt;ANCHOR id=&quot;</xsl:text>
        <xsl:value-of select="@id"/>
        <xsl:text>&quot; href=&quot;</xsl:text>
        <xsl:if test="$gtkdoc.bookname">
          <xsl:value-of select="$gtkdoc.bookname"/>
          <xsl:text>/</xsl:text>
        </xsl:if>
        <xsl:call-template name="href.target"/>
        <xsl:text>&quot;&gt;
        </xsl:text>
      </xsl:if>
    </xsl:if>
  </xsl:template>
 
  <xsl:param name="gtkdoc.version" select="''"/>
  <xsl:param name="gtkdoc.bookname" select="''"/>

  <xsl:param name="refentry.generate.name" select="0"/>
  <xsl:param name="refentry.generate.title" select="1"/>
  <xsl:param name="chapter.autolabel" select="0"/>

  <xsl:template match="book|article">
    <xsl:apply-imports/>
    <xsl:call-template name="generate.devhelp"/>
    <xsl:call-template name="generate.index"/>
  </xsl:template>
 
</xsl:stylesheet>
