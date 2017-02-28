/**
* --------------------------------------------------------------------
* Copyright (C) 2016 Chi Tai Dang.
*
* @author	Chi-Tai Dang
* @version	1.0
* @remarks
*
* This extension is free software; you can redistribute it and/or modify
* it under the terms of the Eclipse Public License v1.0.
* A copy of the license may be obtained at:
* http://www.eclipse.org/org/documents/epl-v10.html
* --------------------------------------------------------------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

bool g_verbose = false;


bool EstablishSocket ( int & _sock, const char * host, int port )
{
    int sock = ( int ) socket ( PF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 ) {
		printf ( "Failed to create socket.\n" );
        return false;
    }

    struct sockaddr_in  addr;
    memset ( &addr, 0, sizeof ( addr ) );

    addr.sin_family		    = PF_INET;
    addr.sin_port		    = htons ( port );

    inet_aton ( host, &addr.sin_addr );

    if ( g_verbose ) printf ( "Connecting ...\n" );

    int rc = ::connect ( sock, (struct sockaddr *) & addr, sizeof ( struct sockaddr_in ) );
    if ( rc < 0 ) {
		printf ( "Failed to connect [ %s : %i ].\n", host, port );
        ::shutdown ( sock, 2 );
        ::close ( sock );
        return false;
    }
    _sock = sock;
    return true;
}


int main ( int argc, char * argv [ ] )
{
	if ( argc < 4 ) {
		printf ( "socktool (Chi Tai Dang))\n" );
		printf ( "Usage: ./socktool IP Port Command\n" );
		printf ( "Usage: ./socktool IP Port Password Command\n" );
		printf ( "Usage: ./socktool IP Port Password Command 1\n\t1 = verbose output.\n" );
		return 1;
	}

	int port = atoi ( argv [ 2 ] );
	if ( port <= 0 ) {
		printf ( "Error: Invalid port %s [ %i ].\n", argv [ 2 ], port );
		return 1;
	}
	
    if ( argc >= 6 ) {
        if ( *(argv[5]) == '1' )
            g_verbose = true;
    }
    else if ( argc >= 5 ) {
        if ( *(argv[4]) == '1' )
            g_verbose = true;
    }

	int ret = 1, sock = -1;

	while ( EstablishSocket ( sock, argv [ 1 ], port ) )
	{
		int len = 0; int bytesSent;

        if ( argc == 4 || (g_verbose && argc == 5) ) {
            len = strlen ( argv[3] );

            int bytesSent = ( int ) ::send ( sock, argv[3], len, MSG_NOSIGNAL );
            if ( bytesSent != ( int ) len ) {
		        printf ( "Error: Failed to send [ %s ].\n", argv [ 3 ] );
                break;
            }
            bytesSent = ( int ) ::send ( sock, "\n", 1, MSG_NOSIGNAL );
            if ( bytesSent != 1 ) {
		        printf ( "Error: Failed to send newline.\n" );
                break;
            }
            if ( g_verbose ) printf ( "[ %s ]\n", argv[ 3 ] );
            ret = 0;
            break;
        }

        struct timeval tv;
        tv.tv_sec	= 10;
        tv.tv_usec	= 0;

        len = setsockopt ( sock, SOL_SOCKET, SO_RCVTIMEO, ( const char * ) &tv, sizeof ( tv ) );
        if ( len < 0 ) {
            printf ( "Error: Failed to set socket timeout.\n" );
        }

		char buffer [ 128 ] = { };
        len = ::recv ( sock, buffer, 128, 0 );

        if ( len > 0 ) {
            if ( strstr ( buffer, "Password:" ) ) {
                if ( g_verbose ) printf ( "\nread: [ %i ] [ %s ]\n", len, buffer );

                len = strlen ( argv[3] );
                bytesSent = ( int ) ::send ( sock, argv[3], len, MSG_NOSIGNAL );
                if ( bytesSent != ( int ) len ) {
                    printf ( "Error: Failed to send password.\n" );
                    break;
                }
                bytesSent = ( int ) ::send ( sock, "\n", 1, MSG_NOSIGNAL );
                if ( bytesSent != 1 ) {
                    printf ( "Error: Failed to send newline.\n" );
                    break;
                }
                break;
            }
        }

        len = strlen ( argv[4] );
        bytesSent = ( int ) ::send ( sock, argv[4], len, MSG_NOSIGNAL );
        if ( bytesSent != ( int ) len ) {
            printf ( "Error: Failed to send [ %s ].\n", argv [ 4 ] );
            break;
        }
        bytesSent = ( int ) ::send ( sock, "\n", 1, MSG_NOSIGNAL );
        if ( bytesSent != 1 ) {
            printf ( "Error: Failed to send newline.\n" );
            break;
        }
        ret = 0;
        break;
    }

    if ( sock >= 0 ) {
        ::shutdown ( sock, 2 );
        ::close ( sock );
    }

	return ret;
}