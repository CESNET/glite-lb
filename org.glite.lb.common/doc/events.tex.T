@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

\begin{description}
@@@{
for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                $event->getTypes) {
        my $c = getTypeComment $event $e;
        gen "\\item\[$e:\] $c\n";
}
@@@}
\end{description}
