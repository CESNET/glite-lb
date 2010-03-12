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
service $c 'http://egee.cesnet.cz/en/WSDL/HEAD/LB.wsdl';

$c->serializer->register_ns('http://glite.org/wsdl/types/lb','lbt');

ns $c 'http://glite.org/wsdl/elements/lb';

$req = SOAP::Data->value(
		SOAP::Data->name(jobid => $job),
		SOAP::Data->name(flags => \SOAP::Data->value(
				SOAP::Data->name(flag => 'CLASSADS')->type('lbt:jobFlagsValue'),
				SOAP::Data->name(flag => 'CHILDSTAT')->type('lbt:jobFlagsValue')
		)),
	);

on_fault $c sub { print Dumper($_[1]->fault); $fault = 1; };

$resp = $c -> JobStatus($req);

print Dumper($resp->result),"\n";

