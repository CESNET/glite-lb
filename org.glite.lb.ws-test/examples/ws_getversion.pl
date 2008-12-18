#!/usr/bin/perl

# version known to support enough from document/literal to work
use SOAP::Lite 0.69;

use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$ENV{HTTPS_CA_FILE}= $ENV{HTTPS_CERT_FILE} = $ENV{HTTPS_KEY_FILE} =
	$ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";

die "usage: $0 https://server:port\n" unless $#ARGV == 0;

$srv = shift;


$c = SOAP::Lite
	-> uri('http://glite.org/wsdl/services/lb')
	-> proxy("$srv/lb") ;


# TODO: replace with $srv/lb/?wsdl once it works
service $c 'http://egee.cesnet.cz/en/WSDL/HEAD/LB.wsdl';

ns $c 'http://glite.org/wsdl/elements/lb';

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = GetVersion $c;

print $resp->result,"\n";

