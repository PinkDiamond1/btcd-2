/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifdef DEFINES_ONLY
#ifndef tourney777_h
#define tourney777_h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "../utils/bits777.c"
#include "../common/hostnet777.c"

struct tourney777
{
    char transport[16],ipaddr[64],name[128];
    uint32_t started,finished; uint64_t tableids[128]; uint16_t port,numtables;
    union hostnet777 hn[];
} *Tournament;

#endif
#else
#ifndef tourney777_c
#define tourney777_c

#ifndef tourney777_h
#define DEFINES_ONLY
#include "tourney777.c"
#undef DEFINES_ONLY
#endif


int32_t tourney777_findtable(struct tourney777 *tp,uint64_t tableid)
{
    int32_t i;
    if ( tp->numtables >  0 )
    {
        for (i=0; i<tp->numtables; i++)
            if ( tp->tableids[i] == tableid )
                return(i);
    }
    return(-1);
}

void tourney777_newhand(union hostnet777 *hn,uint64_t tableid,cJSON *json,uint8_t *data,int32_t datalen)
{
    char *handhist; int32_t i,handid,numplayers,n,busted,rebuy; cJSON *handjson,*array,*item;
    handid = juint(json,"handid");
    numplayers = juint(json,"numplayers");
    if ( (handhist= pangea_dispsummary(1,data,datalen,tableid,handid,numplayers)) != 0 )
    {
        printf("GOT HANDHIST.(%s)\n",handhist);
        if ( (handjson= cJSON_Parse(handhist)) != 0 )
        {
            if ( (array= jarray(&n,handjson,"hand")) != 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = jitem(array,i);
                    rebuy = juint(item,"rebuy");
                    if ( (busted= juint(item,"busted")) != 0 )
                    {
                        
                    }
                }
            }
            free_json(handjson);
        }
        free(handhist);
    }
}

void tourney777_poll(union hostnet777 *hn)
{
    char *jsonstr; uint64_t senderbits,tableid; uint8_t *buf=0; int32_t maxlen,len,senderind; uint32_t timestamp; char *cmdstr,*hexstr; cJSON *json;
    maxlen = 65536;
    if ( (buf= malloc(maxlen)) == 0 )
    {
        printf("tourney777_poll: cant allocate buf\n");
        return;
    }
    if ( (jsonstr= queue_dequeue(&hn->server->H.Q,1)) != 0 )
    {
        printf("tourney slot.%d GOT.(%s)\n",hn->client->H.slot,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            tableid = j64bits(json,"tableid");
            if ( tourney777_findtable(Tournament,tableid) < 0 )
                return;
            senderbits = j64bits(json,"sender");
            if ( (senderind= juint(json,"myind")) < 0 || senderind >= hn->server->num )
            {
                printf("pangea_poll: illegal senderind.%d cardi.%d turni.%d\n",senderind,juint(json,"cardi"),juint(json,"turni"));
                goto cleanup;
            }
            timestamp = juint(json,"timestamp");
            hn->client->H.state = juint(json,"state");
            len = juint(json,"n");
            cmdstr = jstr(json,"cmd");
            if ( (hexstr= jstr(json,"data")) != 0 && strlen(hexstr) == (len<<1) )
            {
                if ( len > maxlen )
                {
                    printf("len too big for tourney777_poll\n");
                    goto cleanup;
                }
                decode_hex(buf,len,hexstr);
            } else printf("len.%d vs hexlen.%ld (%s)\n",len,strlen(hexstr)>>1,hexstr);
            if ( cmdstr != 0 )
            {
                if ( strcmp(cmdstr,"newhand") == 0 )
                    tourney777_newhand(hn,tableid,json,buf,len);
            }
cleanup:
            free_json(json);
        }
        free_queueitem(jsonstr);
    }
    free(buf);
}

struct tourney777 *tourney777_init(char *name,bits256 privkey,char *transport,char *ipaddr,uint16_t port,int32_t maxplayers)
{
    struct hostnet777_server *srv; bits256 pubkey; struct tourney777 *tourney; int32_t i; char endpoint[128];
    tourney = calloc(1,sizeof(*tourney) + sizeof(*tourney->hn)*maxplayers);
    pubkey = acct777_pubkey(privkey);
    safecopy(tourney->name,name,sizeof(tourney->name)-1);
    if ( transport == 0 || transport[0] == 0 )
        transport = "tcp";
    strcpy(tourney->transport,transport);
    if ( ipaddr == 0 || ipaddr[0] == 0 )
        ipaddr = "127.0.0.1";
        strcpy(tourney->ipaddr,ipaddr);
    if ( port == 0 )
        port = 8897;
    tourney->port = port;
    if ( (srv= hostnet777_server(privkey,pubkey,tourney->transport,tourney->ipaddr,tourney->port,maxplayers)) == 0 )
    {
        printf("tourney777_init: cant create hostnet777 server\n");
        return(0);
    }
    srv->H.privkey = privkey, srv->H.pubkey = pubkey;
    for (i=0; i<maxplayers; i++)
    {
        sprintf(endpoint,"%s://%s:%u",srv->ep.transport,srv->ep.ipaddr,srv->ep.port + i + 1);
        srv->clients[i].pmsock = nn_createsocket(endpoint,1,"NN_PULL",NN_PULL,srv->ep.port + i + 1,10,10);
    }
    srv->H.pollfunc = tourney777_poll;
    tourney->hn[0].server = srv;
    if ( portable_thread_create((void *)hostnet777_idler,&tourney->hn[0]) == 0 )
        printf("error launching server thread\n");
    Tournament = tourney;
    return(tourney);
}

char *tourney777_start(char *name,cJSON *json)
{
    if ( Tournament != 0 && strcmp(Tournament->name,name) == 0 )
    {
        Tournament->started = (uint32_t)time(NULL);
        return(clonestr("{\"result\":\"tournament started\"}"));
    } else return(clonestr("{\"error\":\"no matching tournament\"}"));
}

char *tourney777_register(char *name,bits256 pubkey,cJSON *json)
{
    int32_t i; struct hostnet777_server *srv;
    if ( Tournament != 0 && strcmp(Tournament->name,name) == 0 && (srv= Tournament->hn[0].server) != 0 )
    {
        if ( Tournament->started != 0 )
            return(clonestr("{\"error\":\"tournament already started\"}"));
        else
        {
            for (i=0; i<srv->num; i++)
                if ( memcmp(pubkey.bytes,srv->clients[i].pubkey.bytes,sizeof(bits256)) == 0 )
                    return(clonestr("{\"error\":\"already registered in tournament\"}"));
            srv->clients[srv->num].pubkey = pubkey;
            srv->clients[srv->num].nxt64bits = acct777_nxt64bits(pubkey);
            srv->clients[srv->num].lastcontact = (uint32_t)time(NULL);
            srv->num++;
            return(clonestr("{\"result\":\"registered in tournament\"}"));
        }
    } else return(clonestr("{\"error\":\"no matching tournament\"}"));
}

char *tourney777_deregister(char *name,bits256 pubkey,cJSON *json)
{
    int32_t i; struct hostnet777_server *srv;
    if ( Tournament != 0 && strcmp(Tournament->name,name) == 0 && (srv= Tournament->hn[0].server) != 0 )
    {
        if ( Tournament->started != 0 )
            return(clonestr("{\"error\":\"tournament already started\"}"));
        else
        {
            for (i=0; i<srv->num; i++)
                if ( memcmp(pubkey.bytes,srv->clients[i].pubkey.bytes,sizeof(bits256)) == 0 )
                {
                    memset(&srv->clients[i],0,sizeof(srv->clients[i]));
                    if ( i == srv->num-1 )
                        srv->num--;
                    return(clonestr("{\"result\":\"deregistered from tournament\"}"));
                }
            return(clonestr("{\"error\":\"not registered in tournament\"}"));
        }
    } else return(clonestr("{\"error\":\"no matching tournament\"}"));
}


#endif
#endif
