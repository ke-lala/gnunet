#!/usr/local/php5/bin/php

<?php

ini_set('display_errors', 1); 
error_reporting(E_ALL);

print_r(extractor_getkeywords("/usr/local/apache2/htdocs/index.html.de"));
print_r(extractor_getkeywords("/usr/local/apache2/htdocs/apache_pb.gif"));

?>

