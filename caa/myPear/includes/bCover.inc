<?php
if (True){
  //  if (cnf_dev) myPear::MESSAGE('bCover_2017 bCover_2017 bCover_2017 bCover_2017 bCover_2017');
  locateAndInclude('bCover_2017');
  class bCover extends bCover_2017{
    function __construct($set=array(),$start=NULL,$end=NULL,$id=NULL){
      parent::__construct($set);
    }
  }
}else{
  //  if (cnf_dev) myPear::MESSAGE('bCover_2010 bCover_2010 bCover_2010 bCover_2010 bCover_2010');
  locateAndInclude('bCover_2010');
  class bCover extends bCover_2010{
    function __construct($set=array(),$start=NULL,$end=NULL,$id=NULL){
      parent::__construct($set,$start,$end,$id);
    }
  }
}
