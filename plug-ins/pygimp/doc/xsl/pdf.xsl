<?xml version='1.0'?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY RE "&#10;">
<!ENTITY nbsp "&#160;">
]>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<xsl:param name="body.margin.top">0.5in</xsl:param>

<xsl:template name="is.graphic.extension">
  <xsl:param name="ext"></xsl:param>
  <xsl:if test="$ext = 'png'
                or $ext = 'pdf'
                or $ext = 'jpeg'
                or $ext = 'gif'
                or $ext = 'tif'
                or $ext = 'tiff'
                or $ext = 'bmp'">1</xsl:if>
</xsl:template>

  <!-- support for Python language for synopsises -->
  <xsl:template match="classsynopsis
                     |fieldsynopsis
                     |methodsynopsis
                     |constructorsynopsis
                     |destructorsynopsis">
    <xsl:param name="language">
      <xsl:choose>
        <xsl:when test="@language">
          <xsl:value-of select="@language"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$default-classsynopsis-language"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:param>
    <xsl:choose>
      <xsl:when test="$language='python'">
        <xsl:apply-templates select="." mode="python"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-imports/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="classsynopsis" mode="python">
  <fo:block wrap-option='no-wrap'
            white-space-collapse='false'
            linefeed-treatment="preserve"
            xsl:use-attribute-sets="monospace.verbatim.properties"
            background-color="#E0E0E0">
    <xsl:text>class </xsl:text>
    <xsl:apply-templates select="ooclass[1]" mode="python"/>
    <xsl:if test="ooclass[position() &gt; 1]">
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="ooclass[position() &gt; 1]" mode="python"/>
      <xsl:text>)</xsl:text>
    </xsl:if>
    <xsl:text>&nbsp;:</xsl:text>

      <xsl:apply-templates select="constructorsynopsis
                                   |destructorsynopsis
                                   |fieldsynopsis
                                   |methodsynopsis
                                   |classsynopsisinfo" mode="python"/>

  </fo:block>
  <xsl:text>&RE;</xsl:text>
  </xsl:template>

  <xsl:template match="classsynopsisinfo" mode="python">
    <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="ooclass|oointerface|ooexception" mode="python">
    <xsl:if test="position() &gt; 1">
      <xsl:text>, </xsl:text>
   </xsl:if>
  <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="modifier" mode="python">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
  </xsl:template>

  <xsl:template match="classname" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'classname'">
      <xsl:text>, </xsl:text>
    </xsl:if>
      <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="interfacename" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'interfacename'">
      <xsl:text>, </xsl:text>
    </xsl:if>
      <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="exceptionname" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'exceptionname'">
      <xsl:text>, </xsl:text>
    </xsl:if>
      <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="fieldsynopsis" mode="python">
  <fo:block wrap-option='no-wrap'
            white-space-collapse='false'
            linefeed-treatment="preserve"
            xsl:use-attribute-sets="monospace.verbatim.properties">
      <xsl:text>&nbsp;&nbsp;&nbsp;&nbsp;</xsl:text>
      <xsl:apply-templates mode="python"/>
    <xsl:call-template name="synop-break"/>
  </fo:block>
  </xsl:template>

  <xsl:template match="type" mode="python">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
  </xsl:template>

  <xsl:template match="varname" mode="python">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
  </xsl:template>

  <xsl:template match="initializer" mode="python">
      <xsl:text>=</xsl:text>
      <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="void" mode="python">
      <xsl:text>void&nbsp;</xsl:text>
  </xsl:template>

  <xsl:template match="methodname" mode="python">
      <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="methodparam" mode="python">
    <xsl:if test="position() &gt; 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
      <xsl:apply-templates mode="python"/>
  </xsl:template>

 <xsl:template mode="python"
    match="destructorsynopsis|methodsynopsis">

  <fo:block wrap-option='no-wrap'
            white-space-collapse='false'
            linefeed-treatment="preserve"
            xsl:use-attribute-sets="monospace.verbatim.properties">
      <xsl:text>    def </xsl:text>
      <xsl:apply-templates select="methodname" mode="python"/>
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="methodparam" mode="python"/>
      <xsl:text>)</xsl:text>
<!--    <xsl:call-template name="synop-break"/> -->
   </fo:block>
  </xsl:template>

 <xsl:template mode="python"
    match="constructorsynopsis">
    <fo:block  wrap-option='no-wrap'
            white-space-collapse='false'
            linefeed-treatment="preserve"
            xsl:use-attribute-sets="monospace.verbatim.properties">
      <xsl:text>    </xsl:text>
      <xsl:apply-templates select="methodname" mode="python"/>
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="methodparam" mode="python"/>
      <xsl:text>)</xsl:text>
  </fo:block>
  </xsl:template>

  <!-- hack -->
  <xsl:template match="link" mode="python">
    <xsl:apply-templates select="."/>
  </xsl:template>

<!--
<xsl:template match="variablelist" mode="vl.as.blocks">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:if test="title">
    <xsl:apply-templates select="title" mode="list.title.mode"/>
  </xsl:if>

  <fo:block id="{$id}" 
            xsl:use-attribute-sets="list.block.spacing"
            background-color="#FFECCE">
    <xsl:apply-templates mode="vl.as.blocks"/>
  </fo:block>
</xsl:template>
-->

<!--
<xsl:template match="variablelist">
  <fo:table border="0"
            width="100%" 
            background-color="#FFECCE"
            table-layout="fixed">
    <fo:table-column
            align="left" 
            column-width="20%"
            column-number="1">
    </fo:table-column>
    <fo:table-column
            align="left" 
            column-width="80%"
            column-number="2">
    </fo:table-column>
    <fo:table-body>
      <xsl:apply-templates select="varlistentry"/>
    </fo:table-body>
  </fo:table>
</xsl:template>

<xsl:template match="varlistentry">
  <fo:table-row>
       <fo:table-cell>
          <fo:block
            background-color="#FFECCE">
          <xsl:apply-templates select="term"/>
          </fo:block>
       </fo:table-cell>
       <fo:table-cell>
          <fo:block
            background-color="#FFECCE">
          <xsl:apply-templates select="listitem"/>
          </fo:block>
       </fo:table-cell>
  </fo:table-row>
</xsl:template>

<xsl:template match="varlistentry/term">
    <xsl:apply-templates/>
    <xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="varlistentry/term[position()=last()]" priority="2">
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="varlistentry/listitem">
      <xsl:apply-templates/>
</xsl:template>
-->

</xsl:stylesheet>
