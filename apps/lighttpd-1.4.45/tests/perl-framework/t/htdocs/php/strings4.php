<?php 
setlocale (LC_CTYPE, "C");
echo htmlspecialchars ("<>\"&��\n", ENT_COMPAT, "ISO-8859-1");
echo htmlentities ("<>\"&��\n", ENT_COMPAT, "ISO-8859-1");
?>
