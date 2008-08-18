<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.devhelp.net/book"
                exclude-result-prefixes="#default">

  <xsl:template name="generate.devhelp">
    <xsl:call-template name="write.chunk">
      <xsl:with-param name="filename">
        <xsl:choose>
          <xsl:when test="$gtkdoc.bookname">
            <xsl:value-of select="$gtkdoc.bookname"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>book</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>.devhelp</xsl:text>
      </xsl:with-param>
      <xsl:with-param name="method" select="'xml'"/>
      <xsl:with-param name="indent" select="'yes'"/>
      <xsl:with-param name="encoding" select="'utf-8'"/>
      <xsl:with-param name="content">
        <xsl:call-template name="devhelp"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="devhelp">
    <xsl:variable name="title">
      <xsl:apply-templates select="." mode="generate.devhelp.toc.title.mode"/>
    </xsl:variable>
    <xsl:variable name="link">
      <xsl:call-template name="href.target"/>
    </xsl:variable>
    <xsl:variable name="author">
      <xsl:if test="articleinfo|bookinfo">
        <xsl:apply-templates mode="generate.devhelp.authors"
                             select="articleinfo|bookinfo"/>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="toc.nodes" select="part|reference|preface|chapter|
                                           appendix|article|bibliography|
                                           glossary|index|refentry|
                                           bridgehead|sect1"/>

    <book title="{$title}" link="{$link}" author="{$author}" name="{$gtkdoc.bookname}">
      <xsl:if test="$toc.nodes">
        <chapters>
          <xsl:apply-templates select="$toc.nodes"
                               mode="generate.devhelp.toc.mode"/>
        </chapters>
      </xsl:if>
      <functions>
        <xsl:apply-templates select="//refsect1"
                             mode="generate.devhelp.constructor.index.mode"/>
        <xsl:apply-templates select="//refsect2"
                             mode="generate.devhelp.index.mode"/>
      </functions>
    </book>
  </xsl:template>

  <xsl:template match="*" mode="generate.devhelp.toc.mode">
    <xsl:variable name="title">
      <xsl:apply-templates select="." mode="generate.devhelp.toc.title.mode"/>
    </xsl:variable>
    <xsl:variable name="target">
      <xsl:variable name="anchor" select="title/anchor"/>
      <xsl:choose>
        <xsl:when test="$anchor">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$anchor"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="href.target"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <sub name="{$title}" link="{$target}">
      <xsl:apply-templates select="section|sect1|
                                   refentry|refsect|
                                   bridgehead|part|chapter"
                           mode="generate.devhelp.toc.mode"/>
    </sub>
  </xsl:template>

  <xsl:template match="*" mode="generate.devhelp.index.mode">
    <xsl:variable name="title" select="title"/>
    <xsl:variable name="anchor" select="title/anchor"/>
    <xsl:variable name="target">
      <xsl:choose>
        <xsl:when test="$anchor">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$anchor"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="href.target"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <function name="{$title}" link="{$target}"/>
  </xsl:template>

  <xsl:template match="*" mode="generate.devhelp.constructor.index.mode">
    <xsl:variable name="title" select="title"/>
    <xsl:variable name="anchor" select="title/anchor"/>
    <xsl:variable name="target">
      <xsl:choose>
        <xsl:when test="$anchor">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$anchor"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="href.target"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$title='Constructor'">
      <xsl:variable name ="constructor" select="programlisting//methodname"/>
      <function name="{$constructor}" link="{$target}"/>
    </xsl:if>
  </xsl:template>

  <!-- get title -->
  <xsl:template match="article" mode="generate.devhelp.toc.title.mode">
    <xsl:value-of select="articleinfo/title"/>
  </xsl:template>
  <xsl:template match="book" mode="generate.devhelp.toc.title.mode">
    <xsl:value-of select="bookinfo/title"/>
  </xsl:template>
  <xsl:template match="refentry" mode="generate.devhelp.toc.title.mode">
    <xsl:value-of select="refnamediv/refname"/>
  </xsl:template>
  <xsl:template match="*" mode="generate.devhelp.toc.title.mode">
    <xsl:value-of select="title"/>
  </xsl:template>

  <!-- generate list of authors ... -->
  <xsl:template match="articleinfo|bookinfo" mode="generate.devhelp.authors">
    <xsl:for-each select="authorgroup/author">
      <xsl:value-of select="firstname"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="surname"/>
      <xsl:if test="not(last())">
        <xsl:text>, </xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
