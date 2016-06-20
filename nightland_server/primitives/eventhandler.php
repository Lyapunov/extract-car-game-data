<?php

require( "event.php" );

class EventHandler
{
   public function __construct() {
      $this->handlers    = array();
   }

   public function addCallback( $event_type, $fn ) {
      $this->handlers[ $event_type ] = $fn;
   }
 
   public function handleEvent( $event ) {
      if ( array_key_exists( $event->getEventType(), $this->handlers ) ) {
         $handler_pair = $this->handlers[ $event->getEventType() ];
         $obj      = $handler_pair[0];  
         $funcname = $handler_pair[1];
         $obj->$funcname( $event->getArgArray() );
      }
   }

  public function handleEvents( $events ) {
     foreach ( $events as $event ) {
        $this->handleEvent( $event );
     }
  }
}
