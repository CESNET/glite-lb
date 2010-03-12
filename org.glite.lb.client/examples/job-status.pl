#!/usr/bin/perl
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

use SOAP::Lite;
use Data::Dumper;

# see user-jobs.pl for comments on this magic

$ENV{HTTPS_CA_DIR}='/etc/grid-security/certificates';
$ENV{HTTPS_VERSION}='3';

$proxy = $ENV{X509_USER_PROXY} ? $ENV{X509_USER_PROXY} : "/tmp/x509up_u$<";
$ENV{HTTPS_CERT_FILE}= $ENV{HTTPS_KEY_FILE} = $ENV{HTTPS_CA_FILE} = $proxy;

$job = shift or die "usage: $0 jobid\n";
$job =~ /(https:\/\/.*):([0-9]+)\/.*/ or die "$job: invalid jobid\n";
$host = $1; $port = $2;
$server = $host . ':' . ($port + 3);

$c = SOAP::Lite
	-> proxy($server)
	-> uri('http://glite.org/wsdl/services/lb');

service $c 'http://egee.cesnet.cz/cms/export/sites/egee/en/WSDL/HEAD/LB.wsdl';
ns $c 'http://glite.org/wsdl/elements/lb';

# we need another namespace explicitely here
$c->serializer->register_ns('http://glite.org/wsdl/types/lb','lbtype');

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

# construct the request, see SOAP::Data docs for details
$req = SOAP::Data->value(
		SOAP::Data->name(jobid=>$job),
# here the other namespace is used
		SOAP::Data->name(flags=>'')->type('lbtype:jobFlags')
	);

# call the WS op
$resp = JobStatus $c $req;
$body = $resp->body();

# print Dumper $resp->body();

# pick something reasonable from the response to print
unless ($fault) {
	@fields = qw/state jobtype owner networkServer doneCode exitCode childrenNum/;
	$stat = $resp->body()->{JobStatusResponse}->{stat};

	for (@fields) {
		print "$_: $stat->{$_}\n";
	}
}

