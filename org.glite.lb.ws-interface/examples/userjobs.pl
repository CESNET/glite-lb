#!/usr/bin/perl

use SOAP::Lite;
use Data::Dumper;
use XML::LibXML;
use XML::LibXML::XPathContext;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$ENV{HTTPS_CA_FILE}= $ENV{HTTPS_CERT_FILE} = $ENV{HTTPS_KEY_FILE} = 
	$ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";

$proxy = shift or die "usage: $0 https://server.to.query:port/\n";

$c = SOAP::Lite -> proxy($proxy) -> uri('http://glite.org/wsdl/services/lb');

$c->service('http://egee.cesnet.cz/en/WSDL/3.1/LB.wsdl');
$c->ns('http://glite.org/wsdl/elements/lb');

$c->on_fault(sub { print Dumper($_[1]->fault); $fault = 1; });

$resp = $c->UserJobs();

print Dumper $resp;

