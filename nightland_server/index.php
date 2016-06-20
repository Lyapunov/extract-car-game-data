<?php
/*
 :) http://stackoverflow.com/questions/13847462/how-do-i-stop-a-php-script-running-on-the-background
 */

//ignore_user_abort(true);    // run script in background
error_reporting(~E_NOTICE); // 
set_time_limit (0);         // run script forever
//ob_implicit_flush(true);    // flush after each echo()

require("logger.php");
require("primitives/timer.php");
require("sockets.php");
require("world.php");

$address = "127.0.0.1";
$port = 4096;
$max_clients = 10;

$logger  = new Logger();
$timer   = new Timer(100);

$world   = new World( $logger );
$sockets = new Sockets( $logger, $address, $port, $max_clients );

if ( !$sockets->isAlive() ) {
   die( "Could not establish sockets.\n");
}

while (1) {
   $logger->log("Time is ".microtime(true), 1);
   $world->tick();
   $sockets->addToOutEventQueue( $world->getWorldEvents() );
   $sockets->readAndWriteSockets();
   $world->handleEvents( $sockets->getAndDestroyInEventQueue() );
   $timer->waitUntilNextTick();
}

microtime(true) * 1000;
?>
