<?php
error_reporting(0);
print "
<BODY BGCOLOR=white>
<center>
<TABLE width='95%' BORDER='0' BGCOLOR='#F0F0FF' style='border-spacing:3px;border-collapse:separate;'>
";
$n=(int)@$_GET['n'];
$regexp = (isset($_GET['s']) ? $_GET['s'] : '.*');
$nColumns = ($n ? $n : 4); 
$maxSize = (int)(400/$nColumns);

$handle = opendir('.');
while ($file = readdir($handle)){
    if (preg_match("/$regexp/",$file)){
        if (is_file($file)) {
            if (!preg_match("/^(\.|index)|sh$|~$|CVS/",$file)) $allFiles[] = $file;
        }elseif(is_dir($file)){
            if (strpos($file,'.') !== 0)        $allDirs[] = $file;
        }
    }
 }
closedir($handle);

sort($allDirs);
sort($allFiles);

foreach($allDirs as $dir){
    echo "<tr><td colspan=5><a href='$dir?n=$n'>$dir</td></tr>";
}


foreach($allFiles as $file){
    if     (preg_match('/^bb-/',$file))                  $title = 'B 16x16';
    elseif (preg_match('/^i-user/',$file))               $title = 'User\'s-1';
    elseif (preg_match('/^(user)/',$file))               $title = 'User\'s-3';
    elseif (preg_match('/^(i-usr)/',$file))              $title = 'User\'s-2';
    elseif (preg_match('/^(star)/',$file))               $title = 'Stars';
    elseif (preg_match('/^(i-|a_|.*_off|.*_on)/',$file)) $title = 'Icons 16x16';
    elseif (preg_match('/^f-/',$file))                   $title = 'favicon\'s';
    elseif (preg_match('/^32[_-]/',$file))               $title = 'Icons 32x32';
    elseif (preg_match('/^b-/',$file))                   $title = 'Backgrounds';
    elseif (preg_match('/^0000/',$file))                 $title = 'B Icons 16x16 (unused)';
    else                                                 $title = 'Unsorted';
    $files_byType[$title][] = $file;
}

ksort($files_byType);

foreach($files_byType as $title=>$allFiles){
    sort($allFiles);

    $files = array();
    echo "</tr><tr><td colspan=10><h2>$title</h2></td></tr><tr>";
    
    $n_row = 0;
    $n_rows = (int)(count($allFiles)/$nColumns);
    if ($n_rows * $nColumns < count($allFiles)) $n_rows++;
    
    while(True){
        for($n=0; $n<$nColumns;$n++){
            $file = @$allFiles[$n_row + $n*$n_rows];
            if (is_file($file)) $files[$n][$n_row] = $file; 
        }
        if (++$n_row > $n_rows) break;
    }
    
    for($r=0; $r<$n_rows; $r++) {
        echo "<TR>";
        for($p=0; $p<$nColumns; $p++){
            $scale = 1;
            if (is_file($file=$files[$p][$r])){
                $size = getimagesize($file=$file);
                $width  = $size[0];
                $height = $size[1];
                if ($width > $maxSize || $height>$maxSize){
                    $scale = max($width,$height) / $maxSize;
                    $width = (int)($width / $scale);
                    $height= (int)($height/ $scale);
                }
            }else{
                $file = '';
            }
            $scale = ((int)($scale*100)) / 100;
            if ($scale == 1) $scale=''; else $scale = "<i>Scale $scale</i>";
            if (empty($file)){
                print "<td></td><td></td>";
            }else{            
                echo "<TD style='text-align:right;' VALIGN=top>";
                if (preg_match('/svg$/i',$file)) print "<embed src='$file' type='image/svg+xml' width='48' height='48'>";
                else                                      print "<IMG SRC='$file' width='$width' height='$height' BORDER=0>";
                echo "</td>";
                print "<td align=left valign=top>$file<br/>$scale</td>";
            }
        }
        echo "</TR>\n";
    }
}

?>
</TABLE>
</CENTER>
</BODY>
