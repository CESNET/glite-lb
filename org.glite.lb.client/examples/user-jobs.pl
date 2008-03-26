#!/usr/bin/perl

use SOAP::Lite;
use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$proxy = $ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";
$ENV{HTTPS_CERT_FILE}= $ENV{HTTPS_KEY_FILE} = $ENV{HTTPS_CA_FILE} = $proxy;

$server = shift or die "usage: $0 server\n";

$c = SOAP::Lite
	-> proxy($server) -> uri('http://glite.org/wsdl/services/lb');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/LB.wsdl';
ns $c 'http://glite.org/wsdl/elements/lb';

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = UserJobs $c;
$body = $resp->body();

# print Dumper $resp->body();

unless ($fault) {
	$njobs = $#{$body->{UserJobsResponse}->{jobs}};
	for ($i = 0; $i < $njobs; $i++) {
		print "$body->{UserJobsResponse}->{jobs}->[$i]\t",
		      $body->{UserJobsResponse}->{states} ? $body->{UserJobsResponse}->{states}->[$i]->{state} : '',
		      "\n";
	}
}

