<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output  method="xml" omit-xml-declaration="no" encoding="UTF-8"/>

  <xsl:template match="node()|@*">
    <xsl:copy>
       <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="//release[@date='TODO']"/>

</xsl:stylesheet>
