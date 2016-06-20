<?php
class Logger
{
   public function __construct( ) {}

   public function log( $text, $level ) {
      print "(level$level) $text\n";
   }

   public function error( $text ) {
      print "[ERROR]".$text."\n";
   }

}
