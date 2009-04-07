#!/usr/bin/perl

use Socket;
use Fcntl;

$notif = $ENV{NOTIFY_CMD} ? $ENV{NOTIFY_CMD} : './notify';

$notifid = shift or die "usage: $0 notifid\n";

socket SOCK,PF_INET,SOCK_STREAM,getprotobyname('tcp')	|| die "socket: $!\n";
bind SOCK,sockaddr_in(0,INADDR_ANY)			|| die "bind: $!\n";
listen SOCK,10;

$fd = fileno SOCK;

$flags = fcntl SOCK,F_GETFD,0;
fcntl SOCK,F_SETFD,$flags & ~FD_CLOEXEC;


$cmd = "$notif bind -s $fd $notifid"; 

system $cmd;

die "failed $cmd\n" if $? == 1;
$ret = $? >> 8;
die "$cmd: $ret\n" if $ret;

print "Bind ok. Press Enter ... "; <>;

$fdin = '';
vec($fdin,$fd,1) = 1;

while (1) {
	select($fdin,undef,undef,undef);
	print "got connection\n";

	$cmd = "$notif receive -s $fd -i 3";
	system $cmd;
	die "failed $cmd\n" if $? == 1;
	$ret = $? >> 8;
	die "$cmd: $ret\n" if $ret;

#	print "Receive OK. Press Enter ..."; <>;
}


