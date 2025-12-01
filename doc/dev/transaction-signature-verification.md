# Transaction Signature Verification

This document provides a detailed explanation of how transaction signature verification is performed in Komodo Classic, with a focus on the validation process within `CheckBlock()` and `ValidateTransactions()`.

## Overview

Transaction signature verification is a critical security mechanism that ensures only the rightful owner of funds can spend them. In Komodo Classic, signature verification happens at multiple stages:

1. **CheckBlock()** - Initial transaction structure validation
2. **CheckTransaction()** - Basic transaction checks
3. **ContextualCheckTransaction()** - Context-dependent validation
4. **ConnectBlock()** - Full signature verification when connecting blocks

## High-Level Flow

```mermaid
flowchart TD
    A[CheckBlock] --> B[For Each Transaction]
    B --> C[CheckTransaction]
    C --> D[CheckTransactionWithoutProofVerification]
    C --> E[Verify zk-SNARK Proofs]
    B --> F[ContextualCheckBlock]
    F --> G[ContextualCheckTransaction]
    G --> H[Verify JoinSplit Signatures]
    G --> I[Verify Sapling Signatures]
    B --> J[ConnectBlock]
    J --> K[CheckTxInputs]
    K --> L[VerifyScript for Each Input]
    L --> M[SignatureHash]
    L --> N[ECDSA Verification]
    
    style A fill:#fff9c4
    style C fill:#e1bee7
    style G fill:#e1bee7
    style L fill:#bbdefb
    style N fill:#c8e6c9
```

## Detailed Verification Flow

### 1. CheckBlock() - Transaction Validation Entry Point

**Location:** `src/main.cpp:5169`

Within `CheckBlock()`, transactions are validated in a loop:

```cpp
for (uint32_t i = 0; i < block.vtx.size(); i++)
{
    const CTransaction& tx = block.vtx[i];
    if (!CheckTransaction(tiptime,tx, state, verifier, i, (int32_t)block.vtx.size()))
        return error("CheckBlock: CheckTransaction failed");
}
```

**Flow:**
```mermaid
flowchart TD
    A[CheckBlock] --> B[Loop Through block.vtx]
    B --> C[CheckTransaction]
    C --> D{Valid?}
    D -->|No| E[Return Error]
    D -->|Yes| F[Next Transaction]
    F --> G{More Transactions?}
    G -->|Yes| B
    G -->|No| H[Check SigOps Count]
    H --> I[Return Success]
    
    style A fill:#fff9c4
    style C fill:#e1bee7
    style E fill:#ffcdd2
    style I fill:#c8e6c9
```

### 2. CheckTransaction() - Basic Transaction Validation

**Location:** `src/main.cpp:1378`

This function performs basic transaction structure validation:

```mermaid
flowchart TD
    A[CheckTransaction] --> B{Banned TX Check}
    B -->|KMD Chain| C[Check Banned TX IDs]
    B -->|Other| D[Skip]
    C --> E{Using Banned TX?}
    E -->|Yes| F[Return false]
    E -->|No| G[CheckTransactionWithoutProofVerification]
    G --> H{Valid?}
    H -->|No| F
    H -->|Yes| I{Has JoinSplits?}
    I -->|Yes| J[Verify zk-SNARK Proofs]
    I -->|No| K[Return true]
    J --> L{Valid?}
    L -->|No| F
    L -->|Yes| K
    
    style A fill:#e1bee7
    style F fill:#ffcdd2
    style K fill:#c8e6c9
```

**Key Checks:**
- Banned transaction validation (KMD-specific)
- Staking transaction validation
- Basic transaction structure via `CheckTransactionWithoutProofVerification()`
- zk-SNARK proof verification for JoinSplits

### 3. ContextualCheckTransaction() - Context-Dependent Validation

**Location:** `src/main.cpp:1150`

This function performs context-dependent checks including signature verification for shielded transactions:

```mermaid
flowchart TD
    A[ContextualCheckTransaction] --> B[Check Version/Overwinter Flags]
    B --> C[Check Expiry Height]
    C --> D[Check Size Limits]
    D --> E{Has JoinSplits/Shielded?}
    E -->|Yes| F[Compute SignatureHash]
    E -->|No| G[Skip Signature Hash]
    F --> H{Has JoinSplits?}
    H -->|Yes| I[Verify JoinSplit Signature]
    I --> J{Valid?}
    J -->|No| K[Return false]
    J -->|Yes| L{Has Sapling?}
    H -->|No| L
    L -->|Yes| M[Verify Sapling Spend Auth]
    M --> N[Verify Sapling Output Proofs]
    N --> O[Verify Sapling Binding Signature]
    O --> P{All Valid?}
    P -->|No| K
    P -->|Yes| Q[Return true]
    G --> Q
    
    style A fill:#e1bee7
    style I fill:#bbdefb
    style M fill:#bbdefb
    style K fill:#ffcdd2
    style Q fill:#c8e6c9
```

#### 3.1 JoinSplit Signature Verification

For transactions with JoinSplits, the signature is verified using libsodium:

```cpp
if (!(tx.IsMint() || tx.vjoinsplit.empty()))
{
    if (crypto_sign_verify_detached(&tx.joinSplitSig[0],
                                    dataToBeSigned.begin(), 32,
                                    tx.joinSplitPubKey.begin()) != 0) {
        return state.DoS(..., "bad-txns-invalid-joinsplit-signature");
    }
}
```

**Process:**
1. Compute `dataToBeSigned` using `SignatureHash()` with `NOT_AN_INPUT`
2. Verify detached signature using Ed25519 (libsodium)
3. Signature must match `joinSplitPubKey`

#### 3.2 Sapling Signature Verification

For Sapling transactions, multiple signature checks are performed:

```mermaid
flowchart TD
    A[Sapling Verification] --> B[Init Verification Context]
    B --> C[For Each Spend]
    C --> D[Check Spend Auth Signature]
    D --> E[Verify zk-SNARK Proof]
    C --> F[For Each Output]
    F --> G[Verify Output zk-SNARK Proof]
    C --> H[Final Check]
    H --> I[Verify Binding Signature]
    I --> J{All Valid?}
    J -->|Yes| K[Success]
    J -->|No| L[Failure]
    
    style A fill:#bbdefb
    style D fill:#fff9c4
    style I fill:#fff9c4
    style K fill:#c8e6c9
    style L fill:#ffcdd2
```

**Components:**
- **Spend Auth Signatures:** Each shielded spend includes an authentication signature
- **Output Proofs:** Each shielded output includes a zk-SNARK proof
- **Binding Signature:** A single signature that binds all shielded inputs/outputs

### 4. ConnectBlock() - Full Signature Verification

**Location:** `src/main.cpp:3314`

When connecting a block to the chain, full ECDSA signature verification is performed for transparent transactions:

```mermaid
flowchart TD
    A[ConnectBlock] --> B[CheckBlock Again]
    B --> C[For Each Transaction]
    C --> D{Is Coinbase?}
    D -->|Yes| E[Skip Input Checks]
    D -->|No| F[CheckTxInputs]
    F --> G[Consensus::CheckTxInputs]
    G --> H{All Inputs Valid?}
    H -->|No| I[Return false]
    H -->|Yes| J{fScriptChecks?}
    J -->|Yes| K[For Each Input]
    K --> L[Create CScriptCheck]
    L --> M["CScriptCheck::operator()"]
    M --> N[VerifyScript]
    N --> O{Valid?}
    O -->|No| I
    O -->|Yes| P{More Inputs?}
    P -->|Yes| K
    P -->|No| Q[Next Transaction]
    J -->|No| Q
    Q --> R{More Transactions?}
    R -->|Yes| C
    R -->|No| S[Return true]
    
    style A fill:#fff9c4
    style F fill:#e1bee7
    style N fill:#bbdefb
    style I fill:#ffcdd2
    style S fill:#c8e6c9
```

### 5. CScriptCheck - Individual Input Verification

**Location:** `src/main.cpp:2716`

Each transparent transaction input is verified using `CScriptCheck`:

```cpp
bool CScriptCheck::operator()() {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    ServerTransactionSignatureChecker checker(ptxTo, nIn, amount, cacheStore, *txdata);
    if (!VerifyScript(scriptSig, scriptPubKey, nFlags, checker, consensusBranchId, &error)) {
        return ::error("CScriptCheck(): %s:%d VerifySignature failed: %s", ...);
    }
    return true;
}
```

**Flow:**
```mermaid
flowchart TD
    A[CScriptCheck::operator] --> B[Get scriptSig from Input]
    B --> C[Create ServerTransactionSignatureChecker]
    C --> D[VerifyScript]
    D --> E[EvalScript scriptSig]
    E --> F[EvalScript scriptPubKey]
    F --> G{Stack Valid?}
    G -->|No| H[Return false]
    G -->|Yes| I{P2SH?}
    I -->|Yes| J[EvalScript Redeem Script]
    J --> K{Valid?}
    K -->|No| H
    K -->|Yes| L[Return true]
    I -->|No| L
    
    style A fill:#bbdefb
    style D fill:#fff9c4
    style H fill:#ffcdd2
    style L fill:#c8e6c9
```

### 6. VerifyScript() - Script Execution

**Location:** `src/script/interpreter.cpp:1500`

This is the core function that executes Bitcoin script and verifies signatures:

```mermaid
flowchart TD
    A[VerifyScript] --> B[Check SIGPUSHONLY Flag]
    B --> C{CryptoConditions?}
    C -->|Yes| D[EvalCryptoConditionSig]
    C -->|No| E[EvalScript scriptSig]
    D --> F[EvalScript scriptPubKey]
    E --> F
    F --> G{Stack Empty?}
    G -->|Yes| H[Return false]
    G -->|No| I{Top Value True?}
    I -->|No| H
    I -->|Yes| J{P2SH?}
    J -->|Yes| K[Restore Stack]
    K --> L[EvalScript Redeem Script]
    L --> M{Valid?}
    M -->|No| H
    M -->|Yes| N[Check CLEANSTACK]
    J -->|No| N
    N --> O[Return true]
    
    style A fill:#fff9c4
    style E fill:#bbdefb
    style F fill:#bbdefb
    style H fill:#ffcdd2
    style O fill:#c8e6c9
```

### 7. EvalScript() - Script Interpreter

**Location:** `src/script/interpreter.cpp:193`

The script interpreter executes opcodes. When it encounters `OP_CHECKSIG` or `OP_CHECKSIGVERIFY`, it calls the signature checker:

```mermaid
flowchart TD
    A[EvalScript] --> B[Loop Through Opcodes]
    B --> C{OP_CHECKSIG?}
    C -->|Yes| D[Extract Signature]
    C -->|No| E{OP_CHECKSIGVERIFY?}
    E -->|Yes| D
    E -->|No| F[Execute Other Opcode]
    D --> G[Extract Public Key]
    G --> H[TransactionSignatureChecker::CheckSig]
    H --> I[SignatureHash]
    I --> J[VerifySignature]
    J --> K{Valid?}
    K -->|No| L[Return false]
    K -->|Yes| M[Push Result to Stack]
    F --> N{More Opcodes?}
    M --> N
    N -->|Yes| B
    N -->|No| O[Return true]
    
    style A fill:#bbdefb
    style H fill:#fff9c4
    style I fill:#e1bee7
    style J fill:#e1bee7
    style L fill:#ffcdd2
    style O fill:#c8e6c9
```

### 8. SignatureHash() - Computing the Hash to Sign

**Location:** `src/script/interpreter.cpp:1218`

This function computes the hash that will be signed. The process differs based on transaction version:

```mermaid
flowchart TD
    A[SignatureHash] --> B{Transaction Version}
    B -->|Sprout| C[Legacy Signature Hash]
    B -->|Overwinter/Sapling| D[New Signature Hash]
    C --> E[Serialize Transaction Parts]
    E --> F[Hash with SHA256]
    D --> G[Compute Hash Prevouts]
    G --> H[Compute Hash Sequence]
    H --> I[Compute Hash Outputs]
    I --> J[Compute Hash JoinSplits]
    J --> K[Compute Hash Shielded]
    K --> L[Serialize with BLAKE2b]
    L --> M[Include Consensus Branch ID]
    M --> N[Include Input Specific Data]
    N --> O[Return Hash]
    F --> O
    
    style A fill:#e1bee7
    style D fill:#fff9c4
    style L fill:#bbdefb
    style O fill:#c8e6c9
```

**Key Differences:**

1. **Sprout (Legacy):**
   - Uses SHA256
   - Serializes transaction parts based on `SIGHASH` flags
   - No consensus branch ID

2. **Overwinter/Sapling (New):**
   - Uses BLAKE2b with personalization
   - Includes consensus branch ID in personalization
   - Pre-computes hashes of prevouts, sequences, outputs
   - More efficient for transactions with many inputs/outputs

**Signature Hash Components (Overwinter/Sapling):**
- Transaction header (version, versionGroupId)
- Hash of all prevouts (unless SIGHASH_ANYONECANPAY)
- Hash of all sequences (unless SIGHASH_SINGLE/NONE)
- Hash of outputs (varies by SIGHASH type)
- Hash of JoinSplits
- Hash of shielded spends/outputs (Sapling only)
- Locktime
- Expiry height
- Value balance (Sapling only)
- Hash type
- Input-specific: prevout, scriptCode, amount, sequence

### 9. TransactionSignatureChecker::CheckSig() - ECDSA Verification

**Location:** `src/script/interpreter.cpp:1342`

This function performs the actual ECDSA signature verification:

```mermaid
flowchart TD
    A[CheckSig] --> B[Extract Public Key]
    B --> C{Valid PubKey?}
    C -->|No| D[Return false]
    C -->|Yes| E[Extract Hash Type]
    E --> F[Remove Hash Type from Sig]
    F --> G[Compute SignatureHash]
    G --> H{Success?}
    H -->|No| D
    H -->|Yes| I[VerifySignature]
    I --> J[pubkey.Verify]
    J --> K{Valid?}
    K -->|Yes| L[Return true]
    K -->|No| D
    
    style A fill:#fff9c4
    style G fill:#e1bee7
    style J fill:#bbdefb
    style D fill:#ffcdd2
    style L fill:#c8e6c9
```

**Process:**
1. Extract and validate public key from script
2. Extract hash type (last byte of signature)
3. Compute `SignatureHash()` with the hash type
4. Call `pubkey.Verify(sighash, vchSig)` which uses secp256k1 ECDSA verification

### 10. CryptoConditions Support

Komodo Classic also supports CryptoConditions for advanced smart contracts:

```mermaid
flowchart TD
    A[CryptoCondition Check] --> B[Extract Fulfillment]
    B --> C[Parse CryptoCondition]
    C --> D{Supported Type?}
    D -->|No| E[Return false]
    D -->|Yes| F{Signed?}
    F -->|No| E
    F -->|Yes| G[Compute SignatureHash]
    G --> H[cc_verify]
    H --> I{Valid?}
    I -->|Yes| J[Return true]
    I -->|No| E
    
    style A fill:#bbdefb
    style H fill:#fff9c4
    style E fill:#ffcdd2
    style J fill:#c8e6c9
```

## Signature Verification Timing

```mermaid
sequenceDiagram
    participant CB as CheckBlock
    participant CT as CheckTransaction
    participant CCT as ContextualCheckTransaction
    participant Conn as ConnectBlock
    participant SC as CScriptCheck
    participant VS as VerifyScript
    participant SH as SignatureHash
    participant ECDSA as ECDSA Verify

    CB->>CT: Validate structure
    CT->>CCT: Context checks
    CCT->>CCT: JoinSplit/Sapling sigs
    CB->>Conn: Connect to chain
    Conn->>SC: For each input
    SC->>VS: Verify script
    VS->>SH: Compute hash
    SH-->>VS: Return hash
    VS->>ECDSA: Verify signature
    ECDSA-->>VS: Result
    VS-->>SC: Success/Failure
    SC-->>Conn: All inputs valid
```

## Security Considerations

1. **CPU Exhaustion Protection:**
   - Expensive signature checks are deferred until after basic validation
   - All inputs must pass basic checks before signature verification begins

2. **Checkpoint Optimization:**
   - Blocks before the last checkpoint may skip signature verification
   - Merkle root validation still ensures integrity

3. **Signature Caching:**
   - Verified signatures can be cached to avoid redundant checks
   - Controlled by `cacheStore` parameter

4. **Script Flags:**
   - Various flags control script validation behavior
   - `STANDARD_SCRIPT_VERIFY_FLAGS` defines standard validation rules

## Code Locations Summary

| Component | File | Line Range |
|-----------|------|------------|
| CheckBlock | `src/main.cpp` | 5169-5360 |
| CheckTransaction | `src/main.cpp` | 1378-1437 |
| ContextualCheckTransaction | `src/main.cpp` | 1150-1376 |
| ConnectBlock | `src/main.cpp` | 3314-3500+ |
| CScriptCheck::operator() | `src/main.cpp` | 2716-2723 |
| VerifyScript | `src/script/interpreter.cpp` | 1500-1586 |
| EvalScript | `src/script/interpreter.cpp` | 193-1100+ |
| SignatureHash | `src/script/interpreter.cpp` | 1218-1334 |
| TransactionSignatureChecker::CheckSig | `src/script/interpreter.cpp` | 1342-1370 |
| VerifySignature | `src/script/interpreter.cpp` | 1336-1340 |

## Conclusion

Transaction signature verification in Komodo Classic is a multi-layered process that ensures:

1. **Structure Validation:** Basic transaction format is correct
2. **Context Validation:** Transaction is valid for the current block height
3. **Signature Verification:** All cryptographic signatures are valid
4. **Script Execution:** All script conditions are satisfied

The system supports multiple signature types:
- **ECDSA:** For transparent transactions (standard Bitcoin-style)
- **Ed25519:** For JoinSplit transactions (via libsodium)
- **Sapling Signatures:** For shielded transactions (zk-SNARK-based)
- **CryptoConditions:** For advanced smart contracts

Each signature type has its own verification path, but all ultimately ensure that only authorized parties can spend funds.
