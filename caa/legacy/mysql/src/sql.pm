package sql;

use Mysql;
use Text::Abbrev;
use strict;

sub opendb;
sub prtRecord;
sub prtHelp;
sub prtList;
sub abbrFields;
sub setSilentlyUpdated;
sub unset;
sub set;
sub get;
sub getField;
sub delete;
sub addSimple;
sub shortMess;
sub fqn;

$sql::appendSeparator = '~';
$sql::tab             = '<-';
$sql::linesMax = 5;   # Number of lines for repetetive entries (if $shortListing selected)
%sql::tables  = ();   # key = lc column  value = table (find from which table is the field)
$sql::debug = 0;
$ENV{"MYSQL_TCP_PORT"}=3306;
  
my (
    $DBH, $DATABASE,	
    $ID,
    $tieIt,
    );
my %tieDB = ();
my @silentlyUpdated = (); 
my $tieWhere  = 'where clause';
my $tieRecord = 'record name';

sub opendb {
   $DATABASE = $_[0] || return;
   return if $sql::openDatabases{$DATABASE}++;
   
   die "\n" if emu::warning ("Mysql server is not defined\n    Probably no config file",
			     $sql::server !~ /\w/);
   
   die "\n" if emu::warning ("Can\'t open $sql::server $DATABASE:\n    $!",
			     !($DBH=Mysql->connect($sql::server,
						   $DATABASE,
						   $DATABASE,
						   $DATABASE)));
   $sql::DBH = $DBH;      
   print "Open database ", $sql::server, "::", $DATABASE, "\n" if $sql::debug || $main::debug;
   
   ajustLength();
}


sub getID { my $id = @_ ? $sql::primaries{lc($_[0])} : $ID
		or uty::dieTrace ("Cant find ID for fields '@_'");
	    print "sql::getID: '",&shortMess("@_"),"' -> '$id'\n" if $sql::debug; 
 	    return $id; }

sub tie {
    my $record = $_[0];    $tieIt++;

    &sql::untie;

    my @data =  &sql::get({&sql::getID() => $record});
    
    if ($data[0] =~ /\w/) { $tieDB{$tieRecord} = $record; }
    else                  { undef $tieIt; }
}

sub untie {
    if (my $record = $tieDB{$tieRecord}){
	delete $tieDB{$tieWhere};
	delete $tieDB{$tieRecord};  
	&sql::set ($record, \%tieDB, undef, 'flash');
    } # elsif ($tieIt) { &uty::dieTrace ("Bordel in 'sql::untie''"); }
    %tieDB = ();
}


sub moveOld { # rename record
    my ($subCheckValue, $from, $to) = @_;

    die "*** Syntax error: -mv '$from' '$to'\n"
	if !$to || !&sql::getField($from, &sql::getID());

    &sql::tie   ($from);
    &sql::delete($from, $subCheckValue)   or return;
    
    my %buf = %tieDB;    %tieDB = (); undef $tieIt;
    delete $buf{$tieWhere};
    delete $buf{$tieRecord};  
    $buf{&sql::getID()} = $to;  
    
    &sql::set ($to, \%buf, undef, 'add');
}

sub move { # rename record
    my ($subCheckValue, $from, $to) = @_;

    my $field = &sql::getID();
    die "*** Syntax error: -mv '$from' '$to'\n"
	if !$to || !&sql::getField($from, $field) || &sql::getField($to, $field);

    &sql::untie;

    my $table = $sql::tables{lc $field};    $field = fqn($field);

    my $iam = $sql::iam || uc $0; while ($iam =~ /\//) { $iam =~ s/^.*\///; }
    my $query = "UPDATE $table SET $field='$to',".
	"cmt='Renamed \"$from\" ".$sql::time." by $iam' ".
	    "WHERE $field = '$from'";
    print "move:\t",shortMess ($query),"\n"   if $sql::debug;
    
    my $sth =  $DBH->query($query);
    &uty::dieTrace() if &emu::warning ($DBH->errmsg."\n    $query",
				       !$sth);
}


my %dejaVu;    
sub prtRecord { # print record(s) "vertically".
    my ($record,
	$searchStr,
	$shortListing,
	$subSortFields, 
	$subPrtField, 
	) = @_;      return 1 if $dejaVu{$record}++;
    
    my $ID = &sql::getID(); # Assuming last db open
    print "===============prtRecord $ID='$record' '$searchStr'\n" if $sql::debug;

    my ($sth, $linesPrinted, $prevLine);
    if ($searchStr) { 
#	return undef if emu::warning ("Syntax error ...",
#				      !&main::administrator);
	$searchStr =~ s/:/./g   if $searchStr =~ /:\d+:/; # MAC address
	my @like; foreach (grep (/\./,keys %sql::tables)){push @like,"$_ RLIKE '$searchStr'"; }
	my ($dummy, $table, $where) =  sql::prepareQuery (join(' OR ',@like),
							  '*');
	my $query = "SELECT * FROM $table $where"; print "prtRec: ",shortMess($query),"\n" if $sql::debug;
	$sth   = $DBH->query($query);
	return undef if emu::warning ($DBH->errmsg."    $query",
				      !$sth);
	
	@sql::maxlength =  $sth->maxlength;   
	@sql::name      =  $sth->name;
    } else {
	&sql::get ({fqn($ID) => $record});
    }

    while (1) {
	my %hash = ($searchStr
		    ? $sth->fetchhash
		    : %tieDB ); last unless %hash;
	
	$record = $hash{$ID};
	$sql::L = uty::max ($sql::L , uty::max (13 , length $record));

	my @fields = $subSortFields
	    ? &$subSortFields (@sql::name)
	    : sort @sql::name; 
	
	foreach my $field (@fields){
	    my $value = $hash{$field} or next; 
	    
	    next if ((defined $subPrtField && 
		      !(($field,$value) = &$subPrtField ($ID, $field, $value, \%hash))) ||
		     $field eq $ID);
	  	    
	    my $lineCounter;

	    foreach my $line (split /\s?$sql::appendSeparator\s?/,$value) {
		next if $line =~ /0000-00-00/; # not elegant, but cant supress default NULL for DATE
		foreach my $line2 (split/;\s?/,$line) {
		    # Sometime the date is 'in future' Suppress it
		    # (2000-12-xx, but where it comes from ???)
		    if ($line2 =~ /(\d\d\d\d-\d\d-\d\d)/) {
			next if !$searchStr && $1 gt $sql::time;
		    }
		    next if $line2=~/root/ && !&main::root($sql::iam);
		    next if $shortListing  && ++$lineCounter>$sql::linesMax;
		    next if $searchStr && !($hash{$ID} =~ /$searchStr/i ||
					    $field     =~ /$searchStr/i ||
					    $line2     =~ /$searchStr/i);
		    ($line2) = (substr($line2, 0,4).'-'.
				substr($line2, 4,2).'-'.
				substr($line2, 6,2).' '.
				substr($line2, 8,2).':'.
				substr($line2,10,2).':'.
				substr($line2,12,2) )
			if $field eq 'timestamp';
		    next if$field eq 'timestamp' && $shortListing;
		    my $l = sprintf
			"%-".$sql::L."s %-15s %s",
			$record,
			ucfirst $field,  
			$line2;
		    print &uty::extStr($prevLine, $l), "\n";   $prevLine = $l; ++$linesPrinted;
		} } }
	last unless $searchStr;
    }
    return  $linesPrinted;
}


sub prtList { 
  my ($records,       # List of records (all if empty)
      $specSep,       # fields separator 
      $claus,         # ref to the hash of clauses
      @fields) = @_;  # the fields list to print (in this order)
  return unless @fields;
  
  my $ID  = $sql::primaries{$fields[0]};
  my (@list, $ac);
  foreach (sort split /\s+/,$records) { push @list, "$ID='$_'"; }
  my $wc = join ' OR ', @list;
  
  foreach my $cl (keys %$claus) { # separate WHERE clause from others
    if ($cl =~ /where/i) { $wc .= ($wc?" AND ":"").$$claus{$cl} }
    else                 { $ac .= uc (" $cl ") .   $$claus{$cl} }
  }
  my ($fld, $table, $where) =  sql::prepareQuery ($wc,
						  $ID,
						  @fields);
  
  my $query = "SELECT $fld FROM  $table $where $ac"; print "prtLis:\t$query\n" if $sql::debug; 
  my $sth = $DBH->query($query);
  return 1 if emu::warning ($DBH->errmsg . "\n    ".$query,
			    !$sth);
  
  
  my @lng   = $sth->maxlength; # the actual maximum length for the selection
  my @colms = $sth->name;      # the actual names of the columns (might be case problem...)
  
  printHeaderLine:{
    @fields = @colms;  my @line;
    grep $_ = ucfirst lc $_ , @fields;
    foreach my $n (0..$#colms) { $lng[$n] = uty::max ($lng[$n] , length($fields[$n]));
				 $lng[$n] = uty::min ($lng[$n] , 40);
				 push @line, &prt ($fields[$n], $lng[$n], $specSep); };
    print "\n", join ($specSep, @line), "\n\n";
  }
  
  my $linesPrinted;
  while (my %hash = $sth->fetchhash){ 
    my $nonempty;
    foreach my $n (1..$#colms) {
      if ($hash{$colms[$n]} =~ /[a-zA-Z1-9]+/) {++$nonempty;
						last; }}
    next unless $nonempty;
    
    ++$linesPrinted;	my @line;
    foreach my $n (1..$#colms) { push @line,
				 &prt ($hash{$colms[$n]}, $lng[$n], $specSep); };
    print join($specSep, 
	       &prt($hash{$colms[0]},$lng[0],$specSep), 
	       @line), "\n"  if "@line" =~ /\S/;
  }
  sub prt { my ($v,$f,$sep) = @_;
	    undef $v if  $v =~ /^0000-00-00/; # not elegant, but cant supress default NULL for DATE
	    undef $v if !$v;
	    $v =~ s/^(($sql::appendSeparator|$sql::tab)([\s\-]+)?)//g;
	    $v =~ s/($sql::appendSeparator|$sql::tab).*$//;
	    return ($sep =~ /\s/  
		    ? sprintf "%${f}s", substr ($v, 0, $f)
		    : $v);
	  }
  sub prtAddress {
  }
  return  $linesPrinted;
}


sub prtHelp { # provide rudimentary help
  my ($wanted, $commands , $clauses, $ifs, $db, ) = @_;  return unless $wanted;
  
  my $lines;
  print "\nDescrpition of the $db control commands:\n\n";
  foreach (sort keys %$commands){++$lines;
				 printf "  -%-12s %s\n", $_, $$commands{$_}; }
  
  print "\nDatabase query clauses:\n\n";
  foreach (sort keys %$clauses) {++$lines;
				 printf "  -%-12s %s\n", uc $_, $$clauses{$_}; }  
  
  print "\nOptions:\n\n";
  foreach (sort keys %$ifs)     {++$lines;
				 printf "  -if %-8s %s\n",  $_, $$ifs{$_};}print "\n";
  return $lines;
}

sub abbrFields { # return array of non-abbriated column names
                 # fil=val fi f,f --> field=val field field,field   
    my @reply;
    my ($separ,$eq) = (',' , '=');
    my $skip = $_[0] eq 'SKIP';
    shift @_ if $skip;
    foreach my $F (split /\s+/,join(' ',@_)){
	if (my ($f,$v) = ($F =~ /(.*)$eq(.*)$/)){
	    my $field = uty::getAbbr (lc $f, keys %sql::tables);  
	    die "*** Unknown field '$f$eq$v'\n" if !$field && !$skip;
	    push @reply, "$field$eq$v"  if $field;
	}else{
	    my @ff;
	    foreach my $f (split /$separ/,$F){
		my $field = uty::getAbbr (lc $f, keys %sql::tables);
		die "*** Unknown field '$f$eq$v'\n" if !$field && !$skip;
		push @ff, $field  if $field;
	    }
	    push @reply, join ($separ, @ff);
	}
    }
    return @reply;
}

sub setSilentlyUpdated  { push @silentlyUpdated , @_; } # list of fields/columns

my %length  = (); # actual maximum column lengths
sub ajustLength { # Setup hashes, increase field length if needed via "alter table..."
  my ($column,$value) = @_;
  
  if (!%length or !$column){
    print "Enquire column lengths... \n" if $sql::debug; 
    foreach my $table ($DBH->listtables){
      next unless $table =~ /master/;
      $table = "master";
      print "\t$table\n" if $sql::debug;
      my $sth   = $DBH->listfields($table);
      my @col   = $sth->name;      # the names of the columns
      my @lng   = $sth->length;    # the posible maximum length 
      my @types = $sth->type;
      my @prikey= $sth->isprikey;
      my $fqtn = $DATABASE.".$table";
      
      undef $ID;
      foreach my $n (0..$#col) { $ID =  lc $col[$n]  if $prikey[$n];
				 # YB 2009-03-28 workaround...
				 $ID = 'user' if $DATABASE eq 'maildb'; 
			       }
      
      foreach my $n (0..$#col) { 
	if ($sql::tables{lc $col[$n]} && !($col[$n] =~ /timestamp/)){
	  print 
	    "*** Duplicated fields cant be accessed by this package...\n" ,
	    "*** Field '".$col[$n]."':\n" ,
	    "***   '$table'\n",
	    "***   '".$sql::tables{lc $col[$n]}."'\n";
	  next;
	}
	
	my $c = lc $col[$n];
	$sql::tables{"$fqtn.$c"} = $fqtn;
	$sql::tables{$c}  = $fqtn;
	$sql::types {$c}  = $types[$n];
	$sql::primaries{$fqtn}   = $ID;
	$sql::primaries{$c}      = $ID;
	$sql::primaries{fqn($c)} = $ID;
	$sql::primaries{"database:".$DATABASE} = $ID;
	$length{$c}  = $lng[$n];
	printf "\t\t%-12s %s\n", $c, $ID if $sql::debug>0;
      } }
    
    # setup ENUM (how to fetch that from Mysql.pm ??)
    my $sth = $DBH->query("DESC ".$sql::tables{&sql::getID()});
    while (my @a = $sth->fetchrow){
      next unless my ($enum) = ($a[1] =~ /^enum\(([\'\w\,\,]+)\)/);
      $enum =~ s/\,/\|/g;	    $enum =~ s/\'//g;	    $enum =~ s/^\||\|$//;
      $sql::enum{lc $a[0]} = lc $enum;
    }
  }
  return unless $column;

  my $ok = $Mysql::SYSVAR_TYPE || 253; ############ not defined in Mysql.pm
  return unless $sql::types {lc $column} eq $ok;
  
  my $l = length($value);
  return if $l <= $length{lc $column};
  $l += 16;
  my $table = $sql::tables{lc $column};
  my $query = "ALTER TABLE $table MODIFY  $column varchar($l)";
  my $sth   = $DBH->query($query) or die "$!\n";
  $length{lc $column} = $l;
}


sub getAnyField {    # OBSOLETE (but used yet?)
    my ($columnName, $columnValue, @requestedColumns) = @_;
    return get ({$columnName => $columnValue},
		undef,
		@requestedColumns);
}

sub get { 
    my ($eqH,      # 'equil' hash
	$lkH,      # 'like'  hash
	@requestedColumns) = @_;

    print "=========get '",&shortMess("@requestedColumns"), "'\n" if $sql::debug;

    my (@clause , @answer);
    foreach (keys %$eqH){print "\tWHERE $_ = ", $$eqH{$_}, "\n"    if $sql::debug;
			 push @clause , fqn($_) . "='"      .$$eqH{$_}."'"; }
    foreach (keys %$lkH){print "\tWHERE $_ like ", $$lkH{$_}, "\n" if $sql::debug;
			 push @clause , fqn($_) . " like '%".$$lkH{$_}."%'";}
    
    my ($fld,$table,$where) = sql::prepareQuery (join (' AND ',@clause),
						 @requestedColumns);
    $where .= ($where ? ' AND ' : 'WHERE ') . $sql::WHEREclause if $sql::WHEREclause;
    undef $sql::WHEREclause;

    if (!defined $tieIt || 
	(defined $tieIt && !%tieDB) || # get it for the first time
	(defined $tieIt && ($tieDB{$tieWhere} ne $where))) {

	my $query = "SELECT $fld FROM $table $where"; 	print "get:\t",shortMess($query),"\n"  if $sql::debug;
	my $sth   = $DBH->query($query);
	&uty::dieTrace() if &emu::error ($DBH->errmsg . "\n    " . $query,
					 !defined $sth);
	
	@sql::maxlength  =  $sth->maxlength;   
	eval {@sql::name =  $sth->name;};
	while (my %hash =  $sth->fetchhash){ 
	    foreach my $c (@sql::name) { push @answer, $hash{$c}; }
	}

	if (!%tieDB || !$tieIt) { # get it for prtRecord
	    %tieDB = ();	    %sql::LL = ();
	    $tieDB{$tieWhere} = $where;
	    foreach my $n (0..$#sql::name) { $tieDB{  lc $sql::name[$n]} = $answer[$n]; 
					     $sql::LL{lc $sql::name[$n]} = $sql::maxlength[$n];  }
	}
    }
    
    if ($tieIt && ($tieDB{$tieWhere} eq $where)) {
	@answer=(); @sql::maxlength=();
	while ($fld =~ /\./) { $fld =~ s/^\w+\.//; $fld =~ s/,\w+\./,/; }
	@sql::name  = split /,/, $fld;
	foreach my $n (0..$#sql::name) { push @answer,          $tieDB{lc $sql::name[$n]};
					 push @sql::maxlength,$sql::LL{lc $sql::name[$n]}; }
    }	
    print "   >>>\t'",shortMess("@answer"),"'\n"  if $sql::debug;
    return @answer;
}


sub getField { 
    my ($record,@columns) = @_;
    my $ID    = &sql::getID(@columns);
    my @reply = &get ({$ID => $record},
		      undef, 
		      @columns); 
    return (@columns > 1
	    ? @reply
	    : $reply[0]);
}


my ($s,$m,$h,$d,$M,$y,$wd) = localtime(); ++$M; $y += 1900;   
$sql::time = sprintf "%d-%2.2d-%2.2d %2.2d:%2.2d", $y,$M,$d,$h,$m;

sub set { # set fields, add a record. Note, 'unset' is the same as 'set'
    my ($record, $dataHash, $subCheckValue, $mode) = @_;   
    
    my $ID = &sql::getID(keys %$dataHash);
    my (%mode, @fields, @oldValues);
    &uty::dieTace() if emu::error("Unknown mode '$mode'",
				  ($mode = $mode||'set') !~ /^(set|unset|add|flash)/);
    
    if($mode=~/^set/ && !&getField($record, $ID)) { $mode = 'add'; }
    if($mode=~/^add/ &&  &getField($record, $ID)) { $mode = 'set'; }
    if($mode=~/^add/) { $$dataHash{$ID} = $record;
			$$dataHash{edt} = $sql::time if $sql::tables{edt};
			foreach (keys %$dataHash) { push @fields, $_;
						    push @oldValues, $$dataHash{$_}; }}
    else              { @fields    = keys %$dataHash;
		        @oldValues = getField ($record, @fields);  }
    print "=======$mode==set '",&shortMess("$ID=$record @fields"), "'\n" if $sql::debug;
    
    my ($table , @q , $errors, $messages, $logMessages);
    foreach my $n (0..$#fields) {
	my $field    = $fields[$n];
	my $newValue = $$dataHash{$field};
	&emu::warning("'$field' might be ".$sql::enum{$field},
		      $newValue &&  
		      !($newValue=uty::getAbbr(lc $newValue,split /\|/,$sql::enum{$field})))
	    if $sql::enum{$field};
	
	$newValue = &$subCheckValue ($record, $field,
				     $newValue,
				     $dataHash, $mode)
	    if $subCheckValue;
	
	++$errors if !$newValue && $$dataHash{$field};
	next      if lc $newValue eq lc $oldValues[$n]  && $mode !~ /add/;

	printf "DEBUG ---set: '%s' '%s'\n", $field,$newValue if  $main::ifs{'debug'};
	ajustLength ($field,$newValue);
	$table  = $table || $sql::tables{lc $field};
	push @q , "$field='$newValue'";

	&logChanges ($record, $field, $oldValues[$n], $newValue,
		     \$messages, \$logMessages)
	    unless $mode =~ /^flash/;
	
    }   return if $errors || !@q;
    
    &storeChanges (\@q, $record, $table, \$messages, \$logMessages);
    
     
    foreach (@q){ my ($k,$v) = /(\w+)=\'(.*)\'$/;
		  $tieDB{$k} = $v; }
    
    if (($mode=~/flash|add/) || !$tieIt) {
	my $query = ($mode=~/add/ ? "REPLACE" : "UPDATE") . " $table SET " . join(',',@q); 
	$query .=  " WHERE ".fqn(sql::getID(@fields))." = '$record'"      unless $mode=~/add/;
	print "set:\t",shortMess ($query),"\n"   if $sql::debug;

	my $sth =  $DBH->query($query);
	&uty::dieTrace() if &emu::warning ($DBH->errmsg."\n    $query",
					   !$sth);
    }
}


my $cl = 'changeslog';

sub logChanges {
    my ($record, $field, $oldValue, $newValue, $messages,$logMessages ) = @_; 
    return if grep /^$field$/i,(@silentlyUpdated,$cl);

    my ($o,$n) = ($oldValue,$newValue); $o=~s/[\W\s]+//g; $n=~s/[\W\s]+//g; return if $o eq $n;

    $$messages .= ">>> del $record\'s $field=".   shortMess($oldValue)."\n" if !$newValue && $oldValue;
    $$messages .= ">>> set $record\'s $field to ".shortMess($newValue)."\n" if  $newValue;
    
    $$logMessages .= ';' . join '|', $field, $oldValue, $newValue 
	if $oldValue && $oldValue ne '?'; 
}




sub storeChanges {  # sql::mailChanges flag
    my ($buffer, $record, $table, $messages, $logMessages ) = @_;

    print $$messages if defined $$messages;    

    return unless $$logMessages;
    return if $sql::tables{$cl} ne $table;

    my @txt;
    foreach (split /;/, $$logMessages){ 
	next unless /\w/;
	my ($field, $oldValue, $newValue) = split /\|/; 
	push @txt, ($newValue ? "Rep" : "Del") . " $field=$oldValue";
    }
    
    my $iam = $sql::iam || uc $0; while ($iam =~ /\//) { $iam =~ s/^.*\///; }
    my (%stack, @stack);
    if (0) { # merge all updates made at the same time
	my $oldV;
	foreach my $entry ($sql::time." by $iam: ".join(';',@txt),
			   split /~/, &getField ($record,$cl)){
	    my ($by,$act,$value) = ($entry=~/^(\S+\s\S+\sby\s\S+)\s(\S+)\s(.*)$/);
#	    print "($by\t$act\t$value)\n";
	    $stack{$by} .= "$act $value;" if $value =~ /\=/ && $value ne $oldV; 
	    $oldV = $value;
	}
	foreach (reverse sort keys %stack) { $stack{$_} =~ s/;+$//; 
					     push @stack , $_.' '.$stack{$_}; }
    }else{
	my $stamp = sprintf "%s by %-9s", $sql::time, "$iam:";
	grep $_= "$stamp $_", @txt;
	@stack = reverse sort @txt, split (/~/,&getField ($record,$cl));
    }

    $emu::Message .= $$logMessages  if $sql::mailChanges;
    
    push @$buffer, "$cl='".join ('~',@stack)."'" if @stack;
}


sub delete { # DELETE Record
    my ($record, $subCheckValue) = @_   or return;

    my $ID = &sql::getID();
    &getField ($record, $ID) 	or return undef;
    
    my ($errors, $table, );
    foreach my $f (keys %tieDB) {
	next if !$sql::tables{$f};
	$table = $sql::tables{$f}; 
	++$errors 
	    if $subCheckValue &&
		&emu::warning ("", $tieDB{$f} &&
			       ! &$subCheckValue($record, $f, $tieDB{$f}, \%tieDB, 'deleteR')); }
    return if $errors;

    undef $tieIt; # !!!!!!

    my $query = "DELETE FROM $table WHERE $ID='$record'"; print "delete:\t$query\n"   if $sql::debug;
    my $sth =  $DBH->query($query);
    die if emu::warning ($DBH->errmsg."\n    $query",
			 !$sth);
    &emu::warning ("Record '$record' is deleted\n***",1);

    # delete fields referencing the deleted record
    my @fields = grep (/\./,keys %sql::tables);
    my @q; foreach (@fields) { push @q, "($_ = '$record' AND $_ rlike '[a-z]')"; }

    $query = "SELECT * FROM $table WHERE ". join(' OR ',@q); print "delete:\t",shortMess($query),"\n"  if $sql::debug;
    $sth = $DBH->query($query);
    die if &emu::error ($DBH->errmsg . "\n    ".$query,
			!defined $sth);
    
    my @name        = $sth->name;
    while (my %hash = $sth->fetchhash){ 
	my %unsets; foreach my $c (@name) { $unsets{$c} = '' if $hash{$c} eq $record; }
	&sql::set ($hash{$ID}, \%unsets, $subCheckValue, 'unset');
    }
    return 1;
}

sub addSimple { 
    # of limited use, sql::set should be used instead.
    # This routine is there to avoid the recusion loop when 'set' shuld be  
    # called from "subCheckValue" (recursion does not work in perl?)
 
    my ($dataHash) = @_;   
    my ($table, @q);
    foreach my $key (keys %$dataHash) {     # timestamp
	$table = $sql::tables{$key};
	ajustLength ($key, $$dataHash{$key});
	push @q , "$key='".$$dataHash{$key}."'";
    }   
    my $query = "REPLACE $table SET ".join(',',@q); print "\nsql::addSimple\t$query\n" if $sql::debug;
    my $h = $DBH->query($query);
    die if &emu::error ( $DBH->errmsg . "\n    $query",
			 !$h);
    print  $DBH->errmsg;
}


sub unset { # delete field(s) contents by setting them to empty string
    my ($record, $subCheckValue, @fields) = @_;   return unless grep /\w/, @fields; # shit with empty flds
    my %hash; foreach (@fields) { $hash{$_} = ''; }
    &sql::set ($record,
	       \%hash,
	       \&subCheckValue,
	       );
}


sub prepareQuery {
    my ($whereClause, @fields) = @_; print "\npreSQL:\t$whereClause\n\t@fields\n" if $sql::debug;
    my $allF = !@fields || $fields[0] eq '*';
  
    my (@qf, %tables);
    @fields = grep (/\./,keys %sql::tables) if $allF;
    foreach my $f (@fields) { next unless $f =~ /\w/;
			      $tables{$sql::tables{lc $f}}++; push @qf, fqn($f); }
    
    # convert field names to fqn 
    $whereClause  =~ s/=/ = /g;
    while ($whereClause  =~ /\'\S+\s+\S+\'/) { $whereClause  =~ s/\'(\S+)\s+(\S+)\'/\'$1\#$2\'/g; }
    my @cl = split /\s+/, $whereClause;    undef $whereClause; my $n=0;
    while ($n<$#cl) { 
	my ($f,$oper,$v) = @cl[$n..$n+2];
	$tables{$sql::tables{lc $f}}++;
	$whereClause .= &fqn(lc $f).uc(" $oper ").$v;
	if (my ($and)=($cl[$n+3]=~/(and|or)/i)){ $whereClause .= uc " $and "; ++$n; }
	$n += 3;
    }
    $whereClause  =~ s/\#/ /g;

    my @cond;
    foreach my $n (1..(keys %tables)-1) {
	my $tab0=(keys %tables)[0];	my $key0=$sql::primaries{$tab0};
	my $tabN=(keys %tables)[$n];	my $keyN=$sql::primaries{$tabN};
	print "tab0=$tab0 \tkey0=$key0  ",&sql::getID(),"\n";
	print "tabN=$tabN \tkeyN=$keyN\n";
	my $p0 = $key0 eq $ID ? $ID : $sql::tables{$sql::primaries{$tab0}} . ".$ID";
	my $pN = $keyN eq $ID ? $ID : $sql::tables{$sql::primaries{$tabN}} . ".$ID";
	push @cond, "$p0 = $pN";
    }
    
    push @cond, $whereClause  if $whereClause;
    $whereClause = "WHERE " . join (' AND ', @cond)
	if @cond;
	
#    @qf = ('*')      if $allF;
    my ($f,$t,$c)=(
		   join(',',@qf),          # Fully qualified field names
		   join(',',keys %tables), # Fully qualified table names
		   $whereClause            # WHERE clause
		   );  
    print "   >>>\t",($_[1] eq '*'||@_ < 2 ? '*' : $f),"\n\t$t\n\t",shortMess($c),"\n" if $sql::debug;
    return ($f, $t, $c);
}


my $maxl=99;
sub shortMess { my $l = length $_[0];
		return ($l < $maxl
			? $_[0]
			: substr ($_[0],0,$maxl) . ' ...'); }

sub fqn { 
  &uty::dieTrace ("Unknown field '$_[0]'")
    unless $sql::tables{$_[0]};
  
  return ($_[0] =~ /\./
	  ? $_[0]
	  : $sql::tables{$_[0]}.".".$_[0]);
}




package emu;

$emu::Message = "";

sub warning { # print message
  my ($errorMessage, $errorCond, $silent) = @_;
  return undef unless $errorCond;
  if (!$silent || &main::root){
    print "<font color=red>" if $main::ifs{html};
    print 	"*** $errorMessage\n";
    print "</font>"          if $main::ifs{html};
  }
  return 1;
}

sub error { # print and store message
  my ($errorMessage,$errorCond) = @_;
  $emu::Message .= "$errorMessage\n"   if warning(@_);
  return $errorCond || 0;
}

sub mail {
  print 
    "\n\nDB messages:\n\n",
    $emu::Message,
    "\n"
      if $emu::Message;
}


package uty;

sub getAbbr{
  my($abbriviatedWord,@compleWords) = @_;
  return $abbriviatedWord                      unless "@compleWords" =~ /\w/;
  
  @compleWords = split /\s/,$compleWords[0]    if (@compleWords == 1);
  
  my ($word,$digits) = ($abbriviatedWord =~ /([a-zA-Z\_]+)([\s\d].*)?$/);
  
  %uty::hash = ();    Text::Abbrev::abbrev(*uty::hash,@compleWords);
  
  my $answer = $uty::hash{$abbriviatedWord} || $uty::hash{$word}.$digits || '';
#    print "\tuty::getAbbr  '$abbriviatedWord' -> '$answer'\n"  if $sql::debug>9;
    return  $answer;
}

$sql::localDomain = 'physto.se';

sub filter { # t3o3p18.telia.com -> telia.com  
  my $host = $_[0];
  if ($host =~ /\W/){
    $host =~ s/\:.*$//;
#    print $host,"\n";
    $host = (nslookup($host))[0] || $host
      if $host =~ /^\d+\.\d+\.\d+\.\d+/;
    $host =~ s/^t\w+\.telia(.*)?$/telia.com/i;
    $host =~ s/^ppp(-)?\d+\.students.*$/ppp.students.su/i;
    $host =~ s/^(s)?du\d+-\d+\.ppp\..*$/ppp.algonet.se/i;
    $host =~ s/asyn\w+\.modempo(.*)?/modempool.kth.se/;
    $host =~ s/\.nada\.k.*/.nada.kth.se/;
    $host =~ s/dialup[\d\-]+\.abc\.se/dialup.abc.se/;
    $host =~ s/poolman[\d\-]+\.student.*/poolman.students.su.se/;
    $host =~ s/^d[\d\-]+\.swipnet.*/d.swipnet.se/;
    $host =~ s/^md[\d\-a-f]+\.utfor.*/md.utfors.se/;
    $host =~ s/^(\w+)\.spole.*/$1.spole.gov/;
    $host =~ s/.*\.fis\.unic.*$/fis.unico.it/i;
    $host =~ s/.*\.cern.*$/CERN/i;
    $host =~ s/[a-z\-]+137.138..*/CERN/; # CERN DHCP
    $host =~ s/^pb-s-[\w\-]+.*/CERN/;    # CERN sockets for portables
    $host =~ s/([a-zA-Z]+)[\-\d]+[\d\-a-f]+\.(\w+)\.(.*)/$1.$2.$3/;
    $host =~ s/\.nordit.*/.nordita.dk/;
    $host =~ s/([a-z]+)\d+\./$1./;
    $host =~ s/(.*)?poolman(.*)?$/poolman/i;
    $host =~ s/ppp.*\.libe.*/ppp.annecy.libertysurf.fr/i;
#    print $host,"\n";
  }
  return $host;
}

sub extStr { # compare 2 strings and return the "non eq pat" of the second one
    my ($str1, $str2) = @_; 
    my ($l, $l1, $l2, $lspace, $s1, $s2, );
    
    $l = 0;
    while ($l < uty::min ($l1=length $str1, $l2=length $str2)){
	($s1=substr($str1,$l,1)) =~ /\s/;
	($s2=substr($str2,$l,1)) =~ /\s/;
	$lspace = $l if ($s1 =~ /\s/) &&
	                ($s2 =~ /\s/);
	last if  $s1 ne $s2; ++$l;
    }
    return ($lspace > 1
	    ? sprintf ("%".$lspace."s", ' ') . substr ($str2,$lspace,$l2-$lspace)
	    : $str2);
}    

sub dieTrace { print "\n*** Fatal error\n*** @_\n";
	       &traceq;
	       die "\n"; }

sub traceq { 
    my $n = 1;
    print "\n*** Calls stack:\n";
    while (my ($package, $file, $line, $sub, @rest) = caller($n++)) {
	printf " %-20s Called from %s : %3d \n", $sub, $file, $line;
    }
}

sub min { return (int $_[0] < int $_[1] ? int $_[0] : int $_[1]); }
sub max { return (int $_[0] > int $_[1] ? int $_[0] : int $_[1]); }



use Socket;
sub nslookup { # strips out local domain postfix if the latter is not in the input string  
    my ($name,$aliases,$addr,$length,@addrs, $nameL);
    eval { 
	($name,$aliases,$addr,$length,@addrs) = gethostbyaddr(inet_aton($_[0]), AF_INET) if $_[0] =~ /^\d/; 
	($name,$aliases,$addr,$length,@addrs) = gethostbyname($_[0])                     if $_[0] =~ /^\D/; 
	($nameL = $name) =~ s/.$sql::localDomain//;     };
    eval { @addrs = ((($_[0] =~ /$sql::localDomain/) ? $name : $nameL),
		     inet_ntoa ($addrs[0]),
		     grep (!/$sql::localDomain|^($nameL|$name)$/,split (/\s+/,$aliases)));   };
    printf "nslookup: %20s -> @addrs\n", $_[0]  if $sql::debug;
    return @addrs;
}
1;
