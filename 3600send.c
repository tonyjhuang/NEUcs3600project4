/*
 * CS3600, Spring 2013
 * Project 4 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600sendrecv.h"

static int DATA_SIZE   = 1460;
static int WINDOW_SIZE = 5;

unsigned int last_ackd = 0;
unsigned int sequence  = 0;

void usage() {
  printf("Usage: 3600send host:port\n");
  exit(1);
}

/**
 * Reads the next block of data from stdin
 */
int get_next_data(char *data, int size) {
  return read(0, data, size);
}

/**
 * Builds and returns the next packet info, or NULL
 * if no more data is available.
 * max on data_len is 254, size of header is 8
 */
packet_info *get_next_packet_info(int sequence) {
  char *data = malloc(DATA_SIZE);
  int data_len = get_next_data(data, DATA_SIZE);

  if (data_len == 0) {
    free(data);
    return NULL;
  }

  header *myheader = make_header(sequence, data_len, 0, 0);
  void *packet = malloc(sizeof(header) + data_len);
  memcpy(packet, myheader, sizeof(header));
  memcpy(((char *) packet) +sizeof(header), data, data_len);

  free(data);
  free(myheader);

  packet_info *info = (packet_info *) malloc(sizeof(packet_info));
  info->packet    = packet;
  info->retrieved = time(0);
  info->data_len  = data_len;
  info->sequence  = sequence;

  return info;
}

// They don't think it be like it is, but it do.
int send_packet_info(int sock, struct sockaddr_in out, packet_info *info) {
  if (info->packet == NULL) 
    return 0;

  mylog("[send data] sequence: %d, data_len: %d\n", info->sequence, info->data_len);
  
  if(sendto(sock, info->packet, info->data_len + sizeof(header), 
	    0, (struct sockaddr *) &out, sizeof(out)) < 0) {

  //  if (sendto(sock, ptr, 0, 0, //info->data_len + sizeof(header), 0, 
  //	     (const struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
    mylog("error in sendto?\n");
    perror("sendto");
    //exit(1);
  }

  return 1;  
}

void send_final_packet(int sock, struct sockaddr_in out) {
  header *myheader = make_header(sequence+1, 0, 1, 0);
  mylog("[send eof]\n");

  if (sendto(sock, myheader, sizeof(header), 0, (struct sockaddr *) &out, (socklen_t) sizeof(out)) < 0) {
    mylog("final packet errored out!\n");
    perror("sendto");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  /**

   * I've included some basic code for opening a UDP socket in C, 
   * binding to a empheral port, printing out the port number.
   * 
   * I've also included a very simple transport protocol that simply
   * acknowledges every received packet.  It has a header, but does
   * not do any error handling (i.e., it does not have sequence 
   * numbers, timeouts, retries, a "window"). You will
   * need to fill in many of the details, but this should be enough to
   * get you started.
   */

  // extract the host IP and port
  if ((argc != 2) || (strstr(argv[1], ":") == NULL)) {
    usage();
  }

  char *tmp = (char *) malloc(strlen(argv[1])+1);
  strcpy(tmp, argv[1]);

  char *ip_s = strtok(tmp, ":");
  char *port_s = strtok(NULL, ":");
 
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the local port
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(atoi(port_s));
  out.sin_addr.s_addr = inet_addr(ip_s);

  // socket for received packets
  struct sockaddr_in in;
  socklen_t in_len = sizeof(in);

  // construct the socket set
  fd_set socks;

  // construct the timeout
  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  packet_info packet_buffer[WINDOW_SIZE];
  // Oldest packet in buffer that has been sent but has not been ackd
  int packet_unackd = 0;
  // next free spot in buffer
  int packet_next   = 0; 

  // Note:
  // To send packet: send_packet(sock, out, packet_info)
  // Make sure to end with send_final_packet(sock, out)
  packet_info *info;

  /**
   * main program loop. Do the following steps:
   * -check if buffer has empty spots
   * -fill one empty buffer spot & send off that packet
   * -check if any acks came in
   * -remove the ack'd packet
   */
  while(1) {
    /**
     * if buffer isn't full, get the next packet and add it to the buffer
     * then immediately send it off
     */
    if(packet_next - packet_unackd < WINDOW_SIZE) {
      mylog("looking for next packet.., next: %d, unackd: %d\n", packet_next, packet_unackd);
      // Are there any new packets to send? if so, store and send exactly one.
      if((info = get_next_packet_info(sequence)) != NULL) {
	int packet_next_index = packet_next % WINDOW_SIZE;
	packet_buffer[packet_next_index] = *info;
	send_packet_info(sock, out, info);
	packet_next++;
	sequence += info->data_len;
      } else {
	send_final_packet(sock, out);
	break;
      }
    } else {
      send_final_packet(sock, out);
      break;
    }
  }



    //dump_packet(packet_buffer[0].packet, packet_buffer[0].data_len + sizeof(header));
    //dump_packet(packet, packet_len);

  /*  while (send_next_packet(sock, out)) {
    int done = 0; // fuck timeouts

    while (!done) {
      FD_ZERO(&socks);
      FD_SET(sock, &socks);

      // wait to receive, or for a timeout
      if (select(sock + 1, &socks, NULL, NULL, &t)) {
        unsigned char buf[10000];
        int buf_len = sizeof(buf);
        int received;
        if ((received = recvfrom(sock, &buf, buf_len, 0, (struct sockaddr *) &in, (socklen_t *) &in_len)) < 0) {
          perror("recvfrom");
          exit(1);
        }

        header *myheader = get_header(buf);

        if ((myheader->magic == MAGIC) && (myheader->sequence >= sequence) && (myheader->ack == 1)) {
          mylog("[recv ack] %d\n", myheader->sequence);
          sequence = myheader->sequence;
          done = 1;
        } else {
          mylog("[recv corrupted ack] %x %d\n", MAGIC, sequence);
        }
      } else {
        mylog("[error] timeout occurred\n");
      }
    }
  }
  */
  //  send_final_packet(sock, out);

  mylog("[completed]\n");

  return 0;
}
