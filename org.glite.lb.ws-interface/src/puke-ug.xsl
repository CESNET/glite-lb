<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="">

<xsl:output indent="yes"/>


<xsl:template match="/service">

	<chapter>
		<title><xsl:value-of select="@name"/></title>
		<para> <xsl:value-of select="text()"/> </para>
		<sect1>
			<title>Operation summary</title>
				<itemizedlist>
					<xsl:apply-templates select="op" mode="summary"/>
				</itemizedlist>
		</sect1>
	
		<sect1>
			<title>Types</title>
			<xsl:apply-templates select="types"/>
		</sect1>
	
		<sect1>
			<title>Operation detail</title>
			<xsl:apply-templates select="op" mode="detail"/>
		</sect1>
	</chapter>
	
</xsl:template>

<xsl:template match="op" mode="summary">
	<listitem>
		<funcsynopsis>
			<funcprototype>
				<funcdef>
					<xsl:value-of select="./output/@type"/>
					<function>
						<xsl:value-of select="@name"/>
					</function>
				</funcdef>
				<xsl:for-each select="./input">
					<paramdef>
						<xsl:value-of select="@type "/>
						<parameter><xsl:value-of select="@name"/></parameter>
					</paramdef>
				</xsl:for-each>
			</funcprototype>
		</funcsynopsis>
	</listitem>
</xsl:template>

<xsl:template match="op" mode="detail">
</xsl:template>

<xsl:template match="types">
	<xsl:for-each select="flags|enum|struct">
		<xsl:sort select="@name"/>
		<xsl:choose>
			<xsl:when test="name(.)='struct'">
				<variablelist>
					<title>Complex type <classname><xsl:value-of select="@name"/></classname></title>
					<xsl:for-each select="elem">
						<varlistentry>
							<term>
								<type><xsl:value-of select="@type"/></type>
								<structfield><xsl:value-of select="@name"/></structfield>
							</term>
							<listitem>
								<para><xsl:value-of select="text()"/></para>
							</listitem>
						</varlistentry>
					</xsl:for-each>
				</variablelist>
			</xsl:when>
		</xsl:choose>
	</xsl:for-each>
</xsl:template>


</xsl:stylesheet>
