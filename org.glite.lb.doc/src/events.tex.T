@@@{
gen qq{
%  !! Automatically generated file. Do not edit.
%  !! Change the corresponding template file $ARGV
};
@@@}

Events for gLite world:

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
                gen "\\end{tabularx}\n";
                gen " Events for $flesh world:\n\n";
                gen "\\begin{tabularx}{\\textwidth}{l>{\\bfseries}lX}\n";
                $fleshno += 100;
                $eventno = $fleshno;
        }
        $eventno++;
        gen "$eventno. \& $e: \& $c \\\\ \n";
}
@@@}
\end{tabularx}

\endinput

