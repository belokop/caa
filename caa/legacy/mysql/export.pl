#! /usr/local/bin/perl
#
# Usage: export <package> [<destination direcory>]
#
use strict;
my $package = shift;

my $devArea = "/home/div/belokop/mysql/$package";
my $release = shift;
$release = "/usr/pkg/$package" unless $release;

&usage unless -d $devArea && $package;

opendir D,$devArea or &usage;
my $snap = 0;
foreach my $file (readdir D){
    next unless $file =~ /(p.|config)$/;
#   my ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime, @rest) = stat("$devArea/$file");
    my $t = (stat("$devArea/$file"))[9];
    $snap = $t  if int $t > int $snap;
}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =  localtime($snap);
$year += 1900; $mon++;
$snap = sprintf "%d-%02d-%02d", $year, $mon, $mday;

system ("mkdir -p $release/$snap ; rm -f $release/pro ; cd $release ; ln -s $snap pro") == 0 
   or die "*** Command aborted\n";
system "rsync -av --delete --copy-links --exclude AXI --exclude \"#*#\" --exclude \"*~\" $devArea/ $release/$snap/";

sub usage { die "Usage: $0 <package> [<destination directory>]\n"; }
