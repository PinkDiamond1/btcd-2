//
//  quotes.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_quotes_h
#define xcode_quotes_h

#ifdef oldway
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

struct InstantDEX_quote *AllQuotes;

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->s.closed = iQ->s.sent = iQ->s.matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->s.closed = 1; }

int32_t InstantDEX_uncalcsize() { struct InstantDEX_quote iQ; return(sizeof(iQ.hh) + sizeof(iQ.s.quoteid) + sizeof(iQ.s.price) + sizeof(iQ.s.vol)); }

int32_t iQcmp(struct InstantDEX_quote *iQA,struct InstantDEX_quote *iQB)
{
    if ( iQA->s.isask == iQB->s.isask && iQA->baseid == iQB->baseid && iQA->relid == iQB->relid && iQA->baseamount == iQB->baseamount && iQA->relamount == iQB->relamount )
        return(0);
    else if ( iQA->s.isask != iQB->s.isask && iQA->baseid == iQB->relid && iQA->relid == iQB->baseid && iQA->baseamount == iQB->relamount && iQA->relamount == iQB->baseamount )
        return(0);
    return(-1);
}

uint64_t calc_quoteid(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote Q;
    if ( iQ == 0 )
        return(0);
    if ( iQ->s.quoteid == 0 )
    {
        Q = *iQ;
        clear_InstantDEX_quoteflags(&Q);
        if ( Q.s.isask != 0 )
        {
            Q.baseid = iQ->relid, Q.baseamount = iQ->relamount;
            Q.relid = iQ->baseid, Q.relamount = iQ->baseamount;
            Q.s.isask = Q.s.minperc = 0;
        }
        return(calc_txid((uint8_t *)((long)&Q + InstantDEX_uncalcsize()),sizeof(Q) - InstantDEX_uncalcsize()));
    } return(iQ->s.quoteid);
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
    }
    return(iQ);
}

struct InstantDEX_quote *findquoteid(uint64_t quoteid,int32_t evenclosed)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= find_iQ(quoteid)) != 0 )
    {
        if ( evenclosed != 0 || iQ->s.closed == 0 )
        {
            if ( calc_quoteid(iQ) == quoteid )
                return(iQ);
            else printf("calc_quoteid %llu vs %llu\n",(long long)calc_quoteid(iQ),(long long)quoteid);
        } //else printf("quoteid.%llu closed.%d\n",(long long)quoteid,iQ->closed);
    } else printf("couldnt find %llu\n",(long long)quoteid);
    return(0);
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->s.offerNXT == calc_nxt64bits(NXTaddr) && iQ->exchangeid == INSTANTDEX_EXCHANGEID )
    {
        cancel_InstantDEX_quote(iQ);
        return(1);
    }
    return(0);
}

char *InstantDEX_cancelorder(uint64_t orderid,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ; cJSON *json,*array,*item; char numstr[64],*retstr; uint64_t quoteids[256]; int32_t i,n=0;
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    if ( quoteid != 0 )
        quoteids[n++] = quoteid;
    //n += InstantDEX_quoteids(quoteids+n,orderid);
    for (i=0; i<n; i++)
    {
        quoteid = quoteids[i];
        if ( (retstr= cancel_NXTorderid(SUPERNET.NXTADDR,quoteid)) != 0 )
        {
            if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->s.offerNXT == calc_nxt64bits(SUPERNET.NXTADDR) )
                cancel_InstantDEX_quote(iQ);
            if ( (item= cJSON_Parse(retstr)) != 0 )
                jaddi(array,item);
            free(retstr);
        }
        cancelquote(SUPERNET.NXTADDR,quoteid);
    }
    if ( orderid != 0 )
    {
        if ( cancelquote(SUPERNET.NXTADDR,orderid) != 0 )
            sprintf(numstr,"%llu",(long long)orderid), jaddstr(json,"ordercanceled",numstr);
    }
    return(jprint(json,1));
}

struct InstantDEX_quote *create_iQ(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote *newiQ; struct prices777 *prices; int32_t inverted;
    if ( (newiQ= find_iQ(iQ->s.quoteid)) != 0 )
        return(newiQ);
    newiQ = calloc(1,sizeof(*newiQ));
    *newiQ = *iQ;
    HASH_ADD(hh,AllQuotes,s.quoteid,sizeof(newiQ->s.quoteid),newiQ);
    if ( (prices= prices777_find(&inverted,iQ->baseid,iQ->relid,INSTANTDEX_NAME)) != 0 )
        prices->dirty++;
    {
        struct InstantDEX_quote *checkiQ;
        if ( (checkiQ= find_iQ(iQ->s.quoteid)) == 0 || iQcmp(iQ,checkiQ) != 0 )//memcmp((uint8_t *)((long)checkiQ + sizeof(checkiQ->hh) + sizeof(checkiQ->quoteid)),(uint8_t *)((long)iQ + sizeof(iQ->hh) + sizeof(iQ->quoteid)),sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->quoteid)) != 0 )
        {
            int32_t i;
            for (i=(sizeof(iQ->hh) - sizeof(iQ->s.quoteid)); i<sizeof(*iQ) - sizeof(iQ->hh) - sizeof(iQ->s.quoteid); i++)
                printf("%02x ",((uint8_t *)iQ)[i]);
            printf("iQ\n");
            for (i=(sizeof(checkiQ->hh) + sizeof(checkiQ->s.quoteid)); i<sizeof(*checkiQ) - sizeof(checkiQ->hh) - sizeof(checkiQ->s.quoteid); i++)
                printf("%02x ",((uint8_t *)checkiQ)[i]);
            printf("checkiQ\n");
            printf("error finding iQ after adding %llu vs %llu\n",(long long)checkiQ->s.quoteid,(long long)iQ->s.quoteid);
        }
    }
    return(newiQ);
}

char *InstantDEX_str(char *buf,int32_t extraflag,struct InstantDEX_quote *iQ)
{
    char _buf[4096],extra[512],base[64],rel[64];
    if ( buf == 0 )
        buf = _buf;
    if ( extraflag != 0 )
        sprintf(extra,",\"plugin\":\"relay\",\"destplugin\":\"InstantDEX\",\"method\":\"busdata\",\"submethod\":\"%s\"",(iQ->s.isask != 0) ? "ask" : "bid");
    else extra[0] = 0;
    unstringbits(base,iQ->s.basebits), unstringbits(rel,iQ->s.relbits);
    sprintf(buf,"{\"quoteid\":\"%llu\",\"base\":\"%s\",\"baseid\":\"%llu\",\"baseamount\":\"%llu\",\"rel\":\"%s\",\"relid\":\"%llu\",\"relamount\":\"%llu\",\"price\":%.8f,\"volume\":%.8f,\"offerNXT\":\"%llu\",\"timestamp\":\"%u\",\"isask\":\"%u\",\"exchange\":\"%s\",\"gui\":\"%s\"%s}",(long long)iQ->s.quoteid,base,(long long)iQ->baseid,(long long)iQ->baseamount,rel,(long long)iQ->relid,(long long)iQ->relamount,iQ->s.price,iQ->s.vol,(long long)iQ->s.offerNXT,iQ->s.timestamp,iQ->s.isask,exchange_str(iQ->exchangeid),iQ->gui,extra);
    if ( buf == _buf )
        return(clonestr(buf));
    else return(buf);
}

char *InstantDEX_orderstatus(uint64_t orderid,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ = 0;
    if ( (iQ= find_iQ(orderid)) != 0 || (iQ= find_iQ(quoteid)) != 0 )
        return(InstantDEX_str(0,0,iQ));
    return(clonestr("{\"error\":\"couldnt find orderid\"}"));
}

char *InstantDEX_openorders(char *NXTaddr,int32_t allorders)
{
    struct InstantDEX_quote *iQ,*tmp; char buf[4096],*jsonstr; cJSON *json,*array,*item; uint64_t nxt64bits = calc_nxt64bits(NXTaddr);
    json = cJSON_CreateObject(), array = cJSON_CreateArray();
    HASH_ITER(hh,AllQuotes,iQ,tmp)
    {
        if ( iQ->s.offerNXT == nxt64bits && (allorders != 0 || iQ->s.closed == 0) )
        {
            if ( (jsonstr= InstantDEX_str(buf,0,iQ)) != 0 && (item= cJSON_Parse(jsonstr)) != 0 )
                jaddi(array,item);
        }
    }
    jadd(json,"openorders",array);
    return(jprint(json,1));
}

int _decreasing_quotes(const void *a,const void *b)
{
#define order_a ((struct InstantDEX_quote *)a)
#define order_b ((struct InstantDEX_quote *)b)
 	if ( order_b->s.price > order_a->s.price )
		return(1);
	else if ( order_b->s.price < order_a->s.price )
		return(-1);
	return(0);
#undef order_a
#undef order_b
}

int _increasing_quotes(const void *a,const void *b)
{
#define order_a ((struct InstantDEX_quote *)a)
#define order_b ((struct InstantDEX_quote *)b)
 	if ( order_b->s.price > order_a->s.price )
		return(-1);
	else if ( order_b->s.price < order_a->s.price )
		return(1);
	return(0);
#undef order_a
#undef order_b
}

cJSON *prices777_orderjson(struct InstantDEX_quote *iQ)
{
    cJSON *item = cJSON_CreateArray();
    jaddinum(item,iQ->s.price);
    jaddinum(item,iQ->s.vol);
    jaddinum(item,iQ->s.quoteid);
    return(item);
}

cJSON *InstantDEX_orderbook(struct prices777 *prices)
{
    struct InstantDEX_quote *ptr,iQ,*tmp,*askvals,*bidvals; cJSON *json,*bids,*asks; int32_t i,isask,iter,n,m,numbids,numasks,invert;
    json = cJSON_CreateObject(), bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    for (iter=numbids=numasks=n=m=0; iter<2; iter++)
    {
        HASH_ITER(hh,AllQuotes,ptr,tmp)
        {
            //printf("iterate quote.%llu\n",(long long)iQ->quoteid);
            if ( prices777_equiv(ptr->baseid) == prices777_equiv(prices->baseid) && prices777_equiv(ptr->relid) == prices777_equiv(prices->relid) )
                invert = 0;
            else if ( prices777_equiv(ptr->relid) == prices777_equiv(prices->baseid) && prices777_equiv(ptr->baseid) == prices777_equiv(prices->relid) )
                invert = 1;
            else continue;
            iQ = *ptr;
            isask = iQ.s.isask;
            if ( invert != 0 )
                isask ^= 1;
            if ( invert != 0 )
            {
                if ( iQ.s.price > SMALLVAL )
                    iQ.s.vol *= iQ.s.price, iQ.s.price = 1. / iQ.s.price;
                else iQ.s.price = prices777_price_volume(&iQ.s.vol,iQ.relamount,iQ.baseamount);
            }
            else if ( iQ.s.price <= SMALLVAL )
                iQ.s.price = prices777_price_volume(&iQ.s.vol,iQ.baseamount,iQ.relamount);
            if ( iter == 0 )
            {
                if ( isask != 0 )
                    numasks++;
                else numbids++;
            }
            else
            {
                if ( isask == 0 && n < numbids )
                    bidvals[n++] = iQ;
                else if ( isask != 0 && m < numasks )
                    askvals[m++] = iQ;
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

char *check_ordermatch(int32_t pollflag,struct prices777 *prices,struct InstantDEX_quote *refiQ)
{
  /*  int32_t i,besti,n=0,dir = 1; struct InstantDEX_quote *iQ,_iQ; struct prices777_order *order;
    uint64_t assetA,amountA,assetB,amountB; char otherNXTaddr[64],exchange[64],*retstr = 0; double metric,price,vol,perc,bestmetric = 0.;
    besti = -1;
    if ( refiQ->s.isask != 0 )
        dir = -1;
    if ( dir > 0 )
        n = prices->O.numasks;
    else if ( dir < 0 )
        n = prices->O.numbids;
    if ( n > 0 )
    {
        for (i=0; i<n; i++)
        {
            order = (dir > 0) ? &prices->O.book[MAX_GROUPS][i].bid : &prices->O.book[MAX_GROUPS][i].ask;
            if ( order->source == 0 )
                continue;
            iQ = prices777_convorder(&_iQ,prices,order,dir);
            expand_nxt64bits(otherNXTaddr,iQ->s.offerNXT);
            if ( iQ->s.closed != 0 )
                continue;
            if ( iQ->exchangeid == INSTANTDEX_EXCHANGEID && is_unfunded_order(iQ->s.offerNXT,dir > 0 ? iQ->baseid : iQ->relid,dir > 0 ? iQ->baseamount : iQ->relamount) != 0 )
            {
                iQ->s.closed = 1;
                printf("found unfunded order!\n");
                continue;
            }
            strcpy(exchange,order->source->exchange);
            if ( strcmp(otherNXTaddr,SUPERNET.NXTADDR) != 0 && iQ->s.matched == 0 && iQ->exchangeid == INSTANTDEX_NXTAEID )
            {
                price = order->s.price, vol = order->s.vol;
                if ( vol > (refvol * INSTANTDEX_MINVOLPERC) && refvol > (vol * iQ->s.minperc * .01) )
                {
                    if ( vol < refvol )
                        metric = (vol / refvol);
                    else metric = 1.;
                    if ( dir > 0 && price < (refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL) )
                        metric *= (1. + (refprice - price)/refprice);
                    else if ( dir < 0 && price > (refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL) )
                        metric *= (1. + (price - refprice)/refprice);
                    else metric = 0.;
                    if ( metric != 0. )
                    {
                        printf("matchedflag.%d exchange.(%s) %llu/%llu from (%s) | ",iQ->s.matched,exchange,(long long)iQ->baseamount,(long long)iQ->relamount,otherNXTaddr);
                        printf("price %.8f vol %.8f | %.8f > %.8f? %.8f > %.8f?\n",price,vol,vol,(refvol * INSTANTDEX_MINVOLPERC),refvol,(vol * INSTANTDEX_MINVOLPERC));
                        printf("price %f against %f or %f\n",price,(refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL),(refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL));
                        printf("metric %f\n",metric);
                    }
                    if ( metric > bestmetric )
                    {
                        bestmetric = metric;
                        besti = i;
                    }
                }
            }// else printf("NXTaddr.%s: skip %s from %s\n",NXTaddr,exchange,otherNXTaddr);
        }
    }
    //printf("n.%d\n",n);
    if ( besti >= 0 )
    {
        order = (dir > 0) ? &prices->O.book[MAX_GROUPS][besti].bid : &prices->O.book[MAX_GROUPS][besti].ask;
        iQ = prices777_convorder(&_iQ,prices,order,dir);
        if ( dir < 0 )
            assetA = refiQ->relid, amountA = iQ->relamount, assetB = refiQ->baseid, amountB = iQ->baseamount;
        else assetB = refiQ->relid, amountB = iQ->relamount, assetA = refiQ->baseid, amountA = iQ->baseamount;
        price = order->s.price, vol = order->s.vol;
        strcpy(exchange,order->source->exchange);
        expand_nxt64bits(otherNXTaddr,iQ->s.offerNXT);
        perc = 100.;
        if ( perc >= iQ->s.minperc )
        {
            //retstr = makeoffer3(NXTaddr,NXTACCTSECRET,price,refvol,0,perc,refiQ->baseid,refiQ->relid,iQ->quoteid,dir > 0,exchange,iQ->baseamount*refvol/vol,iQ->relamount*refvol/vol,iQ->nxt64bits,iQ->minperc);
        }
    } else printf("besti.%d\n",besti);
    return(retstr);*/
    return(0);
}

char *InstantDEX_placebidask(char *remoteaddr,uint64_t orderid,char *exchangestr,char *name,char *base,char *rel,struct InstantDEX_quote *iQ)
{
    extern queue_t InstantDEXQ;
    char *retstr = 0; int32_t inverted,dir; struct prices777 *prices; double price,volume;
    if ( iQ->exchangeid < 0 || (exchangestr= exchange_str(iQ->exchangeid)) == 0 )
        return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
    if ( (prices= prices777_find(&inverted,iQ->baseid,iQ->relid,exchangestr)) == 0 )
        prices = prices777_poll(exchangestr,name,base,iQ->baseid,rel,iQ->relid);
    if ( prices != 0 )
    {
        price = iQ->s.price, volume = iQ->s.vol;
        if ( price < SMALLVAL || volume < SMALLVAL )
            return(clonestr("{\"error\":\"prices777_trade invalid price or volume\"}\n"));
        if ( iQ->s.isask != 0 )
            dir = -1;
        else dir = 1;
        if ( inverted != 0 )
        {
            dir *= -1;
            volume *= price;
            price = 1. / price;
            printf("price inverted (%f %f) -> (%f %f)\n",iQ->s.price,iQ->s.vol,price,volume);
        }
        if ( strcmp(exchangestr,"InstantDEX") != 0 && strcmp(exchangestr,"active") != 0 && strcmp(exchangestr,"basket") != 0 )
        {
            create_iQ(iQ);
            return(prices777_trade(prices,dir,price,volume));
        }
        if ( remoteaddr == 0 && iQ->s.automatch != 0 && (SUPERNET.automatch & 1) != 0 && (retstr= check_ordermatch(0,prices,iQ)) != 0 )
            return(retstr);
        retstr = InstantDEX_str(0,1,iQ);
        if ( Debuglevel > 2 )
            printf("placequote.(%s) remoteaddr.(%s)\n",retstr,remoteaddr==0?"local":remoteaddr);
        create_iQ(iQ);
        queue_enqueue("InstantDEX",&InstantDEXQ,queueitem(retstr));
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"error\":\"cant get prices ptr\"}");
    return(retstr);
}
#endif
#endif
