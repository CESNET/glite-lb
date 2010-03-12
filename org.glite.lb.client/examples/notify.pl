#!/usr/bin/perl
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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


