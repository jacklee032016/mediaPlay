
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	WSADATA wsaData;
	int iResult;

	DWORD dwError;

	struct hostent *remoteHost;
	char *host_name;
	struct in_addr addr;

	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s ipv4 address\n", argv[0]);
		printf("  to return the host\n");
		printf("       %s 127.0.0.1\n", argv[0]);
		return 1;
	}
	
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	host_name = argv[1];

	// If the user input is an alpha name for the host, use gethostbyname()
	// If not, get host by addr (assume IPv4)
	if (isalpha(host_name[0])) {        /* host address is a name */
		printf("Calling gethostbyname with %s\n", host_name);
		remoteHost = gethostbyname(host_name);
	}
	else
	{
		printf("Calling gethostbyaddr with %s\n", host_name);
		addr.s_addr = inet_addr(host_name);
		if (addr.s_addr == INADDR_NONE) {
			printf("The IPv4 address entered must be a legal address\n");
			return 1;
		}
		else
			remoteHost = gethostbyaddr((char *) &addr, 4, AF_INET);
	}

	if (remoteHost == NULL)
	{
		dwError = WSAGetLastError();
		if (dwError != 0)
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				printf("Host not found\n");
				return 1;
			} 
			else if (dwError == WSANO_DATA)
			{
				printf("No data record found\n");
				return 1;
			}
			else
			{
				printf("Function failed with error: %ld\n", dwError);
				return 1;
			}
		}
	}
	else
	{
		printf("Function returned:\n");
		printf("\tOfficial name: %s\n", remoteHost->h_name);
		printf("\tAlternate names: %s\n", remoteHost->h_aliases);
		printf("\tAddress type: ");
		switch (remoteHost->h_addrtype)
		{
			case AF_INET:
				printf("AF_INET\n");
				break;
				
			case AF_INET6:
				printf("AF_INET\n");
				break;
				
			case AF_NETBIOS:
				printf("AF_NETBIOS\n");
				break;
				
			default:
				printf(" %d\n", remoteHost->h_addrtype);
				break;
		}
		
		printf("\tAddress length: %d\n", remoteHost->h_length);
		addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
		printf("\tFirst IP Address: %s\n", inet_ntoa(addr));
	}

	return 0;
}


