#!/usr/bin/perl

use Socket;
use Fcntl;

$notif = $ENV{NOTIFY_CMD} ? $ENV{NOTIFY_CMD} : './notify';

socket SOCK,PF_INET,SOCK_STREAM,getprotobyname('tcp')	|| die "socket: $!\n";
bind SOCK,sockaddr_in(0,INADDR_ANY)			|| die "bind: $!\n";
listen SOCK,10;

$fd = fileno SOCK;

$flags = fcntl SOCK,F_GETFD,0;
fcntl SOCK,F_SETFD,$flags & ~FD_CLOEXEC;


$cmd = "$notif new -s $fd"; 

for (@ARGV) { $cmd .= " '$_'"; }
system $cmd;

$fdin = '';
vec($fdin,$fd,1) = 1;

while (1) {
	select($fdin,undef,undef,undef);
	print "got connection\n";

	system "$notif receive -s $fd -i 3";
}


