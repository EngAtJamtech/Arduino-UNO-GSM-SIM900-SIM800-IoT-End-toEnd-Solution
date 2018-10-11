<!DOCTYPE html>
<html>
  <head>
    <title>Simple Dash board</title>
    <meta http-equiv="refresh" content="10" >
  </head>
  <body>

    <!-- Dispaly a simple table containing most recent 25 rows of data in time
    decending order. -->

    <!-- Table headers -->
 	<table width='600' border = "1" >
    <tr>
    <th width="35%"><h5>Log time</h5></th>
    <th width="25%"><h5>Device ID</h5></th>
    <th width="25%"><h5>Field 1</h5></th>
    </tr>


    <!-- Table rows -->
    <?php
    // Open data base.
    //-----------------
    $servername = "localhost";
    $username = "username";
    $password = "password";
    $dbname = "data_base_name";
    $conn = new mysqli($servername, $username, $password, $dbname);


    // Get data from table for device_type_1 .
    //--------------------------------
    $sql = "SELECT log_time, device_id, field1 FROM device_type_1
        ORDER BY log_time DESC LIMIT 25";
    $result = $conn->query($sql);


    // Output each row of table data.
    //------------------------------
    while($row = $result->fetch_assoc())
    {
        // Convert log time to human readable time.
        $ut = $row['log_time'];
        $time = date('Y-M-d H:i:s', $ut);

        // Output html table code with table values.
        echo "<tr>";
        echo "<td>" . $time . " </td>";
        echo "<td>" . $row['device_id'] . " </td>";
        echo "<td>" . $row['field1'] . " </td>";
        echo "</tr>";
    }
	?>

    <!-- End of table -->
	</table>
  </body>
</html>






