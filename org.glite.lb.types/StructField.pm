package StructField;

$lang = 'C';
1;

sub new {
	shift;
	my $self = {};
	$self->{name} = shift;
	$self->{type} = shift;
	$self->{comment} = shift;
	$self->{order} = shift;
	$self->{null} = $main::DefaultNullValue{$self->{type}};
	bless $self;
}

sub addCode {
	my $self = shift;
	my $code = shift;
	my $comment = shift;
	push @{$self->{codes}},{name=>$code,comment=>$comment};
	1;
}

sub addSpecial {
	my $self = shift;
	my $special = shift;
	$self->{special} = $special;
	1;
}

sub addAlias {
	my $self = shift;
	my $name = shift;
	my $lang = shift;
	$self->{aliases}->{$lang} = $name;
	1;
}

sub hasAlias {
	my $self = shift;
	my $lang = shift;
	return $self->{aliases}->{$lang} ? 1 : 0;
}

sub getName {
	my $self = shift;
	my $lang = shift || $lang;
	$self->{aliases}->{$lang} || $self->{name};
#	return $self->{aliases}->{$lang} ? $self->{aliases}->{$lang} : $self->{name};
}

sub getComment {
	my $self = shift;
	$self->{comment};
}

sub getDefaultNullValue {
	my $self = shift;
	$self->{null};
}

sub toString {
	my $self = shift;
	my $src = shift;
	my $dst = shift;

	eval $main::toString{$lang}->{$self->{type}};
}

sub fromString {
	my $self = shift;
	my $src = shift;
	my $dst = shift;

	eval $main::fromString{$lang}->{$self->{type}};
}

sub isNULL {
	my $self = shift;
	my $a = shift;
	my $b = $self->{null};

	eval $main::compare{$lang}->{$self->{type}};
}

sub isnotNULL {
	my $self = shift;
	my $src = shift;

	'!('.$self->isNULL($src).')';
}

sub compare {
	my $self = shift;
	my $a = shift;
	my $b = shift;
	eval $main::compare{$lang}->{$self->{type}};
}

sub toFormatString {
	my $self = shift;

	eval $main::toFormatString{$lang}->{$self->{type}};
}

sub setNull {
	my $self = shift;
	$self->{null} = shift;
}

sub getType {
	my $self = shift;

	eval $main::types{$lang}->{$self->{type}};
}
