<?xml version="1.0"?>

@@@LANG: wsdl
<!--
@@@AUTO
-->

<service name="LoggingAndBookkeeping"
	ns="http://glite.org/wsdl/services/lb"
	prefix="lb">

LB web service interface currently reflects the functionality of legacy
LB query API. 


	<types ns="http://glite.org/wsdl/types/lb" prefix="lbt">
		<flags name="jobFlags">
			Flags determining which fields of job status should be retrieved.
			<val name="CLASSADS">Include also long job descriptions (JDL).</val>
			<val name="CHILDREN">Return list of subjobs of a DAG.</val>
			<val name="CHILDSTAT">Return state of the subjobs, i.e. apply other flags recursively </val>
		</flags>

		<enum name="statName">
			Classification of job states.
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
!			Auxiliary type; values of the jobStatus.$n field.
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
			May be returned by any of the operations.
			<elem name="source" type="xsd:string">Source component (module) of the error.</elem>
			<elem name="code" type="xsd:int">Numeric error code.</elem>
			<elem name="text" type="xsd:string">Error text corresponding to the code.</elem>
			<elem name="description" type="xsd:string" optional="yes">Additional description of the error (e.g. filename)</elem>
			<elem name="reason" type="genericFault" optional="yes">Reason of the error, coming from lower levels.</elem>
		</struct>

		<simple name="string"/>
		<simple name="int"/>

		<struct name="queryConditions">
			An element of outer list of query conditions in job queries.
			It expresses possibly several conditions (records) on a single job attribute.
			These conditions are logically OR-ed.
			<elem name="attr" type="queryAttr"> The job attribute to which the query conditions apply. </elem>
			<elem name="tagName" type="string" optional="yes"> Name of the queried user tag if attr is USERTAG.</elem>
			<elem name="statName" type="statName" optional="yes"> Name of the job state to which "attr = TIME" condition refers.</elem>
			<elem name="record" type="queryRecord" list="yes"> The conditions. </elem>
		</struct>

		<enum name="queryAttr">
			Specification of a job attribute in query.
			<val name="JOBID">A concrete JobId</val>
			<val name="OWNER">Owner of the job (X509 certificate subject).</val>
			<val name="STATUS">Status of the job (see statName type).</val>
			<val name="LOCATION">Where the job is currently handled (hostname).</val>
			<val name="DESTINATION">Where the job is or was scheduled to be executed.</val>
			<val name="DONECODE">How the job terminated (see doneCode type)</val>
			<val name="USERTAG">Value of particular user tag. The tag name has to be specified in queryConditions.tagName.</val>
			<val name="TIME">When the job entered a particular state. The state has to be specified in queryCondition.statName.</val>
			<val name="RESUBMITTED">The job was resubmitted.</val>
			<val name="PARENT">JobId of the job parend (DAG).</val>
			<val name="EXITCODE">UNIX exit code of the job.</val>
		</enum>

		<struct name="queryRecord">
			A single query condition.
			<elem name="op" type="queryOp"> Relational operator of the condition.</elem>
			<elem name="value1" type="queryRecValue"> Value to compare the attribute with.</elem>
			<elem name="value2" type="queryRecValue" optional="yes"> Another value to compare the attribute with (op = WITHIN only).</elem>
		</struct>

		<enum name="queryOp">
			Relational operator of query conditions.
			<val name="EQUAL"> Attribute is equal to the specified value </val>
			<val name="LESS"> Attribute is less than the specified value or equal </val>
			<val name="GREATER"> Attribute is greater than the specified value or equal </val>
			<val name="WITHIN"> Attribute is withing a range (queryRecord.value2 must be specified) </val>
			<val name="UNEQUAL"> Attribute is not equal to the specified value.</val>
		</enum>

		<struct name="queryRecValue">
			A value to compare an attribute with in queries.
			Exactly one of the elements must be specified.
			<elem name="i" type="int">Integer.</elem>
			<elem name="c" type="string">String.</elem>
			<elem name="t" type="timeval">Timestamp.</elem>
		</struct>

	</types>



	<op name="JobStatus">
		Query state of a single job.
		<input name="jobid" type="string"> Id of the queried job. </input>
		<input name="flags" type="jobFlags"> Which data fields to retrieve. </input>
		<output name="stat" type="jobStatus"> Current state of the job. </output>
		<fault name="genericFault" type="genericFault"> Any error. </fault>
	</op>

	<op name="QueryJobs">
		Retrieve a list of jobs, including their states, based on
		query conditions.
		The conditions take the form of a list of lists.
		Elements of the inner lists refer to a single job attribute, the conditions are or-ed.
		Elements of the outer list may refer to different job attributes, they are and-ed.

		<input name="conditions" type="queryConditions" list="yes">The query conditions.</input>
		<input name="flags" type="jobFlags"> Which data fields to retrieve.</input>
		<output name="jobs" type="string" list="yes"> JobId's of jobs matching the query.</output>
		<output name="states" type="jobStatus" list="yes"> States of jobs matching the query.</output>
		<fault name="genericFault" type="genericFault"> Any error. </fault>
	</op>

	<op name="GetVersion">
		Return version of the service.
		<output name="version" type="string">Returned version.</output>
		<fault name="genericFault" type="genericFault"> Any error. </fault>
	</op>

	<op name="UserJobs">
		Simplified query, return all jobs of the authenticated user.
		<output name="jobs" type="string" list="yes"> JobId's of jobs matching the query.</output>
		<output name="states" type="jobStatus" list="yes"> States of jobs matching the query.</output>
		<fault name="genericFault" type="genericFault"> Any error. </fault>
	</op>

</service>
