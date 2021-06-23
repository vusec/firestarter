#!/bin/bash

# Keep this as install/conf/nginx.conf content:

set -e 

: ${ONLY_REQUEST=0}

create_conf()
{
  cat << EOF > install/conf/nginx.conf
worker_processes  1;

events {
    worker_connections  1024;
}

http {

    server {
        listen 1080;
        server_name  localhost;

        location / {
            root   html;
            index  index.html index.htm;
        }

        location /loc1.html {
            add_after_body /loc2.html;
        }

        location /loc2.html {
            ssi on;
        }
    }
}
EOF

  cat << EOF2 > install/html/loc1.html
<!DOCTYPE html>
<html>
<head>
<title>Loc1 HTML</title>
</head>
</html>
EOF2

  cat << EOF3 > install/html/loc2.html
<!DOCTYPE html>
<html>
<head>
<title>Loc2 HTML</title>
</head>
<body>
<p>Hi from location 2 on <!--#echo var="host" --> !</p>
</body>
</html>
EOF3

}

start_server()
{
  ./serverctl start
  echo "[Before crash] nginx processes: `pidof nginx`"
  tail -10 install/logs/error.log || true
}

send_request()
{
  curl -i 127.0.0.1:1080/loc1.html || true
}

main()
{
  if [ ${ONLY_REQUEST} -ne 1 ]; then
    create_conf
    ./serverctl stop
    start_server
  fi
  send_request
  sleep 1
  tail -10 install/logs/error.log || true

  sleep 5
  echo
  echo "[After crash] nginx processes: `pidof nginx`"
  tail -10 install/logs/error.log || true
}

main
