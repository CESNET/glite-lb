@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

\begin{tabularx}{\textwidth}{>{\bfseries}lX}
@@@{
for my $stat ($status->getTypesOrdered) {
	my $c = getTypeComment $status $stat;

        gen "$stat: \& $c \\\\ \n";
        }
@@@}
\end{tabularx}

\endinput

