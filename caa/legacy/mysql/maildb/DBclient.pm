package maildb;

use strict;
use MTA;  # sendmail or postfix
use Mail::Mailer;
use Sys::Hostname;

my ($dir)   = ($0 =~/(.*)\/[\w\.\-]+$/);
my $debug   = 0;
my $hostname= hostname();
my ($HLO, $IF);
my ($ID,$MD,$MN,$NOME,$RESP)=qw(user
				maildrop
				mailname 
				namefam
				responsible
				);

my %rate = (
	    $NOME     => 10, 
	    pers_numb => 12,
	    $ID       => 14, # E-mail
	    mail      => 16, # E-mail
	    phone     => 20,
	    phone_    => 21,
	    office    => 26,
	    address   => 32,
	    position  => 42,
	    group     => 44,
	    institut  => 43,
	    status    => 48, 
	    rating    => 49, 
	    cmt       => 53,
	    created_by=> 54,
	    changesd  => 60,
	    last_l    => 90,
	    expir     => 92,
	    edt       => 94,
	    changesl  => 96,
	    );

sub init {
  ($HLO, $IF) = @_;
  $sql::iam    = $$HLO{iam} || getlogin || (getpwuid($<))[0] || "Intruder";
  $maildb::iam = $sql::iam;
  $debug++    if $sql::debug;
  &sql::setSilentlyUpdated (sql::abbrFields('SKIP',
					    qw(
					       changeslog changesdate last altname
					       )));
  return (
	  \&subCheckValue,
	  \&subDaemonUpdate,
	  \&subMaintanence,
	  \&subSortFields,
	  \&subPrtField,
	    );
}


sub subMaintanence {
  # Create hashes to speedup sql program and avoid 'getpwnam' stack problems
  my (%SQLmdrops, %SQLnames, %SQLregi, $n, $key, %userHash);
  
  foreach my $i (&sql::get(undef,undef,$ID,$MD,$NOME)){
    if   ($n % 3 == 0) { $key = $i; $SQLregi{$key}++; }
    elsif($n % 3 == 1) { $SQLmdrops{$key} = $i; }
    elsif($n % 3 == 2) { $SQLnames{$key}  = $i; }
    $n++;
  }
  
  while (my ($login, $gcos) = (getpwent)[0,6] ) { 
    $userHash{$login} = $gcos;
  }
  while (my ($login,$uic,$vmsGcos) = &getvmsent() ){
    $userHash{$login} = $userHash{$login} || $vmsGcos;
  }
  
  &MTA::open;
  my %sendmail = &MTA::getAll;
  &MTA::close();

  # copy from sendmail to sql
  foreach my $key (keys %sendmail) {
    my ($login,$x) = split /:/,$key;

    if (lc $SQLmdrops{$login} ne lc $sendmail{$key}) {
      printf ("%-9s - no $MD field in SQL DB, but '%s' in sendmail\n",
	      $login,
	      $sendmail{$key});
      
      if ($SQLmdrops{$login}) { &sql::set ($login,
					   {$MD => lc $sendmail{$key}},
					   \&subCheckValue); }
      else { &sql::addSimple( {user=> $login,
			       $MD => lc $sendmail{$key}});}
    }

    # redirected accounts...
    next if $sendmail{$key} =~/\@/i;
    next if $sendmail{$sendmail{$key}.":$MD"} =~/\@/i;
    next if $sendmail{$sendmail{$sendmail{$key}.":$MD"}.":$MD"} =~/\@/i;
    next if $sendmail{$sendmail{$sendmail{$sendmail{$key}.":$MD"}.":$MD"}.":$MD"} =~/\@/i;
  
    # Either the drop or record should be a valid login name
    next if (getpwnam $login)[0];
    next if (getpwnam $sendmail{$key})[0];
    next if (getpwnam $sendmail{$sendmail{$key}.":$MD"})[0];

#    printf  "%-20s - %-25s - %-25s\n", $key,$sendmail{$key},$sendmail{$sendmail{$key}.":$MD"};
    
    printf ("%s / %s - not a valid user\n",$login,$sendmail{$key});
  }
  return;
  
  # Process all known users and compare them with the SQL database
  
  foreach my $login (sort keys %userHash) {
    my $gcos = $userHash{$login};
    
    next if &emu::warning(sprintf ("%-9s - user not in the database",$login),
			  !$SQLregi{$login});
    delete $SQLregi{$login};

    if (&emu::warning(sprintf ("%-9s - no firstname/surname in DB",$login),
		      !$SQLnames{$login})){
      my ($von,$f,$n) = fn2nf ($gcos);
      print "    imposing $n $von $f  ($gcos)\n"; 
      &sql::set   ($login,
		   { von   => $von,
		     $NOME => $f,
		     name  => $n },
		   \&subCheckValue);
    }
    
    
    if (0) {
      &emu::warning(sprintf ("%-9s - user not in sendmail DB",$login),
		    !$sendmail{"$login:$MD"});
      
      if (&emu::warning("$login\t$MD discrepancy (SQL takes precedence)\n" .
			"\tSM db: ".$sendmail{"$login:$MD"}."\n".
			"\tSQL:   ".$SQLmdrops{$login},
			$SQLmdrops{$login} &&
			$sendmail{"$login:$MD"} ne $SQLmdrops{$login})){
	&MTA::open;
	&MTA::put ($login, $MD, $SQLmdrops{$login});
	&MTA::close();
      }
    }
  }
  
  # Find the sendmail entries which do not correspond to SQL database
#  foreach my $key (sort keys %sendmail){
#    &emu::warning(sprintf ("%-9s - maildrop in sendmail DB, but not in SQL (info)",$mbox),
#		  !$SQLmdrops{$mbox} && !$SQLmdrops{$sendmail{$key}});
#	print "sendmail - $mbox\t", $sendmail{$key}," - ",$SQLmdrops{$mbox} , " \n";
#  }
  
  # Find SQL entries which do not corrspond to valid accounts
  foreach my $login (sort keys %SQLregi){
    my $err = &emu::warning(sprintf ("%-9s - not a legal user",$login),
			    $SQLnames{$login});
    foreach my $l (keys %SQLmdrops) { next if $login ne $SQLmdrops{$l};
				      print "    $l \n" if $err; 
				      delete $SQLmdrops{$l}; 
				    }
    delete $SQLmdrops{$login};
  }
  
  foreach my $login (keys %userHash) {
    foreach my $l (keys %SQLmdrops) { delete $SQLmdrops{$l} if $login eq $SQLmdrops{$l}; }
    delete $SQLmdrops{$login};
  }
  
  foreach my $login (sort keys %SQLmdrops) { print "??? orphalin entry '$login'\n"; }
}


sub subDaemonUpdate { # update values from the passwd files
  return unless &main::root;
  
  my ($record,  $pwd, $uid, $gid, $uic, $gcos, @rest) = @_;
  my ($d1,      $uic, $fn)               = getvmsnam  ($record) unless $uic;
  (   $d1,      $pwd, $uid, $gid, $gcos) =(getpwnam   ($record))[0,1,2,3,6] unless $gid;
  my $poolman                            = getannexnam($record);
  return unless $uid || $uic || $poolman;
    
  $gcos = $gcos || $fn;
  
  my %updates=(
#	       uid      => $uid,
#	       gid      => $gid,
#	       uic      => $uic,
	       altname  => join (',',ucw (&sql::get({$MD   => "$record",
						     $NOME => ""},       undef, $ID))),
#	       altacct  => join (',',sort &sql::get({$MD   => "$record"},
#						    {$NOME => "A-Za-z" },       $ID)),
#	       dependant=> join (',',sort &sql::get({$RESP => "$record"},undef, $ID)),
	       );
  $updates{'poolman'}  = $poolman if $poolman; # do not delete...
  
  unless (sql::getField ($record, $MN)){ # Recover 'mailname'
    (my $mn = $updates{altname}) =~s/,.*$//;
    $updates{$MN} =	$mn if $mn =~/\./;
  }
  
  
  my %unset; foreach (keys %updates) { $unset{$_} = '' unless $updates{$_}=~/\w/;}
  &sql::set   ($record,
	       \%unset,
	       \&subCheckValue,
	       'unset');
  &sql::set   ($record,
	       \%updates,
	       \&subCheckValue,
	       );
}


sub subSortFields {
  return sort byRating @_;
  sub byRating { my $ra = &rated($a);
		 my $rb = &rated($b);
		 return $rate{$ra} cmp $rate{$rb} if $ra && $rb; 
		 return -1      if $ra;
		 return +1      if $rb;
		 return $a cmp $b; }
  sub rated { my $i=shift; foreach (keys %rate){ return $_ if $i=~/^$_/; } } 
}


sub subPrtField {
  my ($ID, $field, $value, $hash) = @_;
  my $user = $$hash{$ID};    
  (my $fieldP = $field) =~ s/_/ /g;

  #print STDERR "$ID=$user $field, $value...\n";
  
  # non-directly printed fields
  return &postfix::getByValue($user) if $field =~/$MN/;
  return undef if $field =~ /^(timestamp|gid|alt.*|von|name|$MN|institut.*|m_.*|su_code)$/i;
  return undef if $field =~ /^(ortnamn|postnummer)/i; 
  
  # administrator fields
  return undef if $field =~ /^(address|changes|number|status|cmt|pers.number|position)/i
    && !&main::administrator;
  return undef if $field =~ /^(rating|changeslog|sukat|created_by|last_l|edt|su_code)/i
    && !&main::root;
  
  return ('name',
	  join(' ',
	       $$hash{name},
	       $$hash{von},
	       $$hash{namefam}))
    if $field eq $NOME;
  
  return ('name',
	  $value)
    if $field eq 'pers_number';

  return ('address',
	  join(' ',
	       $$hash{postnummer},$$hash{ortnamn}).
	  '~'.$$hash{address})
    if $field eq 'address';
  
  $value =~ s/(\~|\<).*$// unless &main::root;
  return ($fieldP,&subCheckFilterPhone($value))     if $field =~ /^(phone)$/i;
  return ($fieldP,$value)
    if $field =~ /^(phone|office|last_)/;

  return ('VMS  uic',$value)
    if $field eq 'uic';
  
  if ($field eq $ID) { $field='E-mail';
		       open F,$main::postfixDB;
		       my $adr = $main::MX{$$hash{institution}} || $main::MX{'default'};		       
		       while (<F>) {
			 chomp; my ($k,$v) = split /:\s+/;
			 return ('E-mail',"$k\@$adr")
			   if $v eq $user && $k=~/\./;
		       }
		       return "E-mail","$user\@$adr";
		     }
  if ($field eq $MD) {return ($field,&postfix::get($user)); }
  return ($fieldP, $value);
}


sub subCheckValue { # check value before writing it to the DB
  my ($record, $column, $newValue, $hash, $flag) = @_;  
  print "subCheckValue - ",sprintf("%-5s ",$flag),"$record:$column\t",sql::shortMess($newValue),"\n"  if $debug;
  
  return undef if emu::warning("You can\'t delete field '$column'\n",
			       ($flag   =~ /unset/) &&
			       ($column =~ /^(edt|name|von)/)
			       );
  
  return undef if emu::warning("You are not authorized to update the database, '".$maildb::iam,
			       !&main::administrator && !($maildb::iam eq $record));
  
  $newValue = checkUser ($hash, $flag)                       if $column eq $ID;
  if ($flag =~ /^set/) {
    $newValue = checkMaildrop  (lc $record, lc $newValue)  if $column eq $MD;
    $newValue = checkMailname  (lc $record, lc $newValue)  if $column eq $MN;
    $newValue = checkPhone     ($newValue)                 if $column eq 'phone';
    $newValue = checkHomephone ($newValue)                 if $column eq 'phone_home';
    $newValue = checkMobilphone($newValue)                 if $column eq 'phone_mobile';
    $newValue = checkOffice    ($newValue)                 if $column eq 'office';
    &emu::warning("Server refuses to set $record\'s $column to '".$_[2]."'",
		  $_[2] && !$newValue);
  } elsif  ($flag =~ /^unset/) {
    unsetMaildrop (lc $record) if $column eq $MD;
    unsetMailname (lc $record) if $column eq $MN;
  }
  return $newValue;
}

sub checkUser {
  my ($hash, $flag) = @_;
  my $user = $$hash{$ID};
  my $legalUser = getpwnam $user || getvmsnam($user) || getannexnam($user);
  
  return if emu::warning ("Can\'t delete active user '$user'",
			  ($flag =~ /delete/) && $legalUser);
  return if emu::warning ("'$user' is not a legal login name",
			  ($flag !~ /rem|del|add/) && !$legalUser);
  return $user;
} 

my $mailserver = "mailserver";
my $vmsSearch  = '(((van|hag)((z\d\d\b)|[a-hz]\b))|vms)(.' . $maildb::domain .')?';
sub checkMailname { 
    my ($user, $mname) = @_;

    my $drop = &postfix::get (lc $mname);
    return undef if &emu::warning ("$MN '$mname' already used by $drop",
				   $drop && (lc $drop ne lc $user));

    return undef if &emu::warning ("'$mname' is not a valid $MN",
				   !($mname =~ /^[\w\.\-]+$/));

    &postfix::put(lc $mname, lc $user);
    
    return lc $mname;
}


sub checkMaildrop { 
  my ($user, $mdrop) = @_;
  if ($mdrop !~ /\@/) {
      return unless (getpwnam $mdrop)[0];}
  &postfix::put ($user,$mdrop);
  return $mdrop;
}


sub unsetMailname {
    my ($user) = @_;
    my $mname = sql::getField ($user, $MN);
    unsetMaildrop ($mname);  # kill name in the sendmail DB
    &sql::delete ($mname);   # kill name in SQL db
}

sub unsetMaildrop {
  &postfix::del(@_);
}

sub checkPhone {
    my $phone = $_[0]; 
    my @phones;
    foreach my $ph (split /\,|$sql::appendSeparator/,$phone){
	push @phones, subCheckFilterPhone($ph,'16');   
    }
    return join ($sql::appendSeparator,@phones);
}

sub checkHomephone {
    my $homephone = $_[0]; 
    my @homephones;
    foreach my $ph (split /\,|$sql::appendSeparator/,$homephone){
	push @homephones, subCheckFilterPhone($ph);   
    }
    return join ($sql::appendSeparator,@homephones);
}

sub checkMobilphone {
    (my $phone = $_[0]) =~ s/\D+//g;
    $phone =~ s/^(46|0)/+46/;   
    $phone =~ s/^(\+\d\d)(\d\d\d)(\d\d\d)(\d\d\d)$/$1 $2 $3 $4/;   
    return $phone;
}

sub subCheckFilterPhone { # Filter out the Stockholm code
    (my $ph = $_[0]) =~ s/[\D\s]+//g;
    $ph =~ s/^(46)[78]// if $ph =~ /\d\d\d\d\d\d/; 
    $ph =~ s/^0[78]//; 
    $ph =~ s/^16(\d\d\d\d)$/$1/;   # cut 16 in front 
    if (my $extra = $_[1]) { $ph =~ s/^$extra//; } 
    
    my $a0 = int ($ph)     % 100; 
    my $a1 = int ($ph/100) % 100; 
    my $a2 = int ($ph/10000);
    $ph = sprintf ("%s %2.2d %2.2d", $a2, $a1, $a0)
	unless $a2 < 10;
    if (length($ph)>7) { $ph =~ s/^([34][345])/+$1 /; }
    return $ph;
}


sub checkOffice {
    my $office = $_[0];
#    undef $office if
#	int($office) < 4000 || int($office) > 5999 || "$office" ne $_[0];
    return $office;
}


sub ucw { my @reply = ();
	  foreach my $mname (@_) {
	    my @n; foreach (split /[\.\- ]+/,$mname) {push @n, ucfirst $_;}
	    push @reply, join('.',@n) if @n;
	  }
	  return @reply;
	}

%maildb::poolman = ();
my $nodial = 'nodial';

sub getannexnam {
  return undef;  # poor old poolman...
  unless (%maildb::poolman) {
    my $gf = "$dir/acp_userinfo";
    
    if (open FILE, $gf) {
      my ($login,@poolman, $ac);
      while(<FILE>){
	s/^\s+//;
	my ($f,$p) = split;  $p = $p || ''; $f = $f || '';
	if ($f eq 'user'){
	  $maildb::poolman{$login} = join('~',@poolman) || $nodial if $login;
	  ($login,$ac,@poolman) = ($p);
	}elsif ($p ne $nodial){
	  $ac  = $p               if $f eq 'accesscode';
	  push @poolman, "$ac ".checkHomephone($p) 
	    if $f eq 'phone_no';
	}
      }
      close FILE;
      $maildb::poolman{$login} = join('~',@poolman) || $nodial if $login;
      print "Read in  ".(keys %maildb::poolman)." POOLMAN users from $gf\n" if $debug>9;
    }else{
      &emu::warning ("$hostname - Cant open $gf: $!",1);
    }
    
    $gf = "$dir/acp_passwd";
    if (open FILE, $gf) { 
      while(<FILE>){
	s/#.*$//;
	my ($login,@rest) = split ':';	next   unless @rest;
	$maildb::poolman{$login} = $nodial  unless $maildb::poolman{$login};
      }
      close FILE;
      print "Read in ".(keys %maildb::poolman)." POOLMAN users from $gf\n" if $debug>9;
    }else{
      &emu::warning ("Cant open $gf: $!",1);
    }
  }
  return $maildb::poolman{$_[0]};
}



%maildb::sysuaf = ();
my  $sysuafF = "$dir/user.sysuaf";
sub getvmsnam { # returns array (login uic fullname)
  return;
  my $user = $_[0];
    unless (%maildb::sysuaf) {
	&emu::warning ("Cant open $sysuafF: $!",
		       !open(FILE, $sysuafF));
	while(<FILE>) {
	    my ($vmsLogin , $vmsUic) = split /\s+/,substr $_, 21;  next unless $vmsLogin;
	    next unless $vmsUic =~ /\[\d+,\d+\]/;

	    my  $vmsGcos = substr $_,0,21;   $vmsGcos =~ s/\s+$//;
	    &swedish2english ($vmsGcos);
	    $maildb::sysuaf{lc $vmsLogin} = join ':',lc $vmsLogin,$vmsUic,$vmsGcos||"Anonymous VMS User"
		unless $vmsLogin =~ /(\$|guest|test)/i;
	}
	close FILE;
	print "Read in ".(keys %maildb::sysuaf)." VMS users table $sysuafF\n" if $debug>9;
    }
    return  split /:/,$maildb::sysuaf{$user};
}

my $pointer;
sub getvmsent { getvmsnam();
		my $n    = $pointer++;
		$pointer = 0      if $pointer > (keys  %maildb::sysuaf);
		my $key  = (keys  %maildb::sysuaf)[$n];
		return split /:/,$maildb::sysuaf{$key};  
	    }


my @vons = ('de los', 'a', 'de', 'al', 'von', 'di', 'le', 'Mc', 'e');
my $vons = join '|', @vons;
sub fn2nf {
    my $fullname = $_[0];
    my ($prenom,$nom,$von);
    
    $fullname =~ s/(\w)\. (.*)$/$1 $2/;
    $fullname =~ s/[\,\(\.].*$//; # supress everytnig after ","
    my @a = split /\s+/, $fullname;
    if (@a>2) {
	unless (($prenom,$von,$nom) = ($fullname =~ /(.*)\s+($vons)\s+(.*)$/i)){
	    ($prenom,$nom) = (join (' ',@a[0..$#a-1]) ,
			      @a[$#a]);
	}
    }else{
        ($prenom,$nom) = @a;
        ($nom,$prenom) = @a if @a==1;
    }
    swedish2english ($nom);
    swedish2english ($prenom);
#    print "@_ -> $von | $nom | $prenom\n";# if @a>2;

    return ($von,
	    $nom,
	    $prenom
	    );
}

sub swedish2english {
    my $name = $_[0];
    my @a;
    foreach my $n (split /[\s_]+/, $name) { 
	my @m;
	foreach  (split /\-/, $n) { s/\}/å/g;    s/\{/ä/g;    s/\|/ö/g;   s/\~/ü/g;
				    s/\]/Å/g;    s/\[/Ä/g;    s/\\/Ö/g;   s/[\,\(].*$//;
				    push @m, ucfirst lc $_; }
	push @a, join '-',@m;
    }
    $_[0] = join ' ',@a; 
}

1;
