@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

\begin{description}
@@@{
for my $stat ($status->getTypesOrdered) {
	my $c = getTypeComment $status $stat;
        gen "\\item\[$stat:\] $c\n";
        }
@@@}
\end{description}
