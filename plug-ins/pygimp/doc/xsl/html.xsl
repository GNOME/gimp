<?xml version='1.0'?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY RE "&#10;">
<!ENTITY nbsp "&#160;">
]>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<xsl:param name="html.stylesheet">style.css</xsl:param>
<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="chunk.fast" select="1"/>
<xsl:param name="chunker.output.encoding" select="'utf-8'"/>

<xsl:param name="linenumbering.extension" select="1"/>
<xsl:param name="variablelist.as.table" select="1"/>

<xsl:template match="blockquote">
  <div class="{local-name(.)}">
    <xsl:if test="@lang or @xml:lang">
      <xsl:call-template name="language.attribute"/>
    </xsl:if>
    <xsl:call-template name="anchor"/>

    <xsl:choose>
      <xsl:when test="attribution">
        <table border="0" width="100%"
               cellspacing="0" cellpadding="0" class="blockquote"
               summary="Block quote">
          <tr>
            <td width="10%" valign="top">&#160;</td>
            <td width="80%" valign="top">
              <xsl:apply-templates select="child::*[local-name(.)!='attribution']"/>
            </td>
            <td width="10%" valign="top">&#160;</td>
          </tr>
          <tr>
            <td colspan="2" align="right" valign="top">
              <xsl:text>--</xsl:text>
              <xsl:apply-templates select="attribution"/>
            </td>
            <td width="10%" valign="top">&#160;</td>
          </tr>
        </table>
      </xsl:when>
      <xsl:when test="@role = 'properties' or @role = 'prototypes'">
        <table width="100%" border="0">
          <tr>
            <td valign="top">
              <xsl:apply-templates select="child::*[local-name(.)!='attribution']"/>
            </td>
          </tr>
        </table>
      </xsl:when>
      <xsl:otherwise>
        <blockquote class="{local-name(.)}">
          <xsl:apply-templates/>
        </blockquote>
      </xsl:otherwise>
    </xsl:choose>
  </div>
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
    <table width="100%">
      <tr><td>
    <pre class="{name(.)}">
      <xsl:text>class </xsl:text>
      <xsl:apply-templates select="ooclass[1]" mode="python"/>
      <xsl:if test="ooclass[position() &gt; 1]">
        <xsl:text>(</xsl:text>
        <xsl:apply-templates select="ooclass[position() &gt; 1]" mode="python"/>        <xsl:text>)</xsl:text>
      </xsl:if>
      <xsl:text>:&RE;</xsl:text>

      <xsl:apply-templates select="constructorsynopsis
                                   |destructorsynopsis
                                   |fieldsynopsis
                                   |methodsynopsis
                                   |classsynopsisinfo" mode="python"/>
    </pre></td></tr></table>
  </xsl:template>

  <xsl:template match="classsynopsisinfo" mode="python">
    <xsl:apply-templates mode="python"/>
  </xsl:template>

  <xsl:template match="ooclass|oointerface|ooexception" mode="python">
    <xsl:if test="position() &gt; 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="modifier" mode="python">
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
    </span>
  </xsl:template>

  <xsl:template match="classname" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'classname'">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="interfacename" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'interfacename'">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="exceptionname" mode="python">
    <xsl:if test="name(preceding-sibling::*[1]) = 'exceptionname'">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="fieldsynopsis" mode="python">
    <code class="{name(.)}">
      <xsl:text>&nbsp;&nbsp;&nbsp;&nbsp;</xsl:text>
      <xsl:apply-templates mode="python"/>
    </code>
    <xsl:call-template name="synop-break"/>
  </xsl:template>

  <xsl:template match="type" mode="python">
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
    </span>
  </xsl:template>

  <xsl:template match="varname" mode="python">
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
      <xsl:text>&nbsp;</xsl:text>
    </span>
  </xsl:template>

  <xsl:template match="initializer" mode="python">
    <span class="{name(.)}">
      <xsl:text>=</xsl:text>
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="void" mode="python">
    <span class="{name(.)}">
      <xsl:text>void&nbsp;</xsl:text>
    </span>
  </xsl:template>

  <xsl:template match="methodname" mode="python">
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

  <xsl:template match="methodparam" mode="python">
    <xsl:if test="position() &gt; 1">
      <xsl:text>, </xsl:text>
    </xsl:if>
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>

 <xsl:template mode="python"
    match="destructorsynopsis|methodsynopsis">

    <code class="{name(.)}">
      <xsl:text>    def </xsl:text>
      <xsl:apply-templates select="methodname" mode="python"/>
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="methodparam" mode="python"/>
      <xsl:text>)</xsl:text>
    </code>
    <xsl:call-template name="synop-break"/>
  </xsl:template>

 <xsl:template mode="python"
    match="constructorsynopsis">

    <code class="{name(.)}">
      <xsl:text>    </xsl:text>
      <xsl:apply-templates select="methodname" mode="python"/>
      <xsl:text>(</xsl:text>
      <xsl:apply-templates select="methodparam" mode="python"/>
      <xsl:text>)</xsl:text>
    </code>
    <xsl:call-template name="synop-break"/>
  </xsl:template>

<!-- this was  the original parameter python mode styling
  <xsl:template match="parameter" mode="python">
    <span class="{name(.)}">
      <xsl:apply-templates mode="python"/>
    </span>
  </xsl:template>
-->

  <!-- hack -->
  <xsl:template match="link" mode="python">
    <xsl:apply-templates select="."/>
  </xsl:template>

  <!-- ========================================================= -->
  <!-- template to output gtkdoclink elements for the unknown targets -->

  <xsl:template match="link">
    <xsl:choose>
      <xsl:when test="id(@linkend)">
        <xsl:apply-imports/>
      </xsl:when>
      <xsl:otherwise>
        <PYGTKDOCLINK HREF="{@linkend}">
          <xsl:apply-templates/>
        </PYGTKDOCLINK>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

 <xsl:template match="parameter" mode="python">
    <span class="{name(.)}">
	<xsl:choose>
		<xsl:when test="@role = 'keyword'">
			<xsl:call-template name="inline.boldmonoseq"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="inline.italicmonoseq"/>
		</xsl:otherwise>
	</xsl:choose>
    </span>
</xsl:template>

<xsl:template match="variablelist">
  <table border="0" width="100%">
    <col align="left" valign="top" width="0*">
    </col>
    <tbody>
      <xsl:apply-templates select="varlistentry" mode="varlist-table"/>
    </tbody>
  </table>
</xsl:template>

</xsl:stylesheet>
