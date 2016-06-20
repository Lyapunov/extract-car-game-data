<?php

  $host    = "127.0.0.1";
  $port    = 4096;

  echo "<pre>\n";
  $socket = socket_create(AF_INET, SOCK_STREAM, 0) or die("Could not create socket\n");
  $result = socket_connect($socket, $host, $port) or die("Could not connect to server\n");  
  $result = socket_read ($socket, 1024) or die("Could not read server response\n");

  $i = 0;
  while ( $i < 5 ) {
     $result = socket_read ($socket, 1024);
     echo "Reply From Server  :".$result."\n";
     $i = $i + 1;
  }

  // close socket
  socket_close($socket);
  echo "</pre>\n";

?>
