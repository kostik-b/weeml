# weeml
* =================================================================
* This file is part of “Ecoscale Weeml Library software” (hereinafter the “Software”)
*
* =================================================================
* Developer(s):     Konstantin Bakanov, The Queen's University of Belfast, Northern Ireland.
* Principal
* Investigator(s):      Prof. Dimitrios Nikolopoulos, The Queen's University of Belfast, Northern Ireland.
*
* The project (“Energy-efficient Heterogeneous COmputing at exaSCALE - EcoScale”) leading to this Software has received 
* funding from the European Commission under 671632 — ECOSCALE — H2020-FETHPC-2014/H2020-FETHPC-2014.
*
* Copyright 2015-2019, The Queen's University of Belfast, Northern Ireland
* =================================================================
* Licensed under the GNU Lesser General Public License, as published by the Free Software Foundation, either version 3 or at your option any later version (hereinafter the "License");
* you must not use this Software except in compliance with the terms of the License.
*
* Unless required by applicable law or agreed upon in writing, this Software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, neither express nor implied.
* For details see terms of the License (see attached file: README. The License is also available at https://www.gnu.org/licenses/lgpl-3.0.en.html
* =================================================================

1. Purpose.
   The purpose of weeml (WEE Messaging Layer) library is to enable easy reliable p2p connections.
   This is achieved by using KCP library, which provides reliable UDP.
   KCP in and of itself does not perform any UDP operations - it operates
   in the application layer.
   By itself KCP is not very useful - it is meant to be integrated into applications, when there is a need for a reliable UDP.
   This work aims to provide a simple wrapper around KCP, so that it could be used to setup easy and ad-hoc 
   connections between 2 peers.

2. Compilation and setup.
    Weeml needs pthread and c++11 libraries.
    Compilation can be done out-of-source-tree by using make's -f flag, E.g. "make -f PATH_TO_SRC/Makefile".
    This will build the library itself as well as 3 test cases (each is made of 2 files). See tests directory for more info.
    net.core.rmem_max and net.core.rmem_default need to be set to large values (e.g. 26214400) so as not to drop packets.

3. Usage.
   -  Create a handle. For listening socket peer ip and port can be ignored - these will
      be copied from the first incoming message. For sending socket, own ip and port
      can be ignored - these will be randomly allocated.
   -  Send a message: perform a send call. This adds the message to KCP's queue of segments and attempts to flush it immediately.
   -  Receive a message - either continually poll on "peek" method followed by "recv_msg" call or register a callback function.

4. TODOs and IDEAs. 
    - How do I detect connection breakdown? Can I use any of the KCP functionality?
      e.g. is there anything in the ikcp_flush function? When do I report an error? Probably in the _send call.
    - Maybe allow setting mtu and window size?
    - Introduce priming messages: this will allow to establish connection - signal to the user - and exchange peer address.
    - Introduce a server socket-like interface with a accept-like function. This would work well with priming messages as we'd
      have to define a connection protocol anyway.
    - The drawback of the current implementation is that it is mostly suited for small messages (see below for more info).
      When integrating the current implementation into the wider system the following solution is possible:
      * Setup a control channel, where small messages and other information would be exchanged.
      * When a larger message needs to be sent a separate ad-hoc connection can be setup. The first message
        would be a description/confirmation of the message to be sent. The next messages of the size of 1 MTU
        would be the actual data transmission. The receiving end would have them written directly into the receiving
        buffer. The last message would be the md5sum (-> "picosha" project).
    - TheBuffer has a default size of 100, what if it is not enough? Need to report error back to the user.
      
5. Clarification bits.
    - MTU size - set by calling ikcp_setmtu(). This is the actual buffer size that is used to send a message.
      I.e. it is a buffer that is passed to the "send_udp_msg" function. It cannot be larger than max UDP size, which
      is 65507 bytes. At the moment this is hardcoded to 65507.
      When the user calls Wml::send_msg() inside KCP the entire message is broken down into segments, where
      the max size of a segment is equal to MTU minus KCP header size bytes. If the entire message fits into
      one segment, then it is not broken down.
      In case of multi-segment messages each segment carries a fragment number, the maximum value of which
      is 2^8=256. I.e. the largest theoretical message we can send is 256*65507=~16MB. However, the ikcp_send()
      function has a check, which does not allow any more than 128 fragments, which means that max real message
      size is ~8MB. Obviously, setting MTU to a lower value will reduct the max message size.
    - Another parameter is ikcp_wndsize (), which allows to set the sending and receiving window sizes.
      Sending window seems to indicate the maximum number of segments that can be sent in one go. KCP
      peers also exchange the information about remote window size, so that a number of segments will be the lesser
      of sending window and of the remote window size.
      Likewise receiving window seems to be related to the number of segments that can be received in one go.
      WEEML keeps those set to their default values: 128 segments for receive window and 32 segments for send
      window.
      
