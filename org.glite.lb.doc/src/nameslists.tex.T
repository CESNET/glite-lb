@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

@@@{
        for my $n ($status->getAllFieldsOrdered) {
                my $f = selectField $status $n;
                if ($f->{codes}) {
                        my $n = lc (ucfirst getName $f);
                        printf("\\def\\${n}names{");
                        my $num = 0;
                        for (@{$f->{codes}}) {
                                my $uc = lc(uc $_->{name});
				next if ($uc =~ m/unused/);
				$uc =~ s/_/\\_/g;
                                $uc =~ tr/-/_/;
                                if ( $num != 0 ) { printf (", "); }
                                printf("\\texttt{$uc}");
                                $num++;
                        }
			printf("}\n\n");
                }
        }
@@@}

