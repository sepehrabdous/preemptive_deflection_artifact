/*
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/INETUtils.h"
#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/checksum/EthernetCRC.h"
#include "inet/linklayer/common/FcsMode_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/Ieee802SapTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/common/VlanTag_m.h"
#include "./PABOHostEtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/linklayer/ethernet/EthernetCommand_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/linklayer/ieee8022/Ieee8022LlcHeader_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"

using namespace inet;

Define_Module(PABOHostEtherEncap);

simsignal_t PABOHostEtherEncap::encapPkSignal = registerSignal("encapPk");
simsignal_t PABOHostEtherEncap::decapPkSignal = registerSignal("decapPk");
simsignal_t PABOHostEtherEncap::pauseSentSignal = registerSignal("pauseSent");

PABOHostEtherEncap::~PABOHostEtherEncap()
{
    for (auto it : socketIdToSocketMap)
        delete it.second;
}

bool PABOHostEtherEncap::Socket::matches(Packet *packet, const Ptr<const EthernetMacHeader>& ethernetMacHeader)
{
    if (!sourceAddress.isUnspecified() && !ethernetMacHeader->getSrc().isBroadcast() && ethernetMacHeader->getSrc() != sourceAddress)
        return false;
    if (!destinationAddress.isUnspecified() && !ethernetMacHeader->getDest().isBroadcast() && ethernetMacHeader->getDest() != destinationAddress)
        return false;
    if (protocol != nullptr && packet->getTag<PacketProtocolTag>()->getProtocol() != protocol)
        return false;
    if (vlanId != -1 && packet->getTag<VlanInd>()->getVlanId() != vlanId)
        return false;
    return true;
}

void PABOHostEtherEncap::initialize(int stage)
{
    Ieee8022Llc::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        fcsMode = parseFcsMode(par("fcsMode"));
        seqNum = 0;
        WATCH(seqNum);
        totalFromHigherLayer = totalFromMAC = totalPauseSent = 0;
        useSNAP = par("useSNAP");
        WATCH(totalFromHigherLayer);
        WATCH(totalFromMAC);
        WATCH(totalPauseSent);
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        if (par("registerProtocol").boolValue()) {    //FIXME //KUDGE should redesign place of PABOHostEtherEncap and LLC modules
            //register service and protocol
            registerService(Protocol::ethernetMac, gate("upperLayerIn"), nullptr);
            registerProtocol(Protocol::ethernetMac, nullptr, gate("upperLayerOut"));
        }
    }
}

void PABOHostEtherEncap::processCommandFromHigherLayer(Request *msg)
{
    auto ctrl = msg->getControlInfo();
    if (dynamic_cast<Ieee802PauseCommand *>(ctrl) != nullptr)
        handleSendPause(msg);
    else if (auto bindCommand = dynamic_cast<EthernetBindCommand *>(ctrl)) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        Socket *socket = new Socket(socketId);
        socket->sourceAddress = bindCommand->getSourceAddress();
        socket->destinationAddress = bindCommand->getDestinationAddress();
        socket->protocol = bindCommand->getProtocol();
        socket->vlanId = bindCommand->getVlanId();
        socketIdToSocketMap[socketId] = socket;
        delete msg;
    }
    else if (dynamic_cast<EthernetCloseCommand *>(ctrl) != nullptr) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketMap.find(socketId);
        delete it->second;
        socketIdToSocketMap.erase(it);
        delete msg;
        auto indication = new Indication("closed", ETHERNET_I_SOCKET_CLOSED);
        auto ctrl = new EthernetSocketClosedIndication();
        indication->setControlInfo(ctrl);
        indication->addTag<SocketInd>()->setSocketId(socketId);
        send(indication, "transportOut");
    }
    else if (dynamic_cast<EthernetDestroyCommand *>(ctrl) != nullptr) {
        int socketId = check_and_cast<Request *>(msg)->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketMap.find(socketId);
        delete it->second;
        socketIdToSocketMap.erase(it);
        delete msg;
    }
    else
        Ieee8022Llc::processCommandFromHigherLayer(msg);
}

void PABOHostEtherEncap::refreshDisplay() const
{
    Ieee8022Llc::refreshDisplay();
    char buf[80];
    sprintf(buf, "passed up: %ld\nsent: %ld", totalFromMAC, totalFromHigherLayer);
    getDisplayString().setTagArg("t", 0, buf);
}

void PABOHostEtherEncap::processPacketFromHigherLayer(Packet *packet)
{
    delete packet->removeTagIfPresent<DispatchProtocolReq>();
    if (packet->getDataLength() > MAX_ETHERNET_DATA_BYTES)
        throw cRuntimeError("packet length from higher layer (%s) exceeds maximum Ethernet payload length (%s)", packet->getDataLength().str().c_str(), MAX_ETHERNET_DATA_BYTES.str().c_str());

    totalFromHigherLayer++;
    emit(encapPkSignal, packet);

    // Creates MAC header information and encapsulates received higher layer data
    // with this information and transmits resultant frame to lower layer

    // create Ethernet frame, fill it in from Ieee802Ctrl and encapsulate msg in it
    EV_DETAIL << "Encapsulating higher layer packet `" << packet->getName() << "' for MAC\n";

    int typeOrLength = -1;
    if (!useSNAP) {
        auto protocolTag = packet->findTag<PacketProtocolTag>();
        if (protocolTag) {
            const Protocol *protocol = protocolTag->getProtocol();
            if (protocol) {
                int ethType = ProtocolGroup::ethertype.findProtocolNumber(protocol);
                if (ethType != -1)
                    typeOrLength = ethType;
            }
        }
    }
    if (typeOrLength == -1) {
        Ieee8022Llc::encapsulate(packet);
        typeOrLength = packet->getByteLength();
    }
    auto macAddressReq = packet->getTag<MacAddressReq>();
    const auto& ethHeader = makeShared<EthernetMacHeader>();
    ethHeader->setSrc(macAddressReq->getSrcAddress());    // if blank, will be filled in by MAC
    ethHeader->setDest(macAddressReq->getDestAddress());
    ethHeader->setTypeOrLength(typeOrLength);
    packet->insertAtFront(ethHeader);

    packet->insertAtBack(makeShared<EthernetFcs>(fcsMode));

    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
    EV_INFO << "Sending " << packet << " to lower layer.\n";
    send(packet, "lowerLayerOut");
}

const Ptr<const EthernetMacHeader> PABOHostEtherEncap::decapsulateMacHeader(Packet *packet)
{
    auto ethHeader = packet->popAtFront<EthernetMacHeader>();
    packet->popAtBack<EthernetFcs>(ETHER_FCS_BYTES);

    // add Ieee802Ctrl to packet
    auto macAddressInd = packet->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(ethHeader->getSrc());
    macAddressInd->setDestAddress(ethHeader->getDest());

    // remove Padding if possible
    if (isIeee8023Header(*ethHeader)) {
        b payloadLength = B(ethHeader->getTypeOrLength());
        if (packet->getDataLength() < payloadLength)
            throw cRuntimeError("incorrect payload length in ethernet frame");
        packet->setBackOffset(packet->getFrontOffset() + payloadLength);
        packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ieee8022);
    }
    else if (isEth2Header(*ethHeader)) {
        if (auto protocol = ProtocolGroup::ethertype.findProtocol(ethHeader->getTypeOrLength()))
            packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
        else
            delete packet->removeTagIfPresent<PacketProtocolTag>();
    }
    return ethHeader;
}

void PABOHostEtherEncap::processPacketFromMac(Packet *packet)
{
    auto main_frame = packet->peekAtFront<EthernetMacHeader>();

    if (main_frame->getBouncedHop() == 0 && main_frame->getBouncedDistance() != 0)
        throw cRuntimeError("Total bounced count is 0 but frame is indicated as bounced.");

    if (main_frame->getBouncedDistance() < 0)
        throw cRuntimeError("bouncedDistance < 0, something is wrong!");

    if (main_frame->getBouncedDistance() > 0){
        EV << "Bounced packet received at host. Fw it to send buffer." << endl;

        auto frame = packet->removeAtFront<EthernetMacHeader>();
        auto ipHeader = packet->removeAtFront<Ipv4Header>();

        short ttl = ipHeader->getTimeToLive() - 1;
        if (ttl <= 0) {
            EV << "ttl is " << ttl << ". setting ttl to 1!" << endl;
//            emit(bounceLimitPassedSignal, packet->getId());
//            delete packet;
//            return;
            ttl = 1;
        }
        EV << "SEPEHR: packet's old ttl is: " << ipHeader->getTimeToLive() << " and it's new ttl is: " << ttl << endl;
        ipHeader->setTimeToLive(ttl);

        int bounce_distance = frame->getBouncedDistance();
        int total_hop_num = frame->getTotalHopNum();

        if (bounce_distance >= 1) {
            bounce_distance--;
            frame->setBouncedDistance(bounce_distance);
        }
        total_hop_num++;
        frame->setTotalHopNum(total_hop_num);

        packet->insertAtFront(ipHeader);
        packet->insertAtFront(frame);

        EV << "Re-sending bounced back" << packet << " to lower layer.\n";
        send(packet, "lowerLayerOut");
    }
    else {

        const Protocol *payloadProtocol = nullptr;
        auto ethHeader = decapsulateMacHeader(packet);

        // remove llc header if possible
        if (isIeee8023Header(*ethHeader)) {
            Ieee8022Llc::processPacketFromMac(packet);
            return;
        }
        else if (isEth2Header(*ethHeader)) {
            payloadProtocol = ProtocolGroup::ethertype.findProtocol(ethHeader->getTypeOrLength());
            if (payloadProtocol != nullptr) {
                packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
                packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
            }
            else {
                delete packet->removeTagIfPresent<PacketProtocolTag>();
                delete packet->removeTagIfPresent<DispatchProtocolReq>();
            }
            bool stealPacket = false;
            for (auto it : socketIdToSocketMap) {
                auto socket = it.second;
                if (socket->matches(packet, ethHeader)) {
                    auto packetCopy = packet->dup();
                    packetCopy->setKind(ETHERNET_I_DATA);
                    packetCopy->addTagIfAbsent<SocketInd>()->setSocketId(it.first);
                    send(packetCopy, "upperLayerOut");
                    stealPacket |= socket->vlanId != -1;
                }
            }
            // TODO: should the socket configure if it steals packets or not?
            if (stealPacket)
                delete packet;
            else if (payloadProtocol != nullptr && upperProtocols.find(payloadProtocol) != upperProtocols.end()) {
                EV_DETAIL << "Decapsulating frame `" << packet->getName() << "', passing up contained packet `"
                          << packet->getName() << "' to higher layer\n";

                totalFromMAC++;
                emit(decapPkSignal, packet);

                // pass up to higher layers.
                EV_INFO << "Sending " << packet << " to upper layer.\n";
                send(packet, "upperLayerOut");
            }
            else {
                EV_WARN << "Unknown protocol, dropping packet\n";
                PacketDropDetails details;
                details.setReason(NO_PROTOCOL_FOUND);
                emit(packetDroppedSignal, packet, &details);
                delete packet;
            }
        }
        else
            throw cRuntimeError("Unknown ethernet header");
    }
}

void PABOHostEtherEncap::handleSendPause(cMessage *msg)
{
    Ieee802PauseCommand *etherctrl = dynamic_cast<Ieee802PauseCommand *>(msg->getControlInfo());
    if (!etherctrl)
        throw cRuntimeError("PAUSE command `%s' from higher layer received without Ieee802PauseCommand controlinfo", msg->getName());
    MacAddress dest = etherctrl->getDestinationAddress();
    int pauseUnits = etherctrl->getPauseUnits();
    delete msg;

    EV_DETAIL << "Creating and sending PAUSE frame, with duration = " << pauseUnits << " units\n";

    // create Ethernet frame
    char framename[40];
    sprintf(framename, "pause-%d-%d", getId(), seqNum++);
    auto packet = new Packet(framename);
    const auto& frame = makeShared<EthernetPauseFrame>();
    const auto& hdr = makeShared<EthernetMacHeader>();
    frame->setPauseTime(pauseUnits);
    if (dest.isUnspecified())
        dest = MacAddress::MULTICAST_PAUSE_ADDRESS;
    hdr->setDest(dest);
    packet->insertAtFront(frame);
    hdr->setTypeOrLength(ETHERTYPE_FLOW_CONTROL);
    packet->insertAtFront(hdr);
    packet->insertAtBack(makeShared<EthernetFcs>(fcsMode));
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);

    EV_INFO << "Sending " << frame << " to lower layer.\n";
    send(packet, "lowerLayerOut");

    emit(pauseSentSignal, pauseUnits);
    totalPauseSent++;
}

