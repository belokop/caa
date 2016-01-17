#
# parse command line
#  -boolean -text txt txt ( not yet done ... -list k1=v1 k2=v2 )
#

sub getOptions ($) {
  my ($argDefs,$options) = @_;
  $options ||= \%main::opts;
  
  my $OK  = "For a while YES";
  # help & verbose & debug by default
  unless (%main::helpList){
    foreach my $l (keys(%$argDefs),qw(-config D v h HLO)) {
      $main::helpList{$l} = $$argDefs{$l} || '';
      $main::dejv{$l}++ if !$$argDefs{$l};
    }
  }
  
  my $opt = '-';
  foreach my $v (@ARGV) {
    if ($v =~ /^-/) {
      ($opt=$v) =~ s/^-//;
      unless (defined($main::helpList{$opt})) {
	print STDERR "??? unknown option '$v'\n";
	undef $OK;
      }
      $$options{$opt} ||= '1';
    }else{
      if  (!$$options{$opt})       {$$options{$opt} =  $v;}
      elsif($$options{$opt} eq '1'){$$options{$opt} =  $v;}
      else                           {$$options{$opt}.=" $v";}
      }
  }

  $$options{'v'}++   if $$options{'D'};
  # Print help message and exit,
  # both in case of wrong parameters and/or a help request
  if ($$options{'h'} || $argDefs eq '-h' || !$OK) {
    chomp (my $sc = `basename $0`);
    print STDERR "Usage: $sc [-option]\n";
    foreach my $l (sort keys %main::helpList){
      next if $main::dejv{$l}++;
      printf " %8s ","-$l";
      print "print configuration (if applicable)" if $l eq '-config';
      print "this message"        if $l eq 'h';
      print "verbose output"      if $l eq 'v';
      print "debug ..."           if $l eq 'D';
      print $main::helpList{$l};
      print "\n";
    }
    exit;
  }

   # Print all the input parameter for the debug
  if ($$options{'D'}) {
    print "getOptions:\n";
    foreach (keys %$options) { print "\t  -$_\t",$$options{$_},"\n"; }
  }
  return $OK;
}

1;
