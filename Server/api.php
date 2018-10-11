<?php


    // Open data base.
    //-----------------
    $servername = "localhost";
    $username = "username";
    $password = "password";
    $dbname = "data_base_name";
    $conn = new mysqli($servername, $username, $password, $dbname);


    // Collect posted data and get the current time.
    //------------------------------------------------
    $device_id =  $_POST['device_id'];
    $field1 =  $_POST['field1'];
    $log_time = time();


    // Create and execute SQL query string
    // to put data into table.
    //--------------------------------------
    $sql = "INSERT INTO device_type_1 VALUES ($log_time, \"$device_id\", $field1 )";
    $conn->query($sql);


    // Respond to client with the text 'OK'.
    echo "OK<br>";


?>