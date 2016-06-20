<?php
require("primitives/event.php");

class Sockets
{
   public function __construct( $logger, $address, $port, $max_clients, $non_block = 1 ) {
      $this->logger      = $logger;

      $this->logger->log("Constructing Sockets, address = $address, port = $port, max_clients = $max_clients, non_block = $non_block", 2);
      $this->address     = $address;
      $this->port        = $port;
      $this->max_clients = $max_clients;

      $this->alive       = 0;
      $this->non_block   = $non_block; 

      if(!($this->sock = socket_create(AF_INET, SOCK_STREAM, 0)))
      {
          $errorcode = socket_last_error();
          $errormsg = socket_strerror($errorcode);

          $this->logger->error( "Couldn't create socket: [$errorcode] $errormsg" );
          return;
      }
       
      $this->logger->log("Socket created, $this->sock", 2);
       
      // Bind the source address
      if( !socket_bind($this->sock, $address , $port) )
      {
          $errorcode = socket_last_error();
          $errormsg = socket_strerror($errorcode);
           
          $this->logger->error("Could not bind socket : [$errorcode] $errormsg");
          return;
      }
       
      $this->logger->log("Socket bind OK", 2);
       
      if(!socket_listen ($this->sock, 10))
      {
          $errorcode = socket_last_error();
          $errormsg = socket_strerror($errorcode);
           
          $this->logger->error("Could not listen on socket : [$errorcode] $errormsg");
          return;
      }
       
      $this->logger->log("Socket listen OK", 2);
       
      if ( $this->non_block ) {
         if( !socket_set_nonblock($this->sock) )
         {
             $errorcode = socket_last_error();
             $errormsg = socket_strerror($errorcode);
              
             $this->logger->error("Could not set to nonblock : [$errorcode] $errormsg");
             return;
         }

         $this->logger->log("Set not-blocking flag on $this->sock", 2);
      }
      
      //array of client sockets
      $this->client_socks = array();
      for ($i = 0; $i < $this->max_clients; $i++)
      {
          $this->client_socks[$i] = null;
      }
       
      $this->alive = 1;
      $this->inEventQueue  = array();
      $this->outEventQueue = array();
   }

   public function isAlive() {
      return $this->alive;
   }

   public function getAndDestroyInEventQueue() {
      $retval = $this->inEventQueue;
      $this->inEventQueue = array();
      return $retval;
   }

   public function addToOutEventQueue( $events ) {
      $this->outEventQueue = array_merge( $this->outEventQueue, $events );
   }

   public function readAndWriteSockets() {
      if ( !$this->alive ) {
         return;
      }

      //prepare array of readable client sockets
      $local_client_read = array();
       
      //first socket is the master socket
      $local_client_read[0] = $this->sock;
      $j = 0;
       
      //now add the existing client sockets
      for ($i = 0; $i < $this->max_clients; $i++)
      {
         if($this->client_socks[$i] != null)
         {
            $j = $j + 1;
            $local_client_read[$j] = $this->client_socks[$i];
         }
      }
      $this->client_write = $local_client_read;
      $this->except       = $local_client_read;
       
      $this->logger->log( "Waiting for incoming connections... socket_select( ".join(',', $local_client_read). " )", 2);

      //now call select - blocking call
      if(socket_select($local_client_read , $write , $except , ( $this->non_block ? 0 : null )) === false)
      {
         $errorcode = socket_last_error();
         $errormsg = socket_strerror($errorcode);
       
         $this->logger->error("Could not listen on socket : [$errorcode] $errormsg");
         $this->alive = 0;
         return;
      }
       
      //if ready contains the master socket, then a new connection has come in
      if (in_array($this->sock, $local_client_read)) 
      {
         for ($i = 0; $i < $this->max_clients; $i++)
         {
            if ($this->client_socks[$i] == null) 
            {
               $this->client_socks[$i] = socket_accept($this->sock);
                
               //display information about the client who is connected
               if(socket_getpeername($this->client_socks[$i], $this->address, $this->port))
               {
                  $val = $this->client_socks[$i];
                  $this->logger->log( "Client $this->address : $this->port is now connected to us, socket: [$val].", 2);
                  array_push( $this->inEventQueue, new Event( Event::BORN, array( $i ) ) );
               }
                
               //Send Welcome message to client
               $message = "Bravo.\n";
               socket_write($this->client_socks[$i] , $message);
               break;
            }
         }
      }
    
      //check each client if they send any data
      for ($i = 0; $i < $this->max_clients; $i++)
      {
         if (in_array($this->client_socks[$i] , $local_client_read))
         {
            $input = socket_read($this->client_socks[$i] , 1024);
             
            if ($input == null) 
            {
               //zero length string meaning disconnected, remove and close the socket
               $val = $this->client_socks[$i];
               $this->logger->log("Closing socket [$val].", 2);
               socket_close($this->client_socks[$i]);
               unset($this->client_socks[$i]);
               $this->client_socks[$i] = null;
               array_push( $this->inEventQueue, new Event( Event::DIED, array( $i ) ) );
            } else {
               $n = trim($input);
               
               $output = "OK ... $input";
                
               $this->logger->log("Sending output to client", 2);
                
               //send response to client
               socket_write($this->client_socks[$i] , $output);
            }
         }
      }

      // send our messages to each client
      if ( count( $this->outEventQueue ) ) {
         $eventText = '';

         foreach( $this->outEventQueue as $event ) {
            $eventText = $eventText . $event->convertToString().";";
         }
         $this->logger->log("Output message is: $eventText.", 2);

         for ($i = 0; $i < $this->max_clients; $i++)
         {
            if($this->client_socks[$i] != null)
            {
               $output = socket_write($this->client_socks[$i], $eventText, 1024);
               if ($output == null) 
               {
                  $val = $this->client_socks[$i];
                  $this->logger->log("Closing socket [$val].", 2);
                  socket_close($this->client_socks[$i]);
                  unset($this->client_socks[$i]);
                  $this->client_socks[$i] = null;
                  array_push( $this->inEventQueue, new Event( Event::DIED, array( $i ) ) );
               }
            }
         }

         #destroy
         unset( $this->outEventQueue );
         $this->outEventQueue = array();
      }

   }
}


