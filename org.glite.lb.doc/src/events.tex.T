@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

\begin{tabularx}{\textwidth}{l>{\bfseries}lX}
@@@{
my $flesh = 'gLite';    #XXX
my $fleshno = 0;
my $eventno = $fleshno;
for my $e (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
                $event->getTypes) {
        my $u = uc $e;
        my $c = getTypeComment $event $e;

        if ($flesh ne $event->{flesh}->{$e}) {
                $flesh = $event->{flesh}->{$e};
                gen "% new flesh $flesh \n";
                $fleshno += 100;
                $eventno = $fleshno;
        }
        $eventno++;
        gen "$eventno. \& $e: \& $c \\\\ \n";
}
@@@}
\end{tabularx}

\endinput

