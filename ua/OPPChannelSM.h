 /**
 * ,--. ,--, ,--, .--. ,-  ,--,
 * |  | |__| |__| |__   |  |__|
 * |  | |    |       |  |  |
 * '--' '    '    `--` --- '
 *
 * Copyright 2016 by Li Yucun <liycliyc@hotmail.com>
 * 
 * This file is part of OPPSIP.
 * 
 *  OPPSIP is free software:  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OPPSIP is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OPPSIP.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "OPPStateMachine.h"
#include "OPPSipChannel.h"
#include "OPPEvents.h"

class OPPChannelSM :
	public OPPStateMachine<OPPSipChannel,OPPSipEvent>
{
public:
	OPPChannelSM();
	virtual ~OPPChannelSM();

	static OPPChannelSM *sGetInstance();

	static void OnInCallWhileReady( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnRingBackWhileRing( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnAcceptWhileRing( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnSetupWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnOnHookWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnPeerHangupWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnTimeoutWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnPeerHangupWhileRing( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnOutCallWhileReady( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnRingBackWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnRingBackWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnSetupWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnSetupWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnRecv2xxWhileSetup(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnOnHookWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnOnHookWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnErrorWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnErrorWhileRingBack(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnTimeoutWhileCallOut(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnInCallWhileSetup( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnReInviteWhileSetup(OPPSipChannel *ch,OPPSipEvent *evt);
	static void OnOnHookWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt );
	static void OnInCallWhileAccept( OPPSipChannel *ch,OPPSipEvent *evt );

	static void OnDtmfWhileSetup( OPPSipChannel *ch, OPPSipEvent *evt );
private:
	static OPPChannelSM smInst;
};

