/*
	------------------------------------------------------------------------------------
	LICENSE:
	------------------------------------------------------------------------------------
	This file is part of EVEmu: EVE Online Server Emulator
	Copyright 2006 - 2008 The EVEmu Team
	For the latest information visit http://evemu.mmoforge.org
	------------------------------------------------------------------------------------
	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free Software
	Foundation; either version 2 of the License, or (at your option) any later
	version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place - Suite 330, Boston, MA 02111-1307, USA, or go to
	http://www.gnu.org/copyleft/lesser.txt.
	------------------------------------------------------------------------------------
	Author:		Zhur
*/

/*
Dynamic Bodies:
	- shape
	- `mass`
	- `radius`
	- `volume`???
	- `Inertia`???
	- inertia information
	- position
	- velocity
	- angular velocity
	- collide with things
		- approximate eve "run into and stop/turn around" collisions

Ship: extends Dynamic
	- `maxVelocity`
	- thrust (propulsion + speed ratio)
	- angular thrust
	- "stopping" algorithm
	- "orbit" algorithm
	- "turning" algorithm 

Static Bodies:
	- shape
	- position

detect clients moving into agro radius
*/

#ifndef EVE_CLIENT_H
#define EVE_CLIENT_H

#include "../common/packet_types.h"
#include "../common/timer.h"
#include "../common/gpoint.h"
#include "../common/EVEUtils.h"
#include "../common/EVEPresentation.h"

#include "ClientSession.h"
#include "system/SystemEntity.h"
#include "ship/ModuleManager.h"

class CryptoChallengePacket;
class EVENotificationStream;
class PyRepSubStream;
class InventoryItem;
class SystemManager;
class PyServiceMgr;
class PyCallStream;
class PyRepTuple;
class LSCChannel;
class PyAddress;
class PyRepList;
class PyRepDict;
class PyPacket;
class Client;
class PyRep;

class CharacterData {
public:
	uint32 charid;
	
	std::string name;
	std::string title;
	std::string description;
	bool gender;

	double bounty;
	double balance;
	double securityRating;
	uint32 logonMinutes;

	uint32 corporationID;
	uint64 corporationDateTime;
	uint32 allianceID;
	
	uint32 stationID;
	uint32 solarSystemID;
	uint32 constellationID;
	uint32 regionID;

	uint32 typeID;
	uint32 bloodlineID;
	uint8 raceID;	//must correspond to our bloodlineID!

	uint32 ancestryID;
	uint32 careerID;
	uint32 schoolID;
	uint32 careerSpecialityID;
	
	uint8 intelligence;
	uint8 charisma;
	uint8 perception;
	uint8 memory;
	uint8 willpower;

	uint64 startDateTime;
	uint64 createDateTime;
};

class CharacterAppearance {
public:
	CharacterAppearance();
	CharacterAppearance(const CharacterAppearance &from);
	~CharacterAppearance();

#define INT(v) \
	uint32 v;
#define INT_DYN(v) \
	bool IsNull_##v() const { \
		return(v == NULL); \
	} \
	uint32 Get_##v() const { \
		return(IsNull_##v() ? NULL : *v); \
	} \
	void Set_##v(uint32 val) { \
		Clear_##v(); \
		v = new uint32(val); \
	} \
	void Clear_##v() { \
		if(!IsNull_##v()) \
			delete v; \
		v = NULL; \
	}
#define REAL(v) \
	double v;
#define REAL_DYN(v) \
	bool IsNull_##v() const { \
		return(v == NULL); \
	} \
	double Get_##v() const { \
		return(IsNull_##v() ? NULL : *v); \
	} \
	void Set_##v(double val) { \
		Clear_##v(); \
		v = new double(val); \
	} \
	void Clear_##v() { \
		if(!IsNull_##v()) \
			delete v; \
		v = NULL; \
	}
#include "character/CharacterAppearance_fields.h"

	void Build(const std::map<std::string, PyRep *> &from);
	void operator=(const CharacterAppearance &from);

protected:
#define INT_DYN(v) \
	uint32 *v;
#define REAL_DYN(v) \
	double *v;
#include "character/CharacterAppearance_fields.h"
};

class CorpMemberInfo {
public:
	CorpMemberInfo() : corpHQ(0), corprole(0), rolesAtAll(0), rolesAtBase(0), rolesAtHQ(0), rolesAtOther(0) {}
	uint32 corpHQ;	//this really doesn't belong here...
	uint64 corprole;
	uint64 rolesAtAll;
	uint64 rolesAtBase;
	uint64 rolesAtHQ;
	uint64 rolesAtOther;
};
// not needed now:
/*
class Functor {
public:
	Functor() {}
	virtual ~Functor() {}
	virtual void operator()() = 0;
};

class ClientFunctor : public Functor {
public:
	ClientFunctor(Client *c) : m_client(c) {}
	virtual ~ClientFunctor() {}
	//leave () pure
protected:
	Client *const m_client;
};

class FunctorTimerQueue {
public:
	typedef uint32 TimerID;
	
	FunctorTimerQueue();
	~FunctorTimerQueue();

	//schedule takes ownership of the functor.
	TimerID Schedule(Functor **what, uint32 in_how_many_ms);
	bool Cancel(TimerID id);
	
	void Process();
	
protected:
	//this could be done a TON better, but want to get something
	//working first, it is call encapsulated in here, so it should
	//be simple to gut this object and put in a real scheduler.
	class Entry {
	public:
		Entry(TimerID id, Functor *func, uint32 time_ms);
		~Entry();
		const TimerID id;
		Functor *const func;
		Timer when;
	};
	TimerID m_nextID;
	std::vector<Entry *> m_queue;	//not ordered or anything useful.
};
*/
//DO NOT INHERIT THIS OBJECT!
class Client
: public DynamicSystemEntity
{
public:
	Client(PyServiceMgr &services, EVETCPConnection *&con);
	virtual ~Client();
	
	ClientSession session;
	ModuleManager modules;

	virtual void    QueuePacket(PyPacket *p);
	virtual void    FastQueuePacket(PyPacket **p);
	bool            ProcessNet();
	virtual void    Process();

	uint32 GetAccountID() const                     { return m_accountID; }
	uint32 GetRole() const                          { return m_role; }

	// character data
	InventoryItem *Char() const                     { return m_char; }

	uint32 GetCharacterID() const                   { return m_chardata.charid; }
	uint32 GetCorporationID() const                 { return m_chardata.corporationID; }
	uint32 GetAllianceID() const                    { return m_chardata.allianceID; }
	uint32 GetStationID() const                     { return m_chardata.stationID; }
	uint32 GetSystemID() const                      { return m_chardata.solarSystemID; }
	uint32 GetConstellationID() const               { return m_chardata.constellationID; }
	uint32 GetRegionID() const                      { return m_chardata.regionID; }

	uint32 GetLocationID() const
	{
		if (IsInSpace() == true)
			return GetSystemID();
		else
			return GetStationID();
	}

	uint32 GetCorpHQ() const                        { return m_corpstate.corpHQ; }
	uint32 GetCorpRole() const                      { return m_corpstate.corprole; }
	uint32 GetRolesAtAll() const                    { return m_corpstate.rolesAtAll; }
	uint32 GetRolesAtBase() const                   { return m_corpstate.rolesAtBase; }
	uint32 GetRolesAtHQ() const                     { return m_corpstate.rolesAtHQ; }
	uint32 GetRolesAtOther() const                  { return m_corpstate.rolesAtOther; }

	uint32 GetShipID() const { return(GetID()); }
	InventoryItem *Ship() const { return(Item()); }

	bool IsInSpace() const { return(GetStationID() == 0); }
	inline double x() const { return(GetPosition().x); }	//this is terribly inefficient.
	inline double y() const { return(GetPosition().y); }	//this is terribly inefficient.
	inline double z() const { return(GetPosition().z); }	//this is terribly inefficient.

	double GetBounty() const                       { return m_chardata.bounty; }
	double GetSecurityRating() const               { return m_chardata.securityRating; }
	double GetBalance() const                      { return m_chardata.balance; }
	bool AddBalance(double amount);

	PyServiceMgr &services() const { return(m_services); }

	void Login(CryptoChallengePacket *pack);
	void BoardShip(InventoryItem *new_ship);
	void MoveToLocation(uint32 location, const GPoint &pt);
	void MoveToPosition(const GPoint &pt);
	void MoveItem(uint32 itemID, uint32 location, EVEItemFlags flag);
	bool EnterSystem();
	bool SelectCharacter(uint32 char_id);
	void JoinCorporationUpdate(uint32 corp_id);
	void SavePosition();
	
	double GetPropulsionStrength() const;
	
	bool LaunchDrone(InventoryItem *drone);
	
	void SendNotification(const PyAddress &dest, EVENotificationStream *noti, bool seq=true);
	void SendNotification(const char *notifyType, const char *idType, PyRepTuple **payload, bool seq=true);
	void SessionSync();

	//destiny stuff...
	void WarpTo(const GPoint &p, double distance);
	void StargateJump(uint32 fromGate, uint32 toGate);
	
	//SystemEntity interface:
	virtual EntityClass GetClass() const { return(ecClient); }
	virtual bool IsClient() const { return true; }
	virtual Client *CastToClient() { return(this); }
	virtual const Client *CastToClient() const { return(this); }

	virtual const char *GetName() const { return(m_chardata.name.c_str()); }
	virtual PyRepDict *MakeSlimItem() const;
	virtual void QueueDestinyUpdate(PyRepTuple **du);
	virtual void QueueDestinyEvent(PyRepTuple **multiEvent);

	virtual void TargetAdded(SystemEntity *who);
	virtual void TargetLost(SystemEntity *who);
	virtual void TargetedAdd(SystemEntity *who);
	virtual void TargetedLost(SystemEntity *who);
	virtual void TargetsCleared();

	virtual void ApplyDamageModifiers(Damage &d, SystemEntity *target);
	virtual bool ApplyDamage(Damage &d);
	virtual void Killed(Damage &fatal_blow);
	virtual SystemManager *System() const { return(m_system); }
	
	void SendErrorMsg(const char *fmt, ...);
	void SendNotifyMsg(const char *fmt, ...);
	void SendInfoModalMsg(const char *fmt, ...);
	void SelfChatMessage(const char *fmt, ...);
	void SelfEveMail(const char *subject, const char *fmt, ...);
	void ChannelJoined(LSCChannel *chan);
	void ChannelLeft(LSCChannel *chan);

	//FunctorTimerQueue::TimerID Delay( uint32 time_in_ms, void (Client::* clientCall)() );
	//FunctorTimerQueue::TimerID Delay( uint32 time_in_ms, ClientFunctor **functor );

	/** character notification messages
	  */
	void OnCharNoLongerInStation();
	void OnCharNowInStation();
	
protected:
	void _SendPingRequest();
	
	void _ProcessNotification(PyPacket *packet);
	void _CheckSessionChange();
	
	void _ReduceDamage(Damage &d);
	
	//call stuff
	void _ProcessCallRequest(PyPacket *packet);
	void _SendCallReturn(PyPacket *req, PyRep **return_value, const char *channel = NULL);
	void _SendException(PyPacket *req, MACHONETERR_TYPE type, PyRep **payload);

	InventoryItem *m_char;

	PyServiceMgr &m_services;
	EVEPresentation m_net;
	Timer m_pingTimer;

	uint32 m_accountID;
	uint64 m_role;
	uint32 m_gangRole;

	SystemManager *m_system;	//we do not own this

	CharacterData m_chardata;
	CorpMemberInfo m_corpstate;

	std::set<LSCChannel *> m_channels;	//we do not own these.
	
	//this whole move system is a piece of crap:
	typedef enum {
		msIdle,
		msJump
	} _MoveState;
	void _postMove(_MoveState type, uint32 wait_ms=500);
	_MoveState m_moveState;
	Timer m_moveTimer;
	uint32 m_moveSystemID;
	GPoint m_movePoint;
	void _ExecuteJump();
	
private:
	//queues for destiny updates:
	std::vector<PyRep *> m_destinyEventQueue;	//we own these. These are events as used in OnMultiEvent
	std::vector<PyRep *> m_destinyUpdateQueue;	//we own these. They are the `update` which go into DoDestinyAction
	void _SendQueuedUpdates();

	//FunctorTimerQueue m_delayQueue;
	
	uint32 m_nextNotifySequence;
};


//simple functor for void calls.
//not needed right now
/*class SimpleClientFunctor : public ClientFunctor {
public:
	typedef void (Client::* clientCall)();
	SimpleClientFunctor(Client *c, clientCall call) : ClientFunctor(c), m_call(call) {}
	virtual ~SimpleClientFunctor() {}
	virtual void operator()() {
		(m_client->*m_call)();
	}
public:
	const clientCall m_call;
};*/

#endif
