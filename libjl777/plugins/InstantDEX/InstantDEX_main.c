//
//  InstantDEX.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "InstantDEX"
#define PLUGNAME(NAME) InstantDEX ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

#define ORDERBOOK_EXPIRATION 3600
#define INSTANTDEX_MINVOL 75
#define INSTANTDEX_MINVOLPERC ((double)INSTANTDEX_MINVOL / 100.)
#define INSTANTDEX_PRICESLIPPAGE 0.001

#define INSTANTDEX_TRIGGERDEADLINE 15
#define JUMPTRADE_SECONDS 100
#define INSTANTDEX_ACCT "4383817337783094122"
#define INSTANTDEX_FEE ((long)(2.5 * SATOSHIDEN))

#define INSTANTDEX_NAME "InstantDEX"
#define INSTANTDEX_NXTAENAME "nxtae"
#define INSTANTDEX_NXTAEUNCONF "unconf"
#define INSTANTDEX_BASKETNAME "basket"
#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_UNCONFID 1
#define INSTANTDEX_NXTAEID 2
#define MAX_EXCHANGES 64

#define DEFINES_ONLY
#include "../agents/plugin777.c"
#include "../utils/NXT777.c"
#include "../includes/portable777.h"
#undef DEFINES_ONLY

#define INSTANTDEX_LOCALAPI "allorderbooks", "orderbook", "lottostats", "LSUM", "makebasket", "disable", "enable", "peggyrates", "tradesequence", "placebid", "placeask", "openorders", "cancelorder", "tradehistory"

#define INSTANTDEX_REMOTEAPI "msigaddr", "bid", "ask"
char *PLUGNAME(_methods)[] = { INSTANTDEX_REMOTEAPI}; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { INSTANTDEX_REMOTEAPI }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "echo" }; // list of supported methods that require authentication

typedef char *(*json_handler)(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr);

queue_t InstantDEXQ,TelepathyQ;
struct InstantDEX_info INSTANTDEX;
struct pingpong_queue Pending_offersQ;
cJSON *Lottostats_json;
cJSON *InstantDEX_lottostats();

#include "NXT_tx.h"
#include "quotes.h"
#include "atomic.h"

uint32_t prices777_NXTBLOCK;
int32_t InstantDEX_idle(struct plugin_info *plugin)
{
    static double lastmilli; static cJSON *oldjson;
    char *jsonstr,*retstr; cJSON *json; uint32_t NXTblock; int32_t n = 0;
    if ( INSTANTDEX.readyflag == 0 )
        return(0);
    if ( (jsonstr= queue_dequeue(&InstantDEXQ,1)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            printf("Got InstantDEX.(%s)\n",jsonstr);
            if ( (retstr = InstantDEX(jsonstr,jstr(json,"remoteaddr"),juint(json,"localaccess"))) != 0 )
                printf("InstantDEX.(%s)\n",retstr), free(retstr);
            free_json(json);
            n++;
        } else printf("error parsing (%s) from InstantDEXQ\n",jsonstr);
        free_queueitem(jsonstr);
    }
    if ( milliseconds() < (lastmilli + 5000) )
        return(n);
    NXTblock = _get_NXTheight(0);
    if ( NXTblock != prices777_NXTBLOCK )
    {
        if ( oldjson != 0 )
            free_json(oldjson);
        oldjson = Lottostats_json;
        Lottostats_json = InstantDEX_lottostats();
        prices777_NXTBLOCK = NXTblock;
        poll_pending_offers(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET);
    }
    lastmilli = milliseconds();
    return(0);
}

int32_t prices777_key(char *key,char *exchange,char *name,char *base,uint64_t baseid,char *rel,uint64_t relid)
{
    int32_t len,keysize = 0;
    memcpy(&key[keysize],&baseid,sizeof(baseid)), keysize += sizeof(baseid);
    memcpy(&key[keysize],&relid,sizeof(relid)), keysize += sizeof(relid);
    strcpy(&key[keysize],exchange), keysize += strlen(exchange) + 1;
    strcpy(&key[keysize],name), keysize += strlen(name) + 1;
    memcpy(&key[keysize],base,strlen(base)+1), keysize += strlen(base) + 1;
    if ( rel != 0 && (len= (int32_t)strlen(rel)) > 0 )
        memcpy(&key[keysize],rel,len+1), keysize += len+1;
    return(keysize);
}

int32_t get_assetname(char *name,uint64_t assetid)
{
    char assetidstr[64],*jsonstr; cJSON *json;
    expand_nxt64bits(assetidstr,assetid);
    name[0] = 0;
    if ( (jsonstr= _issue_getAsset(assetidstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            extract_cJSON_str(name,15,json,"name");
            free_json(json);
        }
        free(jsonstr);
    }
    return((int32_t)strlen(assetidstr));
}

uint64_t InstantDEX_name(char *key,int32_t *keysizep,char *exchange,char *name,char *base,uint64_t *baseidp,char *rel,uint64_t *relidp)
{
    uint64_t baseid,relid,assetbits = 0; char *s,*str;
    baseid = *baseidp, relid = *relidp;
    //printf(">>>>>> name.(%s)\n",name);
    if ( strcmp(base,"5527630") == 0 || baseid == 5527630 )
        strcpy(base,"NXT");
    if ( strcmp(rel,"5527630") == 0 || relid == 5527630 )
        strcpy(rel,"NXT");
    if ( strcmp("nxtae",exchange) == 0 || strcmp("unconf",exchange) == 0 )
    {
        if ( strcmp(rel,"NXT") == 0 )
            s = "+", relid = stringbits("NXT"), strcpy(rel,"NXT");
        else if ( strcmp(base,"NXT") == 0 )
            s = "-", baseid = stringbits("NXT"), strcpy(base,"NXT");
        else s = "";
        if ( relid == 0 && rel[0] != 0 )
        {
            if ( is_decimalstr(rel) != 0 )
                relid = calc_nxt64bits(rel);
            else relid = is_MGWcoin(rel);
        }
        else if ( (str= is_MGWasset(relid)) != 0 )
            strcpy(rel,str);
        if ( baseid == 0 && base[0] != 0 )
        {
            if ( is_decimalstr(base) != 0 )
                baseid = calc_nxt64bits(base);
            else baseid = is_MGWcoin(base);
        }
        else if ( (str= is_MGWasset(baseid)) != 0 )
            strcpy(base,str);
        if ( base[0] == 0 )
            get_assetname(base,baseid);
        if ( rel[0] == 0 )
            get_assetname(rel,relid);
        if ( name[0] == 0 )
        {
            if ( relid == NXT_ASSETID )
                sprintf(name,"%llu",(long long)baseid);
            else if ( baseid == NXT_ASSETID )
                sprintf(name,"-%llu",(long long)relid);
            else sprintf(name,"%llu/%llu",(long long)baseid,(long long)relid);
        }
    }
    else
    {
        if ( base[0] != 0 && rel[0] != 0 && baseid == 0 && relid == 0 )
        {
            baseid = peggy_basebits(base), relid = peggy_basebits(rel);
            if ( name[0] == 0 && baseid != 0 && relid != 0 )
            {
                strcpy(name,base); // need to be smarter
                strcat(name,"/");
                strcat(name,rel);
            }
        }
        if ( name[0] == 0 || baseid == 0 || relid == 0 || base[0] == 0 || rel[0] == 0 )
        {
            if ( baseid == 0 && base[0] != 0 )
                baseid = stringbits(base);
            else if ( baseid != 0 && base[0] == 0 )
                sprintf(base,"%llu",(long long)baseid);
            if ( relid == 0 && rel[0] != 0 )
                relid = stringbits(rel);
            else if ( relid != 0 && rel[0] == 0 )
                sprintf(rel,"%llu",(long long)relid);
            if ( name[0] == 0 )
                strcpy(name,base), strcat(name,"/"), strcat(name,rel);
        }
    }
    *baseidp = baseid, *relidp = relid;
    *keysizep = prices777_key(key,exchange,name,base,baseid,rel,relid);
    //printf("<<<<<<< name.(%s)\n",name);
    return(assetbits);
}

cJSON *InstantDEX_lottostats()
{
    char cmdstr[1024],NXTaddr[64],buf[1024],receiverstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,totaltickets = 0;
    uint64_t amount,senderbits;
    uint32_t timestamp = 0;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getAccountTransactions&account=%s&timestamp=%u&type=0&subtype=0",INSTANTDEX_ACCT,timestamp);
    //printf("cmd.(%s)\n",cmdstr);
    if ( (jsonstr= issue_NXTPOST(cmdstr)) != 0 )
    {
        // printf("jsonstr.(%s)\n",jsonstr);
        // mm string.({"requestProcessingTime":33,"transactions":[{"fullHash":"2a2aab3b84dadf092cf4cedcd58a8b5a436968e836338e361c45651bce0ef97e","confirmations":203,"signatureHash":"52a4a43d9055fe4861b3d13fbd03a42fecb8c9ad4ac06a54da7806a8acd9c5d1","transaction":"711527527619439146","amountNQT":"1100000000","transactionIndex":2,"ecBlockHeight":360943,"block":"6797727125503999830","recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"423766016895692955","timestamp":38929220,"ecBlockId":"10121077683890606382","height":360949,"subtype":0,"senderPublicKey":"4e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb19","deadline":1440,"blockTimestamp":38929430,"senderRS":"NXT-8E6V-YBWH-5VMR-26ESD","signature":"4318f36d9cf68ef0a8f58303beb0ed836b670914065a868053da5fe8b096bc0c268e682c0274e1614fc26f81be4564ca517d922deccf169eafa249a88de58036"}]})
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr,INSTANTDEX_ACCT) == 0 )
                    {
                        if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                        {
                            expand_nxt64bits(NXTaddr,senderbits);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( amount == INSTANTDEX_FEE )
                                totaltickets++;
                            else if ( amount >= 2*INSTANTDEX_FEE )
                                totaltickets += 2;
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    sprintf(buf,"{\"result\":\"lottostats\",\"totaltickets\":\"%d\"}",totaltickets);
    return(cJSON_Parse(buf));
}

char *InstantDEX_dotrades(struct prices777_order *trades,int32_t numtrades,int32_t dotrade)
{
    cJSON *retjson,*retarray,*item; int32_t i;
    retjson = cJSON_CreateObject(), retarray = cJSON_CreateArray();
    for (i=0; i<numtrades; i++)
    {
        item = InstantDEX_tradejson(&trades[i],dotrade);
        //printf("GOT%d.(%s)\n",i,jprint(item,0));
        jaddi(retarray,item);
    }
    jadd(retjson,"traderesults",retarray);
    return(jprint(retjson,1));
}

char *InstantDEX_tradesequence(cJSON *json)
{
    //"trades":[[{"basket":"bid","rootwt":-1,"groupwt":1,"wt":-1,"price":40000,"volume":0.00015000,"group":0,"trade":"buy","exchange":"nxtae","asset":"17554243582654188572","base":"BTC","rel":"NXT","orderid":"3545444239044461477","orderprice":40000,"ordervolume":0.00015000}], [{"basket":"bid","rootwt":-1,"groupwt":1,"wt":1,"price":0.00376903,"volume":1297.41480000,"group":10,"trade":"sell","exchange":"coinbase","name":"BTC/USD","base":"BTC","rel":"USD","orderid":"1","orderprice":265.32000000,"ordervolume":4.89000000}]]}
    cJSON *array,*item; int32_t i,n,dir; char *tradestr,*exchangestr,base[512],rel[512],name[512]; struct prices777_order trades[16],*order;
    uint64_t orderid,assetid,currency,baseid,relid; double orderprice,ordervolume; struct prices777 *prices; uint32_t timestamp;
    memset(trades,0,sizeof(trades));
    if ( (array= jarray(&n,json,"trades")) != 0 )
    {
        if ( n > sizeof(trades)/sizeof(*trades) )
            return(clonestr("{\"error\":\"exceeded max trades possible in a tradesequence\"}"));
        timestamp = (uint32_t)time(NULL);
        for (i=0; i<n; i++)
        {
            item = jitem(array,i);
            tradestr = jstr(item,"trade"), exchangestr = jstr(item,"exchange");
            copy_cJSON(base,jobj(item,"base")), copy_cJSON(rel,jobj(item,"rel")), copy_cJSON(name,jobj(item,"name"));
            orderid = j64bits(item,"orderid"), assetid = j64bits(item,"asset"), currency = j64bits(item,"currency");
            baseid = j64bits(item,"baseid"), relid = j64bits(item,"relid");
            orderprice = jdouble(item,"orderprice"), ordervolume = jdouble(item,"ordervolume");
            if ( tradestr != 0 )
            {
                if ( strcmp(tradestr,"buy") == 0 )
                    dir = 1;
                else if ( strcmp(tradestr,"sell") == 0 )
                    dir = -1;
                else if ( strcmp(tradestr,"swap") == 0 )
                    dir = 0;
                else return(clonestr("{\"error\":\"invalid trade direction\"}"));
                if ( (prices= prices777_initpair(1,0,exchangestr,base,rel,0.,name,baseid,relid,0)) != 0 )
                {
                    order = &trades[i];
                    order->source = prices;
                    order->id = orderid, order->assetid = assetid != 0 ? assetid : currency;
                    order->wt = dir, order->price = orderprice, order->vol = ordervolume;
                } else return(clonestr("{\"error\":\"invalid exchange or contract pair\"}"));
            } else return(clonestr("{\"error\":\"no trade specified\"}"));
        }
        return(InstantDEX_dotrades(trades,n,juint(json,"dotrade")));
    }
    printf("error parsing.(%s)\n",jprint(json,0));
    return(clonestr("{\"error\":\"couldnt process trades\"}"));
}

char *InstantDEX(char *jsonstr,char *remoteaddr,int32_t localaccess)
{
    char *prices777_allorderbooks();
    char *InstantDEX_openorders();
    char *InstantDEX_tradehistory();
    char *InstantDEX_cancelorder(uint64_t orderid);
    char *retstr = 0,key[512],retbuf[1024],exchangestr[MAX_JSON_FIELD],method[MAX_JSON_FIELD],name[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD];
    cJSON *json; uint64_t orderid,baseid,relid,assetbits; uint32_t maxdepth; int32_t dir,invert,keysize,allfields; struct prices777 *prices;
    if ( jsonstr != 0 && (json= cJSON_Parse(jsonstr)) != 0 )
    {
        // instantdex orders and orderbooks, openorders/cancelorder/tradehistory
        // placebid/ask, makeoffer3/bid/ask/respondtx verify phasing, asset/nxtae, asset/asset, asset/external, external/external
        // autofill and automatch
        // tradehistory and other stats -> peggy integration
        baseid = j64bits(json,"baseid"), relid = j64bits(json,"relid");
        copy_cJSON(name,jobj(json,"name"));
        copy_cJSON(base,jobj(json,"base"));
        copy_cJSON(rel,jobj(json,"rel"));
        copy_cJSON(method,jobj(json,"method"));
        orderid = j64bits(json,"orderid");
        allfields = juint(json,"allfields");
        if ( (maxdepth= juint(json,"maxdepth")) > MAX_DEPTH )
            maxdepth = MAX_DEPTH;
        copy_cJSON(exchangestr,jobj(json,"exchange"));
        if ( exchangestr[0] == 0 )
        {
            if ( baseid != 0 && relid != 0 )
                strcpy(exchangestr,"nxtae");
            else strcpy(exchangestr,"basket");
        }
        assetbits = InstantDEX_name(key,&keysize,exchangestr,name,base,&baseid,rel,&relid);
        if ( strcmp(method,"allorderbooks") == 0 )
            retstr = prices777_allorderbooks();
        else if ( strcmp(method,"openorders") == 0 )
            retstr = InstantDEX_openorders(SUPERNET.NXTADDR);
        else if ( strcmp(method,"cancelorder") == 0 )
            retstr = InstantDEX_cancelorder(orderid);
        else if ( strcmp(method,"tradehistory") == 0 )
            retstr = InstantDEX_tradehistory();
        else if ( strcmp(method,"lottostats") == 0 )
            retstr = jprint(Lottostats_json,0);
        else if ( strcmp(method,"tradesequence") == 0 )
            retstr = InstantDEX_tradesequence(json);
        else if ( strcmp(method,"makebasket") == 0 )
        {
            if ( (prices= prices777_makebasket(0,json)) != 0 )
                retstr = clonestr("{\"result\":\"basket made\"}");
            else retstr = clonestr("{\"error\":\"couldnt make basket\"}");
        }
        else if ( strcmp(method,"peggyrates") == 0 )
        {
            if ( SUPERNET.peggy != 0 )
                retstr = peggyrates(juint(json,"timestamp"));
            else retstr = clonestr("{\"error\":\"peggy disabled\"}");
        }
        else if ( strcmp(method,"LSUM") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\",\"amount\":%d}",(rand() & 1) ? "BUY" : "SELL",(rand() % 100) * 100000);
            retstr = clonestr(retbuf);
        }
        if ( retstr == 0 && (prices= prices777_poll(exchangestr,name,base,baseid,rel,relid)) != 0 )
        {
            if ( prices->baseid == baseid && prices->relid == relid )
                invert = 0;
            else if ( prices->baseid == relid && prices->relid == baseid )
                invert = 1;
            else invert = 0, printf("baserel not matching (%s %s) vs (%s %s)\n",prices->base,prices->rel,base,rel);
            if ( strcmp(method,"placebid") == 0 || strcmp(method,"placeask") == 0 )
            {
                if ( strcmp(method,"placebid") == 0 )
                    dir = 1 - invert*2;
                else dir = -(1 - invert*2);
                return(InstantDEX_quote(prices,dir,jdouble(json,"price"),jdouble(json,"volume"),orderid,juint(json,"minperc"),juint(json,"automatch"),juint(json,"duration")));
            }
            else if ( strcmp(method,"disable") == 0 )
            {
                if ( prices != 0 )
                {
                    prices->disabled = 1;
                    return(clonestr("{\"result\":\"success\"}"));
                }
                else return(clonestr("{\"error\":\"no prices to disable\"}"));
            }
            else if ( strcmp(method,"enable") == 0 )
            {
                if ( prices != 0 )
                {
                    prices->disabled = 0;
                    return(clonestr("{\"result\":\"success\"}"));
                }
                else return(clonestr("{\"error\":\"no prices to enable\"}"));
            }
    //printf("return invert.%d allfields.%d (%s %s) vs (%s %s)  [%llu %llu] vs [%llu %llu]\n",invert,allfields,base,rel,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,(long long)baseid,(long long)relid);
            else if ( strcmp(method,"orderbook") == 0 )
            {
                if ( (retstr= prices->orderbook_jsonstrs[invert][allfields]) == 0 )
                {
                    char *prices777_orderbook_jsonstr(int32_t invert,uint64_t nxt64bits,struct prices777 *prices,struct prices777_basketinfo *OB,int32_t maxdepth,int32_t allflag);
                    retstr = prices777_orderbook_jsonstr(invert,SUPERNET.my64bits,prices,&prices->O,MAX_DEPTH,allfields);
                    portable_mutex_lock(&prices->mutex);
                    if ( prices->orderbook_jsonstrs[invert][allfields] != 0 )
                        free(prices->orderbook_jsonstrs[invert][allfields]);
                    prices->orderbook_jsonstrs[invert][allfields] = retstr;
                    portable_mutex_unlock(&prices->mutex);
                    if ( retstr == 0 )
                        retstr = clonestr("{}");
                }
                if ( 0 && maxdepth != MAX_DEPTH && retstr != 0 )
                {
                    int32_t i; cJSON *bids,*asks,*bids2,*asks2,*clone;
                    if ( (json= cJSON_Parse(retstr)) != 0 )
                    {
                        bids = jobj(json,"bids");
                        asks = jobj(json,"asks");
                        //printf("parsed bids.%p asks.%p\n",bids,asks);
                        if ( bids != 0 && asks != 0 )
                        {
                            bids2 = cJSON_CreateObject();
                            asks2 = cJSON_CreateObject();
                            for (i=0; i<maxdepth; i++)
                            {
                                jaddi(bids2,jitem(bids,i));
                                jaddi(asks2,jitem(asks,i));
                            }
                            clone = cJSON_Duplicate(json,0);
                            printf("new.(%s) (%s)\n",jprint(bids2,0),jprint(asks2,0));
                            jadd(clone,"bids",bids2), jadd(clone,"asks",asks2);
                            retstr = jprint(clone,1);
                        }
                        free_json(json);
                    }
                }
            }
        }
        if ( Debuglevel > 2 )
            printf("(%s) %p exchange.(%s) base.(%s) %llu rel.(%s) %llu | name.(%s) %llu\n",retstr!=0?retstr:"",prices,exchangestr,base,(long long)baseid,rel,(long long)relid,name,(long long)assetbits);
    }
    return(retstr);
}


/*char *placebid_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,localaccess,1,SUPERNET.NXTADDR,valid,objs,numobjs,origargstr));
}

char *placeask_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,localaccess,-1,SUPERNET.NXTADDR,valid,objs,numobjs,origargstr));
}*/

char *bid_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD];
    copy_cJSON(offerNXT,objs[12]);
    printf("bid_func %s vs offerNXT %s\n",SUPERNET.NXTADDR,offerNXT);
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
        return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,0,1,sender,valid,objs,numobjs,origargstr));
    else return(0);
}

char *ask_func(int32_t localaccess,int32_t valid,char *sender,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD];
    copy_cJSON(offerNXT,objs[12]);
    printf("ask_func %s vs offerNXT %s\n",SUPERNET.NXTADDR,offerNXT);
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
        return(placequote_func(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,0,-1,sender,valid,objs,numobjs,origargstr));
    else return(0);
}

char *InstantDEX_parser(char *forwarder,char *sender,int32_t valid,char *origargstr,cJSON *origargjson)
{
    /*static char *allorderbooks[] = { (char *)allorderbooks_func, "allorderbooks", "", 0 };
    static char *orderbook[] = { (char *)orderbook_func, "orderbook", "", "baseid", "relid", "allfields", "oldest", "maxdepth", "base", "rel", "gui", "showall", "exchange", "name", 0 };
     static char *jumptrades[] = { (char *)jumptrades_func, "jumptrades", "", 0 };
     static char *tradehistory[] = { (char *)tradehistory_func, "tradehistory", "", "timestamp", 0 };
     static char *lottostats[] = { (char *)lottostats_func, "lottostats", "", "timestamp", 0 };
    static char *cancelquote[] = { (char *)cancelquote_func, "cancelquote", "", "quoteid", 0 };
    static char *openorders[] = { (char *)openorders_func, "openorders", "", 0 };
    static char *placebid[] = { (char *)placebid_func, "placebid", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT", "base", "rel", "name", 0 };
    static char *placeask[] = { (char *)placeask_func, "placeask", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", ",gui", "automatch", "minperc", "duration", "exchange", "offerNXT",  "base", "rel", "name", 0 };
    */static char *bid[] = { (char *)bid_func, "bid", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT",  "base", "rel", "name", 0 };
    static char *ask[] = { (char *)ask_func, "ask", "", "baseid", "relid", "volume", "price", "timestamp", "baseamount", "relamount", "gui", "automatch", "minperc", "duration", "exchange", "offerNXT",  "base", "rel", "name", 0 };
    static char *makeoffer3[] = { (char *)makeoffer3_func, "makeoffer3", "", "baseid", "relid", "quoteid", "perc", "deprecated", "baseiQ", "reliQ", "askoffer", "price", "volume", "exchange", "baseamount", "relamount", "offerNXT", "minperc", "jumpasset",  "base", "rel", "name", 0 };
    static char *respondtx[] = { (char *)respondtx_func, "respondtx", "", "cmd", "assetid", "quantityQNT", "priceNQT", "triggerhash", "quoteid", "sig", "data", "minperc", "offerNXT", "otherassetid", "otherqty", 0 };
     static char **commands[] = { respondtx, makeoffer3, bid, ask };
    int32_t i,j,localaccess = 0;
    cJSON *argjson,*obj,*nxtobj,*secretobj,*objs[64];
    char NXTaddr[MAX_JSON_FIELD],NXTACCTSECRET[MAX_JSON_FIELD],command[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],**cmdinfo,*argstr,*retstr=0;
    memset(objs,0,sizeof(objs));
    command[0] = 0;
    valid = 1;
    memset(NXTaddr,0,sizeof(NXTaddr));
    if ( is_cJSON_Array(origargjson) != 0 )
    {
        argjson = cJSON_GetArrayItem(origargjson,0);
        argstr = cJSON_Print(argjson), _stripwhite(argstr,' ');
    } else argjson = origargjson, argstr = origargstr;
    NXTACCTSECRET[0] = 0;
    if ( argjson != 0 )
    {
        if ( (obj= cJSON_GetObjectItem(argjson,"requestType")) == 0 )
            obj = cJSON_GetObjectItem(argjson,"method");
        nxtobj = cJSON_GetObjectItem(argjson,"NXT");
        secretobj = cJSON_GetObjectItem(argjson,"secret");
        copy_cJSON(NXTaddr,nxtobj);
        copy_cJSON(offerNXT,cJSON_GetObjectItem(argjson,"offerNXT"));
        if ( NXTaddr[0] == 0 && offerNXT[0] == 0 )
        {
            safecopy(NXTaddr,SUPERNET.NXTADDR,64);
            safecopy(offerNXT,SUPERNET.NXTADDR,64);
            safecopy(sender,SUPERNET.NXTADDR,64);
            ensure_jsonitem(argjson,"NXT",NXTaddr);
            ensure_jsonitem(argjson,"offerNXT",offerNXT);
        }
        else if ( offerNXT[0] == 0 && strcmp(NXTaddr,SUPERNET.NXTADDR) == 0 )
        {
            strcpy(offerNXT,SUPERNET.NXTADDR);
            strcpy(sender,SUPERNET.NXTADDR);
            ensure_jsonitem(argjson,"offerNXT",offerNXT);
        }
        else strcpy(sender,offerNXT);
        if ( strcmp(offerNXT,SUPERNET.NXTADDR) == 0 )
            localaccess = 1;
        if ( localaccess != 0 && strcmp(NXTaddr,SUPERNET.NXTADDR) != 0 )
        {
            strcpy(NXTaddr,SUPERNET.NXTADDR);
            ensure_jsonitem(argjson,"NXT",NXTaddr);
            //printf("subsititute NXT.%s\n",NXTaddr);
        }
//printf("localaccess.%d myaddr.(%s) NXT.(%s) offerNXT.(%s)\n",localaccess,SUPERNET.NXTADDR,NXTaddr,offerNXT);
        copy_cJSON(command,obj);
        copy_cJSON(NXTACCTSECRET,secretobj);
        if ( NXTACCTSECRET[0] == 0 )
        {
            if ( localaccess != 0 || strcmp(command,"findnode") != 0 )
            {
                safecopy(NXTACCTSECRET,SUPERNET.NXTACCTSECRET,sizeof(NXTACCTSECRET));
                strcpy(NXTaddr,SUPERNET.NXTADDR);
             }
        }
//printf("(%s) argstr.(%s) command.(%s) NXT.(%s) valid.%d\n",cJSON_Print(argjson),argstr,command,NXTaddr,valid);
        //fprintf(stderr,"SuperNET_json_commands sender.(%s) valid.%d | size.%d | command.(%s) orig.(%s)\n",sender,valid,(int32_t)(sizeof(commands)/sizeof(*commands)),command,origargstr);
        for (i=0; i<(int32_t)(sizeof(commands)/sizeof(*commands)); i++)
        {
            cmdinfo = commands[i];
            if ( strcmp(cmdinfo[1],command) == 0 )
            {
                //printf("needvalid.(%c) valid.%d %d of %d: cmd.(%s) vs command.(%s)\n",cmdinfo[2][0],valid,i,(int32_t)(sizeof(commands)/sizeof(*commands)),cmdinfo[1],command);
                if ( cmdinfo[2][0] != 0 && valid <= 0 )
                    return(0);
                for (j=3; cmdinfo[j]!=0&&j<3+(int32_t)(sizeof(objs)/sizeof(*objs)); j++)
                    objs[j-3] = cJSON_GetObjectItem(argjson,cmdinfo[j]);
                retstr = (*(json_handler)cmdinfo[0])(localaccess,valid,sender,objs,j-3,argstr);
                break;
            }
        }
    } else printf("not JSON to parse?\n");
    if ( argstr != origargstr )
        free(argstr);
    return(retstr);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct InstantDEX_info));
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

void init_exchanges(cJSON *json)
{
    static char *exchanges[] = { "bitfinex", "btc38", "bitstamp", "btce", "poloniex", "bittrex", "huobi", "coinbase", "okcoin", "bityes", "lakebtc", "exmo" }; // "bter" <- orderbook is backwards and all entries are needed, later to support
    cJSON *array; int32_t i,n;
    find_exchange(0,INSTANTDEX_NAME);
    find_exchange(0,INSTANTDEX_NXTAEUNCONF);
    find_exchange(0,INSTANTDEX_NXTAENAME);
    find_exchange(0,INSTANTDEX_BASKETNAME);
    for (i=0; i<sizeof(exchanges)/sizeof(*exchanges); i++)
        find_exchange(0,exchanges[i]);
    if ( (array= jarray(&n,json,"baskets")) != 0 )
    {
        for (i=0; i<n; i++)
            prices777_makebasket(0,jitem(array,i));
    }
    //prices777_makebasket("{\"name\":\"NXT/BTC\",\"base\":\"NXT\",\"rel\":\"BTC\",\"basket\":[{\"exchange\":\"bittrex\"},{\"exchange\":\"poloniex\"},{\"exchange\":\"btc38\"}]}",0);
}

void init_InstantDEX(uint64_t nxt64bits,int32_t testflag,cJSON *json)
{
    int32_t a,b;
    init_pingpong_queue(&Pending_offersQ,"pending_offers",process_Pending_offersQ,0,0);
    Pending_offersQ.offset = 0;
    init_exchanges(json);
    find_exchange(&a,INSTANTDEX_NXTAENAME), find_exchange(&b,INSTANTDEX_NAME);
    if ( a != INSTANTDEX_NXTAEID || b != INSTANTDEX_EXCHANGEID )
        printf("invalid exchangeid %d, %d\n",a,b);
    printf("NXT-> %llu BTC -> %llu\n",(long long)stringbits("NXT"),(long long)stringbits("BTC"));
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*retstr = 0;
    retbuf[0] = 0;
   // if ( Debuglevel > 2 )
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        plugin->allowremote = 1;
        portable_mutex_init(&plugin->mutex);
        init_InstantDEX(calc_nxt64bits(SUPERNET.NXTADDR),0,json);
        //update_NXT_assettrades();
        INSTANTDEX.readyflag = 1;
        strcpy(retbuf,"{\"result\":\"InstantDEX init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        if ( (methodstr= cJSON_str(cJSON_GetObjectItem(json,"method"))) == 0 )
            methodstr = cJSON_str(cJSON_GetObjectItem(json,"requestType"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        //portable_mutex_lock(&plugin->mutex);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
#ifdef INSIDE_MGW
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            char *devMGW_command(char *jsonstr,cJSON *json);
            if ( SUPERNET.gatewayid >= 0 )
            {
                if ( (retstr= devMGW_command(jsonstr,json)) != 0 )
                {
                    //should_forward(sender,retstr);
                }
            } //else retstr = nn_loadbalanced((uint8_t *)jsonstr,(int32_t)strlen(jsonstr)+1);
        }
#endif
        else if ( strcmp(methodstr,"LSUM") == 0 )
        {
            sprintf(retbuf,"{\"result\":\"%s\",\"amount\":%d}",(rand() & 1) ? "BUY" : "SELL",(rand() % 100) * 100000);
            retstr = clonestr(retbuf);
        }
        else if ( SUPERNET.iamrelay <= 1 )
        {
            retstr = InstantDEX_parser(forwarder,sender,valid,jsonstr,json);
printf("InstantDEX_parser return.(%s)\n",retstr);
        } else retstr = clonestr("{\"result\":\"relays only relay\"}");
        //else sprintf(retbuf,"{\"error\":\"method %s not found\"}",methodstr);
        //portable_mutex_unlock(&plugin->mutex);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../agents/plugin777.c"
