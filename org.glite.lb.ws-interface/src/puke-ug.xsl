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
			<title>Operations</title>
				<itemizedlist>
					<xsl:apply-templates select="op"/>
				</itemizedlist>
		</sect1>
	
		<sect1>
			<title>Types</title>
			<xsl:apply-templates select="types"/>
		</sect1>
	
	</chapter>
	
</xsl:template>

<xsl:template match="op" >
	<listitem>
		<para>
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
		</para>
		<para> <xsl:value-of select="text()"/> </para>
	</listitem>
</xsl:template>

<xsl:template match="op" mode="detail">
</xsl:template>

<xsl:template match="types">
	<xsl:for-each select="flags|enum|struct">
		<xsl:sort select="@name"/>
		<sect2>
			<title> <xsl:value-of select="@name"/> </title>
			<xsl:choose>
				<xsl:when test="name(.)='struct'">
					<para> <emphasis>Structure</emphasis> (sequence complex type in WSDL)</para>
				</xsl:when>
				<xsl:when test="name(.)='enum'">
					<para> <emphasis>Enumeration</emphasis> (restriction of xsd:string in WSDL)</para>
				</xsl:when>
				<xsl:when test="name(.)='flags'">
					<para> <emphasis>Flags</emphasis> (sequence of restricted xsd:string in WSDL)</para>
				</xsl:when>
			</xsl:choose>
			<para> <xsl:value-of select="text()"/> </para>
			<variablelist>
				<xsl:for-each select="elem|val">
					<varlistentry>
						<term>
							<xsl:choose>
								<xsl:when test="name(.)='elem'">
									<type><xsl:value-of select="@type"/></type>
									<xsl:value-of select="' '"/>
									<structfield><xsl:value-of select="@name"/></structfield>
								</xsl:when>
								<xsl:otherwise>
									<constant><xsl:value-of select="@name"/></constant>
								</xsl:otherwise>
							</xsl:choose>
						</term>
						<listitem>
							<simpara><xsl:value-of select="text()"/></simpara>
						</listitem>
					</varlistentry>
				</xsl:for-each>
			</variablelist>
		</sect2>
	</xsl:for-each>
</xsl:template>


</xsl:stylesheet>
