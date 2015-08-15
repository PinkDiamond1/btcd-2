//
//  quotes.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_quotes_h
#define xcode_quotes_h

#ifdef oldway
struct normal_fields { uint64_t nxt64bits,quoteid; struct InstantDEX_quote *baseiQ,*reliQ; };
union quotefields { struct normal_fields normal; };

struct InstantDEX_quote *AllQuotes;

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 1; }

uint64_t calc_quoteid(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote Q;
    if ( iQ == 0 )
        return(0);
    if ( iQ->quoteid == 0 )
    {
        Q = *iQ;
        Q.baseiQ = (struct InstantDEX_quote *)calc_quoteid(iQ->baseiQ);
        Q.reliQ = (struct InstantDEX_quote *)calc_quoteid(iQ->reliQ);
        clear_InstantDEX_quoteflags(&Q);
        if ( Q.isask != 0 )
        {
            Q.baseid = iQ->relid, Q.baseamount = iQ->relamount;
            Q.relid = iQ->baseid, Q.relamount = iQ->baseamount;
            Q.isask = Q.minperc = 0;
        }
        return(calc_txid((uint8_t *)((long)&Q + sizeof(Q.hh) + sizeof(Q.quoteid)),sizeof(Q) - sizeof(Q.hh) - sizeof(Q.quoteid)));
    } return(iQ->quoteid);
}

uint64_t get_iQ_jumpasset(struct InstantDEX_quote *iQ)
{
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
    {
        if ( iQ->baseiQ->baseid == iQ->reliQ->baseid || iQ->baseiQ->baseid == iQ->reliQ->relid )
            return(iQ->baseiQ->baseid);
        else if ( iQ->baseiQ->relid == iQ->reliQ->relid || iQ->baseiQ->relid == iQ->reliQ->baseid )
            return(iQ->baseiQ->relid);
    }
    return(0);
}

int32_t make_jumpiQ(uint64_t refbaseid,uint64_t refrelid,int32_t flip,struct InstantDEX_quote *iQ,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ,char *gui,int32_t duration)
{
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel;
    double vol;
    char exchange[64];
    uint32_t timestamp;
    frombase = baseiQ->baseamount, fromrel = baseiQ->relamount;
    tobase = reliQ->baseamount, torel = reliQ->relamount;
    if ( make_jumpquote(refbaseid,refrelid,&baseamount,&relamount,&frombase,&fromrel,&tobase,&torel) == 0. )
        return(0);
    if ( (timestamp= reliQ->timestamp) > baseiQ->timestamp )
        timestamp = baseiQ->timestamp;
    iQ_exchangestr(exchange,iQ);
    create_InstantDEX_quote(iQ,timestamp,0,calc_quoteid(baseiQ) ^ calc_quoteid(reliQ),0.,0.,refbaseid,baseamount,refrelid,relamount,exchange,0,gui,baseiQ,reliQ,duration);
    if ( Debuglevel > 2 )
        printf("jump%s: %f (%llu/%llu) %llu %llu (%f %f) %llu %llu\n",flip==0?"BID":"ASK",calc_price_volume(&vol,iQ->baseamount,iQ->relamount),(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel,calc_price_volume(&vol,frombase,fromrel),calc_price_volume(&vol,tobase,torel),(long long)tobase,(long long)torel);
    iQ->isask = flip;
    iQ->minperc = baseiQ->minperc;
    if ( reliQ->minperc > iQ->minperc )
        iQ->minperc = reliQ->minperc;
    return(1);
}
#else

int32_t prices777_conviQ(struct prices777_order *order,struct InstantDEX_quote *iQ)
{
    struct prices777 *prices; int32_t inverted;
    memset(order,0,sizeof(*order));
    if ( (prices= prices777_find(&inverted,iQ->baseid,iQ->relid,INSTANTDEX_NAME)) != 0 )
    {
        order->source = prices;
        order->price = prices777_price_volume(&order->vol,iQ->baseamount,iQ->relamount);
        order->timestamp = iQ->timestamp;
        if ( iQ->isask != 0 )
            order->wt = -1;
        else order->wt = 1;
        order->id = order->quoteid = iQ->quoteid;
        return(0);
    }
    return(-1);
}

cJSON *prices777_orderjson(struct prices777_order *order)
{
    cJSON *item = cJSON_CreateArray();
    jaddinum(item,order->price);
    jaddinum(item,order->vol);
    jaddinum(item,order->id);
    return(item);
}

struct InstantDEX_quote *AllQuotes;

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->closed = 1; }

int32_t iQcmp(struct InstantDEX_quote *iQA,struct InstantDEX_quote *iQB)
{
    if ( iQA->isask == iQB->isask && iQA->baseid == iQB->baseid && iQA->relid == iQB->relid && iQA->baseamount == iQB->baseamount && iQA->relamount == iQB->relamount )
        return(0);
    else if ( iQA->isask != iQB->isask && iQA->baseid == iQB->relid && iQA->relid == iQB->baseid && iQA->baseamount == iQB->relamount && iQA->relamount == iQB->baseamount )
        return(0);
    return(-1);
}

uint64_t calc_quoteid(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote Q;
    if ( iQ == 0 )
        return(0);
    if ( iQ->quoteid == 0 )
    {
        Q = *iQ;
        //Q.baseiQ = (struct InstantDEX_quote *)calc_quoteid(iQ->baseiQ);
        //Q.reliQ = (struct InstantDEX_quote *)calc_quoteid(iQ->reliQ);
        clear_InstantDEX_quoteflags(&Q);
        if ( Q.isask != 0 )
        {
            Q.baseid = iQ->relid, Q.baseamount = iQ->relamount;
            Q.relid = iQ->baseid, Q.relamount = iQ->baseamount;
            Q.isask = Q.minperc = 0;
        }
        return(calc_txid((uint8_t *)((long)&Q + sizeof(Q.hh) + sizeof(Q.quoteid)),sizeof(Q) - sizeof(Q.hh) - sizeof(Q.quoteid)));
    } return(iQ->quoteid);
}

struct InstantDEX_quote *find_iQ(uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    HASH_FIND(hh,AllQuotes,&quoteid,sizeof(quoteid),iQ);
    return(iQ);
}

struct InstantDEX_quote *delete_iQ(uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= find_iQ(quoteid)) != 0 )
    {
        HASH_DELETE(hh,AllQuotes,iQ);
        cancel_InstantDEX_quote(iQ);
    }
    return(iQ);
}

struct InstantDEX_quote *findquoteid(uint64_t quoteid,int32_t evenclosed)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= find_iQ(quoteid)) != 0 )
    {
        if ( evenclosed != 0 || iQ->closed == 0 )
        {
            if ( calc_quoteid(iQ) == quoteid )
                return(iQ);
            else printf("calc_quoteid %llu vs %llu\n",(long long)calc_quoteid(iQ),(long long)quoteid);
        } //else printf("quoteid.%llu closed.%d\n",(long long)quoteid,iQ->closed);
    } else printf("couldnt find %llu\n",(long long)quoteid);
    return(0);
}

struct InstantDEX_quote *create_iQ(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote *newiQ; struct prices777 *prices; int32_t inverted;
    if ( (newiQ= find_iQ(iQ->quoteid)) != 0 )
        return(newiQ);
    newiQ = calloc(1,sizeof(*newiQ));
    *newiQ = *iQ;
    HASH_ADD(hh,AllQuotes,quoteid,sizeof(newiQ->quoteid),newiQ);
    if ( (prices= prices777_find(&inverted,iQ->baseid,iQ->relid,INSTANTDEX_NAME)) != 0 )
        prices->dirty++;
    {
        struct InstantDEX_quote *checkiQ;
        if ( (checkiQ= find_iQ(iQ->quoteid)) == 0 || iQcmp(iQ,checkiQ) != 0 )//memcmp((uint8_t *)((long)checkiQ + sizeof(checkiQ->hh) + sizeof(checkiQ->quoteid)),(uint8_t *)((long)iQ + sizeof(iQ->hh) + sizeof(iQ->quoteid)),sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->quoteid)) != 0 )
        {
            int32_t i;
            for (i=(sizeof(iQ->hh) - sizeof(iQ->quoteid)); i<sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->quoteid); i++)
                printf("%02x ",((uint8_t *)iQ)[i]);
            printf("iQ\n");
            for (i=(sizeof(checkiQ->hh) + sizeof(checkiQ->quoteid)); i<sizeof(*checkiQ) - sizeof(checkiQ->hh) - sizeof(checkiQ->quoteid); i++)
                printf("%02x ",((uint8_t *)checkiQ)[i]);
            printf("checkiQ\n");
            printf("error finding iQ after adding %llu vs %llu\n",(long long)checkiQ->quoteid,(long long)iQ->quoteid);
        }
    }
    return(newiQ);
}

int32_t iQ_exchangestr(char *exchange,struct InstantDEX_quote *iQ)
{
    /*char basexchg[64],relxchg[64];
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
    {
        strcpy(basexchg,exchange_str(iQ->baseiQ->exchangeid));
        strcpy(relxchg,exchange_str(iQ->reliQ->exchangeid));
        sprintf(exchange,"%s_%s",basexchg,relxchg);
    }
    else*/
        if ( iQ->exchangeid == INSTANTDEX_NXTAEID && iQ->timestamp > time(NULL) )
        strcpy(exchange,INSTANTDEX_NXTAEUNCONF);
    else strcpy(exchange,exchange_str(iQ->exchangeid));
    return(0);
}

struct tradehistory { uint64_t assetid,purchased,sold; };

struct tradehistory *_update_tradehistory(struct tradehistory *hist,uint64_t assetid,uint64_t purchased,uint64_t sold)
{
    int32_t i = 0;
    if ( hist == 0 )
        hist = calloc(1,sizeof(*hist));
    if ( hist[i].assetid != 0 )
    {
        for (i=0; hist[i].assetid!=0; i++)
            if ( hist[i].assetid == assetid )
                break;
    }
    if ( hist[i].assetid == 0 )
    {
        hist = realloc(hist,(i+2) * sizeof(*hist));
        memset(&hist[i],0,2 * sizeof(hist[i]));
        hist[i].assetid = assetid;
    }
    if ( hist[i].assetid == assetid )
    {
        hist[i].purchased += purchased;
        hist[i].sold += sold;
        printf("hist[%d] %llu +%llu -%llu -> (%llu %llu)\n",i,(long long)hist[i].assetid,(long long)purchased,(long long)sold,(long long)hist[i].purchased,(long long)hist[i].sold);
    } else printf("_update_tradehistory: impossible case!\n");
    return(hist);
}

struct tradehistory *update_tradehistory(struct tradehistory *hist,uint64_t srcasset,uint64_t srcamount,uint64_t destasset,uint64_t destamount)
{
    hist = _update_tradehistory(hist,srcasset,0,srcamount);
    hist = _update_tradehistory(hist,destasset,destamount,0);
    return(hist);
}

cJSON *_tradehistory_json(struct tradehistory *asset)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    sprintf(numstr,"%llu",(long long)asset->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased)), cJSON_AddItemToObject(json,"purchased",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->sold)), cJSON_AddItemToObject(json,"sold",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased) - dstr(asset->sold)), cJSON_AddItemToObject(json,"net",cJSON_CreateString(numstr));
    return(json);
}

cJSON *tradehistory_json(struct tradehistory *hist,cJSON *array)
{
    int32_t i; char assetname[64],numstr[64]; cJSON *assets,*netpos,*item,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"rawtrades",array);
    assets = cJSON_CreateArray();
    netpos = cJSON_CreateArray();
    for (i=0; hist[i].assetid!=0; i++)
    {
        cJSON_AddItemToArray(assets,_tradehistory_json(&hist[i]));
        item = cJSON_CreateObject();
        get_assetname(assetname,hist[i].assetid);
        cJSON_AddItemToObject(item,"asset",cJSON_CreateString(assetname));
        sprintf(numstr,"%.8f",dstr(hist[i].purchased) - dstr(hist[i].sold)), cJSON_AddItemToObject(item,"net",cJSON_CreateString(numstr));
        cJSON_AddItemToArray(netpos,item);
    }
    cJSON_AddItemToObject(json,"assets",assets);
    cJSON_AddItemToObject(json,"netpositions",netpos);
    return(json);
}

cJSON *tabulate_trade_history(uint64_t mynxt64bits,cJSON *array)
{
    int32_t i,n;
    cJSON *item;
    long balancing;
    struct tradehistory *hist = 0;
    uint64_t src64bits,srcamount,srcasset,dest64bits,destamount,destasset,jump64bits,jumpamount,jumpasset;
    //{"requestType":"processjumptrade","NXT":"5277534112615305538","assetA":"5527630","amountA":"6700000000","other":"1510821971811852351","assetB":"12982485703607823902","amountB":"100000000","feeA":"250000000","balancing":0,"feeAtxid":"1234468909119892020","triggerhash":"34ea5aaeeeb62111a825a94c366b4ae3d12bb73f9a3413a27d1b480f6029a73c"}
    if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            src64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"NXT"));
            srcamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountA"));
            srcasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetA"));
            dest64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"other"));
            destamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountB"));
            destasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetB"));
            jump64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumper"));
            jumpamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpasset"));
            jumpasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpamount"));
            balancing = (long)get_API_int(cJSON_GetObjectItem(item,"balancing"),0);
            if ( src64bits != 0 && srcamount != 0 && srcasset != 0 && dest64bits != 0 && destamount != 0 && destasset != 0 )
            {
                if ( src64bits == mynxt64bits )
                    hist = update_tradehistory(hist,srcasset,srcamount,destasset,destamount);
                else if ( dest64bits == mynxt64bits )
                    hist = update_tradehistory(hist,destasset,destamount,srcasset,srcamount);
                else if ( jump64bits == mynxt64bits )
                    continue;
                else printf("illegal tabulate_trade_entry %llu: (%llu -> %llu) via %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)dest64bits,(long long)jump64bits);
            } else printf("illegal tabulate_trade_entry %llu: %llu %llu %llu || %llu %llu %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)srcamount,(long long)srcasset,(long long)dest64bits,(long long)destamount,(long long)destasset);
        }
    }
    if ( hist != 0 )
    {
        array = tradehistory_json(hist,array);
        free(hist);
    }
    return(array);
}

cJSON *get_tradehistory(char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],message[MAX_JSON_FIELD],newtriggerhash[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*msgobj,*attachment,*retjson = 0,*histarray = 0;
    int32_t i,j,n,m,duplicates = 0;
    uint64_t senderbits;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getBlockchainTransactions&account=%s&timestamp=%u&withMessage=true",refNXTaddr,timestamp);
    if ( (jsonstr= issue_NXTPOST(cmdstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                    {
                        expand_nxt64bits(NXTaddr,senderbits);
                        if ( refNXTaddr != 0 && strcmp(NXTaddr,refNXTaddr) == 0 )
                        {
                            if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 && (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                            {
                                copy_cJSON(message,msgobj);
                                //printf("(%s) -> ",message);
                                unstringify(message);
                                if ( (msgobj= cJSON_Parse(message)) != 0 )
                                {
                                    //printf("(%s)\n",message);
                                    if ( histarray == 0 )
                                        histarray = cJSON_CreateArray(), j = m = 0;
                                    else
                                    {
                                        copy_cJSON(newtriggerhash,cJSON_GetObjectItem(msgobj,"triggerhash"));
                                        m = cJSON_GetArraySize(histarray);
                                        for (j=0; j<m; j++)
                                        {
                                            copy_cJSON(triggerhash,cJSON_GetObjectItem(cJSON_GetArrayItem(histarray,j),"triggerhash"));
                                            if ( strcmp(triggerhash,newtriggerhash) == 0 )
                                            {
                                                duplicates++;
                                                break;
                                            }
                                        }
                                    }
                                    if ( j == m )
                                        cJSON_AddItemToArray(histarray,msgobj);
                                } else printf("parse error on.(%s)\n",message);
                            }
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( histarray != 0 )
        retjson = tabulate_trade_history(calc_nxt64bits(refNXTaddr),histarray);
    printf("duplicates.%d\n",duplicates);
    return(retjson);
}

char *InstantDEX_tradehistory()
{
    cJSON *history,*json;
    json = cJSON_CreateObject();
    history = get_tradehistory(SUPERNET.NXTADDR,0);
    if ( history != 0 )
        cJSON_AddItemToObject(json,"tradehistory",history);
    return(jprint(json,1));
}

char *check_ordermatch(int32_t polling,char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *refiQ)
{
    return(0);
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->nxt64bits == calc_nxt64bits(NXTaddr) && iQ->exchangeid == INSTANTDEX_EXCHANGEID )
    {
        delete_iQ(quoteid);
        return(1);
    }
    return(0);
}

char *InstantDEX_cancelorder(uint64_t quoteid)
{
    struct InstantDEX_quote *iQ; char *retstr;
    if ( (retstr= cancel_NXTorderid(SUPERNET.NXTADDR,quoteid)) != 0 )
    {
        if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->nxt64bits == calc_nxt64bits(SUPERNET.NXTADDR) )
            cancel_InstantDEX_quote(iQ);
        return(retstr);
    }
    if ( cancelquote(SUPERNET.NXTADDR,quoteid) > 0 )
        return(clonestr("{\"result\":\"quote cancelled\"}"));
    else return(clonestr("{\"result\":\"couldnt cancel order order\"}"));
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

char *prices777_trade(char *exchangestr,char *base,char *rel,int32_t dir,double price,double volume)
{
    char *retstr; struct exchange_info *exchange;
    printf("check %s/%s\n",base,rel);
    if ( (exchange= find_exchange(0,exchangestr)) != 0 )
    {
        if ( exchange->trade != 0 )
        {
            printf(" issue dir.%d %s/%s price %f vol %f -> %s\n",dir,base,rel,price,volume,exchangestr);
            (*exchange->trade)(&retstr,exchange,base,rel,dir,price,volume);
            return(retstr);
        } else return(clonestr("{\"error\":\"no trade function for exchange\"}\n"));
    } else return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
}

cJSON *InstantDEX_tradejson(struct prices777_order *order,int32_t dotrade)
{
    char buf[1024],*retstr,*exchange; uint64_t qty,avail,priceNQT; struct prices777 *prices; cJSON *json = 0;
    if ( (prices= order->source) != 0 )
    {
        exchange = prices->exchange;
        if ( dotrade == 0 )
        {
            sprintf(buf,"{\"trade\":\"%s\",\"exchange\":\"%s\",\"base\":\"%s\",\"rel\":\"%s\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"price\":%.8f,\"volume\":%.8f}",order->wt > 0. ? "buy" : "sell",exchange,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,order->price,order->vol);
            if ( strcmp(exchange,"nxtae") == 0 )
            {
                qty = calc_asset_qty(&avail,&priceNQT,SUPERNET.NXTADDR,0,order->assetid,order->price,order->vol);
                sprintf(buf+strlen(buf)-1,",\"priceNQT\":\"%llu\",\"quantityQNT\":\"%llu\",\"avail\":\"%llu\"}",(long long)priceNQT,(long long)qty,(long long)avail);
                if ( qty == 0 )
                    sprintf(buf+strlen(buf)-1,",\"error\":\"insufficient balance\"}");
            }
            return(cJSON_Parse(buf));
        }
        if ( strcmp(exchange,"nxtae") == 0 )
            retstr = fill_nxtae(SUPERNET.my64bits,order->wt,order->price,order->vol,prices->baseid,prices->relid);
        else retstr = prices777_trade(exchange,prices->base,prices->rel,order->wt,order->price,order->vol);
        if ( retstr != 0 )
        {
            json = cJSON_Parse(retstr);
            free(retstr);
        }
    }
    return(json);
}

cJSON *gen_InstantDEX_json(int32_t localaccess,uint64_t baseamount,uint64_t relamount,int32_t flip,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid,uint64_t jumpasset)
{
    cJSON *json = 0; char numstr[64],base[64],rel[64],exchange[64]; double price,volume; int32_t minperc;
    minperc = (iQ->minperc != 0) ? iQ->minperc : INSTANTDEX_MINVOL;
    json = cJSON_CreateObject();
    //cJSON_AddItemToObject(json,"askoffer",cJSON_CreateNumber(flip));
    cJSON_AddItemToObject(json,"method",cJSON_CreateString("makeoffer3"));
    cJSON_AddItemToObject(json,"plugin",cJSON_CreateString("InstantDEX"));
    get_assetname(base,refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
    get_assetname(rel,refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
    cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(iQ->timestamp));
    cJSON_AddItemToObject(json,"duration",cJSON_CreateNumber(iQ->duration));
    cJSON_AddItemToObject(json,"age",cJSON_CreateNumber((uint32_t)time(NULL) - iQ->timestamp));
    if ( iQ->matched != 0 )
        cJSON_AddItemToObject(json,"matched",cJSON_CreateNumber(1));
    if ( iQ->sent != 0 )
        cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(1));
    if ( iQ->closed != 0 )
        cJSON_AddItemToObject(json,"closed",cJSON_CreateNumber(1));
    iQ_exchangestr(exchange,iQ), cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
    if ( iQ->nxt64bits != 0 )
        sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr)), cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)refbaseid), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)refrelid), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    price = prices777_price_volume(&volume,baseamount,relamount);
    cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price));
    cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
    sprintf(numstr,"%llu",(long long)baseamount), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)relamount), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
    if ( iQ->gui[0] != 0 )
        cJSON_AddItemToObject(json,"gui",cJSON_CreateString(iQ->gui));
    cJSON_AddItemToObject(json,"minperc",cJSON_CreateNumber(minperc));
    if ( 0 && (baseamount < min_asset_amount(refbaseid) || relamount < min_asset_amount(refrelid)) )
    {
        if ( Debuglevel > 2 )
            printf("%.8f < %.8f || rel %.8f < %.8f\n",dstr(baseamount),dstr(min_asset_amount(refbaseid)),dstr(relamount),dstr(min_asset_amount(refrelid)));
        if ( baseamount < min_asset_amount(refbaseid) )
            sprintf(numstr,"%llu",(long long)min_asset_amount(refbaseid)), cJSON_AddItemToObject(json,"minbase_error",cJSON_CreateString(numstr));
        if ( relamount < min_asset_amount(refrelid) )
            sprintf(numstr,"%llu",(long long)min_asset_amount(refrelid)), cJSON_AddItemToObject(json,"minrel_error",cJSON_CreateString(numstr));
    }
    return(json);
}

char *InstantDEX_orderstatus(uint64_t orderid)
{
    cJSON *item; struct InstantDEX_quote *iQ = 0;
    if ( (iQ= find_iQ(orderid)) != 0 )
    {
        if ( (item= gen_InstantDEX_json(0,iQ->baseamount,iQ->relamount,iQ->isask,iQ,iQ->baseid,iQ->relid,0)) != 0 )
            return(jprint(item,1));
    }
    return(clonestr("{\"error\":\"couldnt find orderid\"}"));
}

char *InstantDEX_openorders(char *NXTaddr,int32_t allorders)
{
    struct InstantDEX_quote *iQ,*tmp; cJSON *json,*array,*item; uint64_t nxt64bits = calc_nxt64bits(NXTaddr);
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    HASH_ITER(hh,AllQuotes,iQ,tmp)
    {
        if ( iQ->nxt64bits == nxt64bits && (allorders != 0 || iQ->closed == 0) )
        {
            if ( (item= gen_InstantDEX_json(0,iQ->baseamount,iQ->relamount,iQ->isask,iQ,iQ->baseid,iQ->relid,0)) != 0 )
                jaddi(array,item);
        }
    }
    jadd(json,"openorders",array);
    return(jprint(json,1));
}

int _decreasing_quotes(const void *a,const void *b)
{
#define order_a ((struct prices777_order *)a)
#define order_b ((struct prices777_order *)b)
 	if ( order_b->price > order_a->price )
		return(1);
	else if ( order_b->price < order_a->price )
		return(-1);
	return(0);
#undef order_a
#undef order_b
}

int _increasing_quotes(const void *a,const void *b)
{
#define order_a ((struct prices777_order *)a)
#define order_b ((struct prices777_order *)b)
 	if ( order_b->price > order_a->price )
		return(-1);
	else if ( order_b->price < order_a->price )
		return(1);
	return(0);
#undef order_a
#undef order_b
}

cJSON *InstantDEX_orderbook(struct prices777 *prices)
{
    struct InstantDEX_quote *iQ,*tmp; cJSON *json,*bids,*asks; int32_t i,isask,iter,n,m,numbids,numasks; double price,volume;
    struct prices777_order *bidvals,*askvals,order;
    json = cJSON_CreateObject(), bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    for (iter=numbids=numasks=n=m=0; iter<2; iter++)
    {
        HASH_ITER(hh,AllQuotes,iQ,tmp)
        {
            //printf("iterate quote.%llu\n",(long long)iQ->quoteid);
            isask = iQ->isask;
            if ( prices777_equiv(iQ->baseid) == prices777_equiv(prices->baseid) && prices777_equiv(iQ->relid) == prices777_equiv(prices->relid) )
                price = prices777_price_volume(&volume,iQ->baseamount,iQ->relamount);
            else if ( prices777_equiv(iQ->relid) == prices777_equiv(prices->baseid) && prices777_equiv(iQ->baseid) == prices777_equiv(prices->relid) )
                isask ^= 1, price = prices777_price_volume(&volume,iQ->relamount,iQ->baseamount);
            else continue;
            if ( iter == 0 )
            {
                if ( isask != 0 )
                    numasks++;
                else numbids++;
            }
            else
            {
                prices777_conviQ(&order,iQ);
                if ( isask == 0 && n < numbids )
                    bidvals[n++] = order;
                else if ( isask != 0 && m < numasks )
                    askvals[m++] = order;
            }
        }
        if ( iter == 0 )
        {
            if ( numbids > 0 )
                bidvals = calloc(numbids,sizeof(*bidvals));
            if ( numasks > 0 )
                askvals = calloc(numasks,sizeof(*askvals));
        }
    }
    if ( numbids > 0 )
    {
        if ( n > 0 )
        {
            qsort(bidvals,n,sizeof(*bidvals),_decreasing_quotes);
            for (i=0; i<n; i++)
                jaddi(bids,prices777_orderjson(&bidvals[i]));
        }
        free(bidvals);
    }
    if ( numasks > 0 )
    {
        if ( m > 0 )
        {
            qsort(askvals,m,sizeof(*askvals),_increasing_quotes);
            for (i=0; i<m; i++)
                jaddi(asks,prices777_orderjson(&askvals[i]));
        }
        free(askvals);
    }
    jadd(json,"bids",bids), jadd(json,"asks",asks);
    return(json);
}

char *submitquote_str(int32_t localaccess,struct InstantDEX_quote *iQ,uint64_t baseid,uint64_t relid)
{
    cJSON *json;
    char *jsonstr = 0;
    if ( (json= gen_InstantDEX_json(localaccess,iQ->baseamount,iQ->relamount,iQ->isask,iQ,baseid,relid,0)) != 0 )
    {
        ensure_jsonitem(json,"plugin","relay");
        ensure_jsonitem(json,"destplugin","InstantDEX");
        ensure_jsonitem(json,"method","busdata");
        ensure_jsonitem(json,"submethod",(iQ->isask != 0) ? "ask" : "bid");
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        free_json(json);
    } else printf("gen_InstantDEX_json returns null\n");
    return(jsonstr);
}

char *placequote_str(struct InstantDEX_quote *iQ,double price,double volume)
{
    char iQstr[1024],exchangestr[64],name[64],base[64],rel[64],buf[MAX_JSON_FIELD],key[512]; int32_t keysize; uint64_t baseid,relid;
    init_hexbytes_noT(iQstr,(uint8_t *)iQ,sizeof(*iQ));
    iQ_exchangestr(exchangestr,iQ);
    name[0] = base[0] = rel[0] = 0;
    baseid = iQ->baseid, relid = iQ->relid;
    InstantDEX_name(key,&keysize,exchangestr,name,base,&baseid,rel,&relid);
    sprintf(buf,"{\"quoteid\":\"%llu\",\"base\":\"%s\",\"baseid\":\"%llu\",\"baseamount\":\"%llu\",\"rel\":\"%s\",\"relid\":\"%llu\",\"relamount\":\"%llu\",\"price\":%.8f,\"volume\":%.8f,\"offerNXT\":\"%llu\",\"timestamp\":\"%u\",\"isask\":\"%u\",\"exchange\":\"%s\",\"gui\":\"%s\",\"plugin\":\"relay\",\"destplugin\":\"InstantDEX\",\"method\":\"busdata\",\"submethod\":\"%s\"}",(long long)iQ->quoteid,base,(long long)iQ->baseid,(long long)iQ->baseamount,rel,(long long)iQ->relid,(long long)iQ->relamount,price,volume,(long long)iQ->nxt64bits,iQ->timestamp,iQ->isask,exchangestr,iQ->gui,(iQ->isask != 0) ? "ask" : "bid");
    return(clonestr(buf));
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,uint64_t quoteid,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,uint64_t nxt64bits,char *gui,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ,int32_t duration)
{
    memset(iQ,0,sizeof(*iQ));
    if ( baseamount == 0 && relamount == 0 )
    {
        if ( price < SMALLVAL )
            return(-1);
        set_best_amounts(&baseamount,&relamount,price,volume);
    }
    iQ->timestamp = timestamp;
    if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
        duration = ORDERBOOK_EXPIRATION;
    iQ->duration = duration;
    iQ->isask = isask;
    iQ->nxt64bits = nxt64bits;
    //iQ->baseiQ = baseiQ;
    //iQ->reliQ = reliQ;
    iQ->baseid = baseid, iQ->baseamount = baseamount;
    iQ->relid = relid, iQ->relamount = relamount;
    if ( gui != 0 )
        strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    iQ->exchangeid = INSTANTDEX_EXCHANGEID;
    if ( (iQ->quoteid= quoteid) == 0 )
        iQ->quoteid = calc_quoteid(iQ);
    return(0);
}

int32_t InstantDEX_setiQ(struct InstantDEX_quote *iQ,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,uint64_t baseid,uint64_t relid,double refprice,double refvolume,uint64_t baseamount,uint64_t relamount,char *gui,uint64_t quoteid,int32_t duration)
{
    double price,volume; uint64_t basemult,relmult;
    memset(iQ,0,sizeof(*iQ));
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( baseamount == 0 || relamount == 0 )
        set_best_amounts(&baseamount,&relamount,refprice,refvolume);
    basemult = get_assetmult(baseid), relmult = get_assetmult(relid);
    baseamount = (baseamount + basemult/2) / basemult, baseamount *= basemult;
    relamount = (relamount + relmult/2) / relmult, relamount *= relmult;
    if ( refprice != 0. && refvolume != 0 )
    {
        price = prices777_price_volume(&volume,baseamount,relamount);
        if ( fabs(refprice - price)/price > 0.001 )
        {
            printf("cant create accurate price ref.(%f %f) -> (%f %f)\n",refprice,refvolume,price,volume);
            return(0);
        }
    }
    printf("refprice %f refvol %f\n",refprice,refvolume);
    create_InstantDEX_quote(iQ,timestamp,dir < 0,quoteid,0,0,baseid,baseamount,relid,relamount,nxt64bits,gui,0,0,duration);
    if ( iQ->exchangeid != INSTANTDEX_EXCHANGEID )
        iQ->minperc = 1;
    return(0);
}

char *InstantDEX_quote(int32_t localaccess,char *remoteaddr,struct prices777 *prices,int32_t dir,double price,double volume,uint64_t quoteid,uint32_t minperc,uint32_t automatch,uint32_t duration,char *gui)
{
    extern queue_t InstantDEXQ;
    double minbasevol,minrelvol; char buf[4096],*retstr=0; struct InstantDEX_quote iQ;
    if ( strcmp(prices->exchange,"nxtae") == 0 )
        return(fill_nxtae(SUPERNET.my64bits,dir,price,volume,prices->baseid,prices->relid));
    else if ( strcmp(prices->exchange,"InstantDEX") != 0 )
        return(prices777_trade(prices->exchange,prices->base,prices->rel,dir,price,volume));
    minbasevol = get_minvolume(prices->baseid), minrelvol = get_minvolume(prices->relid);
    /*if ( volume < minbasevol || (volume * price) < minrelvol )
    {
        sprintf(buf,"{\"error\":\"not enough volume\",\"price\":%f,\"volume\":%f,\"minbasevol\":%f,\"minrelvol\":%f,\"relvol\":%f}",price,volume,minbasevol,minrelvol,price*volume);
        return(clonestr(buf));
    }*/
    if ( (remoteaddr != 0 && remoteaddr[0] != 0) || localaccess != 0 )
    {
        if ( price > SMALLVAL && volume > SMALLVAL && dir != 0 )
        {
            if ( InstantDEX_setiQ(&iQ,SUPERNET.my64bits,0,dir,prices->baseid,prices->relid,price,volume,0,0,gui,0,duration) == 0 )
            {
                iQ.minperc = minperc;
                if ( (quoteid= calc_quoteid(&iQ)) != 0 )
                {
                    retstr = placequote_str(&iQ,price,volume);
                    if ( Debuglevel > 2 )
                        printf("placequote.(%s) localaccess.%d\n",retstr,localaccess);
                    if ( localaccess != 0 )
                    {
                        if ( automatch != 0 && (SUPERNET.automatch & 1) != 0 && (retstr= check_ordermatch(0,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,&iQ)) != 0 )
                            return(retstr);
                        else printf("skip automatch.%d %d\n",automatch,SUPERNET.automatch);
                        create_iQ(&iQ);
                        queue_enqueue("InstantDEX",&InstantDEXQ,queueitem(retstr));
                    } else return(clonestr("{\"result\":\"updated rambook\"}"));
                } else printf("not submitquote_str\n");
            } else return(clonestr("{\"error\":\"cant get price close enough due to limited decimals\"}"));
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error %llu/%llu volume %f price %f\"}",dir>0?"bid":"ask",(long long)prices->baseid,(long long)prices->relid,volume,price);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error %llu/%llu dir.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)prices->baseid,(long long)prices->relid,dir,volume,price);
        retstr = clonestr(buf);
    }
    return(retstr);
}
#endif
#endif
