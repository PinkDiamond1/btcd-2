//
//  trades.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_trades_h
#define xcode_trades_h

struct tradehistory { uint64_t assetid,purchased,sold; };

/*struct InstantDEX_quote *prices777_convorder(struct InstantDEX_quote *iQ,struct prices777 *prices,struct prices777_order *order,int32_t dir)
{
    uint64_t baseamount,relamount;
    memset(iQ,0,sizeof(*iQ));
    set_best_amounts(&baseamount,&relamount,order->s.price,order->s.vol);
    if ( dir < 0 )
        iQ->relid = prices->baseid, iQ->relamount = baseamount, iQ->baseid = prices->relid, iQ->baseamount = relamount;
    else iQ->relid = prices->relid, iQ->relamount = relamount, iQ->baseid = prices->baseid, iQ->baseamount = baseamount;
    iQ->s = order->s;
    return(iQ);
}

int32_t prices777_conviQ(struct prices777_order *order,struct InstantDEX_quote *iQ)
{
    struct prices777 *prices; int32_t inverted;
    memset(order,0,sizeof(*order));
    if ( (prices= prices777_find(&inverted,iQ->baseid,iQ->relid,INSTANTDEX_NAME)) != 0 )
    {
        order->source = prices;
        order->price = prices777_price_volume(&order->vol,iQ->baseamount,iQ->relamount);
        order->s = iQ->s;
        if ( iQ->s.isask != 0 )
            order->wt = -1;
        else order->wt = 1;
        return(0);
    }
    return(-1);
}*/

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
                    order->id = orderid;
                    order->s.offerNXT = j64bits(item,"offerNXT");
                    order->wt = dir, order->s.price = orderprice, order->s.vol = ordervolume;
                } else return(clonestr("{\"error\":\"invalid exchange or contract pair\"}"));
            } else return(clonestr("{\"error\":\"no trade specified\"}"));
        }
        return(InstantDEX_dotrades(trades,n,juint(json,"dotrade")));
    }
    printf("error parsing.(%s)\n",jprint(json,0));
    return(clonestr("{\"error\":\"couldnt process trades\"}"));
}

char *prices777_trade(struct prices777 *prices,int32_t dir,double price,double volume)
{
    char *retstr; struct exchange_info *exchange;
    if ( strcmp(prices->exchange,"nxtae") == 0 )
        retstr = fill_nxtae(SUPERNET.my64bits,dir,price,volume,prices->baseid,prices->relid);
    else if ( (exchange= find_exchange(0,prices->exchange)) != 0 )
    {
        if ( exchange->trade != 0 )
        {
            printf(" issue dir.%d %s/%s price %f vol %f -> %s\n",dir,prices->base,prices->rel,price,volume,prices->exchange);
            (*exchange->trade)(&retstr,exchange,prices->base,prices->rel,dir,price,volume);
            return(retstr);
        } else return(clonestr("{\"error\":\"no trade function for exchange\"}\n"));
    }
    return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
}

int32_t is_unfunded_order(uint64_t nxt64bits,uint64_t assetid,uint64_t amount)
{
    char assetidstr[64],NXTaddr[64],cmd[1024],*jsonstr;
    int64_t ap_mult,unconfirmed,balance = 0;
    cJSON *json;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( assetid == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getAccount&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                balance = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT"));
                free_json(json);
            }
            free(jsonstr);
        }
        strcpy(assetidstr,"NXT");
    }
    else
    {
        expand_nxt64bits(assetidstr,assetid);
        if ( (ap_mult= assetmult(assetidstr)) != 0 )
        {
            expand_nxt64bits(NXTaddr,nxt64bits);
            balance = ap_mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        }
    }
    if ( balance < amount )
    {
        printf("balance %.8f < amount %.8f for asset.%s\n",dstr(balance),dstr(amount),assetidstr);
        return(1);
    }
    return(0);
}

cJSON *InstantDEX_tradejson(struct prices777_order *order,int32_t dotrade)
{
    char buf[1024],*retstr,*exchange; uint64_t qty,avail,priceNQT; struct prices777 *prices; cJSON *json = 0;
    if ( (prices= order->source) != 0 )
    {
        exchange = prices->exchange;
        if ( dotrade == 0 )
        {
            sprintf(buf,"{\"trade\":\"%s\",\"exchange\":\"%s\",\"base\":\"%s\",\"rel\":\"%s\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"price\":%.8f,\"volume\":%.8f}",order->wt > 0. ? "buy" : "sell",exchange,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,order->s.price,order->s.vol);
            if ( strcmp(exchange,"nxtae") == 0 )
            {
                qty = calc_asset_qty(&avail,&priceNQT,SUPERNET.NXTADDR,0,prices->baseid,order->s.price,order->s.vol);
                sprintf(buf+strlen(buf)-1,",\"priceNQT\":\"%llu\",\"quantityQNT\":\"%llu\",\"avail\":\"%llu\"}",(long long)priceNQT,(long long)qty,(long long)avail);
                if ( qty == 0 )
                    sprintf(buf+strlen(buf)-1,",\"error\":\"insufficient balance\"}");
            }
            return(cJSON_Parse(buf));
        }
        retstr = prices777_trade(prices,order->wt,order->s.price,order->s.vol);
        if ( retstr != 0 )
        {
            json = cJSON_Parse(retstr);
            free(retstr);
        }
    }
    return(json);
}

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t numtx,int32_t updateNXT) // from poll_pending_offers via main
{
    /* char *retstr;
     return;
     printf("update_openorder iQ.%llu with numtx.%d updateNXT.%d | expires in %ld\n",(long long)iQ->quoteid,numtx,updateNXT,iQ->timestamp+iQ->duration-time(NULL));
     if ( (SUPERNET.automatch & 2) != 0 && (retstr= check_ordermatch(1,SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,iQ)) != 0 )
     {
     printf("automatched order!\n");
     free(retstr);
     }*/
}

void poll_pending_offers(char *NXTaddr,char *NXTACCTSECRET)
{
}

#endif
