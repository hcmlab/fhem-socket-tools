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

#include <openssl/ssl.h>
#include <openssl/err.h>
#if (SSLEAY_VERSION_NUMBER >= 0x0907000L)
#	include <openssl/conf.h>
#endif

bool g_verbose = false;

void sslError ( const char * arg, int res, BIO * bio )
{
	printf ( "Error [ %i ] in [ %s ]\n", res, arg );

	ERR_print_errors_fp ( stderr );

	if ( bio ) { ERR_print_errors ( bio ); }
}


int CertVerifyier ( int preResult, X509_STORE_CTX * ctx )
{
	char buffer [ 1024 ];

	X509 * cert = X509_STORE_CTX_get_current_cert ( ctx );
	if ( cert ) {
		X509_NAME * issuer = X509_get_issuer_name ( cert );
		if ( issuer ) {
			X509_NAME_oneline ( issuer, buffer, 1024 );
			if ( g_verbose ) printf ( "CertVerifyier: Issuer [ %s ]\n", buffer );
		}

		X509_NAME * subject = X509_get_subject_name ( cert );
		if ( subject ) {
			X509_NAME_oneline ( subject, buffer, 1024 );
			if ( g_verbose ) printf ( "CertVerifyier: Subject [ %s ]\n", buffer );
		}
	}
	return 1; // Ignore verification process as we assume self-signed certificates ...
	//return preResult;
}


bool EstablishSSL ( SSL_CTX * &ctx, BIO * &web, BIO * &out, SSL * &ssl, const char * host, int port )
{
	long res = 1;
	char hostAndPort [ 1024 ];

	const SSL_METHOD* method = SSLv23_method ();
	if ( !method ) {
		sslError ( "SSLv23_method", 0, 0 ); return false;
	}
	
	ctx = SSL_CTX_new ( method );
	if ( !ctx ) {
		sslError ( "SSL_CTX_new", 0, 0 ); return false;
	}

	SSL_CTX_set_verify ( ctx, SSL_VERIFY_PEER, CertVerifyier );

	SSL_CTX_set_verify_depth ( ctx, 4 );

	const long flags = SSL_OP_NO_COMPRESSION;
	SSL_CTX_set_options ( ctx, flags );

	res = SSL_CTX_load_verify_locations ( ctx, "certs.pem", NULL );
	if ( res != 1 ) {
		if ( g_verbose ) sslError ( "SSL_CTX_load_verify_locations", res, 0 );
	}

	web = BIO_new_ssl_connect ( ctx );
	if ( !web ) {
		sslError ( "BIO_new_ssl_connect", 0, web ); return false;
	}

	int len = snprintf ( hostAndPort, 1024, "%s:%i", host, port );
	if ( len <= 0 ) {
		printf ( "Error building host and port.\n" );
		return false;
	}

	res = BIO_set_conn_hostname ( web, hostAndPort );
	if ( res != 1 ) {
		sslError ( "BIO_set_conn_hostname", res, web ); return false;
	}
	
	BIO_get_ssl ( web, &ssl );
	if ( !ssl ) {
		sslError ( "BIO_get_ssl", 0, web ); return false;
	}

	const char* const ciphersList = "ALL";
	res = SSL_set_cipher_list ( ssl, ciphersList );
	if ( res != 1 ) {
		sslError ( "SSL_set_cipher_list", res, 0 ); return false;
	}

	res = SSL_set_tlsext_host_name ( ssl, host );
	if ( res != 1 ) {
		sslError ( "SSL_set_tlsext_host_name", res, 0 ); return false;
	}

	if ( g_verbose ) printf ( "BIO_new_fp\n" );
	out = BIO_new_fp ( stdout, BIO_NOCLOSE );
	if ( !out ) {
		sslError ( "BIO_new_fp", 0, 0 ); return false;
	}

	if ( g_verbose ) printf ( "BIO_do_connect\n" );
	res = BIO_do_connect ( web );
	if ( res != 1 ) {
		sslError ( "BIO_do_connect", res, web ); return false;
	}

	if ( g_verbose ) printf ( "BIO_do_handshake\n" );
	res = BIO_do_handshake ( web );
	if ( res != 1 ) {
		sslError ( "BIO_do_handshake", res, web ); return false;
	}

	/* Make sure that a server cert is available */
	X509* cert = SSL_get_peer_certificate ( ssl );
	if ( cert ) { 
		X509_free ( cert ); 
	}
	else {
		sslError ( "SSL_get_peer_certificate", 0, 0 ); return false;
	}
	if ( NULL == cert ) sslError ( "SSL_get_peer_certificate", 0, 0 );

	res = SSL_get_verify_result ( ssl );
	if ( X509_V_OK != res ) {
		if ( g_verbose ) sslError ( "SSL_get_verify_result", 0, 0 );
	}
	return true;
}


void DisposeSSL ( SSL_CTX * &ctx, BIO * &web, BIO * &out )
{
	if ( out ) { BIO_free ( out ); out = 0; }

	if ( web != NULL ) { BIO_free_all ( web ); web = 0; }

	if ( NULL != ctx ) { SSL_CTX_free ( ctx ); ctx = 0; }
}


int main ( int argc, char * argv [ ] )
{
	if ( argc < 4 ) {
		printf ( "sockssl (Chi Tai Dang))\n" );
		printf ( "Usage: sockssl IP Port Command\n" );
		printf ( "Usage: sockssl IP Port Password Command\n" );
		printf ( "Usage: sockssl IP Port Password Command 1\n\t1 = verbose output.\n" );
		return 1;
	}

	int port = atoi ( argv [ 2 ] );
	if ( port <= 0 ) {
		printf ( "Error: Invalid port %s [ %i ]\n", argv [ 2 ], port );
		return 1;
	}

	if ( argc >= 6 ) {
		if ( *( argv [ 5 ] ) == '1' )
			g_verbose = true;
	}
	else if ( argc >= 5 ) {
		if ( *( argv [ 4 ] ) == '1' )
			g_verbose = true;
	}

	( void ) SSL_library_init ();

	SSL_load_error_strings ();

	OPENSSL_config ( NULL );

	int ret = 1;

	SSL_CTX * ctx = NULL;
	BIO *web = NULL, *out = NULL;
	SSL *ssl = NULL;

	if ( EstablishSSL ( ctx, web, out, ssl, argv [ 1 ], port ) )
	{
		int len = 0; int reads = 0;
		do
		{
			if ( argc == 4 || ( g_verbose && argc == 5 ) ) 
			{
				if ( BIO_puts ( web, argv [ 3 ] ) <= 0 ) {
					printf ( "Error writing [ %s ]\n", argv [ 3 ] ); break;
				}
				if ( BIO_puts ( web, "\n" ) <= 0 ) {
					printf ( "Error writing newline" ); break;
				}
				if ( g_verbose ) printf ( "[ %s ]\n", argv [ 3 ] );
				ret = 0;
				break;
			}

			if ( g_verbose ) printf ( "BIO_read\n" );
			char buffer [ 2096 ] = { };
			len = BIO_read ( web, buffer, sizeof ( buffer ) );
			reads++;

			if ( len > 0 ) {
				if ( g_verbose ) BIO_write ( out, buffer, len );

				if ( strstr ( buffer, "Password:" ) ) {
					if ( g_verbose ) printf ( "\nread: [ %i / %i ] [ %s ]\n", reads, len, buffer );
					if ( BIO_puts ( web, argv [ 3 ] ) <= 0 ) {
						printf ( "Error writing password\n" ); break;
					}
					if ( BIO_puts ( web, "\n" ) <= 0 ) {
						printf ( "Error writing newline" ); break;
					}

					if ( g_verbose ) printf ( "[ %s ]\n", argv[4] );

					if ( BIO_puts ( web, argv[4] ) <= 0 ) {
						printf ( "Error writing [ %s ]\n", argv[4] ); break;
					}
					if ( BIO_puts ( web, "\n" ) <= 0 ) {
						printf ( "Error writing newline" ); break;
					}
					ret = 0;
					break;
				}
				buffer [ len ] = 0;
				if ( g_verbose ) printf ( "read: [ %i / %i ] [ %s ]\n", reads, len, buffer );
			}
			else {
				if ( g_verbose ) printf ( "read: [ %i / %i ]\n", reads, len );
			}
		}
		while ( len > 0 || BIO_should_retry ( web ) );
	}

	DisposeSSL ( ctx, web, out );

	return ret;
}



