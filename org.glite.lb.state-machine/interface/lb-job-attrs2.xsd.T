<?xml version="1.0"?>
<!DOCTYPE schema PUBLIC "-//W3C//DTD XMLSCHEMA 200102//EN" "http://www.w3.org/2001/XMLSchema.dtd">

<!-- $Header$ -->

<!--
@@@AUTO
-->

@@@LANG: wsdl

<xsd:schema
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:a="http://egee.cesnet.cz/en/Schema/LB/Attributes"
	xmlns:a2="http://egee.cesnet.cz/en/Schema/LB/Attributes"
	targetNamespace="http://egee.cesnet.cz/en/Schema/LB/Attributes"
	version="2.0"
	elementFormDefault="qualified"
	attributeFormDefault="unqualified"
>

	<xsd:simpleType name="statusType">
		<xsd:restriction base="xsd:string">
@@@{
	for my $s ($status->getTypesOrdered) {
		my $u = uc $s;
		gen qq{			<xsd:enumeration value="$u"/>\n};
	}
@@@}
		</xsd:restriction>
	</xsd:simpleType>

@@@{
# _code_ -> ...Type
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		if ($f->{codes}) {
			my $ln = getName $f;
			@ln = split /_/,$ln;
			my $un = shift @ln; $un .= ucfirst shift @ln while (@ln);
			gen qq{
	<xsd:simpleType name="${un}Type">
		<xsd:restriction base="xsd:string">
};

			for (@{$f->{codes}}) {
				my $uc = uc $_->{name};
				gen qq{			<xsd:enumeration value="$uc"/> \n};
			}
			gen qq{
		</xsd:restriction>
	</xsd:simpleType>
};
		}
	}

@@@}


	<xsd:complexType name="historyStatusType">
		<xsd:complexContent>
			<xsd:restriction base="xsd:anyType">
				<xsd:attribute name="name" type="a:statusType" use="required"/>
				<xsd:attribute name="timestamp" type="xsd:dateTime" use="required"/>
				<xsd:attribute name="reason" type="xsd:string" use="optional"/>
			</xsd:restriction>
		</xsd:complexContent>
	</xsd:complexType>

	<xsd:complexType name="statusSequenceType">
		<xsd:sequence>
			<xsd:element name="status" type="a:historyStatusType" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="jobIdSequenceType">
		<xsd:sequence>
			<xsd:element name="jobId" type="xsd:string" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="stringSequenceType">
		<xsd:sequence>
			<xsd:element name="item" type="xsd:string" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="intSequenceType">
		<xsd:sequence>
			<xsd:element name="item" type="xsd:int" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="tagValue">
		<xsd:sequence>
			<xsd:element name="name" type="xsd:string" minOccurs="1" maxOccurs="1"/>
			<xsd:element name="value" type="xsd:string" minOccurs="0" maxOccurs="1"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="tagSequenceType">
		<xsd:sequence>
			<xsd:element name="tag" type="a:TagValue" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
	</xsd:complexType>

	<xsd:complexType name="timeval">
		<xsd:sequence>
			<xsd:element name="tvSec" type="xsd:long"/>
			<xsd:element name="tvUsec" type="xsd:long"/>
		</xsd:sequence>
	</xsd:complexType>


@@@{
# elems
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		my $type = getType $f;
		my $name = getName $f;

		$type = 'jobtypeType' if $name eq 'jobtype';

		@ln = split /_/,$name;
		my $un = shift @ln; $un .= ucfirst shift @ln while (@ln);

		gen qq{	<xsd:element name="$un" type="$type"/>\n};

	}
@@@}

</xsd:schema>

