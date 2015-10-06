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


uint64_t pangea_totalbet(struct cards777_pubdata *dp)
{
    int32_t j; uint64_t total;
    for (total=j=0; j<dp->N; j++)
        total += dp->hand.bets[j];
    return(total);
}

int32_t pangea_actives(int32_t *activej,struct cards777_pubdata *dp)
{
    int32_t i,n;
    *activej = -1;
    for (i=n=0; i<dp->N; i++)
    {
        if ( dp->hand.betstatus[i] != CARDS777_FOLD )
        {
            if ( *activej < 0 )
                *activej = i;
            n++;
        }
    }
    return(n);
}

struct pangea_info *pangea_usertables(int32_t *nump,uint64_t my64bits,uint64_t tableid)
{
    int32_t i,j,num = 0; struct pangea_info *sp,*retsp = 0;
    *nump = 0;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
    {
        if ( (sp= TABLES[i]) != 0 )
        {
            for (j=0; j<sp->numaddrs; j++)
                if ( sp->addrs[j] == my64bits && (tableid == 0 || sp->tableid == tableid) )
                {
                    if ( num++ == 0 )
                        retsp = sp;
                }
        }
    }
    *nump = num;
    return(retsp);
}

int32_t pangea_bet(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t player,int64_t bet)
{
    int32_t retval = 0; uint64_t sum;
    player %= dp->N;
    if ( Debuglevel > 2 )
        printf("PANGEA_BET[%d] <- %.8f\n",player,dstr(bet));
    if ( dp->hand.betstatus[player] == CARDS777_ALLIN )
        return(CARDS777_ALLIN);
    else if ( dp->hand.betstatus[player] == CARDS777_FOLD )
        return(CARDS777_FOLD);
    if ( bet > 0 && bet >= dp->balances[player] )
    {
        bet = dp->balances[player];
        dp->hand.betstatus[player] = retval = CARDS777_ALLIN;
    }
    else
    {
        if ( bet > dp->hand.betsize && bet > dp->hand.lastraise && bet < (dp->hand.lastraise<<1) )
        {
            printf("pangea_bet %.8f not double %.8f, clip to lastraise\n",dstr(bet),dstr(dp->hand.lastraise));
            bet = dp->hand.lastraise;
            retval = CARDS777_RAISE;
        }
    }
    sum = dp->hand.bets[player];
    if ( sum+bet < dp->hand.betsize && retval != CARDS777_ALLIN )
    {
        dp->hand.betstatus[player] = retval = CARDS777_FOLD;
        if ( Debuglevel > 2 )
            printf("player.%d betsize %.8f < hand.betsize %.8f FOLD\n",player,dstr(bet),dstr(dp->hand.betsize));
        return(retval);
    }
    else if ( bet >= 2*dp->hand.lastraise )
        dp->hand.lastraise = bet, dp->hand.numactions = 1, retval = CARDS777_FULLRAISE; // allows all players to check/bet again
    sum += bet;
    if ( sum > dp->hand.betsize )
    {
        dp->hand.betsize = sum, dp->hand.lastbettor = player;
        if ( sum > dp->hand.lastraise && retval == CARDS777_ALLIN )
            dp->hand.lastraise = sum;
        else retval = CARDS777_BET;
    }
    dp->balances[player] -= bet, dp->hand.bets[player] += bet;
    return(retval);
}

uint64_t pangea_winnings(uint64_t *pangearakep,uint64_t *hostrakep,uint64_t total,int32_t numwinners,int32_t rakemillis)
{
    uint64_t split,pangearake,rake;
    split = (total * (1000 - rakemillis)) / (1000 * numwinners);
    pangearake = (total - split*numwinners);
    if ( rakemillis > PANGEA_MINRAKE_MILLIS )
    {
        rake = (pangearake * (rakemillis - PANGEA_MINRAKE_MILLIS)) / rakemillis;
        pangearake -= rake;
    }
    else rake = 0;
    *hostrakep = rake;
    *pangearakep = pangearake;
    return(split);
}

int32_t pangea_sidepots(uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS],union hostnet777 *hn,struct cards777_pubdata *dp)
{
    int32_t i,j,nonz,n = 0; uint64_t bet,minbet = 0;
    printf("calc sidepots\n");
    for (j=0; j<dp->N; j++)
        sidepots[0][j] = dp->hand.bets[j];
    nonz = 1;
    while ( nonz > 0 )
    {
        for (minbet=j=0; j<dp->N; j++)
        {
            if ( (bet= sidepots[n][j]) != 0 )
            {
                if ( dp->hand.betstatus[j] != CARDS777_FOLD )
                {
                    if ( minbet == 0 || bet < minbet )
                        minbet = bet;
                }
            }
        }
        for (j=nonz=0; j<dp->N; j++)
        {
            if ( sidepots[n][j] > minbet && dp->hand.betstatus[j] != CARDS777_FOLD )
                nonz++;
        }
        if ( nonz > 0 )
        {
            for (j=0; j<dp->N; j++)
            {
                if ( sidepots[n][j] > minbet )
                {
                    sidepots[n+1][j] = (sidepots[n][j] - minbet);
                    sidepots[n][j] = minbet;
                }
            }
        }
        n++;
    }
    if ( hn->server->H.slot == 0 )//&& n > 1 )
    {
        for (i=0; i<n; i++)
        {
            for (j=0; j<dp->N; j++)
                printf("%.8f ",dstr(sidepots[i][j]));
            printf("sidepot.%d of %d\n",i,n);
        }
    }
    return(n);
}

int64_t pangea_splitpot(uint64_t sidepot[CARDS777_MAXPLAYERS],union hostnet777 *hn,int32_t rakemillis)
{
    int32_t winners[CARDS777_MAXPLAYERS],j,n,numwinners = 0; uint32_t bestrank,rank;
    uint64_t total = 0,bet,split,rake=0,pangearake=0;
    char handstr[128],besthandstr[128]; struct cards777_pubdata *dp;
    dp = hn->client->H.pubdata;
    bestrank = 0;
    for (j=n=0; j<dp->N; j++)
    {
        if ( (bet= sidepot[j]) != 0 )
        {
            total += bet;
            if ( dp->hand.betstatus[j] != CARDS777_FOLD )
            {
                if ( dp->hand.handranks[j] > bestrank )
                {
                    bestrank = dp->hand.handranks[j];
                    set_handstr(besthandstr,dp->hand.hands[j],0);
                }
            }
        }
    }
    for (j=0; j<dp->N; j++)
    {
        if ( dp->hand.betstatus[j] != CARDS777_FOLD && sidepot[j] > 0 )
        {
            if ( dp->hand.handranks[j] == bestrank )
                winners[numwinners++] = j;
            rank = set_handstr(handstr,dp->hand.hands[j],0);
            if ( handstr[strlen(handstr)-1] == ' ' )
                handstr[strlen(handstr)-1] = 0;
            //if ( hn->server->H.slot == 0 )
            printf("(p%d %14s)",j,handstr[0]!=' '?handstr:handstr+1);
            //printf("(%2d %2d).%d ",dp->hands[j][5],dp->hands[j][6],(int32_t)dp->balances[j]);
        }
    }
    if ( numwinners == 0 )
        printf("pangea_splitpot error: numwinners.0\n");
    else
    {
        split = pangea_winnings(&pangearake,&rake,total,numwinners,rakemillis);
        dp->pangearake += pangearake;
        for (j=0; j<numwinners; j++)
            dp->balances[winners[j]] += split;
        if ( split*numwinners + rake + pangearake != total )
            printf("pangea_split total error %.8f != split %.8f numwinners %d rake %.8f pangearake %.8f\n",dstr(total),dstr(split),numwinners,dstr(rake),dstr(pangearake));
        //if ( hn->server->H.slot == 0 )
        {
            printf(" total %.8f split %.8f rake %.8f Prake %.8f %s N%d winners ",dstr(total),dstr(split),dstr(rake),dstr(pangearake),besthandstr,dp->numhands);
            for (j=0; j<numwinners; j++)
                printf("%d ",winners[j]);
            printf("\n");
        }
    }
    return(rake);
}

int32_t pangea_showdown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char handstr[128],hex[1024]; int32_t rank,j,n; bits256 hole[2],hand; uint64_t sidepots[CARDS777_MAXPLAYERS][CARDS777_MAXPLAYERS];
    hole[0] = *(bits256 *)data, hole[1] = *(bits256 *)&data[sizeof(bits256)];
    printf("showdown: sender.%d [%d] [%d] dp.%p\n",senderind,hole[0].bytes[1],hole[1].bytes[1],dp);
    for (j=0; j<5; j++)
        hand.bytes[j] = dp->hand.community[j];
    hand.bytes[j++] = hole[0].bytes[1];
    hand.bytes[j++] = hole[1].bytes[1];
    rank = set_handstr(handstr,hand.bytes,0);
    printf("sender.%d (%s) (%d %d) rank.%x\n",senderind,handstr,hole[0].bytes[1],hole[1].bytes[1],rank);
    dp->hand.handranks[senderind] = rank;
    memcpy(dp->hand.hands[senderind],hand.bytes,7);
    dp->hand.handmask |= (1 << senderind);
    if ( dp->hand.handmask == (1 << dp->N)-1 )
    {
        memset(sidepots,0,sizeof(sidepots));
        n = pangea_sidepots(sidepots,hn,dp);
        for (j=0; j<n; j++)
            dp->hostrake += pangea_splitpot(sidepots[j],hn,dp->rakemillis);
        if ( hn->server->H.slot == 0 )
            pangea_anotherhand(hn,dp,0);
    }
    //printf("player.%d got rank %u (%s) from %d\n",hn->client->H.slot,rank,handstr,senderind);
    if ( hn->client->H.slot != 0 && senderind == 0 )
        pangea_sendcmd(hex,hn,"showdown",-1,priv->holecards[0].bytes,sizeof(priv->holecards),juint(json,"cardi"),dp->hand.undergun);
    return(0);
}

uint64_t pangea_bot(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t turni,int32_t cardi,uint64_t betsize)
{
    int32_t r,action=0,n,activej; char hex[1024]; uint64_t threshold,total,sum,amount = 0;
    sum = dp->hand.bets[hn->client->H.slot];
    action = 0;
    n = pangea_actives(&activej,dp);
    if ( (r = (rand() % 100)) == 0 )
        amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
    else
    {
        if ( betsize == sum )
        {
            if ( r < 100/n )
            {
                amount = dp->hand.lastraise * 2;
                action = 2;
            }
        }
        else if ( betsize > sum )
        {
            amount = (betsize - sum);
            total = pangea_totalbet(dp);
            threshold = (100 * amount)/total;
            if ( r/n > threshold )
            {
                action = 1;
                if ( r/n > 3*threshold && amount < dp->hand.lastraise*2 )
                    amount = dp->hand.lastraise * 2, action = 2;
                else if ( r/n > 10*threshold )
                    amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
            } else action = CARDS777_FOLD, amount = 0;
        }
        else printf("pangea_turn error betsize %.8f vs sum %.8f\n",dstr(betsize),dstr(sum));
        if ( amount > dp->balances[hn->client->H.slot] )
            amount = dp->balances[hn->client->H.slot], action = CARDS777_ALLIN;
    }
    pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,action);
    printf("playerbot.%d got pangea_turn.%d for player.%d action.%d bet %.8f\n",hn->client->H.slot,cardi,turni,action,dstr(amount));
    return(amount);
}

cJSON *pangea_handjson(struct cards777_handinfo *hand,uint8_t *holecards,int32_t isbot)
{
    int32_t i,card; char cardAstr[8],cardBstr[8],pairstr[18],cstr[128]; cJSON *array,*json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    cstr[0] = 0;
    for (i=0; i<5; i++)
    {
        if ( (card= hand->community[i]) != 0xff )
        {
            jaddinum(array,card);
            cardstr(&cstr[strlen(cstr)],card);
            strcat(cstr," ");
        }
    }
    jaddstr(json,"community",cstr);
    jadd(json,"cards",array);
    if ( (card= holecards[0]) != 0xff )
    {
        jaddnum(json,"cardA",card);
        cardstr(cardAstr,holecards[0]);
    } else cardAstr[0] = 0;
    if ( (card= holecards[1]) != 0xff )
    {
        jaddnum(json,"cardB",card);
        cardstr(cardBstr,holecards[1]);
    } else cardBstr[0] = 0;
    sprintf(pairstr,"%s %s",cardAstr,cardBstr);
    jaddstr(json,"holecards",pairstr);
    jaddnum(json,"betsize",dstr(hand->betsize));
    jaddnum(json,"lastraise",dstr(hand->lastraise));
    jaddnum(json,"lastbettor",hand->lastbettor);
    jaddnum(json,"numactions",hand->numactions);
    jaddnum(json,"undergun",hand->undergun);
    jaddnum(json,"isbot",isbot);
    return(json);
}

char *pangea_statusstr(int32_t status)
{
    if ( status == CARDS777_FOLD )
        return("folded");
    else if ( status == CARDS777_ALLIN )
        return("ALLin");
    else return("active");
}

int32_t pangea_countdown(struct cards777_pubdata *dp,int32_t player)
{
    if ( dp->hand.undergun == player && dp->hand.userinput_starttime != 0 )
        return((int32_t)(dp->hand.userinput_starttime + PANGEA_USERTIMEOUT - time(NULL)));
    else return(-1);
}

cJSON *pangea_tablestatus(struct pangea_info *sp)
{
    int32_t i,countdown; int64_t total; struct cards777_pubdata *dp; cJSON *bets,*array,*json = cJSON_CreateObject();
    jadd64bits(json,"tableid",sp->tableid);
    jadd64bits(json,"myind",sp->myind);
    dp = sp->dp;
    jaddnum(json,"minbuyin",dp->minbuyin);
    jaddnum(json,"maxbuyin",dp->maxbuyin);
    jaddnum(json,"button",dp->button);
    jaddnum(json,"M",dp->M);
    jaddnum(json,"N",dp->N);
    jaddnum(json,"numcards",dp->numcards);
    jaddnum(json,"numhands",dp->numhands);
    jaddnum(json,"rake",(double)dp->rakemillis/10.);
    jaddnum(json,"hostrake",dstr(dp->hostrake));
    jaddnum(json,"pangearake",dstr(dp->pangearake));
    jaddnum(json,"bigblind",dstr(dp->bigblind));
    jaddnum(json,"ante",dstr(dp->ante));
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddi64bits(array,sp->addrs[i]);
    jadd(json,"addrs",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddinum(array,dstr(dp->turnis[i]));
    jadd(json,"turns",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddinum(array,dstr(dp->balances[i]));
    jadd(json,"balances",array);
    array = cJSON_CreateArray();
    for (i=0; i<dp->N; i++)
        jaddistr(array,pangea_statusstr(dp->hand.betstatus[i]));
    jadd(json,"status",array);
    bets = cJSON_CreateArray();
    for (total=i=0; i<dp->N; i++)
    {
        total += dp->hand.bets[i];
        jaddinum(bets,dstr(dp->hand.bets[i]));
    }
    jadd(json,"bets",bets);
    jaddnum(json,"totalbets",dstr(total));
    jadd(json,"hand",pangea_handjson(&dp->hand,sp->priv->hole,dp->isbot[sp->myind]));
    if ( (countdown= pangea_countdown(dp,sp->myind)) >= 0 )
        jaddnum(json,"timeleft",countdown);
    return(json);
}

void pangea_playerprint(struct cards777_pubdata *dp,int32_t i,int32_t myind)
{
    int32_t countdown; char str[8];
    if ( (countdown= pangea_countdown(dp,i)) >= 0 )
        sprintf(str,"%2d",countdown);
    else str[0] = 0;
    printf("%d: %6s %12.8f %2s  | %12.8f %s\n",i,pangea_statusstr(dp->hand.betstatus[i]),dstr(dp->hand.bets[i]),str,dstr(dp->balances[i]),i == myind ? "<<<<<<<<<<<": "");
}

void pangea_statusprint(struct cards777_pubdata *dp,struct cards777_privdata *priv,int32_t myind)
{
    int32_t i; char handstr[64]; uint8_t hand[7];
    for (i=0; i<dp->N; i++)
        pangea_playerprint(dp,i,myind);
    handstr[0] = 0;
    if ( dp->hand.community[0] != dp->hand.community[1] )
    {
        for (i=0; i<5; i++)
            if ( (hand[i]= dp->hand.community[i]) == 0xff )
                break;
        if ( i == 5 )
        {
            if ( (hand[5]= priv->hole[0]) != 0xff && (hand[6]= priv->hole[1]) != 0xff )
                set_handstr(handstr,hand,1);
        }
    }
    printf("%s\n",handstr);
}

void pangea_startbets(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t cardi)
{
    uint32_t now; char hex[1024];
    if ( dp->hand.betstarted == 0 )
    {
        dp->hand.numactions = 0;
        dp->hand.betstarted = 1;
        dp->hand.cardi = cardi;
    } else dp->hand.betstarted++;
    printf("STARTBETS.%d cardi.%d\n",dp->hand.betstarted,cardi);
    now = (uint32_t)time(NULL);
    dp->hand.undergun = ((dp->button + 2) % dp->N);
    pangea_sendcmd(hex,hn,"turn",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),cardi,dp->hand.undergun);
}

int32_t pangea_turn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    int32_t turni,cardi; char hex[2048]; struct pangea_info *sp = dp->table;
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    printf("got turn.%d from %d | cardi.%d\n",turni,senderind,cardi);
    dp->turnis[senderind] = turni;
    if ( senderind == 0 && sp != 0 )
    {
        dp->hand.cardi = cardi;
        dp->hand.betstarted = 1;
        dp->hand.undergun = turni;
        if ( hn->client->H.slot != 0 )
        {
            printf("player.%d sends confirmturn.%d\n",hn->client->H.slot,turni);
            pangea_sendcmd(hex,hn,"confirmturn",-1,(void *)&sp->tableid,sizeof(sp->tableid),cardi,turni);
        }
    }
    return(0);
}

int32_t pangea_confirmturn(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    uint32_t starttime; int32_t i,turni,cardi; uint64_t betsize=SATOSHIDEN,amount=0; struct pangea_info *sp=0; char hex[1024];
    if ( data == 0 )
    {
        printf("pangea_turn: null data\n");
        return(-1);
    }
    turni = juint(json,"turni");
    cardi = juint(json,"cardi");
    printf("got confirmturn.%d cardi.%d sender.%d\n",turni,cardi,senderind);
    if ( datalen == sizeof(betsize) )
        memcpy(&betsize,data,sizeof(betsize)), starttime = dp->hand.starttime;
    if ( (sp= dp->table) != 0 )
    {
        if ( senderind == 0 )
        {
            dp->hand.undergun = turni;
            dp->hand.cardi = cardi;
            dp->hand.betsize = betsize;
        }
        dp->turnis[senderind] = turni;
        for (i=0; i<dp->N; i++)
        {
            printf("[i%d %d] ",i,dp->turnis[i]);
            if ( dp->turnis[i] != turni )
                break;
        }
        printf("vs turni.%d cardi.%d hand.cardi %d\n",turni,cardi,dp->hand.cardi);
        if ( hn->client->H.slot == 0 && i == dp->N )
        {
            printf("player.%d sends confirmturn.%d cardi.%d\n",hn->client->H.slot,dp->hand.undergun,dp->hand.cardi);
            pangea_sendcmd(hex,hn,"confirmturn",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),dp->hand.cardi,dp->hand.undergun);
        }
        if ( senderind == 0 && (turni= dp->hand.undergun) == hn->client->H.slot )
        {
            if ( dp->hand.betsize != betsize )
                printf("pangea_turn warning hand.betsize %.8f != betsize %.8f\n",dstr(dp->hand.betsize),dstr(betsize));
            if ( dp->isbot[hn->client->H.slot] != 0 )
                pangea_bot(hn,dp,turni,cardi,betsize);
            else if ( dp->hand.betstatus[hn->client->H.slot] == CARDS777_FOLD || dp->hand.betstatus[hn->client->H.slot] == CARDS777_ALLIN )
                pangea_sendcmd(hex,hn,"action",-1,(void *)&amount,sizeof(amount),cardi,0);
            else
            {
                dp->hand.userinput_starttime = (uint32_t)time(NULL);
                dp->hand.cardi = cardi;
                dp->hand.betsize = betsize;
                dp->hand.undergun = -1;
                fprintf(stderr,"Waiting for user input cardi.%d: ",cardi);
            }
            printf("%s\n",jprint(pangea_tablestatus(sp),1));
            pangea_statusprint(dp,priv,hn->client->H.slot);
        }
    }
    return(0);
}

int32_t pangea_action(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    uint32_t now; int32_t action,cardi,i,j,activej; char hex[1024]; bits256 zero; uint64_t split,rake,pangearake,total,amount = 0;
    action = juint(json,"turni");
    cardi = juint(json,"cardi");
    if ( cardi < 2*dp->N )
        printf("pangea_action: illegal cardi.%d\n",cardi);
    memset(zero.bytes,0,sizeof(zero));
    memcpy(&amount,data,sizeof(amount));
    if ( senderind != dp->hand.undergun )
    {
        printf("out of turn action.%d by player.%d cardi.%d amount %.8f\n",action,senderind,cardi,dstr(amount));
        return(-1);
    }
    pangea_bet(hn,dp,senderind,amount);
    dp->hand.actions[senderind] = action;
    dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
    dp->hand.numactions++;
    printf("got action.%d cardi.%d senderind.%d -> undergun.%d numactions.%d\n",action,cardi,senderind,dp->hand.undergun,dp->hand.numactions);
    if ( pangea_actives(&activej,dp) == 1 )
    {
        total = pangea_totalbet(dp);
        split = pangea_winnings(&pangearake,&rake,total,1,dp->rakemillis);
        dp->hostrake += rake;
        dp->pangearake += pangearake;
        dp->balances[activej] += split;
        if ( hn->server->H.slot == activej && priv->autoshow != 0 )
        {
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[0].bytes,sizeof(priv->holecards[0]),priv->cardis[0],-1);
            pangea_sendcmd(hex,hn,"faceup",-1,priv->holecards[1].bytes,sizeof(priv->holecards[1]),priv->cardis[1],-1);
        }
        if ( hn->server->H.slot == 0 )
        {
            printf("player.%d lastman standing, wins %.8f hostrake %.8f pangearake %.8f\n",activej,dstr(split),dstr(rake),dstr(pangearake));
            sleep(5);
            pangea_anotherhand(hn,dp,0);
        }
        return(0);
    }
    if ( hn->client->H.slot == 0 )
    {
        now = (uint32_t)time(NULL);
        for (i=0; i<dp->N; i++)
        {
            j = (dp->hand.undergun + i) % dp->N;
            if ( dp->hand.betstatus[j] == CARDS777_FOLD || dp->hand.betstatus[j] == CARDS777_ALLIN )
            {
                dp->hand.actions[j] = dp->hand.betstatus[j];
                dp->hand.undergun = (dp->hand.undergun + 1) % dp->N;
                dp->hand.numactions++;
            } else break;
        }
        if ( dp->hand.numactions < dp->N )
        {
            printf("undergun.%d cardi.%d\n",dp->hand.undergun,dp->hand.cardi);
            pangea_sendcmd(hex,hn,"turn",-1,(void *)&dp->hand.betsize,sizeof(dp->hand.betsize),dp->hand.cardi,dp->hand.undergun);
        }
        else
        {
            for (i=0; i<5; i++)
            {
                if ( dp->hand.community[i] == 0xff )
                    break;
                printf("%02x ",dp->hand.community[i]);
            }
            printf("COMMUNITY\n");
            if ( i == 0 )
            {
                if ( dp->hand.cardi != dp->N * 2 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2);
                cardi = dp->hand.cardi;
                for (i=0; i<3; i++,cardi++)
                    pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else if ( i == 3 )
            {
                if ( dp->hand.cardi != dp->N * 2+3 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2 + 3);
                cardi = dp->hand.cardi;
                pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else if ( i == 4 )
            {
                if ( dp->hand.cardi != dp->N * 2+4 )
                    printf("cardi mismatch %d != %d\n",dp->hand.cardi,dp->N * 2+4);
                cardi = dp->hand.cardi;
                pangea_sendcmd(hex,hn,"decoded",dp->N-1,dp->hand.final[cardi*dp->N].bytes,sizeof(dp->hand.final[cardi*dp->N]),cardi,dp->hand.undergun);
            }
            else
            {
                cardi = dp->N * 2 + 5;
                pangea_sendcmd(hex,hn,"showdown",-1,priv->holecards[0].bytes,sizeof(priv->holecards),cardi,dp->hand.undergun);
            }
        }
    }
    if ( Debuglevel > 1 || hn->client->H.slot == 0 )
    {
        printf("player.%d got pangea_action.%d for player.%d action.%d amount %.8f | numactions.%d\n%s\n",hn->client->H.slot,cardi,senderind,action,dstr(amount),dp->hand.numactions,jprint(pangea_tablestatus(dp->table),1));
    }
    return(0);
}

char *pangea_input(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    char *actionstr; uint64_t sum,amount=0; int32_t action,num; struct pangea_info *sp; struct cards777_pubdata *dp; char hex[4096];
    if ( (sp= pangea_usertables(&num,my64bits,tableid)) == 0 )
        return(clonestr("{\"error\":\"you are not playing on any tables\"}"));
    if ( num != 1 )
        return(clonestr("{\"error\":\"more than one active table\"}"));
    else if ( (dp= sp->dp) == 0 )
        return(clonestr("{\"error\":\"no pubdata ptr for table\"}"));
    else if ( dp->hand.undergun != sp->myind || dp->hand.betsize == 0 )
        return(clonestr("{\"error\":\"not your turn\"}"));
    else if ( (actionstr= jstr(json,"action")) == 0 )
        return(clonestr("{\"error\":\"on action specified\"}"));
    else
    {
        if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 || strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 || strcmp(actionstr,"allin") == 0 || strcmp(actionstr,"fold") == 0 )
        {
            sum = dp->hand.bets[sp->myind];
            if ( strcmp(actionstr,"allin") == 0 )
                amount = dp->balances[sp->myind], action = CARDS777_ALLIN;
            else
            {
                if ( dp->hand.betsize == sum )
                {
                    if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 )
                        action = 0;
                    else if ( strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 )
                    {
                        action = 1;
                        if ( (amount= dp->hand.lastraise) < j64bits(json,"amount") )
                            amount = j64bits(json,"amount");
                    }
                    else printf("unsupported userinput command.(%s)\n",actionstr);
                }
                else
                {
                    if ( strcmp(actionstr,"check") == 0 || strcmp(actionstr,"call") == 0 )
                        action = 1, amount = (dp->hand.betsize - sum);
                    else if ( strcmp(actionstr,"bet") == 0 || strcmp(actionstr,"raise") == 0 )
                    {
                        action = 2;
                        amount = (dp->hand.betsize - sum);
                        if ( amount < 2*dp->hand.lastraise )
                            amount = 2*dp->hand.lastraise;
                        if ( j64bits(json,"amount") > amount )
                            amount = j64bits(json,"amount");
                    }
                    else if ( strcmp(actionstr,"fold") == 0 )
                        action = 0;
                    else printf("unsupported userinput command.(%s)\n",actionstr);
                }
            }
            if ( amount > dp->balances[sp->myind] )
                amount = dp->balances[sp->myind], action = CARDS777_ALLIN;
            pangea_sendcmd(hex,&sp->tp->hn,"action",-1,(void *)&amount,sizeof(amount),dp->hand.cardi,action);
            printf("ACTION.(%s)\n",hex);
            //dp->hand.userinput_starttime = 0;
            //dp->hand.cardi = -1;
            //dp->hand.betsize = 0;
            return(clonestr("{\"result\":\"action submitted\"}"));
        }
        else return(clonestr("{\"error\":\"illegal action specified, must be: check, call, bet, raise, fold or allin\"}"));
    }
}

int32_t pangea_addfunds(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    uint64_t amount;
    memcpy(&amount,data,sizeof(amount));
    dp->balances[senderind] = amount;
    printf("myind.%d: addfunds.%d <- %.8f total %.8f\n",hn->client->H.slot,senderind,dstr(amount),dstr(dp->balances[senderind]));
    return(0);
}
