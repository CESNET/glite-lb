<?xml version="1.0"?>

@@@LANG: wsdl
<!--
@@@AUTO
-->

<service name="LoggingAndBookkeeping"
	ns="http://glite.org/wsdl/services/lb"
	prefix="lb">

	Keeping track of state of grid jobs.

	<types ns="http://glite.org/wsdl/types/lb" prefix="lbt">
		<flags name="jobFlags">
			Flags determining which fields of job status should be retrieved.
			Can be combined arbitrarily.
			<val name="CLASSADS">Include also long job descriptions (JDL).</val>
			<val name="CHILDREN">Return list of subjobs of a DAG.</val>
			<val name="CHILDSTAT">Return state of the subjobs, i.e. apply other flags recursively </val>
		</flags>

		<enum name="statName">
@@@{
	for my $stat ($status->getTypesOrdered) {
		my $u = uc $stat;
		my $c = getTypeComment $status $stat;
		gen qq{
!			<val name="$u">$c</val>
};
	}
@@@}
		</enum>

@@@{
	for my $n ($status->getAllFieldsOrdered) {
		my $f = selectField $status $n;
		if ($f->{codes}) {
			my $n = getName $f;
			$n = $1.ucfirst $2 while $n =~ /([[:alpha:]]*)_([[:alpha:]]*)/;
				gen qq{
!		<enum name="$n">
};
				for (@{$f->{codes}}) {
					my $uc = uc $_->{name};
					gen qq{
!			<val name="$uc">$_->{comment}</val>
};
				}
				gen qq{
!		</enum>
};
		}
	}
@@@}

<!-- FIXME: list which are arrays in fact are not portable and 
	will not work most likely in this way -->

		<struct name="jobStatus">
			Status of a job, possibly including subjobs.
			<elem name="state" type="statName">Status name.</elem>
@@@{
	for my $n (getAllFieldsOrdered $status) {
		selectField $status $n;
		my $f = getField $status;
		my $name = getName $f;
		$name = $1.ucfirst $2 while $name =~ /([[:alpha:]]*)_([[:alpha:]]*)/;
		my $type = $f->{type};
		my $list = 'no';

		if ($main::baseTypes{$type}) {
			$type = eval $main::types{wsdl}->{$main::baseTypes{$type}};
			$list = 'yes'
		}
		elsif ($f->{codes}) {
			$type = $name;
		}
		else {
			$type = getType $f;
		}

		my $comment = getComment $f;
# XXX: currently nothing is "optional" as we don't know from status.T 
		gen qq{
!			<elem name="$name" type="$type" list="$list" optional="no">$comment</elem>
};
	}
@@@}
		</struct>

		<struct name="tagValue">
			Value of a single user tag.
			<elem name="tag" type="xsd:string">Tag name</elem>
			<elem name="value" type="xsd:string" optional="yes">Tag value</elem>
		</struct>

		<struct name="timeval">
			UNIX time representation.
			<elem name="tvSec" type="xsd:long">Seconds since Jan 1 1970</elem>
			<elem name="tvUsec" type="xsd:long">Microseconds</elem>
		</struct>


		<struct name="genericFault" fault="yes">
			Generic SOAP fault, used to deliver any LB errors.
			<elem name="source" type="xsd:string">Source component (module) of the error.</elem>
			<elem name="code" type="xsd:int">Numeric error code.</elem>
			<elem name="text" type="xsd:string">Error text corresponding to the code.</elem>
			<elem name="description" type="xsd:string" optional="yes">Additional description of the error (e.g. filename)</elem>
			<elem name="reason" type="genericFault" optional="yes">Reason of the error, coming from lower levels.</elem>
		</struct>

		<simple name="string"/>
		<simple name="int"/>

	</types>



	<op name="JobStatus">
		Query state of a single job.
		<input name="jobid" type="string"> Id of the queried job. </input>
		<input name="flags" type="jobFlags"> Which data fields to retrieve. </input>
		<output name="stat" type="jobStatus"> Current state of the job. </output>
		<fault name="genericFault" type="genericFault"> Any error. </fault>
	</op>

</service>
