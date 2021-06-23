#!/bin/bash

numfiles=`find . -name "core" | wc -l`
if [ $numfiles -ne 0 ]; then
    corefile=`find . -name "core"`
    rm -rf $corefile
    echo "Deleted the core file: $corefile"
    echo "Sending kill to `pidof httpd`"
    kill -9 `pidof httpd`
fi
