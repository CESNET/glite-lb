#!/usr/bin/perl

use SOAP::Lite;
use Data::Dumper;

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';

# necessary to get compatible SSL handshake with the Globus server implementation
$ENV{HTTPS_VERSION}='3';

# a trick to make proxy credentials work
$proxy = $ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";
$ENV{HTTPS_CERT_FILE}= $ENV{HTTPS_KEY_FILE} = $ENV{HTTPS_CA_FILE} = $proxy;

$server = shift or die "usage: $0 server\n";

# initialize the server handle, wsdl, and namespace
$c = SOAP::Lite
	-> proxy($server)
	-> uri('http://glite.org/wsdl/services/lb');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/LB.wsdl';
ns $c 'http://glite.org/wsdl/elements/lb';

# setup trivial fault handler
on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

# call the actual WS operation
$resp = UserJobs $c;

$body = $resp->body();

# too much rubbish
# print Dumper $body;

# follow the response structure and pick just jobids and states to print
unless ($fault) {
	$njobs = $#{$body->{UserJobsResponse}->{jobs}};
	for ($i = 0; $i <= $njobs; $i++) {
		print "$body->{UserJobsResponse}->{jobs}->[$i]\t",
		      $body->{UserJobsResponse}->{states} ? $body->{UserJobsResponse}->{states}->[$i]->{state} : '',
		      "\n";
	}
}

