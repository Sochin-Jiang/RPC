/**
 * Sources:
 * http://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mybind.c"
#include "ece454rpc_types.h"

// Socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>

// memcpy
#include <string.h>

// variable arguments
#include <stdarg.h>

typedef struct {
    char *proc_name;
    int num_params;
    arg_type arguments;
} proc_dec_type;

/**
 * Prints an IP address in dotted decimal notation.
 */
void paddr(unsigned char *a) {
    printf("%d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);
}

/**
 * Creates a socket.
 */
int createSocket(const int domain, const int type, const int protocol) {

    int socketDescriptor = socket(domain, type, protocol);

    if (socketDescriptor == -1) {
        perror("Failed to create socket.");
        exit(0);
    }

    printf("Created socket.\n");
    return socketDescriptor;
}

/**
 * Assigns socket an IP address and random port.
 */
void bindSocket(int *socket) {

    struct sockaddr_in myAddress;

    // Let OS pick what IP address is assigned
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddress.sin_family = AF_INET;
    myAddress.sin_port = 0;

    // Assign port
    if(mybind(*socket, (struct sockaddr_in*)&myAddress) < 0 ) {
        perror("Could't bind port to socket.");
        exit(0);
    }

    printf("Binded socket to port: %i \n", ntohs(myAddress.sin_port));
}

/**
 * Fetches host details for a given IP address.
 */
struct hostent* getHostDetails(const char *ipAddress) {

    struct hostent *host;
    host = gethostbyname(ipAddress);
    if (!host) {
        fprintf(stderr, "Could not obtain IP address for %s\n", ipAddress);
    }

    printf("The IP address of %s is: ", ipAddress);
    paddr((unsigned char*) host->h_addr_list[0]);

    return host;
}

int serializeData(char *procedure_name, int nparams, va_list valist, char *buffer ) {
  size_t idx = 0;
  int proc_size = strlen(procedure_name);

  memcpy(&buffer[idx], &proc_size, sizeof proc_size);
  idx += sizeof proc_size;

  memcpy(&buffer[idx], &procedure_name, sizeof procedure_name);
  idx += sizeof procedure_name;

  memcpy(&buffer[idx], &nparams, sizeof nparams);
  idx += sizeof nparams;

  struct arg ptr;
  struct arg list;

  for(int i = 0; i < nparams*2; i++){
    if(i%2 == 0){
      ptr.arg_size = va_arg(valist, int);
    } else {
      ptr.arg_val = va_arg(valist, void*);
      printf("val: %i \n", *(int *)(ptr.arg_val));
      list.next = &ptr;
      list = ptr;
    }
  }


  memcpy(&buffer[idx], &list, sizeof list);
  idx += sizeof list;

  return idx;
}

extern return_type make_remote_call(const char *servernameorip,
  const int serverportnumber,
  const char *procedure_name,
  const int nparams,
  ...) {

    // Result of make remote call
    void *val;
    int size;
    return_type ret = {val,size};

    // Create client socket
    int socket = createSocket(AF_INET, SOCK_DGRAM, 0);
    bindSocket(&socket);

    // TODO: Serialize data instead of a simple message
    char *message = "Test Message.";

    //serialized data processing
    int buff_size = sizeof(int);
    buff_size += sizeof procedure_name;
    buff_size += sizeof(int);
    buff_size += sizeof(arg_type);

    unsigned char *buffer[buff_size];

    va_list valist;
    va_start(valist, nparams*2);

    // printf("before memcpy: %lu \n", sizeof(buffer));
    int buffer_data_size = serializeData(procedure_name, nparams, valist, buffer);
    // printf("after memcpy: %lu \n", sizeof(buffer));


    // Create message destination address
    struct sockaddr_in serverAddress;
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverportnumber);
    struct hostent * serverLookup = getHostDetails(servernameorip); // TODO: Check if IP address or server name
    memcpy((void *)&serverAddress.sin_addr, serverLookup->h_addr_list[0], serverLookup->h_length);

    // Send message to server
    if (sendto(socket, buffer, buffer_data_size,
      0, (struct sockaddr *)&serverAddress,
      sizeof(serverAddress)) < 0) {
        perror("Failed to send message.");
    }

    printf("Program execution complete. \n");

    // TODO: Assign ret properly
    return ret;
}

int main() {
    int a = -10, b = 20;
    return_type ans = make_remote_call("ecelinux3.uwaterloo.ca",
      10000,
      "addtwo", 2,
      sizeof(int), (void *)(&a),
      sizeof(int), (void *)(&b));

    int i = *(int *)(ans.return_val);
    printf("client, got result: %d\n", i);

    return 0;
}
