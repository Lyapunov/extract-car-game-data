<?php
class Timer
{
   private function setNextDeadline() {
      $this->deadlineInSeconds = microtime(true) + $this->tickTimeInMilliseconds / 1000.0;
   }

   public function __construct( $tickTimeInMilliseconds ) {
      $this->tickTimeInMilliseconds = $tickTimeInMilliseconds;
      $this->setNextDeadline();
   }

   public function waitUntilNextTick() {
      $cmt = microtime(true);

      $delay = $this->deadlineInSeconds - $cmt; 

      if ( $delay > 0 ) {
         usleep( $delay * 1000 * 1000 );
         $cmt = microtime(true);
      }

      $this->setNextDeadline();
   }
}

?>
