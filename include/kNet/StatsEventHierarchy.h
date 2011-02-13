/* Copyright 2010 Jukka Jyl�nki

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
#pragma once

/** @file StatsEventHierarchy.h
	@brief Stores a hierarchy of network events for profiling purposes. */

#include <map>
#include <string>

#include "kNet/WaitFreeQueue.h"
#include "kNet/Clock.h"

// This macro is used inside MessageConnection and NetworkServer objects, which have the 'owner' member.
static const int cEventOldAgeMSecs = 30 * 1000;

#ifdef KNET_NETWORK_PROFILING
#define ADDEVENT(name, value) (owner ? owner->Statistics()->AddEventToHierarchy(name, value, cEventOldAgeMSecs) : ((void)0))
#else
#define ADDEVENT(name, value) ((void)0)
#endif

namespace kNet
{

struct StatsEvent
{
	float value;
	tick_t time;
};

inline std::string FirstToken(const char *str, char delimiter, int &nextTokenStart)
{
	int i = 0;
	while(str[i] != '\0' && str[i] != delimiter)
		++i;
	if (str[i] == '\0' || str[i+1] == delimiter)
		nextTokenStart = -1;
	else
		nextTokenStart = i+1;
	return std::string(str, str + i);
}

class StatsEventHierarchyNode
{
public:
	///\todo To improve performance, don't use a std::string as a key to the map, and replace the map with a more efficient data structure.
	typedef std::map<std::string, StatsEventHierarchyNode> NodeMap;
	NodeMap children;

	WaitFreeQueue<StatsEvent> events;

	StatsEventHierarchyNode()
	:events(4) // The default size for the queue must be at least four elements (pow2, >2).
	{
	}

	void PruneOldEventsThisLevel(int ageMSecs)
	{
		assert(ageMSecs >= 0);
		tick_t tooOldMessageTime = Clock::Tick() - (tick_t)ageMSecs * Clock::TicksPerSec() / 1000;
		while(events.Size() > 0)
		{
			StatsEvent *front = events.Front();
			if (Clock::IsNewer(tooOldMessageTime, front->time))
				events.PopFront();
			else
				break; // The items are added to the queue in their time order, so if the oldest item is newer than our limit, they all are.
		}
	}

	void PruneOldEventsHierarchy(int ageMSecs)
	{
		PruneOldEventsThisLevel(ageMSecs);

		for(NodeMap::iterator iter = children.begin(); iter != children.end(); ++iter)
			iter->second.PruneOldEventsHierarchy(ageMSecs);
	}

	void AddEventToThisLevel(float value, int oldAgeMSecs)
	{
		StatsEvent e;
		e.value = value;
		e.time = Clock::Tick();
		PruneOldEventsThisLevel(oldAgeMSecs);
		events.InsertWithResize(e);
	}

	///\ @param name The event track in the profiler hierachy to add the event to, e.g. "connection.messageIn.myMessageName". This
	///              string may not contain two consecutive periods, e.g. "a..b".
	void AddEventToHierarchy(const char *name, float value, int oldAgeMSecs)
	{
		int nextTokenStart = 0;
		std::string childName = FirstToken(name, '.', nextTokenStart);
		if (childName.empty())
			AddEventToThisLevel(value, oldAgeMSecs);
		else
		{
			if (nextTokenStart == -1)
				children[childName].AddEventToThisLevel(value, oldAgeMSecs);
			else
				children[childName].AddEventToHierarchy(name + nextTokenStart, value, oldAgeMSecs);
		}
	}

	int AccumulateTotalCountThisLevel() const
	{
		return events.Size();
	}

	int AccumulateTotalCountHierarchy() const
	{
		int count = AccumulateTotalCountThisLevel();

		for(NodeMap::const_iterator iter = children.begin(); iter != children.end(); ++iter)
			count += iter->second.AccumulateTotalCountHierarchy();

		return count;
	}

	float AccumulateTotalValueThisLevel() const
	{
		float value = 0.f;
		for(int i = 0; i < events.Size(); ++i)
			value += events.ItemAt(i)->value;
		return value;
	}

	float AccumulateTotalValueHierarchy() const
	{
		float value = AccumulateTotalValueThisLevel();

		for(NodeMap::const_iterator iter = children.begin(); iter != children.end(); ++iter)
			value += iter->second.AccumulateTotalValueHierarchy();

		return value;
	}

	float LatestValue()
	{
		if (events.Size() == 0)
			return 0.f;
		else
			return events.Back()->value;
	}
};

} // ~kNet
