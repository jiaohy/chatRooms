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
    unsigned int vrsn : 9;
    unsigned int type : 7;
    int length;
}Header;

//SBCP Attribute
typedef struct AttributeSBCP
{
    int type;
    int length;
    char payload[512];
}Attribute;

//SBCP Message
typedef struct MessageSBCP
{
    Header header;
    Attribute attribute;
}Msg;

#endif /* SBCP_h */
