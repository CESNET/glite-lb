<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xs="http://www.w3.org/2001/XMLSchema"
>

<xsl:output method="text"/>

<xsl:template match="xs:schema">
#ifndef __GLITE_LB_JP_JOB_ATTR_H
#define __GLITE_LB_JP_JOB_ATTR_H
#define GLITE_JP_LB_NS "http://egee.cesnet.cz/en/Schema/LB/Attributes"
#define GLITE_JP_LB_JDL_NS "http://egee.cesnet.cz/en/Schema/LB/Attributes:JDL"
	<xsl:apply-templates select="xs:element"/>
#define GLITE_JP_LB_CLASSAD_NS "http://jdl"
#endif
</xsl:template>

<xsl:template match="xs:element">
/** <xsl:value-of select="xs:documentation/text()"/> */
#define GLITE_JP_LB_<xsl:value-of select="@name"/>	GLITE_JP_LB_NS &quot;:<xsl:value-of select="@name"/>&quot;
</xsl:template>

</xsl:stylesheet>
