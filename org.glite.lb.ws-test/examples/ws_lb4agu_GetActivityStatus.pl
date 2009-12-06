#!/usr/bin/perl

# version known to support enough from document/literal to work
use SOAP::Lite 0.69;

use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$ENV{HTTPS_CA_FILE}= $ENV{HTTPS_CERT_FILE} = $ENV{HTTPS_KEY_FILE} =
	$ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";

die "usage: $0 jobid\n" unless $#ARGV == 0;

$job = shift;

$job =~ ?https://([^:]*):([0-9]*)/(.*)?;
$port = $2 + 3;
$srv = "https://$1:$port/lb";


$c = SOAP::Lite
	-> uri('http://glite.org/wsdl/services/lb')
	-> proxy($srv) ;


# TODO: replace with $srv/lb/?wsdl once it works
service $c 'LB.wsdl';

$c->serializer->register_ns('http://glite.org/wsdl/services/lb4agu','lb4agu');
# ns $c 'http://glite.org/wsdl/elements/lb';

$req = SOAP::Data->value(
		SOAP::Data->name(id => $job),
	);

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = $c -> JobStatus($req);

print Dumper($resp->result),"\n";

