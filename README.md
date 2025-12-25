# Komodo Classic (KMDCL)

> **⚠️ WARNING: kmdclassic is experimental and a work-in-progress. Use at your own risk.**

## Overview

Komodo Classic (KMDCL) is a project that aims to restore Komodo to its original vision as conceived by its creator, jl777. Despite the fundraising campaign not reaching its goal, we have decided to proceed with the implementation because we believe in the original purpose of KMD as it was originally designed.

## Project Website

Visit [https://kmdclassic.com/](https://kmdclassic.com/) for more information about the project.

## Community & Updates

For the latest news and updates, join our Telegram channel: [https://t.me/komodoclassic](https://t.me/komodoclassic)

<div align="center">
  <img src="icons/kmdclassic.png" alt="Komodo Classic" />
</div>

## Key Features & Advantages Over Original KMD

Komodo Classic will restore and maintain the core features that made Komodo unique:

- **Private Transactions**: KMDCL will include private transaction capabilities, restoring the privacy features that were part of the original Komodo vision.

- **5% APR User Rewards (Interest)**: Just as it was in the original KMD, KMDCL will feature a 5% Annual Percentage Rate (APR) reward system for users, providing interest on holdings as originally intended by the creator.

## Technical Foundation

This project is built on the **KomodoOcean** codebase, which was chosen for several important reasons:

- **GUI Support**: Full Qt-based graphical user interface for an enhanced user experience
- **Cross-Platform**: Native support for Windows, Linux, and macOS
- **Performance**: This full node implementation is faster and more progressive than the original `komodod`
- **Modern Architecture**: Built on a more advanced and maintainable codebase

## Building

For detailed build instructions, see [HOW-TO-BUILD.md](HOW-TO-BUILD.md).

## Dormancy Hardfork

The Dormancy hardfork is scheduled for **block 4771595**, which is estimated to occur around **January 5-6**. For more accurate timing and real-time countdown, please visit [https://countdown.kmdclassic.com/](https://countdown.kmdclassic.com/).

## Project Status

Currently, **v1.0.0-beta25** is undergoing testing and will soon transition to **rc1** (Release Candidate). All main features that were promised have been tested and are working.

## Important Notice for Current Notary Nodes (S8)

**⚠️ Important for Current Season 8 (S8) Notary Nodes:**

With the dPoW Sunset, you will no longer be able to mine blocks with difficulty 1 (easy-mining) on the original KMD (Komodo) network. Maintaining nodes for the original KMD may become impractical for you. However, you can easily switch to **KMDCL** (!), as with the Dormancy upgrade we have kept the notary nodes list the same. This means that active KMD notary nodes can continue to mine KMDCL blocks with reduced difficulty, i.e., they can effectively contribute to supporting the KMDCL (Komodo Classic) network and become validator nodes.

To make the switch:
1. A few hours before the hardfork, stop the KMD daemon
2. Rename the `.komodo` folder to `.kmdclassic`
3. Rename the configuration file `komodo.conf` to `kmdclassic.conf`
4. Start the `kmdclassicd` daemon as usual, enabling mining, etc., with your public key

After the Dormancy hardfork occurs, you will receive **3 KMDCL** for each block you create. For splitfunds, instead of iguana, you can use the new RPC command `nn_split` (check the help for details). More detailed instructions will be published by us shortly.

## Origins & Vision

Komodo Classic traces its roots back to the original vision of jl777, the creator of Komodo. This project represents an effort to preserve and implement the foundational principles and features that were part of Komodo's original design, ensuring that the community has access to a version of Komodo that stays true to its creator's initial intent.

## Disclaimer

**kmdclassic is experimental and a work-in-progress. Use at your own risk.**

This software is provided "as is" without warranty of any kind. The development team is not responsible for any loss of funds or other damages that may occur from using this software.

