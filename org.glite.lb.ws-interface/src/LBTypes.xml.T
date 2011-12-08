<?xml version="1.0"?>

@@@LANG: wsdl
<!--
@@@AUTO
-->

<service name="LoggingAndBookkeeping"
	ns="http://glite.org/wsdl/services/lb"
	typePrefix="lbt">

	<version>CVS revision: <![CDATA[ $Header$ ]]></version>

	<types ns="http://glite.org/wsdl/types/lb">
		<flags name="jobFlags">
			Flags determining which fields of job status should be retrieved.
			Can be combined arbitrarily.
			<val name="CLASSADS">Include also long job descriptions (JDL).</val>
			<val name="CHILDREN">Return list of subjobs of a DAG.</val>
			<val name="CHILDSTAT">Return state of the subjobs, i.e. apply other flags recursively </val>
			<val name="CHILDHIST_FAST">Return a histogram of subjob states (fast computation method, certain subjobs will be in an "unknown" state)</val>
			<val name="CHILDHIST_THOROUGH">Return a full histogram of subjob states.</val>
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
			$n = $1.ucfirst $2 while $n =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
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

		<struct name="jobStatus">
			Status of a job, possibly including subjobs.
			<elem name="state" type="statName">Status name.</elem>
@@@{
	for my $n (getAllFieldsOrdered $status) {
		selectField $status $n;
		my $f = getField $status;
		my $name = getName $f;
		$name = $1.ucfirst $2 while $name =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
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
# XXX: if null value is specified, the field can be optional
		if ($name eq 'stateEnterTimes' || $name eq 'childrenHist') {
# XXX: stateEnterTimes and childrenHist are the only case of enum-indexed array
			gen qq{
!				<elem name="$name" type="${name}Item" list="yes" optional="yes">$comment</elem>
};
		}
		else {
# orig XXX: currently nothing is "optional" as we don't know from status.T 
# XXX: if null is defined, the field is optional
			my $opt = $f->{null} ? 'yes' : 'no';
			gen qq{
!				<elem name="$name" type="$type" list="$list" optional="$opt">$comment</elem>
};
		}
	}
@@@}
		</struct>

		<choice name="event">
@@@{
	for my $name (sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event) {
		my $comment = getTypeComment $event $name;
		$name = $1.ucfirst $2 while $name =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
		gen qq{
!			<elem name="$name" type="event$name">$comment</elem>
};
	}
@@@}
		</choice>

@@@{
	selectType $event '_common_';
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		if ($f->{codes}) {
			my $n = ucfirst getName $f;
			$n = $1.ucfirst $2 while $n =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
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
!
};
		}
	}

	for my $type (sort { $event->{order}->{$a} <=> $event->{order}->{$b} } getTypes $event) {
		my $tn = $type;
		$tn = $1.ucfirst $2 while $tn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;

		selectType $event $type;
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			if ($f->{codes}) {
				my $n = $tn . '_' . getName $f;
				$n = $1.ucfirst $2 while $n =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
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
!
};
			}
		}

		gen qq{
!			<struct name="event$tn">
};
		selectType $event '_common_';
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $ftn = $f->getType;

			$fn = $1.ucfirst $2 while $fn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
			$ftn = ucfirst $fn if ($f->{codes});
			$ftn = $1.ucfirst $2 while $ftn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;

			my $comment = getComment $f;
			my $opt = $f->{optional} ? 'yes' : 'no';
			gen qq{\t\t\t\t<elem name="$fn" type="$ftn" optional="$opt">$comment</elem>
};
		}

		selectType $event $type;
		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $ftn = $f->getType;
			my $type = $f->{type};
			my $list = 'no';

			if ($main::baseTypes{$type}) {
				$ftn = eval $main::types{wsdl}->{$main::baseTypes{$type}};
				$list = 'yes'
			}		

			$fn = $1.ucfirst $2 while $fn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;
			$ftn = $tn . '_' . $fn if ($f->{codes});
			$ftn = $1.ucfirst $2 while $ftn =~ /([[:alpha:]]*)_([[:alpha:]_]*)/;


			my $comment = getComment $f;
			my $opt = $f->{optional} ? 'yes' : 'no';
			gen qq{\t\t\t\t<elem name="$fn" type="$ftn" list="$list" optional="$opt">$comment</elem>
};
		}
		gen "</struct>\n\n";
	}
@@@}

		<enum name="eventSource">
			Possible sources of LB events.
			<val name="UserInterface"/>
			<val name="NetworkServer"/>
			<val name="WorkloadManager"/>
			<val name="BigHelper"/>
			<val name="JobSubmission"/>
			<val name="LogMonitor"/>
			<val name="LRMS"/>
			<val name="Application"/>
			<val name="LBServer"/>
			<val name="CREAMInterface"/>
			<val name="CREAMExecutor"/>
		</enum>

		<struct name="stateEnterTimesItem">
			<elem name="state" type="statName">The job state.</elem>
			<elem name="time" type="xsd:dateTime">When the state was entered last time.</elem>
		</struct>

		<struct name="childrenHistItem">
			<elem name="state" type="statName">The job state of the children.</elem>
			<elem name="count" type="xsd:int">How many children are in this state.</elem>
		</struct>

		<struct name="tagValue">
			<elem name="tag" type="xsd:string"/>
			<elem name="value" type="xsd:string" optional="yes"/>
		</struct>

		<struct name="timeval">
			<elem name="tvSec" type="xsd:long"/>
			<elem name="tvUsec" type="xsd:long"/>
		</struct>


		<struct name="genericFault" fault="yes">
			Generic SOAP fault, used to deliver any LB errors.
			<elem name="source" type="xsd:string" optional="yes">Source component (module) of the error.</elem>
			<elem name="code" type="xsd:int">Numeric error code.</elem>
			<elem name="text" type="xsd:string">Error text corresponding to the code.</elem>
			<elem name="description" type="xsd:string" optional="yes">Additional description of the error (e.g. filename)</elem>
			<elem name="reason" type="genericFault" optional="yes">Reason of the error, coming from lower levels.</elem>
		</struct>

		<struct name="queryConditions">
			An element of outer list of query conditions in job queries.
			It expresses possibly several conditions (records) on a single job attribute.
			These conditions are logically OR-ed.
			<elem name="attr" type="queryAttr"> The job attribute to which the query conditions apply. </elem>
			<elem name="tagName" type="xsd:string" optional="yes"> Name of the queried user tag if attr is USERTAG.</elem>
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
			<val name="LEVEL">Logging level (see "dglog.h"), see _edg_wll_QueryRec</val>
			<val name="RESUBMITTED">The job was resubmitted.</val>
			<val name="PARENT">JobId of the job parend (DAG).</val>
			<val name="EXITCODE">UNIX exit code of the job.</val>
			<val name="HOST">Where the event was generated.</val>
			<val name="SOURCE">Source component.</val>
			<val name="INSTANCE">Instance of the source component.</val>
			<val name="EVENTTYPE">Event type.</val>
			<val name="CHKPTTAG">Checkpoint tag.</val>
			<val name="JDLATTR">JDL Attribute.</val>
			<val name="STATEENTERTIME">Staus enter time.</val>
			<val name="LASTUPDATETIME">Last update time.</val>
			<val name="NETWORKSERVER">Network server aka RB aka WMproxy endpoint</val>
		</enum>

		<struct name="queryRecord">
			A single query condition.
			<elem name="op" type="queryOp"> Relational operator of the condition.</elem>
			<elem name="value1" type="queryRecValue" optional="yes"> Value to compare the attribute with.</elem>
			<elem name="value2" type="queryRecValue" optional="yes"> Another value to compare the attribute with (op = WITHIN only).</elem>
		</struct>

		<enum name="queryOp">
			Relational operator of query conditions. 
			<val name="EQUAL"> Attribute is equal to the specified value </val>
			<val name="LESS"> Attribute is less than the specified value or equal </val>
			<val name="GREATER"> Attribute is greater than the specified value or equal </val>
			<val name="WITHIN"> Attribute is withing a range (queryRecord.value2 must be specified) </val>
			<val name="UNEQUAL"> Attribute is not equal to the specified value.</val>
			<val name="CHANGED"> Attribute has changed its value from last check (notification conditions only).</val>
		</enum>
		
		<choice name="queryRecValue"> 
			A value to compare an attribute with in queries.
			Exactly one of the elements must be specified.
			<elem name="i" type="xsd:int">Integer.</elem>
			<elem name="c" type="xsd:string">String.</elem>
			<elem name="t" type="timeval">Timestamp.</elem>
		</choice>


	</types>

</service>
