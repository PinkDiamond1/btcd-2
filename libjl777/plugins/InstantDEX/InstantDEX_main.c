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

#define DEFINES_ONLY
#include "../agents/plugin777.c"
#include "../utils/NXT777.c"
#include "../includes/portable777.h"
#undef DEFINES_ONLY

#define INSTANTDEX_LOCALAPI "allorderbooks", "orderbook", "lottostats", "LSUM", "makebasket", "disable", "enable", "peggyrates", "tradesequence", "placebid", "placeask", "orderstatus", "openorders", "cancelorder", "tradehistory"

#define INSTANTDEX_REMOTEAPI "msigaddr", "bid", "ask", "makeoffer3", "respondtx"
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

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t numtx,int32_t updateNXT) // from poll_pending_offers via main
{
    char *retstr;
    return;
    printf("update_openorder iQ.%llu with numtx.%d updateNXT.%d | expires in %ld\n",(long long)iQ->quoteid,numtx,updateNXT,iQ->timestamp+iQ->duration-time(NULL));
    if ( (SUPERNET.automatch & 2) != 0 && (retstr= check_ordermatch(1,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,iQ)) != 0 )
    {
        printf("automatched order!\n");
        free(retstr);
    }
}

void poll_pending_offers(char *NXTaddr,char *NXTACCTSECRET)
{
}

uint32_t prices777_NXTBLOCK;
int32_t InstantDEX_idle(struct plugin_info *plugin)
{
    static double lastmilli; static cJSON *oldjson;
    char *jsonstr,*str; cJSON *json; uint32_t NXTblock; int32_t n = 0; uint32_t nonce;
    if ( INSTANTDEX.readyflag == 0 )
        return(0);
    if ( (jsonstr= queue_dequeue(&InstantDEXQ,1)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            //printf("Dequeued InstantDEX.(%s)\n",jsonstr);
            if ( (str= busdata_sync(&nonce,jsonstr,"allnodes",0)) != 0 )
                free(str);
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
    if ( is_native_crypto(name,assetid) != 0 )
        return((int32_t)strlen(name));
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

uint64_t prices777_equiv(uint64_t assetid)
{
    char *str;
    if ( (str= is_MGWasset(assetid)) != 0 )
        return(stringbits(str));
               return(assetid);
}

uint64_t InstantDEX_name(char *key,int32_t *keysizep,char *exchange,char *name,char *base,uint64_t *baseidp,char *rel,uint64_t *relidp)
{
    uint64_t baseid,relid,assetbits = 0; char *s,*str;
    baseid = *baseidp, relid = *relidp;
//printf(">>>>>> name.(%s) (%s/%s) %llu/%llu\n",name,base,rel,(long long)baseid,(long long)relid);
    if ( strcmp(base,"5527630") == 0 || baseid == 5527630 )
        strcpy(base,"NXT");
    if ( strcmp(rel,"5527630") == 0 || relid == 5527630 )
        strcpy(rel,"NXT");
    if ( strcmp("nxtae",exchange) == 0 || strcmp("unconf",exchange) == 0 || (baseid != 0 && relid != 0) )
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
        {
            get_assetname(base,baseid);
            //printf("mapped %llu -> (%s)\n",(long long)baseid,base);
        }
        if ( rel[0] == 0 )
        {
            get_assetname(rel,relid);
            //printf("mapped %llu -> (%s)\n",(long long)relid,rel);
        }
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
// printf("<<<<<<< name.(%s) (%s/%s) %llu/%llu\n",name,base,rel,(long long)baseid,(long long)relid);
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
        if ( (maxdepth= juint(json,"maxdepth")) <= 0 )
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
            retstr = InstantDEX_openorders(SUPERNET.NXTADDR,juint(json,"allorders"));
        else if ( strcmp(method,"cancelorder") == 0 )
            retstr = InstantDEX_cancelorder(orderid);
        else if ( strcmp(method,"orderstatus") == 0 )
            retstr = InstantDEX_orderstatus(orderid);
        else if ( strcmp(method,"tradehistory") == 0 )
            retstr = InstantDEX_tradehistory();
        else if ( strcmp(method,"lottostats") == 0 )
            retstr = jprint(Lottostats_json,0);
        else if ( strcmp(method,"tradesequence") == 0 )
            retstr = InstantDEX_tradesequence(json);
        else if ( strcmp(method,"makebasket") == 0 )
        {
            if ( (prices= prices777_makebasket(0,json,1,"basket")) != 0 )
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
        if ( strcmp(exchangestr,"active") == 0 && strcmp(method,"orderbook") == 0 )
            retstr = prices777_activebooks(name,base,rel,baseid,relid,maxdepth,allfields,juint(json,"tradeable"));
        if ( retstr == 0 && (prices= prices777_poll(exchangestr,name,base,baseid,rel,relid)) != 0 )
        {
            if ( prices777_equiv(prices->baseid) == prices777_equiv(baseid) && prices777_equiv(prices->relid) == prices777_equiv(relid) )
                invert = 0;
            else if ( prices777_equiv(prices->baseid) == prices777_equiv(relid) && prices777_equiv(prices->relid) == prices777_equiv(baseid) )
                invert = 1;
            else invert = 0, printf("baserel not matching (%s %s) %llu %llu vs (%s %s) %llu %llu\n",prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,base,rel,(long long)baseid,(long long)relid);
            if ( strcmp(method,"placebid") == 0 || strcmp(method,"placeask") == 0 )
            {
                if ( strcmp(method,"placebid") == 0 )
                    dir = 1 - invert*2;
                else dir = -(1 - invert*2);
                return(InstantDEX_quote(localaccess,remoteaddr,prices,dir,jdouble(json,"price"),jdouble(json,"volume"),orderid,juint(json,"minperc"),juint(json,"automatch"),juint(json,"duration"),jstr(json,"gui")));
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
                if ( retstr != 0 )
                    retstr = clonestr(retstr);
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
        //if ( Debuglevel > 2 )
            printf("(%s) %p exchange.(%s) base.(%s) %llu rel.(%s) %llu | name.(%s) %llu\n",retstr!=0?retstr:"",prices,exchangestr,base,(long long)baseid,rel,(long long)relid,name,(long long)assetbits);
    }
    return(retstr);
}

int32_t bidask_parse(struct InstantDEX_quote *iQ,int32_t dir,cJSON *json,char *origargstr)
{
    char gui[MAX_JSON_FIELD],exchangestr[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],name[MAX_JSON_FIELD],key[512];
    double price,volume; int32_t keysize,automatch; struct exchange_info *exchange;
    memset(iQ,0,sizeof(*iQ));
    iQ->baseid = j64bits(json,"baseid"); iQ->relid = j64bits(json,"relid");
    volume = jdouble(json,"volume"); price = jdouble(json,"price");
    iQ->timestamp = juint(json,"timestamp");
    iQ->baseamount = j64bits(json,"baseamount");
    iQ->relamount = j64bits(json,"relamount");
    copy_cJSON(gui,jobj(json,"gui")), strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    automatch = juint(json,"automatch");
    iQ->minperc = juint(json,"minperc");
    iQ->duration = juint(json,"duration");
    copy_cJSON(exchangestr,jobj(json,"exchange"));//, iQ->exchangeid = exchangeid(exchange);
    InstantDEX_name(key,&keysize,exchangestr,name,base,&iQ->baseid,rel,&iQ->relid);
    if ( (exchange= exchange_find(exchangestr)) != 0 )
        iQ->exchangeid = exchange->exchangeid;
    iQ->nxt64bits = j64bits(json,"offerNXT");
    copy_cJSON(base,jobj(json,"base")), copy_cJSON(rel,jobj(json,"rel")), copy_cJSON(name,jobj(json,"name"));
    iQ->isask = (dir < 0);
    return(0);
}

char *bid_func(int32_t localaccess,int32_t valid,char *sender,cJSON *json,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD],*checkstr; struct InstantDEX_quote iQ;
    copy_cJSON(offerNXT,jobj(json,"offerNXT"));
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
    {
        if ( bidask_parse(&iQ,1,json,origargstr) >= 0 )
        {
            if ( (checkstr= placequote_str(&iQ,jdouble(json,"price"),jdouble(json,"volume"))) != 0 )
                printf("NETWORKBID.(%s) -> (%s)\n",origargstr,checkstr), free(checkstr);
            create_iQ(&iQ);
        }
    } else printf("got my bid from network (%s)\n",origargstr);
    return(0);
}

char *ask_func(int32_t localaccess,int32_t valid,char *sender,cJSON *json,char *origargstr)
{
    char offerNXT[MAX_JSON_FIELD],*checkstr; struct InstantDEX_quote iQ;
    copy_cJSON(offerNXT,jobj(json,"offerNXT"));
    if ( strcmp(SUPERNET.NXTADDR,offerNXT) != 0 )
    {
        if ( bidask_parse(&iQ,-1,json,origargstr) >= 0 )
        {
            if ( (checkstr= placequote_str(&iQ,jdouble(json,"price"),jdouble(json,"volume"))) != 0 )
                printf("NETWORKASK.(%s) -> (%s)\n",origargstr,checkstr), free(checkstr);
            create_iQ(&iQ);
        }
    } else printf("got my ask from network (%s)\n",origargstr);
    return(0);
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
    find_exchange(0,INSTANTDEX_ACTIVENAME);
    for (i=0; i<sizeof(exchanges)/sizeof(*exchanges); i++)
        find_exchange(0,exchanges[i]);
    prices777_initpair(-1,0,0,0,0,0.,0,0,0,0);
    if ( (array= jarray(&n,json,"baskets")) != 0 )
    {
        for (i=0; i<n; i++)
            prices777_makebasket(0,jitem(array,i),1,"basket");
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
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s) initflag.%d\n",plugin->name,jsonstr,initflag);
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
            if ( strcmp(methodstr,"bid") == 0 )
                retstr = bid_func(0,1,sender,json,jsonstr);
            else if ( strcmp(methodstr,"ask") == 0 )
                retstr = ask_func(0,1,sender,json,jsonstr);
        } else retstr = clonestr("{\"result\":\"relays only relay\"}");
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
