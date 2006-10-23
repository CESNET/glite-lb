<?xml version="1.0"?>

<xs:schema
	xmlns:xs="http://www.w3.org/2001/XMLSchema"
	version="1.0"
	elementFormDefault="unqualified"
	attributeFormDefault="unqualified"
>

@@@LANG: wsdl

	<xs:simpleType name="statusType">
		<xs:restriction base="xs:string">
@@@{
	for my $stat ($status->getTypesOrdered) {
		my $u = uc $stat;
		gen qq{\t\t\t<xs:enumeration value="$u" />\n};
	}
@@@}
		</xs:restriction>
	</xs:simpleType>

@@@{
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		if ($f->{codes}) {
			my $n = getName $f;
			while ($n =~ /(.*)_(.*)/) { $n = $1 . ucfirst $2; }
			gen qq{\t<xs:simpleType name="${n}Type">\n\t\t<xs:restriction base="xs:string">\n};

			for (@{$f->{codes}}) {
				my $uc = uc $_->{name};
				gen qq{\t\t\t<xs:enumeration value="$uc"/>\n};
			}

			gen qq{\t\t</xs:restriction>\n\t</xs:simpleType>\n\n};
		}
	}
@@@}

	<xs:complexType name="historyStatusType">
		<xs:complexContent>
			<xs:restriction base="xs:anyType">
				<xs:attribute name="status" type="statusType" use="required"/>
				<xs:attribute name="timestamp" type="xs:dateTime" use="required"/>
				<xs:attribute name="reason" type="xs:string" use="optional"/>
			</xs:restriction>
		</xs:complexContent>
	</xs:complexType>

	<xs:complexType name="statusSequenceType">
		<xs:sequence>
			<xs:element name="status" type="historyStatusType" minOccurs="0" maxOccurs="unbounded"/>
		</xs:sequence>
	</xs:complexType>

	<xs:complexType name="jobIdSequenceType">
		<xs:sequence>
			<xs:element name="jobId" type="xs:string" minOccurs="0" maxOccurs="unbounded"/>
		</xs:sequence>
	</xs:complexType>


</xs:schema>
