<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns="http://schemas.xmlsoap.org/wsdl/"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
	xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"

	xmlns:lb="http://glite.org/wsdl/services/lb"
	xmlns:lbe="http://glite.org/wsdl/elements/lb"
	xmlns:lbt="http://glite.org/wsdl/types/lb">

<xsl:output indent="yes"/>

<xsl:template match="/service">
	<definitions
		xmlns="http://schemas.xmlsoap.org/wsdl/"
		name="{@name}"
		targetNamespace="{@ns}">
  	<documentation>
			<xsl:value-of select="version"/>
			<xsl:value-of select="text()"/> 
		</documentation>

		<xsl:apply-templates select="import"/>

		<xsl:apply-templates select="types"/>
		
<!--		<xsl:apply-templates select="fault"/> -->

		<xsl:apply-templates select="operations"/>
		
	</definitions>
</xsl:template>

<xsl:template match="types">
	<wsdl:types>
		<xsd:schema targetNamespace="{@ns}"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates/>
		</xsd:schema>
	</wsdl:types>
	<!-- <xsl:apply-templates select="struct[@fault='yes']" mode="message"/> -->
</xsl:template>

<!--
<xsl:template match="simple">
	<xsd:element name="{@name}" type="xsd:{@name}"/>
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="xsd:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
	<xsd:element name="{@name}List" type="{/service/@typePrefix}:{@name}List"/>
</xsl:template>

<xsl:template match="list">
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="xsd:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
</xsl:template>
-->

<xsl:template match="enum">
	<xsd:simpleType name="{@name}">
		<xsd:restriction base="xsd:string">
			<xsl:for-each select="val"><xsd:enumeration value="{@name}"/></xsl:for-each>
		</xsd:restriction>
	</xsd:simpleType>
<!--	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/> -->
</xsl:template>

<xsl:template match="flags">
	<xsd:simpleType name="{@name}Value">
		<xsd:restriction base="xsd:string">
			<xsl:for-each select="val"><xsd:enumeration value="{@name}"/></xsl:for-each>
		</xsd:restriction>
	</xsd:simpleType>
	<xsd:complexType name="{@name}">
		<xsd:sequence>
			<xsd:element name="flag" type="{/service/@typePrefix}:{@name}Value" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>
<!--	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/> -->
</xsl:template>

<xsl:template match="struct">
	<xsd:complexType name="{@name}">
		<xsd:sequence>
			<xsl:call-template name="inner-struct"/>
		</xsd:sequence>
	</xsd:complexType>
</xsl:template>

<xsl:template match="choice">
	<xsd:complexType name="{@name}">
		<xsd:choice>
			<xsl:call-template name="inner-struct"/>
		</xsd:choice>
	</xsd:complexType>
</xsl:template>


<xsl:template name="inner-struct">
	<xsl:variable name="nillable">
		<xsl:choose>
			<xsl:when test="local-name(.)='choice'">true</xsl:when>
			<xsl:otherwise>false</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>
			<xsl:for-each select="elem">
				<xsl:variable name="type">
					<xsl:choose>
						<xsl:when test="contains(@type,':')">
							<xsl:value-of select="@type"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="/service/@typePrefix"/>:<xsl:value-of select="@type"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsl:variable name="min">
					<xsl:choose>
						<xsl:when test="@optional='yes'">0</xsl:when>
						<xsl:otherwise>1</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsl:variable name="max">
					<xsl:choose>
						<xsl:when test="@list='yes'">unbounded</xsl:when>
						<xsl:otherwise>1</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<xsd:element name="{@name}" type="{$type}" minOccurs="{$min}" maxOccurs="{$max}" nillable="{$nillable}"/>
			</xsl:for-each>
<!--
	<xsd:complexType name="{@name}List">
		<xsd:sequence>
			<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}" minOccurs="0" maxOccurs="unbounded"></xsd:element>
		</xsd:sequence>
	</xsd:complexType>
	<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
	<xsd:element name="{@name}List" type="{/service/@typePrefix}:{@name}List"/>
-->
</xsl:template>

<xsl:template match="op" mode="message">
	<wsdl:message name="{@name}Request">
		<wsdl:part name="input" element="{/service/@elemPrefix}:{@name}">
			<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		</wsdl:part>
	</wsdl:message>
	<wsdl:message name="{@name}Response">
		<wsdl:part name="output" element="{/service/@elemPrefix}:{@name}Response">
			<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		</wsdl:part>
	</wsdl:message>
</xsl:template>

<xsl:template match="op" mode="element">
	<xsd:element name="{@name}">
		<xsd:complexType>
			<xsd:sequence>
				<xsl:for-each select="input">
					<xsl:variable name="prefix">
						<xsl:choose>
							<xsl:when test="starts-with(@type,'xsd:')"/>
							<xsl:otherwise><xsl:value-of select="/service/@typePrefix"/>:</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsl:variable name="max">
						<xsl:choose>
							<xsl:when test="@list='yes'">unbounded</xsl:when>
							<xsl:otherwise>1</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsl:variable name="min">
						<xsl:choose>
							<xsl:when test="@optional='yes'">0</xsl:when>
							<xsl:otherwise>1</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsd:element name="{@name}" type="{$prefix}{@type}" minOccurs="{$min}" maxOccurs="{$max}"/>
				</xsl:for-each>
			</xsd:sequence>
		</xsd:complexType>
	</xsd:element>
	<xsd:element name="{@name}Response">
		<xsd:complexType>
			<xsd:sequence>
				<xsl:for-each select="output">
					<xsl:variable name="prefix">
						<xsl:choose>
							<xsl:when test="starts-with(@type,'xsd:')"/>
							<xsl:otherwise><xsl:value-of select="/service/@typePrefix"/>:</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsl:variable name="max">
						<xsl:choose>
							<xsl:when test="@list='yes'">unbounded</xsl:when>
							<xsl:otherwise>1</xsl:otherwise>
						</xsl:choose>
					</xsl:variable>
					<xsd:element name="{@name}" type="{$prefix}{@type}" minOccurs="1" maxOccurs="{$max}"/>
				</xsl:for-each>
			</xsd:sequence>
		</xsd:complexType>
	</xsd:element>
</xsl:template>


<xsl:template match="struct[@fault='yes']" mode="message">
	<wsdl:message name="{@name}">
		<wsdl:part name="{@name}" element="{/service/@typePrefix}:{@name}">
			<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		</wsdl:part>
	</wsdl:message>
</xsl:template>

<xsl:template match="op" mode="port-type">
	<wsdl:operation name="{@name}">
		<wsdl:documentation><xsl:value-of select="text()"/></wsdl:documentation>
		<wsdl:input name="i" message="{/service/@prefix}:{@name}Request"/>
		<wsdl:output name="o" message="{/service/@prefix}:{@name}Response"/>
		<wsdl:fault name="f" message="{/service/@prefix}:{fault/@name}"/>
	</wsdl:operation>
</xsl:template>

<xsl:template match="op" mode="binding">
	<wsdl:operation name="{@name}">
		<soap:operation style="document"/>
		<wsdl:input name="i">
			<soap:body use="literal"/>
		</wsdl:input>
		<wsdl:output name="o">
			<soap:body use="literal"/>
		</wsdl:output>
		<wsdl:fault name="f">
			<soap:fault name="f" use="literal"/>
		</wsdl:fault>
	</wsdl:operation>
</xsl:template>

<xsl:template match="import">
	<wsdl:import namespace="{@namespace}" location="{@location}"/>
</xsl:template>

<xsl:template match="operations">
	<wsdl:types>
		<xsd:schema targetNamespace="{/service/@elemNs}"
			elementFormDefault="unqualified"
			attributeFormDefault="unqualified">

			<xsl:apply-templates select="op" mode="element"/>

			<xsl:for-each select="/service/fault">
				<xsd:element name="{@name}" type="{/service/@typePrefix}:{@name}"/>
			</xsl:for-each>
		</xsd:schema>
	</wsdl:types>

		<xsl:apply-templates select="/service/fault"/>

		<xsl:apply-templates select="op" mode="message"/>

		<wsdl:portType name="{/service/@name}PortType">
			<xsl:apply-templates select="op" mode="port-type"/>
		</wsdl:portType>

		<binding name="{/service/@name}" type="{/service/@prefix}:{/service/@name}PortType">
			<soap:binding style="document" transport="http://schemas.xmlsoap.org/soap/http"/>
			<xsl:apply-templates select="op" mode="binding"/>
		</binding>

		<service name="{/service/@name}">
			<documentation><xsl:value-of select="text()"/></documentation>
			<port name="{/service/@name}" binding="{/service/@prefix}:{/service/@name}">
				<soap:address location="http://test.glite.org/{/service/@prefix}:8080"/>
			</port>

		</service>

</xsl:template>

<xsl:template match="fault">
	<wsdl:message name="{@name}">
		<wsdl:part name="{@name}" element="{/service/@elemPrefix}:{@name}" />
	</wsdl:message>
</xsl:template>


</xsl:stylesheet>

