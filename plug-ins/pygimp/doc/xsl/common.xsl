<?xml version='1.0'?>
<!DOCTYPE xsl:stylesheet [
]>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<xsl:template match="parameter">
	<xsl:choose>
		<xsl:when test="@role = 'keyword'">
			<xsl:call-template name="inline.boldmonoseq"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="inline.italicmonoseq"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>
