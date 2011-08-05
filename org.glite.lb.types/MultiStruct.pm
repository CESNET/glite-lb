package gLite::LB::MultiStruct;

use gLite::LB::StructField;

sub new {
	shift;
	my $self = {};
	$self->{comments} = {}; # typ->comment
	$self->{fields} = {};	# typ->{ name->StructField, ... }
	$self->{order} = {};
	$self->{flesh} = {};
	$self->{terminal} = "";

	bless $self;
}

sub selectType {
	my $self = shift;
	my $type = shift;
	$self->{type} = $type;
	1;
}

sub addType {
	my $self = shift;
	my $type = shift;
	my $comment = shift;
	$self->selectType($type);
	$self->{comments}->{$type} = $comment;
	$self->{fields}->{$type} = {};
	1;
}

sub selectField {
	my $self = shift;
	$self->{field} = shift;
	$self->getField;
}

sub addField {
	my $self = shift;
	my $field = shift;

	
	die "unselected type" unless $self->{type};
	$self->{fields}->{$self->{type}}->{$field->{name}} = $field;
	$self->selectField($field->{name});

	my $f = $self->{field};
	my $t = $self->{type};
	push @{$field->{refs}},"$t:$f";
	1;
}

sub getField {
	my $self = shift;
	my $f = $self->{fields}->{$self->{type}}->{$self->{field}};
	return $f ? $f : $self->{fields}->{_common_}->{$self->{field}};
}

sub load {
	my $self = shift;
	my $fh = shift;
	local $_;
	my $flesh = 'common';

	while ($_ = <$fh>) {

		chomp;
		s/#.*$//;
		next if /^\s*$/;

		if (/^\@type\s+(\S+)\s*(.*$)$/) {
			$self->addType($1,$2);
			$self->{order}->{$1} = $.;
			$self->{flesh}->{$1} = $flesh;
			next;
		}

		if (/^\@flesh\s+(\S+)\s*(.*$)$/) {
			$flesh = $1;
			next;
		}

		if (/^\@terminal\s+(\S+)$/) {
			$self->{terminal} = $self->{terminal} . " " . $1;
			next;
		}

		s/^\s*//;
		my ($ftype,$fname,$comment) = split /\s+/,$_,3;
		if ($ftype eq '_code_') {
			my $f = $self->getField();
			addCode $f $fname,$comment;
		}
		elsif ($ftype eq '_alias_') {
			my $f = $self->getField();
			addAlias $f $fname,$comment;
		}
		elsif ($ftype eq '_special_') {
			my $f = $self->getField();
			addSpecial $f $fname;
		}
		elsif ($ftype eq '_null_') {
			my $f = $self->getField();
			setNull $f $fname;
		}
		elsif ($ftype eq '_optional_') {
			my $f = $self->getField();
			$f->{optional} = 1;
		}
		elsif ($ftype eq '_index_') {
			my $f = $self->getField();
			$f->{index} = 1;
		}
		elsif ($ftype eq '_ref_') {
			my ($rt,$rf) = split /:/,$fname;
			$self->addField(my $f = $self->{fields}->{$rt}->{$rf});
		}
		elsif ($ftype eq '_pad_') {
			my $f = $self->getField();
			$f->{pad} = $fname;
		}
		else {
			my $f = new gLite::LB::StructField $fname,$ftype,$comment,$.;
			$f->{ptype} = $self->{type};
			$self->addField($f);
		}
	}
}

sub getTypes {
	my $self = shift;
	my @out;
	local $_;

	for (keys %{$self->{fields}}) {
		push @out,$_ unless $_ eq '_common_';
	}
	@out;
}

sub getTypesOrdered {
	my $self = shift;
	my @names = getTypes $self;

	sort {
		my $oa = $self->{order}->{$a};
		my $ob = $self->{order}->{$b};
		$oa <=> $ob;
	} @names;
}

sub getTerminalStatusOrdered {
	my $self = shift;
	for my $stat ($self->getTypesOrdered) {
		if ($self->{terminal} =~ m/\b$stat\b/) { 
			push(@out, 1); }
		else { push(@out, 0); }
	}
	@out;
}

sub getTypeComment {
	my $self = shift;
	my $type = shift || $self->{type};
	$self->{comments}->{$type};
}

sub getFieldComment {
	my $self = shift;
	my $fname = shift;
	$self->{fields}->{$self->{type}}->{$fname}->{comment};
}

sub getFields {
	my $self = shift;
	keys %{$self->{fields}->{$self->{type}}};
}

sub getFieldsOrdered {
	my $self = shift;
	my @names = $self->getFields;
	sort {
		my $oa = $self->selectField($a)->{order};
		my $ob = $self->selectField($b)->{order};
		$oa <=> $ob;
	} @names;
}

sub getFieldOccurence {
	my $self = shift;
	my $fname = shift;
	my @out;
	local $_;

	for (keys %{$self->{fields}}) {
		push @out,$_ if $self->{fields}->{$_}->{$fname};
	}
	@out;
}

sub getAllFields {
	my $self = shift;
	my %out;
	local $_;

	for my $t (values %{$self->{fields}}) {
		$out{$_->{name}} = 1 for (values %$t);
	}
	keys %out;
}

sub getAllFieldsOrdered {
	my $self = shift;
	my @names = getAllFields $self;

	sort {
		my @occ = $self->getFieldOccurence($a);
		$self->selectType($occ[0]);
		my $oa = $self->selectField($a)->{order};
		@occ = $self->getFieldOccurence($b);
		$self->selectType($occ[0]);
		my $ob = $self->selectField($b)->{order};
		$oa <=> $ob;
	} @names;
}

1;
