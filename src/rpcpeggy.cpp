#ifdef PEGGY
//
// Created by BTCDDev on 8/17/15.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "bitcoinrpc.h"
#include "kernel.h"
#include "wallet.h"
#include "../libjl777/plugins/includes/cJSON.h"
#include <stdlib.h>
#include <stdio.h>
using namespace json_spirit;
using namespace std;
extern "C" char *peggybase(uint32_t blocknum,uint32_t blocktimestamp);
extern "C" char *peggypayments(uint32_t blocknum,uint32_t blocktimestamp);
extern "C" int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry);

extern "C" int8_t isOpReturn(char *hexbits)
{
    if(hexbits == 0) return -1;
    if(strlen(hexbits) < (size_t)4) return -1;
    int8_t isOPRETURN = -1;
    char temp[4];
    sprintf(temp, "%c%c", hexbits[0], hexbits[1]);
    if(strcmp("6a", temp) == 0)
        isOPRETURN = 0;
    return isOPRETURN;
}

extern "C" char* GetPeggyByBlock(CBlock *pblock, CBlockIndex *pindex)
{
    cJSON *array, *arrayObj, *header, *peggybase, *peggypayments, *peggytx;
    header = cJSON_CreateObject();
    peggybase = cJSON_CreateObject();
    peggypayments = cJSON_CreateArray();
    peggytx = cJSON_CreateArray();
    array = cJSON_CreateArray();
    arrayObj = cJSON_CreateObject();
    //header
    jaddnum(header, "blocknum", pindex->nHeight+1);
    jaddnum(header, "blocktimestamp", pblock->nTime);
    jaddstr(header, "blockhash", (char*)pindex->GetBlockHash().ToString().c_str());

    jadd(arrayObj, "header", header);

    if(!pblock->vtx[2].IsPeggyBase())
        throw runtime_error(
            "Block Does not contain a peggybase transaction!\n"
        );

    int index, vouts;

    //peggypayments
    char bits[4096];
    char hex[4096];

    bool fPeggy = false;
    uint64_t nPeggyPayments = 0;

    for(index=0; index<pblock->vtx.size(); index++){

        const CTransaction tempTx = pblock->vtx[index];

        if(tempTx.IsCoinBase() || tempTx.IsCoinStake())
            continue;

        if(tempTx.IsPeggyBase()){
            const CTxOut peggyOut = tempTx.vout[0];
            const CScript priceFeed = peggyOut.scriptPubKey;

            char peggybits[4096];
            strcpy(peggybits, (char*)HexStr(priceFeed.begin(), priceFeed.end(), false).c_str()+2);

            jaddstr(peggybase, "txid", (char*)tempTx.GetHash().ToString().c_str());
            jaddstr(peggybase, "peggybase", peggybits);
            jaddnum(peggybase, "time", tempTx.nTime);
            jaddnum(peggybase, "txind", 2);
            jaddnum(peggybase, "voutind", 0);
            jaddstr(peggybase, "address", "peggypayments");
            fPeggy = true;
        }

        for(vouts=0; vouts<tempTx.vout.size(); vouts++){

            const CTxOut tempVout = tempTx.vout[vouts];

            const CScript tempScriptPubKey = tempVout.scriptPubKey;

            CTxDestination destAddress;
            strcpy(hex, (char*)HexStr(tempScriptPubKey.begin(), tempScriptPubKey.end(), false).c_str()+2);
            sprintf(hex, "%s", hex);

            if(fPeggy && vouts != 0){

                cJSON *peggyOut = cJSON_CreateObject();


                jaddstr(peggyOut, "txid", (char*)tempTx.GetHash().ToString().c_str());
                jaddnum(peggyOut, "time", tempTx.nTime);
                jaddnum(peggyOut, "txind", index);
                jaddnum(peggyOut, "amount", tempVout.nValue);
                jaddnum(peggyOut, "voutind", vouts);
                jaddstr(peggyOut, "scriptPubKey", hex);

                if(ExtractDestination(tempScriptPubKey, destAddress)){
                    jaddstr(peggyOut, "address", (char*)CBitcoinAddress(destAddress).ToString().c_str());
                }
                else{
                    jaddstr(peggyOut, "address", "null");
                }

                nPeggyPayments += (uint64_t)tempVout.nValue;

                jaddi(peggypayments, peggyOut);
            }

            else if(!fPeggy && isOpReturn(hex) == 0){ //peggy lock found

                cJSON *lockVout = cJSON_CreateObject();

                jaddstr(lockVout, "txid", (char*)tempTx.GetHash().ToString().c_str());
                jaddnum(lockVout, "time", tempTx.nTime);
                jaddnum(lockVout, "txind", index);
                jaddnum(lockVout, "voutind", vouts);
                jaddstr(lockVout, "scriptPubKey", hex);

                if(ExtractDestination(tempScriptPubKey, destAddress)){
                    jaddstr(lockVout, "address", (char*)CBitcoinAddress(destAddress).ToString().c_str());
                }
                else{
                    jaddstr(lockVout, "address", "null");
                }

                jaddnum(lockVout, "amount", (uint64_t)tempVout.nValue);

                jaddi(peggytx, lockVout);

                continue; //1 op_return per tx
            }

        }
    }
    jaddnum(peggybase, "amount", nPeggyPayments);

    jadd(arrayObj, "peggybase", peggybase);
    jadd(arrayObj, "peggypayments", peggypayments);
    jadd(arrayObj, "peggytx", peggytx);

    jaddi(array, arrayObj);
    return jprint(array,0);
}

extern "C" char* GetPeggyByHeight(uint32_t blocknum) //(0-based)
{
    CBlockIndex *pindex = FindBlockByHeight(blocknum+1);
    CBlock block;
    if(!block.ReadFromDisk(pindex, true))
        throw runtime_error(
            "Could not find block\n"
        );
    return GetPeggyByBlock(&block, pindex);
}


/*
*
*
*   Begin RPC functions
*
*/

Value peggytx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2)
        throw runtime_error(
            "peggytx\n"
            "Creates a peggy transaction: \n"
            "'<json string>' '{\"<btcd addr>\" : <amount>}' [send?]\n"
            );
    std::string retVal("");
    const std::string peggyJson = params[0].get_str();
    const Object& sendTo = params[1].get_obj();
    bool signAndSend = false;
    if (params.size() > 2)
        signAndSend = params[2].get_bool();
    const Pair& out = sendTo[0];

    CBitcoinAddress returnAddr = CBitcoinAddress(out.name_);
    if (!returnAddr.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid BitcoinDark address: ")+out.name_);

    int64_t amountLocked = AmountFromValue(out.value_);


    retVal = "Json: " + peggyJson + "\n";
    retVal += "Address: " + out.name_;
    retVal += "\nAmount: ";
    stringstream ss;
    ss << amountLocked;
    retVal += ss.str() + "\n";
    retVal += "Send: ";
    retVal += signAndSend ? "yes" : "no\n";
    return retVal;
}

Value getpeggyblock(const Array& params, bool fHelp)
{
    if(fHelp || params.size() != 1)
        throw runtime_error(
            "getpeggyblock <blockheight>\n"
            "returns all peggy information about a block\n"
        );
    int64_t nHeight = params[0].get_int64();

    if(nHeight < nMinPeggyHeight)
        throw runtime_error(
            "getpeggyblock <blockheight>\n"
            "the block height you entered is not a peggy block\n"
        );

    char *peggy = GetPeggyByHeight(nHeight);
    std::string retVal(peggy);
    free(peggy);
    return retVal;

}
Value peggypayments(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1)
        throw runtime_error(
            "peggypayments <block height>\n"
            "Shows all redeems for a certain block: \n"
            "(-1 for latest block)\n"
            );


        int64_t nHeight, nBlockTime;

        nHeight = params[0].get_int64();

        if(nHeight != -1){

            if(nHeight < nMinPeggyHeight || nHeight > pindexBest->nHeight)
                throw runtime_error(
                    "peggypayments <block height> <block time>\n"
                    "the block height you entered is not a peggy block, or the height is out of range\n"
                );


            CBlockIndex *pindex = FindBlockByHeight(nHeight);
            nHeight = pindex->nHeight;
            nBlockTime = pindex->GetBlockTime();
        }
        else{
            nHeight = pindexBest->nHeight;
            nBlockTime = pindexBest->GetBlockTime();
        }
        CWallet wallet;
        char *paymentScript = peggypayments(nHeight+1, nBlockTime);
        //char *priceFeedHash = peggybase(nHeight, nBlockTime);

        std::string retVal = std::string(paymentScript);
        free(paymentScript);
        return std::string(paymentScript);
}
#endif