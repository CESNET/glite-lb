#!/bin/bash
cmd1="$1";
cmd2="$2";
cmd3="$3";
cmd4="$4";
cmd5="$5";
cmd6="$6";
cmd7="$7";
cmd7="$8";

ant $cmd1 $cmd2 $cmd3 $cmd4 $cmd5;
cd ./src_c
make $cmd6 $cmd7 $cmd8;
