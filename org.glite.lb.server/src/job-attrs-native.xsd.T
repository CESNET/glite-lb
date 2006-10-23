<?xml version="1.0"?>

<!-- $Header$ -->

@@@LANG: wsdl

<xsd:schema
	xmlns:xsd="http://www.w3.org/2001/XMLSchema"
	xmlns:a="http://egee.cesnet.cz/en/Schema/LB/Attributes-Native"
	version="1.0"
	targetNamespace="http://egee.cesnet.cz/en/Schema/LB/Attributes-Native"
	elementFormDefault="unqualified"
	attributeFormDefault="unqualified"
>

	<xsd:include location="job-attrs-common.xsd"/>

@@@{
	selectType $status '_common_';
	for (getFieldsOrdered $status) {
		my $f = selectField $status $_;
		next unless $f->{special}->{JP};
		my $t = getType $f;
		my $n = getName $f;

		while ($n =~ /(.*)_(.*)/) { $n = $1 . ucfirst $2; }
		if ($f->{codes}) { $t = $n . 'Type'; }

		$n =~ s/[jJ]dl/JDL/;

		gen qq{\t<xsd:element name="$n" type="$t"/>\n};
	}
@@@}

	<element name="UIHost" type="xsd:string" />

	<element name="finalStatus" type="a:statusType" />
	<element name="finalStatusDate" type="a:statusType" />
	<element name="finalStatusReason" type="a:statusType" />

	<element name="lastStatusHistory" type="a:statusSequenceType" />
	<element name="finalStatusHistory" type="a:statusSequenceType" />

</xsd:schema>
