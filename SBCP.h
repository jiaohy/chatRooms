//
//  SBCP.h
//  chat_rooms
//
//  Created by jiaohongyang on 9/11/16.
//  Copyright © 2016 hongyang. All rights reserved.
//

#ifndef SBCP_h
#define SBCP_h

#define VERSION 3

/* Message Types */
#define JOIN      2
#define FWD       3
#define SEND      4

#define NAK       5
#define OFFLINE   6
#define ACK       7
#define ONLINE    8
#define IDLE      9

/* Attribute Types */
#define REASON      1
#define USERNAME    2
#define CLIENTCOUNT 3
#define MESSAGE     4

//SBCP Header
typedef struct HeaderSBCP
{
    u_char vrsn;
    u_char type;
    uint16_t length;
}Header;

//SBCP Attribute
typedef struct AttributeSBCP
{
    uint16_t type;
    uint16_t length;
}Attribute;

//SBCP Message
typedef struct MessageSBCP
{
    Header header;
    unsigned char* payload;
}Msg;


#endif /* SBCP_h */