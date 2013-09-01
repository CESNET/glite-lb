<?xml version="1.0"?>

<!-- $Header$ -->

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output indent="yes" doctype-public="-//OASIS//DTD DocBook XML V4.4//EN"
      doctype-system="http://www.oasis-open.org/docbook/xml/4.4/docbookx.dtd"/>

<xsl:template match="book">
	<chapter>
		<title><xsl:value-of select="document('LB.xml')/service/@name"/></title>

		<sect1>
			<title>Overview</title>
			<xsl:copy-of select="document('LB.xml')/service/doc/*"/>
		</sect1>

		<sect1>
			<title>Operations</title>
			<para> <emphasis><xsl:value-of select="document('LB.xml')/service/version"/></emphasis> </para>
				<!-- xsl:apply-templates select="operations/op" -->
				<xsl:apply-templates select="document('LB.xml')/service/operations/op">
					<xsl:sort select="@name"/>
				</xsl:apply-templates>
		</sect1>
	
		<sect1>
			<title>Types</title>
			<!--xsl:apply-templates select="types"/ -->
			<para> <emphasis><xsl:value-of select="document('LBTypes.xml')/service/version"/></emphasis> </para>
			<xsl:apply-templates select="document('LBTypes.xml')/service/types"/>
		</sect1>
	</chapter>
</xsl:template>


<xsl:template match="input|output|fault">
	<varlistentry>
		<term>
			<!--type-->
				<xsl:if test = "@list = 'yes'">list of </xsl:if>
				<xsl:choose>
					<xsl:when test="not(starts-with(@type,'xsd:'))">
						<link linkend="type:{@type}">
							<type><xsl:value-of select="@type "/> </type> </link>
					</xsl:when>
					<xsl:otherwise><type><xsl:value-of select="@type "/> </type> </xsl:otherwise>
				</xsl:choose>
			<!--/type-->
			<parameter> <xsl:value-of select=" @name"/></parameter>
		</term>
		<listitem>
			<simpara><xsl:value-of select="text()"/></simpara>
		</listitem>
	</varlistentry>
</xsl:template>

<xsl:template match="op" >
	<sect2 id="op:{@name}">
		<title><xsl:value-of select="@name"/></title>
		<para><xsl:value-of select="text()"/></para>
		<para>
			Inputs:
			<xsl:choose>
				<xsl:when test="count(./input)>0">
					<variablelist>
						<xsl:apply-templates select="./input"/>
					</variablelist>
				</xsl:when>
				<xsl:otherwise>N/A</xsl:otherwise>
			</xsl:choose>
		</para>
		<para>
			Outputs:
			<variablelist>
				<xsl:apply-templates select="./output"/>
			</variablelist>
		</para>
	</sect2>
</xsl:template>

<xsl:template match="types">
	<xsl:for-each select="flags|enum|struct|choice">
		<xsl:sort select="@name"/>
		<sect2 id="type:{@name}">
			<title> <xsl:value-of select="@name"/> </title>
			<para> <xsl:value-of select="text()"/> </para>
			<xsl:choose>
				<xsl:when test="name(.)='struct'">
					<para> <emphasis>Structure</emphasis> (sequence complex type in WSDL)</para>
					<para> Fields: ( <type>type </type> <structfield>name</structfield> description )</para>
				</xsl:when>
				<xsl:when test="name(.)='choice'">
					<para> <emphasis>Union</emphasis> (choice complex type in WSDL)</para>
					<para> Fields: ( <type>type </type> <structfield>name</structfield> description )</para>
				</xsl:when>
				<xsl:when test="name(.)='enum'">
					<para> <emphasis>Enumeration</emphasis> (restriction of xsd:string in WSDL),
						exactly one of the values must be specified.
					</para>
					<para> Values: </para>
				</xsl:when>
				<xsl:when test="name(.)='flags'">
					<para> <emphasis>Flags</emphasis> (sequence of restricted xsd:string in WSDL),
						any number of values can be specified together.
					</para>
					<para> Values: </para>
				</xsl:when>
			</xsl:choose>
			<variablelist>
				<xsl:for-each select="elem|val">
					<varlistentry>
						<term>
							<xsl:choose>
								<xsl:when test="name(.)='elem'">
										<xsl:if test="@list = 'yes'">list of </xsl:if>
										<xsl:choose>
											<xsl:when test="@type!='string' and @type!='int' and not(starts-with(@type,'xsd:'))">
												<link linkend="type:{@type}">
													<type><xsl:value-of select="@type "/> </type>
												</link>
											</xsl:when>
											<xsl:otherwise><type><xsl:value-of select="@type "/></type></xsl:otherwise>
										</xsl:choose>
									<!-- <type><xsl:value-of select="@type"/></type> -->
									<xsl:value-of select="' '"/>
									<structfield><xsl:value-of select="@name"/></structfield>
								</xsl:when>
								<xsl:otherwise>
									<constant><xsl:value-of select="@name"/></constant>
								</xsl:otherwise>
							</xsl:choose>
						</term>
						<listitem>
							<simpara>
								<xsl:if test="@optional = 'yes'"> (optional) </xsl:if>
								<!-- <xsl:if test="@list = 'yes'"> (multiple occurence) </xsl:if> -->
								<xsl:value-of select=" text()"/>
							</simpara>
						</listitem>
					</varlistentry>
				</xsl:for-each>
			</variablelist>
		</sect2>
	</xsl:for-each>
</xsl:template>


</xsl:stylesheet>
