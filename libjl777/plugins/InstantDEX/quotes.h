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

int _decreasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount);//iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount);//iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
	if ( valb > vala )
		return(1);
	else if ( valb < vala )
		return(-1);
	return(0);
#undef iQ_a
#undef iQ_b
}

int _increasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount);//iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount);//iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
    // printf("(%f %f) ",vala,valb);
	if ( valb > vala )
		return(-1);
	else if ( valb < vala )
		return(1);
	return(0);
#undef iQ_a
#undef iQ_b
}

int32_t iQcmp(struct InstantDEX_quote *iQA,struct InstantDEX_quote *iQB)
{
    if ( iQA->isask == iQB->isask && iQA->baseid == iQB->baseid && iQA->relid == iQB->relid && iQA->baseamount == iQB->baseamount && iQA->relamount == iQB->relamount )
        return(0);
    else if ( iQA->isask != iQB->isask && iQA->baseid == iQB->relid && iQA->relid == iQB->baseid && iQA->baseamount == iQB->relamount && iQA->relamount == iQB->baseamount )
        return(0);
    return(-1);
}

int32_t iQ_exchangestr(char *exchange,struct InstantDEX_quote *iQ)
{
    char basexchg[64],relxchg[64];
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
    {
        strcpy(basexchg,exchange_str(iQ->baseiQ->exchangeid));
        strcpy(relxchg,exchange_str(iQ->reliQ->exchangeid));
        sprintf(exchange,"%s_%s",basexchg,relxchg);
    }
    else if ( iQ->exchangeid == INSTANTDEX_NXTAEID && iQ->timestamp > time(NULL) )
        strcpy(exchange,INSTANTDEX_NXTAEUNCONF);
    else strcpy(exchange,exchange_str(iQ->exchangeid));
    return(0);
}

void disp_quote(void *ptr,int32_t arg,struct InstantDEX_quote *iQ)
{
    double price,vol;
    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
    printf("%u: arg.%d %-6ld %12.8f %12.8f %llu/%llu\n",iQ->timestamp,arg,iQ->timestamp-time(NULL),price,vol,(long long)iQ->baseamount,(long long)iQ->relamount);
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,uint64_t quoteid,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *exchange,uint64_t nxt64bits,char *gui,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ,int32_t duration)
{
    struct exchange_info *xchg; int32_t exchangeid;
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
    iQ->baseiQ = baseiQ;
    iQ->reliQ = reliQ;
    iQ->baseid = baseid, iQ->baseamount = baseamount;
    iQ->relid = relid, iQ->relamount = relamount;
    if ( Debuglevel > 2 )
        printf("%s.(%s) %f %f\n",iQ->isask==0?"BID":"ASK",exchange,dstr(baseamount),dstr(relamount));
    if ( gui != 0 )
        strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    if ( baseiQ == 0 && reliQ == 0 )
    {
        if ( (xchg= find_exchange(&exchangeid,exchange)) != 0 )
            iQ->exchangeid = exchangeid;
        else printf("cant find_exchange(%s)??\n",exchange);
    }
    else iQ->exchangeid = INSTANTDEX_EXCHANGEID;
    if ( (iQ->quoteid= quoteid) == 0 )
        iQ->quoteid = calc_quoteid(iQ);
    return(0);
}

cJSON *gen_InstantDEX_json(int32_t localaccess,uint64_t *baseamountp,uint64_t *relamountp,int32_t depth,int32_t flip,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid,uint64_t jumpasset)
{
    cJSON *relobj=0,*baseobj=0,*json = 0;
    char numstr[64],base[64],rel[64],exchange[64];
    struct InstantDEX_quote *baseiQ,*reliQ;
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel,mult;
    double price,volume,ratio;
    int32_t minperc;
    minperc = (iQ->minperc != 0) ? iQ->minperc : INSTANTDEX_MINVOL;
    baseamount = iQ->baseamount, relamount = iQ->relamount;
    baseiQ = iQ->baseiQ, reliQ = iQ->reliQ;
    if ( depth == 0 )
        *baseamountp = baseamount, *relamountp = relamount;
    if ( baseiQ != 0 && reliQ != 0 )
    {
        frombase = baseiQ->baseamount, fromrel = baseiQ->relamount;
        tobase = reliQ->baseamount, torel = reliQ->relamount;
        if ( make_jumpquote(refbaseid,refrelid,baseamountp,relamountp,&frombase,&fromrel,&tobase,&torel) == 0. )
            return(0);
    } else frombase = fromrel = tobase = torel = 0;
    json = cJSON_CreateObject();
    if ( Debuglevel > 2 )
        printf("%p depth.%d %p %p %.8f %.8f: %.8f %.8f %.8f %.8f\n",iQ,depth,baseiQ,reliQ,dstr(*baseamountp),dstr(*relamountp),dstr(frombase),dstr(fromrel),dstr(tobase),dstr(torel));
    cJSON_AddItemToObject(json,"askoffer",cJSON_CreateNumber(flip));
    if ( depth == 0 )
    {
        if ( localaccess == 0 )
            cJSON_AddItemToObject(json,"method",cJSON_CreateString("makeoffer3"));
        cJSON_AddItemToObject(json,"plugin",cJSON_CreateString("InstantDEX"));
        set_assetname(&mult,base,refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
        set_assetname(&mult,rel,refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
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
        if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
        {
            if ( baseiQ->minperc > minperc )
                minperc = baseiQ->minperc;
            if ( reliQ->minperc > minperc )
                minperc = reliQ->minperc;
            baseamount = frombase, relamount = fromrel;
            if ( jumpasset == 0 )
            {
                if ( iQ->baseiQ->relid == iQ->reliQ->relid )
                    jumpasset = iQ->baseiQ->relid;
                else printf("mismatched jumpassset: %llu vs %llu\n",(long long)iQ->baseiQ->relid,(long long)iQ->reliQ->relid), getchar();
            }
            baseobj = gen_InstantDEX_json(localaccess,&baseamount,&relamount,depth+1,iQ->isask,iQ->baseiQ,refbaseid,jumpasset,0);
            *baseamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,frombase,fromrel)) < .999 || ratio > 1.001 )
                printf("WARNING: baseiQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel);
            baseamount = tobase, relamount = torel;
            relobj = gen_InstantDEX_json(localaccess,&baseamount,&relamount,depth+1,!iQ->isask,iQ->reliQ,refrelid,jumpasset,0);
            *relamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,tobase,torel)) < .999 || ratio > 1.001 )
                printf("WARNING: reliQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)tobase,(long long)torel);
        }
        if ( jumpasset != 0 )
            sprintf(numstr,"%llu",(long long)jumpasset), cJSON_AddItemToObject(json,"jumpasset",cJSON_CreateString(numstr));
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price));
        cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        if ( iQ->gui[0] != 0 )
            cJSON_AddItemToObject(json,"gui",cJSON_CreateString(iQ->gui));
        if ( baseobj != 0 )
            cJSON_AddItemToObject(json,"baseiQ",baseobj);
        if ( relobj != 0 )
            cJSON_AddItemToObject(json,"reliQ",relobj);
        cJSON_AddItemToObject(json,"minperc",cJSON_CreateNumber(minperc));
    }
    else
    {
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        iQ_exchangestr(exchange,iQ);
        cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
        if ( iQ->nxt64bits != 0 )
            sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price)), cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
    }
    if ( *baseamountp < min_asset_amount(refbaseid) || *relamountp < min_asset_amount(refrelid) )
    {
        if ( Debuglevel > 2 )
            printf("%.8f < %.8f || rel %.8f < %.8f\n",dstr(*baseamountp),dstr(min_asset_amount(refbaseid)),dstr(*relamountp),dstr(min_asset_amount(refrelid)));
        if ( *baseamountp < min_asset_amount(refbaseid) )
            sprintf(numstr,"%llu",(long long)min_asset_amount(refbaseid)), cJSON_AddItemToObject(json,"minbase_error",cJSON_CreateString(numstr));
       if ( *relamountp < min_asset_amount(refrelid) )
           sprintf(numstr,"%llu",(long long)min_asset_amount(refrelid)), cJSON_AddItemToObject(json,"minrel_error",cJSON_CreateString(numstr));
    }
    return(json);
}

cJSON *gen_orderbook_item(struct InstantDEX_quote *iQ,int32_t allflag,uint64_t baseid,uint64_t relid,uint64_t jumpasset)
{
    char offerstr[MAX_JSON_FIELD];
    uint64_t baseamount=0,relamount=0;
    double price,volume;
    cJSON *json = 0;
    baseamount = iQ->baseamount, relamount = iQ->relamount;
    if ( (iQ->isask == 0 && (baseid != iQ->baseid || relid != iQ->relid)) )
    {
        if ( Debuglevel > 1 )
            printf("gen_orderbook_item: isask.%d %llu/%llu != %llu/%llu\n",iQ->isask,(long long)iQ->baseid,(long long)iQ->relid,(long long)baseid,(long long)relid);
        //return(0);
    }
    if ( (json= gen_InstantDEX_json(0,&baseamount,&relamount,0,iQ->isask,iQ,baseid,relid,jumpasset)) != 0 )
    {
        if ( cJSON_GetObjectItem(json,"minbase_error") != 0 || cJSON_GetObjectItem(json,"minrel_error") != 0 )
        {
            printf("gen_orderbook_item has error (%s)\n",cJSON_Print(json));
            free_json(json);
            return(0);
        }
        if ( allflag == 0 )
        {
            price = calc_price_volume(&volume,baseamount,relamount);
            sprintf(offerstr,"{\"price\":\"%.8f\",\"volume\":\"%.8f\"}",price,volume);
            free_json(json);
            return(cJSON_Parse(offerstr));
        }
    } else printf("error generating InstantDEX_json\n");
    return(json);
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

struct InstantDEX_quote *AllQuotes;

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 1; }

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

struct InstantDEX_quote *find_iQ(uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    HASH_FIND(hh,AllQuotes,&quoteid,sizeof(quoteid),iQ);
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
    struct InstantDEX_quote *newiQ;
    if ( (newiQ= find_iQ(iQ->quoteid)) != 0 )
        return(newiQ);
    newiQ = calloc(1,sizeof(*newiQ));
    *newiQ = *iQ;
    HASH_ADD(hh,AllQuotes,quoteid,sizeof(newiQ->quoteid),newiQ);
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
    char basexchg[64],relxchg[64];
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
    {
        strcpy(basexchg,exchange_str(iQ->baseiQ->exchangeid));
        strcpy(relxchg,exchange_str(iQ->reliQ->exchangeid));
        sprintf(exchange,"%s_%s",basexchg,relxchg);
    }
    else if ( iQ->exchangeid == INSTANTDEX_NXTAEID && iQ->timestamp > time(NULL) )
        strcpy(exchange,INSTANTDEX_NXTAEUNCONF);
    else strcpy(exchange,exchange_str(iQ->exchangeid));
    return(0);
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,uint64_t quoteid,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *exchange,uint64_t nxt64bits,char *gui,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ,int32_t duration)
{
    struct exchange_info *xchg; int32_t exchangeid;
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
    iQ->baseiQ = baseiQ;
    iQ->reliQ = reliQ;
    iQ->baseid = baseid, iQ->baseamount = baseamount;
    iQ->relid = relid, iQ->relamount = relamount;
    if ( Debuglevel > 2 )
        printf("%s.(%s) %f %f\n",iQ->isask==0?"BID":"ASK",exchange,dstr(baseamount),dstr(relamount));
    if ( gui != 0 )
        strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    if ( baseiQ == 0 && reliQ == 0 )
    {
        if ( (xchg= find_exchange(&exchangeid,exchange)) != 0 )
            iQ->exchangeid = exchangeid;
        else printf("cant find_exchange(%s)??\n",exchange);
    }
    else iQ->exchangeid = INSTANTDEX_EXCHANGEID;
    if ( (iQ->quoteid= quoteid) == 0 )
        iQ->quoteid = calc_quoteid(iQ);
    return(0);
}

char *InstantDEX_openorders(char *NXTaddr)
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

char *InstantDEX_tradehistory()
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

char *InstantDEX_cancelorder(uint64_t orderid)
{
    return(clonestr("{\"error\":\"API is not yet\"}"));
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct InstantDEX_quote *iQ;
    char nxtaddr[64];
    if ( (iQ= findquoteid(quoteid,0)) != 0 && iQ->nxt64bits == calc_nxt64bits(NXTaddr) && iQ->baseiQ == 0 && iQ->reliQ == 0 && iQ->exchangeid == INSTANTDEX_EXCHANGEID )
    {
        expand_nxt64bits(nxtaddr,iQ->nxt64bits);
        if ( strcmp(NXTaddr,nxtaddr) == 0 && calc_quoteid(iQ) == quoteid )
        {
            cancel_InstantDEX_quote(iQ);
            return(1);
        }
    }
    return(0);
}

char *check_ordermatch(char *NXTaddr,char *NXTACCTSECRET,struct InstantDEX_quote *refiQ) // called by placequote, should autofill
{
    return(0);
}

int32_t match_unconfirmed(void **obooks,int32_t numbooks,char *account,uint64_t quoteid)
{
    return(-1);
}

void update_openorder(struct InstantDEX_quote *iQ,uint64_t quoteid,struct NXT_tx *txptrs[],int32_t numtx,int32_t updateNXT) // from poll_pending_offers via main
{
    char *retstr;
    return;
    printf("update_openorder iQ.%llu with numtx.%d updateNXT.%d | expires in %ld\n",(long long)iQ->quoteid,numtx,updateNXT,iQ->timestamp+iQ->duration-time(NULL));
    if ( (SUPERNET.automatch & 2) != 0 && (retstr= check_ordermatch(SUPERNET.NXTADDR,SUPERNET.NXTACCTSECRET,iQ)) != 0 )
    {
        printf("automatched order!\n");
        free(retstr);
    }
}

void poll_pending_offers(char *NXTaddr,char *NXTACCTSECRET)
{
    /*static uint32_t prevNXTblock; static double lastmilli;
    struct InstantDEX_quote *iQ; cJSON *json,*array,*item; struct NXT_tx *txptrs[MAX_TXPTRS]; void *ptrs[2];
    int32_t i,n,numtx,NXTblock; uint64_t quoteid,baseid,relid; char *jsonstr;
    ptrs[0] = NXTACCTSECRET, ptrs[1] = txptrs;
    if ( milliseconds() < (lastmilli + 5000) )
        return;
    NXTblock = _get_NXTheight(0);
    memset(txptrs,0,sizeof(txptrs));
    if ( (numtx= update_iQ_flags(txptrs,(sizeof(txptrs)/sizeof(*txptrs))-1,0)) > 0 && (jsonstr= InstantDEX_openorders(NXTaddr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"openorders")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    if ( (quoteid= get_API_nxt64bits(cJSON_GetObjectItem(item,"quoteid"))) != 0 )
                    {
                        baseid = get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid"));
                        relid = get_API_nxt64bits(cJSON_GetObjectItem(item,"relid"));
                        iQ = (struct InstantDEX_quote *)get_API_nxt64bits(cJSON_GetObjectItem(item,"iQ"));
                        if ( iQ->closed == 0 && iQ->baseid == baseid && iQ->relid == relid && calc_quoteid(iQ) == quoteid )
                            update_openorder(iQ,quoteid,txptrs,numtx,NXTblock == prevNXTblock);
                    }
                }
            }
            free_json(json);
        }
        process_pingpong_queue(&Pending_offersQ,ptrs);
        free_txptrs(txptrs,numtx);
    }*/
}

/*
memset(txptrs,0,sizeof(txptrs));
if ( (numtx= update_iQ_flags(txptrs,(sizeof(txptrs)/sizeof(*txptrs))-1,0)) > 0 && (jsonstr= InstantDEX_openorders(NXTaddr)) != 0 )
{
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( (array= cJSON_GetObjectItem(json,"openorders")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                item = cJSON_GetArrayItem(array,i);
                if ( (quoteid= get_API_nxt64bits(cJSON_GetObjectItem(item,"quoteid"))) != 0 )
                {
                    baseid = get_API_nxt64bits(cJSON_GetObjectItem(item,"baseid"));
                    relid = get_API_nxt64bits(cJSON_GetObjectItem(item,"relid"));
                    iQ = (struct InstantDEX_quote *)get_API_nxt64bits(cJSON_GetObjectItem(item,"iQ"));
                    if ( iQ->closed == 0 && iQ->baseid == baseid && iQ->relid == relid && calc_quoteid(iQ) == quoteid )
                        update_openorder(iQ,quoteid,txptrs,numtx,NXTblock == prevNXTblock);
                        }
            }
        }
        free_json(json);
    }
    process_pingpong_queue(&Pending_offersQ,ptrs);
    free_txptrs(txptrs,numtx);
}*/

char *placequote_str(struct InstantDEX_quote *iQ)
{
    char iQstr[1024],exchangestr[64],buf[MAX_JSON_FIELD];
    init_hexbytes_noT(iQstr,(uint8_t *)iQ,sizeof(*iQ));
    iQ_exchangestr(exchangestr,iQ);
    sprintf(buf,"{\"result\":\"success\",\"quoteid\":\"%llu\",\"baseid\":\"%llu\",\"baseamount\":\"%llu\",\"relid\":\"%llu\",\"relamount\":\"%llu\",\"offerNXT\":\"%llu\",\"baseiQ\":\"%llu\",\"reliQ\":\"%llu\",\"timestamp\":\"%u\",\"isask\":\"%u\",\"exchange\":\"%s\",\"gui\":\"%s\",\"iQdata\":\"%s\"}",(long long)iQ->quoteid,(long long)iQ->baseid,(long long)iQ->baseamount,(long long)iQ->relid,(long long)iQ->relamount,(long long)iQ->nxt64bits,(long long)calc_quoteid(iQ->baseiQ),(long long)calc_quoteid(iQ->reliQ),iQ->timestamp,iQ->isask,exchangestr,iQ->gui,iQstr);
    return(clonestr(buf));
}

cJSON *gen_InstantDEX_json(int32_t localaccess,uint64_t *baseamountp,uint64_t *relamountp,int32_t depth,int32_t flip,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid,uint64_t jumpasset)
{
    return(0);
}

char *submitquote_str(int32_t localaccess,struct InstantDEX_quote *iQ,uint64_t baseid,uint64_t relid)
{
    cJSON *json;
    char *jsonstr = 0;
    uint64_t basetmp,reltmp;
    if ( (json= gen_InstantDEX_json(localaccess,&basetmp,&reltmp,0,iQ->isask,iQ,baseid,relid,0)) != 0 )
    {
        ensure_jsonitem(json,"plugin","relay");
        ensure_jsonitem(json,"destplugin","InstantDEX");
        ensure_jsonitem(json,"method",(iQ->isask != 0) ? "ask" : "bid");
        jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' ');
        free_json(json);
    } else printf("gen_InstantDEX_json returns null\n");
    return(jsonstr);
}

char *InstantDEX_quote(struct prices777 *prices,int32_t dir,double price,double volume,uint64_t orderid,uint32_t minperc,uint32_t automatch,uint32_t duration)
{
    //extern queue_t InstantDEXQ;
    //queue_enqueue("InstantDEX",&InstantDEXQ,queueitem(jsonstr));
    //free_json(json);
    char retbuf[1024];
    sprintf(retbuf,"{\"result\":\"success\",\"exchange\":\"%s\",\"name\":\"%s\",\"base\":\"%s\",\"rel\":%s\",\"baseid\":\"%llu\",\"relid\":\"%llu\",\"trade\":\"%s\",\"price\":%.8f,\"volume\":%.8f,\"orderid\":\"%llu\",\"minperc\":%d,\"automatch\":%d,\"duration\":%d}",prices->exchange,prices->contract,prices->base,prices->rel,(long long)prices->baseid,(long long)prices->relid,dir>0?"buy":"sell",price,volume,(long long)orderid,minperc,automatch,duration);
    return(clonestr(retbuf));
}

char *placequote_func(char *NXTaddr,char *NXTACCTSECRET,int32_t localaccess,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t baseamount,relamount,nxt64bits,baseid,relid,quoteid = 0; double price,volume,minbasevol,minrelvol; uint32_t timestamp,nonce;
    uint8_t minperc; struct exchange_info *xchg; struct InstantDEX_quote iQ; int32_t keysize;
    int32_t remoteflag,automatch,duration,exchangeid; void *rb;
    char key[521],buf[MAX_JSON_FIELD],name[MAX_JSON_FIELD],offerNXT[MAX_JSON_FIELD],gui[MAX_JSON_FIELD],exchangestr[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],*str,*jsonstr,*retstr = 0;
    if ( (xchg= find_exchange(&exchangeid,INSTANTDEX_NAME)) == 0 || exchangeid != INSTANTDEX_EXCHANGEID )
        return(clonestr("{\"error\":\"unexpected InstantDEX exchangeid\"}"));
    remoteflag = (localaccess == 0);
    nxt64bits = calc_nxt64bits(sender);
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    if ( baseid == 0 || relid == 0 || baseid == relid )
        return(clonestr("{\"error\":\"illegal asset id\"}"));
    baseamount = get_API_nxt64bits(objs[5]);
    relamount = get_API_nxt64bits(objs[6]);
    if ( baseamount != 0 && relamount != 0 )
        price = prices777_price_volume(&volume,baseamount,relamount);
    else
    {
        volume = get_API_float(objs[2]);
        price = get_API_float(objs[3]);
        set_best_amounts(&baseamount,&relamount,price,volume);
    }
    memset(&iQ,0,sizeof(iQ));
    timestamp = (uint32_t)get_API_int(objs[4],0);
    copy_cJSON(gui,objs[7]), gui[sizeof(iQ.gui)-1] = 0;
    automatch = (int32_t)get_API_int(objs[8],1);
    minperc = (int32_t)get_API_int(objs[9],0);
    duration = (int32_t)get_API_int(objs[10],ORDERBOOK_EXPIRATION);
    if ( duration <= 0 || duration > ORDERBOOK_EXPIRATION )
        duration = ORDERBOOK_EXPIRATION;
    copy_cJSON(exchangestr,objs[11]);
    copy_cJSON(offerNXT,objs[12]);
    copy_cJSON(name,objs[13]);
    copy_cJSON(base,objs[14]);
    copy_cJSON(rel,objs[15]);
    InstantDEX_name(key,&keysize,exchangestr,name,base,&baseid,rel,&relid);
    printf("placequote localaccess.%d dir.%d exchangestr.(%s)\n",localaccess,dir,exchangestr);
    if ( exchangestr[0] == 0 )
        strcpy(exchangestr,INSTANTDEX_NAME);
    else
    {
        if ( remoteflag != 0 && strcmp("InstantDEX",exchangestr) != 0 )
        {
            printf("remote node (%d) (%s) trying to place quote to exchange (%s)\n",localaccess,sender,exchangestr);
            return(clonestr("{\"error\":\"no remote exchange orders: you cannot submit an order from a remote node\"}"));
        }
        else if ( strcmp(exchangestr,"nxtae") == 0 )
            return(fill_nxtae(nxt64bits,dir,price,volume,baseid,relid));
        else if ( strcmp(exchangestr,"InstantDEX") != 0 )
        {
            char *prices777_trade(char *exchangestr,char *base,char *rel,int32_t dir,double price,double volume);
            if ( is_native_crypto(base,baseid) > 0 && is_native_crypto(rel,relid) > 0 && price > 0 && volume > 0 && dir != 0 )
                return(prices777_trade(exchangestr,base,rel,dir,price,volume));
            else return(clonestr("{\"error\":\"illegal parameter baseid or relid not crypto or invalid price\"}\n"));
        } //else printf("alternate else case.(%s)\n",exchangestr);
    }
    if ( Debuglevel > 1 )
        printf("NXT.%s t.%u placequote dir.%d sender.(%s) valid.%d price %.8f vol %.8f %llu/%llu\n",NXTaddr,timestamp,dir,sender,valid,price,volume,(long long)baseamount,(long long)relamount);
    prices777_poll(exchangestr,name,base,baseid,rel,relid);
    minbasevol = get_minvolume(baseid), minrelvol = get_minvolume(relid);
    if ( volume < minbasevol || (volume * price) < minrelvol )
    {
        sprintf(buf,"{\"error\":\"not enough volume\",\"price\":%f,\"volume\":%f,\"minbasevol\":%f,\"minrelvol\":%f,\"relvol\":%f}",price,volume,minbasevol,minrelvol,price*volume);
        return(clonestr(buf));
    }
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            if ( (rb= 0) != 0 )//add_rambook_quote(INSTANTDEX_NAME,&iQ,nxt64bits,timestamp,dir,baseid,relid,price,volume,baseamount,relamount,gui,0,duration)) != 0 )
            {
                iQ.minperc = minperc;
                if ( (quoteid= calc_quoteid(&iQ)) != 0 )
                {
                    retstr = placequote_str(&iQ);
                    if ( Debuglevel > 2 )
                        printf("placequote.(%s) remoteflag.%d\n",retstr,remoteflag);
                }
                if ( (jsonstr= submitquote_str(localaccess,&iQ,baseid,relid)) != 0 )
                {
                    printf("got submitquote_str.(%s)\n",jsonstr);
                    if ( remoteflag == 0 )
                    {
                        if ( automatch != 0 && (SUPERNET.automatch & 1) != 0 && (retstr= check_ordermatch(NXTaddr,NXTACCTSECRET,&iQ)) != 0 )
                        {
                            free(jsonstr);
                            return(retstr);
                        } else printf("skip automatch.%d %d\n",automatch,SUPERNET.automatch);
                        if ( (str= busdata_sync(&nonce,jsonstr,"allnodes",0)) != 0 )
                            free(str);
                        retstr = jsonstr;
                    } else return(clonestr("{\"result\":\"updated rambook\"}"));
                } else printf("not submitquote_str\n");
            } else return(clonestr("{\"error\":\"cant get price close enough due to limited decimals\"}"));
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error %llu/%llu volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,volume,price);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error %llu/%llu dir.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,dir,volume,price);
        retstr = clonestr(buf);
    }
    //printf("placequote.(%s)\n",retstr);
    return(retstr);
}

#endif
#endif
