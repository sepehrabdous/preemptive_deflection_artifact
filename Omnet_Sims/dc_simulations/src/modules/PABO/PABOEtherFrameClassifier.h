//
// Copyright (C) 2011 Zoltan Bojthe
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_PABOETHERFRAMECLASSIFIER_H
#define __INET_PABOETHERFRAMECLASSIFIER_H

#include "inet/queueing/base/PacketClassifierBase.h"

using namespace inet;

/**
 * Ethernet Frame classifier.
 *
 * Ethernet frames are classified as:
 * - PAUSE frames
 * - others
 */
class INET_API PABOEtherFrameClassifier : public queueing::PacketClassifierBase
{
  public:
    /**
     * Sends the incoming packet to either the first or the second gate.
     */
    virtual int classifyPacket(Packet *packet) override;
};

#endif // ifndef __INET_PABOETHERFRAMECLASSIFIER_H

