//
//  JurisdictionListener.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>

#include <OctalCode.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include "JurisdictionListener.h"

JurisdictionListener::JurisdictionListener(NodeType_t type) :
    _nodeType(type),
    _packetSender(JurisdictionListener::DEFAULT_PACKETS_PER_SECOND)
{
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &JurisdictionListener::nodeKilled);
    
    // tell our NodeList we want to hear about nodes with our node type
    NodeList::getInstance()->addNodeTypeToInterestSet(type);
}

void JurisdictionListener::nodeKilled(SharedNodePointer node) {
    if (_jurisdictions.find(node->getUUID()) != _jurisdictions.end()) {
        _jurisdictions.erase(_jurisdictions.find(node->getUUID()));
    }
}

bool JurisdictionListener::queueJurisdictionRequest() {
    static unsigned char buffer[MAX_PACKET_SIZE];
    unsigned char* bufferOut = &buffer[0];
    ssize_t sizeOut = populatePacketHeader(reinterpret_cast<char*>(bufferOut), PacketTypeJurisdictionRequest);
    int nodeCount = 0;

    NodeList* nodeList = NodeList::getInstance();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (node->getType() == getNodeType() && node->getActiveSocket()) {
            _packetSender.queuePacketForSending(node, QByteArray(reinterpret_cast<char*>(bufferOut), sizeOut));
            nodeCount++;
        }
    }

    if (nodeCount > 0){
        _packetSender.setPacketsPerSecond(nodeCount);
    } else {
        _packetSender.setPacketsPerSecond(NO_SERVER_CHECK_RATE);
    }
    
    // keep going if still running
    return isStillRunning();
}

void JurisdictionListener::processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet) {
    if (packetTypeForPacket(packet) == PacketTypeJurisdiction && sendingNode) {
        QUuid nodeUUID = sendingNode->getUUID();
        JurisdictionMap map;
        map.unpackFromMessage(reinterpret_cast<const unsigned char*>(packet.data()), packet.size());
        _jurisdictions[nodeUUID] = map;
    }
}

bool JurisdictionListener::process() {
    bool continueProcessing = isStillRunning();

    // If we're still running, and we don't have any requests waiting to be sent, then queue our jurisdiction requests
    if (continueProcessing && !_packetSender.hasPacketsToSend()) {
        queueJurisdictionRequest();
    }
    
    if (continueProcessing) {
        continueProcessing = _packetSender.process();
    }
    if (continueProcessing) {
        // NOTE: This will sleep if there are no pending packets to process
        continueProcessing = ReceivedPacketProcessor::process();
    }
    
    return continueProcessing;
}
