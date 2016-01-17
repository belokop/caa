package sendmail;

use strict;
use DB_File;
use Fcntl qw( :flock );

%sendmail::a = (); 
$sendmail::debug = 0;

my $DBlock;
my ($READ , $WRITE) = ('read' , 'write');

sub open {
    $sendmail::name = $_[0];
    $sendmail::a = tie (%sendmail::a, 'DB_File', $sendmail::name, O_RDWR|O_CREAT, 0644, $DB_BTREE);
    $sendmail::a = tie (%sendmail::a, 'DB_File', $sendmail::name, O_RDWR,         0644, $DB_BTREE) 
	unless $sendmail::a;
    unless ($sendmail::a){ 
      print "Cant open $sendmail::name: $!\n";
      return;
    }
    print "sendmail::open   $sendmail::name\n\t   Read ",scalar(keys %sendmail::a), " records\n\t   DB hosted on ",`hostname` 
      if $sendmail::debug;
    lockIt ($READ); 
}


sub delete    { my ($key) = (@_==1) ? @_ : join ':',@_ ;
		lockIt ($WRITE); 
		print "sendmail::delete  $key\n" if $sendmail::debug;
		delete $sendmail::a{$key}; }

sub getAll    { return %sendmail::a; }


sub get       { my ($key) = (@_==1) ? @_ : join ':',@_ ;
		lockIt ($READ); 
		return $sendmail::a{$key}; }


sub put       { my ($key, $value) = (@_==2) ? @_ : ($_[0].":".$_[1] , $_[2]);
		lockIt ($WRITE); 
		print "sendmail::put    $key $value\n" if $sendmail::debug;
		$sendmail::a{$key} = $value; }

sub close     { print "sendmail::close\n"  if $sendmail::debug;
		if ($sendmail::a) {
		    $sendmail::a->sync       if $DBlock eq $WRITE;
		    untie %sendmail::a;
		    undef %sendmail::a;
		    undef $sendmail::a; }
		unlock(); }

sub lockIt    { my $type = $_[0];    return  if $type eq $DBlock;
		
		return if $DBlock eq $WRITE; # read lock while WRITE is active
		
		unlock();
		$DBlock = $type;    print "sendmail::lock   $type\n" if $sendmail::debug;
		
		abExit (!open(DBLC,"+>$sendmail::name.lock"),
			"Cant open $sendmail::name.lock: $!");
		
		my ($retries);    my $MAXRETRIES = 10;
		while(!flock(DBLC , 
			     ($DBlock eq $WRITE ? LOCK_EX|LOCK_NB : LOCK_SH|LOCK_NB) )){
		    print "\t... Waiting for the database $DBlock access\n"; sleep 3;
		    abExit(++$retries > $MAXRETRIES,
			   "Failed to obtain the $DBlock lock (pid $$). Try again later");
		} }

sub unlock    { return      unless $DBlock;
		print "sendmail::unlock $DBlock\n" if $sendmail::debug;
		flock(DBLC,LOCK_UN);
		close(DBLC);
		undef $DBlock;   }

sub abExit    { my ($cond,$text) = @_;    return unless $cond;
		print "\n**** $text\n";
		die "\n";  }

1;







