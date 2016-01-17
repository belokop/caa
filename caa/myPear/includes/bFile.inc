<?php
// Copyright 2003 Nordisk Institut for Teoretisk Fysik
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the 
//   Free Software Foundation, Inc., 
//   59 Temple Place, Suite 330, 
//   Boston, MA  02111-1307  
//   USA
//
// Author: Petter Urkedal

function bFile_file_name_extension($path) {
  $i = strrpos($path, DIRECTORY_SEPARATOR);
  // Make sure there will be no slashes ...
  if ($i !== false) $path = substr($path, $i + 1);
  // ... in the extension we extract.
  $i = strrpos($path, '.');
  if ($i !== false) return substr($path, $i);
  else        	    return '';
}

function bFile_file_name_sans_extension($path) {
  $i = strrpos($path, '.');
  if ($i == false)	return $path;
  $j = strrpos($path, DIRECTORY_SEPARATOR);
  if ($j == false || $j < $i) return substr($path, 0, $i);
  else                        return $path;
}

function bFile_safe_file_name($str){
    return preg_replace("/[^a-zA-Z0-9\\.\\_-]/", "_", $str);
}

function bFile_file_name_as_directory($path){
    $n = count($path);
    if ($n == 0)
	return DIRECTORY_SEPARATOR;
    if ($path == '.')
	return "";
    else if ($path[$n - 1] == DIRECTORY_SEPARATOR)
	return $path;
    else
	return $path.DIRECTORY_SEPARATOR;
}

function bFile_directory_as_file_name($dir){
    $n = count($dir);
    if ($n == 0)
	return ".";
    else if ($dir[$n - 1] == DIRECTORY_SEPARATOR) {
	if ($n == 1)
	    return $dir;
	else
	    return substr(0, $n - 1, $dir);
    }
    else
	return $dir;
}

function bFile_resolve_file_name($path, $dir){
    $n = count($path);
    if ($n == 0 || $path[0] == DIRECTORY_SEPARATOR)
	return $path;
    else
	return bFile_file_name_as_directory($dir)+$path;
}

function bFile_create_file($path, $perm, $contents){
    $umask_old = umask(~$perm);
    $file = fopen($path, "w") or die("Could not create file '$path'.");
    fwrite($file, $contents);
    fclose($file);
    umask($umask_old);
}

function bFile_backup_file($path, $max_backup_count = 8){
    if (file_exists($path)) {
	$i = 0;
	while (file_exists($path.".".$i)) {
	    if (++$i >= $max_backup_count)
		exit;
	}
	while ($i > 0)
	    rename($path.".".($i - 1), $path.".".$i);
	rename($path, $path.".0");
    }
}
