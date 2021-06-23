#!/bin/bash

# Reference for bug: https://redmine.lighttpd.net/issues/2780?tab=notes
#
# When other requests mixed with webdav requests on the same socket via keep-alive connection,
# lighttpd 1.4.44 server crashes.
# 
# Bug Description:
# "mod_webdav crash with keep-alive
#
# mod_webdav allocated its own handler context in lighttpd 1.4.43, and cleaned it up in connection_reset hook, but did not properly reset con->plugin_ctx[p->id] = NULL. lighttpd 1.4.44 moved the plugin cleanup check from the connection_reset hook to connection_close hook, leading to possibility that mod_webdav handler_ctx would be cleaned up multiple times if there were other keep-alive requests (not handled by mod_webdav) on the same socket connection, or simply an HTTP/1.1 without Connection: close.

set -e 

: ${ONLY_REQUEST=0}

create_dav_file()
{
cat << EOF > tests/docroot/www/dav/dav.html
<!DOCTYPE html>
<html>
<head>
<title>WebDAV html file</title>
</head>
</html>
EOF
}

start_server()
{
  ./serverctl start
  echo "[Before crash] lighttpd processes: `pidof lighttpd`"
}

send_request()
{
curl -v http://127.0.0.1:1080/index.html -X MOVE --header 'Destination:http://127.0.0.1:1080/dav/dav.html' 'http://127.0.0.1:1080/dav/dav.html' http://127.0.0.1:1080/index.html
}

main()
{
  if [ ${ONLY_REQUEST} -ne 1 ]; then
    create_dav_file
    ./serverctl stop
    start_server
  fi
  send_request
  sleep 1
  echo "Lighttpd servers after sending request:"
  pgrep lighttpd

  echo "[After crash] lighttpd process: `pidof lighttpd`"
  if [ -f /tmp/rcvry_action_dump.txt ]; then
    cat /tmp/rcvry_action_dump.txt
  fi
}

main
