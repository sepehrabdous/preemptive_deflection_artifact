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

#ifndef __INET_PABOPRIORITYSCHEDULER_H
#define __INET_PABOPRIORITYSCHEDULER_H

#include "inet/queueing/base/PacketSchedulerBase.h"
#include "inet/queueing/contract/IPacketCollection.h"

using namespace inet;
using namespace queueing;

class INET_API PABOPriorityScheduler : public PacketSchedulerBase, public IPacketCollection
{
  protected:
    std::vector<IPacketCollection *> collections;

  protected:
    virtual void initialize(int stage) override;
    virtual int schedulePacket() override;

  public:
    virtual int getMaxNumPackets() override { return -1; }
    virtual int getNumPackets() override;

    virtual b getMaxTotalLength() override { return b(-1); }
    virtual b getTotalLength() override;

    virtual bool isEmpty() override { return getNumPackets() == 0; }
    virtual Packet *getPacket(int index) override;
    virtual void removePacket(Packet *packet) override;
};


#endif // ifndef __INET_PABOPRIORITYSCHEDULER_H

