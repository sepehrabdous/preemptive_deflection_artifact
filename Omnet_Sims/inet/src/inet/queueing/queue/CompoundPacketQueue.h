//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#ifndef __INET_COMPOUNDPACKETQUEUE_H
#define __INET_COMPOUNDPACKETQUEUE_H

#include "inet/queueing/base/PacketQueueBase.h"
#include "inet/queueing/contract/IPacketCollection.h"
#include "inet/queueing/contract/IPassivePacketSink.h"
#include "inet/queueing/contract/IPassivePacketSource.h"

namespace inet {
namespace queueing {

class INET_API CompoundPacketQueue : public PacketQueueBase
{
  protected:
    int packetCapacity = -1;
    b dataCapacity = b(-1);

    cGate *inputGate = nullptr;
    IPassivePacketSink *consumer = nullptr;

    cGate *outputGate = nullptr;
    IPassivePacketSource *provider = nullptr;
    IPacketCollection *collection = nullptr;
    simsignal_t customQueueLengthSignal;
    simsignal_t customQueueLengthSignalPacketBytes;


  protected:
    virtual void initialize(int stage) override;

  public:
    virtual int getMaxNumPackets() override;
    virtual int getNumPackets() override;

    virtual b getMaxTotalLength() override;
    virtual b getTotalLength() override;

    virtual bool isEmpty() override { return collection->isEmpty(); }
    virtual Packet *getPacket(int index) override;
    virtual void removePacket(Packet *packet) override;

    virtual bool supportsPushPacket(cGate *gate) override { return inputGate == gate; }
    virtual bool canPushPacket(Packet *packet, cGate *gate) override { return true; }
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPopPacket(cGate *gate) override { return outputGate == gate; }
    virtual Packet *canPopPacket(cGate *gate) override { throw cRuntimeError("Invalid operation"); }
    virtual Packet *popPacket(cGate *gate) override;
};

} // namespace queueing
} // namespace inet

#endif // ifndef __INET_COMPOUNDPACKETQUEUE_H

