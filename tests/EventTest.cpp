/* Copyright The kNet Project.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

/** @file EventTest.cpp
	@brief */

#include "kNet/Event.h"
#include "tassert.h"
#include "kNet/DebugMemoryLeakCheck.h"

void EventTest()
{
	using namespace kNet;

	TEST("Event")

	Event e;
	e.Create(EventWaitSignal);
	for(int i = 0; i < 70000; ++i)
	{
		e.Set();
		assert(e.Test());
	}
	e.Reset();
	assert(!e.Test());

	for(int i = 0; i < 70000; ++i)
	{
		e.Reset();
		assert(!e.Test());
	}
	e.Set();
	assert(e.Test());

	for(int i = 0; i < 1000; ++i)
	{
		e.Reset();
		assert(!e.Test());
		assert(!e.Test());
		assert(!e.Wait(0));
		assert(!e.Wait(10));
		e.Set();
		e.Set();
		e.Set();
		assert(e.Test());
		assert(e.Test());
		assert(e.Wait(0));
		assert(e.Wait(10));
		e.Reset();
		e.Reset();
		assert(!e.Test());
		assert(!e.Test());
	}
	ENDTEST()
}
