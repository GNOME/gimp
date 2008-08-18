<?xml version='1.0'?> <!--*- mode: xml -*-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		version='1.0'>
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>
<xsl:include href="common.xsl"/>
<xsl:include href="pdf.xsl"/>

<xsl:param name="section.autolabel" select="1"/>
<xsl:param name="section.label.includes.component.label" select="1"/>

</xsl:stylesheet>
