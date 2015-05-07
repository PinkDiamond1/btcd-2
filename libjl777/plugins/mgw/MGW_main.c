//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "MGW"
#define PLUGNAME(NAME) MGW ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "storage.c"
#include "system777.c"
#include "NXT777.c"
#include "msig.c"
#undef DEFINES_ONLY

void MGW_idle(struct plugin_info *plugin) {}

STRUCTNAME MGW;
char *PLUGNAME(_methods)[] = { "myacctpubkeys" }; // list of supported methods

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *json)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct MGW_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *coinaddr,char *pubkey)
{
    uint64_t key[3]; char *keycoinaddr; int32_t len,flag;
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 0;
    coinaddr[0] = pubkey[0] = 0;
    //printf("add.(%s) -> (%s)\n",newcoinaddr,newpubkey);
    if ( (keycoinaddr= db777_findM(&len,DB_NXTaccts,key,sizeof(key))) != 0 )
    {
        strcpy(coinaddr,keycoinaddr);
        free(keycoinaddr);
    }
    db777_findstr(pubkey,sizeof(pubkey),DB_NXTaccts,coinaddr);
    return(coinaddr[0] != 0 && pubkey[0] != 0);
}

int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *newcoinaddr,char *newpubkey)
{
    uint64_t key[3]; char *coinaddr,pubkey[513]; int32_t len,flag,updated = 0;
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 1;
    //printf("add.(%s) -> (%s)\n",newcoinaddr,newpubkey);
    if ( (coinaddr= db777_findM(&len,DB_NXTaccts,key,sizeof(key))) != 0 )
    {
        if ( strcmp(coinaddr,newcoinaddr) == 0 )
            flag = 0;
        free(coinaddr);
    }
    if ( flag != 0 )
    {
        if ( db777_add(1,DB_NXTaccts,key,sizeof(key),newcoinaddr,(int32_t)strlen(newcoinaddr)+1) == 0 )
            updated = 1;
        else printf("error adding (%s)\n",newcoinaddr);
    }
    flag = 1;
    if ( db777_findstr(pubkey,sizeof(pubkey),DB_NXTaccts,newcoinaddr) > 0 )
    {
        if ( strcmp(pubkey,newpubkey) == 0 )
            flag = 0;
    }
    if ( flag != 0 )
    {
        if ( db777_addstr(DB_NXTaccts,newcoinaddr,newpubkey) == 0 )
            updated = 1;
        else printf("error adding (%s)\n",newpubkey);
    }
    return(updated);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    if ( userpubkey != 0 && userpubkey[0] != 0 )
        safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        key = cJSON_CreateString(pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        _stripwhite(paramstr,' ');
        free_json(array);
    }
printf("createmultisig_json_params.(%s)\n",paramstr);
    return(paramstr);
}

int32_t generate_multisigaddr(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t addmultisig,char *params)
{
    char addr[1024],*retstr;
    cJSON *json,*redeemobj,*msigobj;
    int32_t flag = 0;
    if ( addmultisig != 0 )
    {
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"addmultisigaddress",params)) != 0 )
        {
            strcpy(multisigaddr,retstr);
            free(retstr);
            sprintf(addr,"\"%s\"",multisigaddr);
            if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",addr)) != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                    {
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            }
        } else printf("error creating multisig address\n");
    }
    else
    {
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"createmultisig",params)) != 0 )
        {
            json = cJSON_Parse(retstr);
            if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
            else
            {
                if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                    {
                        copy_cJSON(multisigaddr,msigobj);
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                free_json(json);
            }
            free(retstr);
        } else printf("error issuing createmultisig.(%s)\n",params);
    }
    return(flag);
}

int32_t issue_createmultisig(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char *params;
    params = createmultisig_json_params(msig->pubkeys,msig->m,msig->n,(use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        flag = generate_multisigaddr(msig->multisigaddr,msig->redeemScript,coinstr,serverport,userpass,use_addmultisig,params);
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *get_NXT_msigaddr(uint64_t *srv64bits,int32_t m,int32_t n,uint64_t nxt64bits,char *coinstr,char coinaddrs[][256],char pubkeys[][1024])
{
    uint64_t key[16]; char NXTpubkey[128],NXTaddr[64]; int32_t flag,i,keylen,len; struct coin777 *coin; struct multisig_addr *msig;
    key[0] = stringbits(coinstr);
    for (i=0; i<n; i++)
        key[i+1] = srv64bits[i];
    key[i+1] = nxt64bits;
    keylen = (int32_t)(sizeof(*key) * (i+2));
    if ( (msig= db777_findM(&len,DB_msigs,key,keylen)) != 0 )
        return(msig);
    if ( (coin= coin777_find(coinstr)) != 0 )
    {
        expand_nxt64bits(NXTaddr,nxt64bits);
        set_NXTpubkey(NXTpubkey,NXTaddr);
        msig = alloc_multisig_addr(coinstr,m,n,NXTaddr,NXTpubkey,0);
        for (i=0; i<msig->n; i++)
        {
            printf("i.%d n.%d msig->n.%d NXT.(%s) (%s) (%s)\n",i,n,msig->n,msig->NXTaddr,coinaddrs[i],pubkeys[i]);
            strcpy(msig->pubkeys[i].coinaddr,coinaddrs[i]);
            strcpy(msig->pubkeys[i].pubkey,pubkeys[i]);
            msig->pubkeys[i].nxt64bits = srv64bits[i];
        }
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,coin->serverport,coin->userpass,coin->use_addmultisig,msig);
        if ( flag == 0 )
        {
            free(msig);
            return(0);
        }
        if ( db777_add(1,DB_msigs,key,keylen,msig,msig->size) != 0 )
            printf("error saving msig.(%s)\n",msig->multisigaddr);
    }
    return(msig);
}

int32_t process_acctpubkeys(char *retbuf,char *jsonstr,cJSON *json)
{
    cJSON *item,*array; uint64_t gatewaybits,nxt64bits; int32_t i,m,g,n=0,gatewayid,count = 0,updated = 0;
    char gatewayNXT[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    char coinaddrs[16][256],pubkeys[16][1024];
    struct multisig_addr *msig;
    copy_cJSON(gatewayNXT,cJSON_GetObjectItem(json,"NXT"));
    copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
    gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
    gatewaybits = calc_nxt64bits(gatewayNXT);
    if ( (array= cJSON_GetObjectItem(json,"pubkeys")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        printf("arraysize.%d\n",n);
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            copy_cJSON(NXTaddr,cJSON_GetObjectItem(item,"NXT"));
            copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"coinaddr"));
            copy_cJSON(pubkey,cJSON_GetObjectItem(item,"pubkey"));
            nxt64bits = calc_nxt64bits(NXTaddr);
            updated += add_NXT_coininfo(gatewaybits,nxt64bits,coinstr,coinaddr,pubkey);
            for (g=m=0; g<MGW.N; g++)
            {
                m += get_NXT_coininfo(MGW.srv64bits[g],nxt64bits,coinstr,coinaddrs[g],pubkeys[g]);
                printf("g.%d: (%s) (%s)\n",g,coinaddrs[g],pubkeys[g]);
            }
            if ( m == MGW.N && (msig= get_NXT_msigaddr(MGW.srv64bits,MGW.M,MGW.N,nxt64bits,coinstr,coinaddrs,pubkeys)) != 0 )
                free(msig), count++;
        }
    }
    sprintf(retbuf,"{\"result\":\"success\",\"gatewayid\":%d,\"gatewayNXT\":\"%s\",\"coin\":\"%s\",\"updated\":%d,\"total\":%d,\"msigs\":%d}",gatewayid,gatewayNXT,coinstr,updated,n,count);
    printf("(%s)\n",retbuf);
    return(updated);
}

int32_t MGW_publish_acctpubkeys(char *coinstr,char *str)
{
    char retbuf[1024],*retstr = 0;
    cJSON *json,*array;
    if ( (array= cJSON_Parse(str)) != 0 )
    {
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString("MGW"));
        cJSON_AddItemToObject(json,"method",cJSON_CreateString("myacctpubkeys"));
        cJSON_AddItemToObject(json,"pubkeys",array);
        cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
        cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(SUPERNET.NXTADDR));
        cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(MGW.gatewayid));
        retstr = cJSON_Print(json);
        _stripwhite(retstr,' ');
        nn_publish(retstr,1);
        process_acctpubkeys(retbuf,retstr,json);
        free(retstr);
        free_json(json);
        printf("processed.(%s)\n",retbuf);
        return(0);
    }
    return(-1);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char NXTaddr[64],nxtaddr[64],ipaddr[64],*resultstr,*coinstr,*methodstr,*retstr = 0; int32_t i,j,n; cJSON *array; uint64_t nxt64bits;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s\n",plugin->name);
    if ( initflag > 0 )
    {
        if ( DB_msigs == 0 )
            DB_msigs = db777_create(0,0,"msigs",0);
        if ( DB_NXTaccts == 0 )
            DB_NXTaccts = db777_create(0,0,"NXTaccts",0);
        strcpy(retbuf,"{\"result\":\"return JSON init\"}");
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("423766016895692955");//conv_rsacctstr("NXT-JXRD-GKMR-WD9Y-83CK7",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("12240549928875772593");//conv_rsacctstr("NXT-3TKA-UH62-478B-DQU6K",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("8279528579993996036");//conv_rsacctstr("NXT-5294-T9F6-WAWK-9V7WM",0);
        if ( (array= cJSON_GetObjectItem(json,"issuers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                copy_cJSON(NXTaddr,cJSON_GetArrayItem(array,i));
                nxt64bits = calc_nxt64bits(NXTaddr);//conv_rsacctstr(NXTaddr,0);
                for (j=0; j<MGW.numissuers; j++)
                    if ( nxt64bits == MGW.issuers[j] )
                        break;
                if ( j == MGW.numissuers )
                    MGW.issuers[MGW.numissuers++] = nxt64bits;
            }
        }
        MGW.N = get_API_int(cJSON_GetObjectItem(json,"N"),0);
        MGW.M = get_API_int(cJSON_GetObjectItem(json,"M"),0);
        MGW.gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        if ( (array= cJSON_GetObjectItem(json,"servers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 && (n & 1) == 0 )
        {
            for (i=j=0; i<n/2&&i<MAX_MGWSERVERS; i++)
            {
                copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i<<1));
                copy_cJSON(nxtaddr,cJSON_GetArrayItem(array,(i<<1)+1));
                if ( strcmp(ipaddr,MGW.bridgeipaddr) != 0 )
                {
                    MGW.srv64bits[j] = calc_nxt64bits(nxtaddr);//conv_rsacctstr(nxtaddr,0);
                    strcpy(MGW.serverips[j],ipaddr);
                    printf("%d.(%s).%llu ",j,ipaddr,(long long)MGW.srv64bits[j]);
                    j++;
                }
            }
            printf("ipaddrs: %s %s %s\n",MGW.serverips[0],MGW.serverips[1],MGW.serverips[2]);
            if ( MGW.gatewayid >= 0 && MGW.N )
            {
                strcpy(SUPERNET.myipaddr,MGW.serverips[MGW.gatewayid]);
            }
            //printf("j.%d M.%d N.%d n.%d (%s).%s gateway.%d\n",j,COINS.M,COINS.N,n,COINS.myipaddr,COINS.myNXTaddr,COINS.gatewayid);
            if ( j != MGW.N )
                sprintf(retbuf+1,"{\"warning\":\"mismatched servers\",\"details\":\"n.%d j.%d vs M.%d N.%d\",",n,j,MGW.M,MGW.N);
            else if ( MGW.gatewayid >= 0 )
            {
                strcpy(MGW.serverips[MGW.N],MGW.bridgeipaddr);
                MGW.srv64bits[MGW.N] = calc_nxt64bits(MGW.bridgeacct);
                //MGW.all.socks.both.bus = make_MGWbus(SUPERNET.port + nn_portoffset(NN_BUS),SUPERNET.myipaddr,MGW.serverips,MGW.N+1);
                MGW.numgateways = MGW.N;
            }
        }
        MGW.readyflag = 1;
        plugin->allowremote = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        printf("MGW.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"myacctpubkeys") == 0 )
            process_acctpubkeys(retbuf,jsonstr,json);
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"
