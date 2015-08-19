#ifdef PEGGY
//
// Created by BTCDDev on 8/17/15.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "bitcoinrpc.h"
#include "kernel.h"

using namespace json_spirit;
using namespace std;
extern "C" char *peggybase(uint32_t blocknum,uint32_t blocktimestamp);
extern "C" char *peggypayments(uint32_t blocknum,uint32_t blocktimestamp);
extern "C" int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry);
Value peggytx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error(
            "peggytx\n"
            "Creates a peggy transaction: '<json string>' <btcd addr> <amount> [send?]");
    if(params.size() >= 4){

    std::string str = params[0].get_str();
    std::string bitcoindarkAddress = params[1].get_str();
    int64_t amount = params[2].get_int64();
    bool fSend = params[3].get_bool();

    }

    char *peg = peggybase((uint32_t)pindexBest->nHeight, (uint32_t)pindexBest->GetBlockTime());
    CTransaction peggyTx = CTransaction();
    peggyTx.vin.resize(1);
    peggyTx.vin[0].prevout.SetNull();
    peggyTx.vin[0].scriptSig.clear();

    peggyTx.vout.resize(1);
    peggyTx.vout[0].nValue = (int64_t)0;
    peggyTx.vout[0].scriptPubKey = CScript();
    int i;
    size_t len = strlen(peg)/2;
    unsigned char buf[4096];
    decode_hex(buf,(int)len,peg);
    for (i=0; i<(int)len; i++)
        peggyTx.vout[0].scriptPubKey << buf[i];
    Object result;
    free(peg);
    TxToJSON(peggyTx, (uint256)0, result);
    return result;

}
#endif