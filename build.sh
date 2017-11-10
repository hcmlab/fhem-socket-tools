#!/bin/bash

if [ ! -e "/usr/include/openssl/ssl.h" ]; then
	echo 'It is required to install openssl development sources.'
	echo ' '
    echo -n "Press return to continue or Ctrl+C to abort: "
    read response
    
	sudo apt-get install libssl-dev libssl1.0.0
	[ $? != 0 ] && echo 'Failed to install libssl-dev libssl1.0.0 ...' && exit 1
fi

gcc -o socktool socktool.cpp
[ $? != 0 ] && echo 'Failed to build socktool ...' && exit 1

echo 'Build of socktool was successful.'

gcc -o sockssl sockssl.cpp -lssl -lcrypto
[ $? != 0 ] && echo 'Failed to build sockssl ...' && exit 1

echo 'Build of sockssl was successful.'