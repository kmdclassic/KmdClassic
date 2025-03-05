// Copyright (c) 2016 Jack Grigg
// Copyright (c) 2016 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "arith_uint256.h"
#include "crypto/sha256.h"
#include "crypto/equihash.h"
#include "uint256.h"
#include "librustzcash.h"

/* for EquihashTestsBoost fixture */
#include "key.h"
#include "util.h"
#include "chainparams.h"
#include "main.h"
#include "noui.h"
#include "komodo_globals.h"

#include <sstream>
#include <set>
#include <vector>
#include <iostream>
#include <functional>

#include <gtest/gtest.h>

namespace EquihashTestsBoost
{

    class EquihashTestsBoost : public ::testing::Test
    {
    private:
        bool OldfPrintToDebugLog;
        bool OldfCheckBlockIndex;
    public:
        EquihashTestsBoost() : OldfPrintToDebugLog(fPrintToDebugLog), OldfCheckBlockIndex(fCheckBlockIndex)
        {
            assert(init_and_check_sodium() != -1);
            fPrintToDebugLog = false;
            fCheckBlockIndex = true;
            STAKED_NOTARY_ID = -1;
            SelectParams(CBaseChainParams::REGTEST);
            chainName = assetchain();
            noui_connect();
        }
        ~EquihashTestsBoost() override
        {
            fCheckBlockIndex = OldfCheckBlockIndex;
            fPrintToDebugLog = OldfPrintToDebugLog;
        }
    };

    static void PrintSolution(std::stringstream &strm, const std::vector<uint32_t> &soln)
    {
        strm << "  {";
        const char *separator = "";
        for (uint32_t index : soln)
        {
            strm << separator << index;
            separator = ", ";
        }
        strm << "}";
    }

    static void PrintSolutions(std::stringstream &strm, const std::set<std::vector<uint32_t>> &solns)
    {
        strm << "{";
        const char *soln_separator = "";
        for (const std::vector<uint32_t> &soln : solns)
        {
            strm << soln_separator << "\n";
            soln_separator = ",";
            PrintSolution(strm, soln);
        }
        strm << "\n}";
    }

    #ifdef ENABLE_MINING
    static void TestEquihashSolvers(unsigned int n, unsigned int k, const std::string &I, const arith_uint256 &nonce, const std::set<std::vector<uint32_t>> &expected_solns) {
        size_t cBitLen = n / (k + 1);
        eh_HashState state;
        EhInitialiseState(n, k, state);
        uint256 V = ArithToUint256(nonce);
        std::cout << "Running solver: n = " << n << ", k = " << k << ", I = " << I << ", V = " << V.GetHex() << std::endl;
        state.Update((unsigned char*)&I[0], I.size());
        state.Update(V.begin(), V.size());

        // Test basic solver
        std::set<std::vector<uint32_t>> ret;
        std::function<bool(std::vector<unsigned char>)> validBlock =
                [&ret, cBitLen](std::vector<unsigned char> soln) {
            ret.insert(GetIndicesFromMinimal(soln, cBitLen));
            return false;
        };
        EhBasicSolveUncancellable(n, k, state, validBlock);
        std::cout << "[Basic] Number of solutions: " << ret.size() << std::endl;
        {
            std::stringstream strm;
            PrintSolutions(strm, ret);
            std::cout << strm.str() << std::endl;
        }
        EXPECT_EQ(ret, expected_solns);

        // Test optimised solver
        std::set<std::vector<uint32_t>> retOpt;
        std::function<bool(std::vector<unsigned char>)> validBlockOpt =
                [&retOpt, cBitLen](std::vector<unsigned char> soln) {
            retOpt.insert(GetIndicesFromMinimal(soln, cBitLen));
            return false;
        };
        EhOptimisedSolveUncancellable(n, k, state, validBlockOpt);
        std::cout << "[Optimised] Number of solutions: " << retOpt.size() << std::endl;
        {
            std::stringstream strm;
            PrintSolutions(strm, retOpt);
            std::cout << strm.str() << std::endl;
        }
        EXPECT_EQ(retOpt, expected_solns);
        EXPECT_EQ(retOpt, ret);
    }
    #endif

    static void TestEquihashValidator(unsigned int n, unsigned int k, const std::string &I, const arith_uint256 &nonce, const std::vector<uint32_t>& soln, bool expected) {
        size_t cBitLen = n / (k + 1);
        auto minimal = GetMinimalFromIndices(soln, cBitLen);

        uint256 V = ArithToUint256(nonce);
        std::cout << "Running validator: n = " << n << ", k = " << k << ", I = " << I << ", V = " << V.GetHex() 
                << ", expected = " << expected << ", soln =" << std::endl;
        {
            std::stringstream strm;
            PrintSolution(strm, soln);
            std::cout << strm.str() << std::endl;
        }

        bool isValid = librustzcash_eh_isvalid(
            n, k,
            (unsigned char*)&I[0], I.size(),
            V.begin(), V.size(),
            minimal.data(), minimal.size());
        EXPECT_EQ(isValid, expected);
    }

    #ifdef ENABLE_MINING
    TEST_F(EquihashTestsBoost, SolverTestvectors) {
        TestEquihashSolvers(96, 5, "block header", 0, {
        {976, 126621, 100174, 123328, 38477, 105390, 38834, 90500, 6411, 116489, 51107, 129167, 25557, 92292, 38525, 56514, 1110, 98024, 15426, 74455, 3185, 84007, 24328, 36473, 17427, 129451, 27556, 119967, 31704, 62448, 110460, 117894},
        {1008, 18280, 34711, 57439, 3903, 104059, 81195, 95931, 58336, 118687, 67931, 123026, 64235, 95595, 84355, 122946, 8131, 88988, 45130, 58986, 59899, 78278, 94769, 118158, 25569, 106598, 44224, 96285, 54009, 67246, 85039, 127667},
        {1278, 107636, 80519, 127719, 19716, 130440, 83752, 121810, 15337, 106305, 96940, 117036, 46903, 101115, 82294, 118709, 4915, 70826, 40826, 79883, 37902, 95324, 101092, 112254, 15536, 68760, 68493, 125640, 67620, 108562, 68035, 93430},
        {3976, 108868, 80426, 109742, 33354, 55962, 68338, 80112, 26648, 28006, 64679, 130709, 41182, 126811, 56563, 129040, 4013, 80357, 38063, 91241, 30768, 72264, 97338, 124455, 5607, 36901, 67672, 87377, 17841, 66985, 77087, 85291},
        {5970, 21862, 34861, 102517, 11849, 104563, 91620, 110653, 7619, 52100, 21162, 112513, 74964, 79553, 105558, 127256, 21905, 112672, 81803, 92086, 43695, 97911, 66587, 104119, 29017, 61613, 97690, 106345, 47428, 98460, 53655, 109002}
                    });
        TestEquihashSolvers(96, 5, "block header", 1, {
        {1911, 96020, 94086, 96830, 7895, 51522, 56142, 62444, 15441, 100732, 48983, 64776, 27781, 85932, 101138, 114362, 4497, 14199, 36249, 41817, 23995, 93888, 35798, 96337, 5530, 82377, 66438, 85247, 39332, 78978, 83015, 123505}
                    });
        // Additional solver tests can be added here...
    }
    #endif

    TEST_F(EquihashTestsBoost, ValidatorTestvectors) {
        // Original valid solution
        TestEquihashValidator(96, 5, "Equihash is an asymmetric PoW based on the Generalised Birthday problem.", 1,
        {2261, 15185, 36112, 104243, 23779, 118390, 118332, 130041, 32642, 69878, 76925, 80080, 45858, 116805, 92842, 111026, 15972, 115059, 85191, 90330, 68190, 122819, 81830, 91132, 23460, 49807, 52426, 80391, 69567, 114474, 104973, 122568},
                    true);
        // Change one index
        TestEquihashValidator(96, 5, "Equihash is an asymmetric PoW based on the Generalised Birthday problem.", 1,
        {2262, 15185, 36112, 104243, 23779, 118390, 118332, 130041, 32642, 69878, 76925, 80080, 45858, 116805, 92842, 111026, 15972, 115059, 85191, 90330, 68190, 122819, 81830, 91132, 23460, 49807, 52426, 80391, 69567, 114474, 104973, 122568},
                    false);
        // Other validator tests (swapping indices, sorting, duplicates, etc.) go here...
    }

    TEST_F(EquihashTestsBoost, ValidatorAllBitsMatter) {
        unsigned int n = 96;
        unsigned int k = 5;
        uint256 V = ArithToUint256(1);
        std::string I = "Equihash is an asymmetric PoW based on the Generalised Birthday problem.";

        std::vector<uint32_t> soln = {2261, 15185, 36112, 104243, 23779, 118390, 118332, 130041, 32642, 69878, 76925, 80080, 45858, 116805, 92842, 111026, 15972, 115059, 85191, 90330, 68190, 122819, 81830, 91132, 23460, 49807, 52426, 80391, 69567, 114474, 104973, 122568};
        size_t cBitLen = n / (k + 1);
        std::vector<unsigned char> sol_char = GetMinimalFromIndices(soln, cBitLen);

        EXPECT_TRUE(librustzcash_eh_isvalid(
            n, k,
            (unsigned char*)&I[0], I.size(),
            V.begin(), V.size(),
            sol_char.data(), sol_char.size()));

        for (size_t i = 0; i < sol_char.size() * 8; i++) {
            std::vector<unsigned char> mutated = sol_char;
            mutated.at(i / 8) ^= (1 << (i % 8));
            EXPECT_FALSE(librustzcash_eh_isvalid(
                n, k,
                (unsigned char*)&I[0], I.size(),
                V.begin(), V.size(),
                mutated.data(), mutated.size()));
        }
    }
}