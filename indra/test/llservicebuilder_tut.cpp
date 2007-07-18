/**
* @file   llservicebuilder_tut.cpp
* @brief  LLServiceBuilder unit tests
* @date   March 2007
*
* Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
* $License$
*/

#include <tut/tut.h>
#include "lltut.h"

#include "llsd.h"
#include "llservicebuilder.h"

namespace tut
{

	struct ServiceBuilderTestData {
		LLServiceBuilder mServiceBuilder;
	};

	typedef test_group<ServiceBuilderTestData>	ServiceBuilderTestGroup;
	typedef ServiceBuilderTestGroup::object	ServiceBuilderTestObject;

	ServiceBuilderTestGroup serviceBuilderTestGroup("ServiceBuilder");

	template<> template<>
	void ServiceBuilderTestObject::test<1>()
	{
		//Simple service build and reply with no mapping
		LLSD test_block;
		test_block["service-builder"] = "/agent/name";
		mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);
		std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest");
		ensure_equals("Basic URL Creation", test_url , "/agent/name");
	}	

	template<> template<>
	void ServiceBuilderTestObject::test<2>()
	{
		//Simple replace test
		LLSD test_block;
		test_block["service-builder"] = "/agent/{$agent-id}/name";
		mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);	
		LLSD data_map;
		data_map["agent-id"] = "257c631f-a0c5-4f29-8a9f-9031feaae6c6";
		std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
		ensure_equals("Replacement URL Creation", test_url , "/agent/257c631f-a0c5-4f29-8a9f-9031feaae6c6/name");
	}	

	template<> template<>
	void ServiceBuilderTestObject::test<3>()
	{
		//Incorrect service test
		LLSD test_block;
		test_block["service-builder"] = "/agent/{$agent-id}/name";
		mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);	
		std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilder");
		ensure_equals("Replacement URL Creation for Non-existant Service", test_url , "");
	}

	template<> template<>
	void ServiceBuilderTestObject::test<4>()
	{
		//Incorrect service test
		LLSD test_block;
		test_block["service-builder"] = "/agent/{$agent-id}/name";
		mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);
		LLSD data_map;
		data_map["agent_id"] = "257c631f-a0c5-4f29-8a9f-9031feaae6c6";
		std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
		ensure_equals("Replacement URL Creation for Non-existant Service", test_url , "/agent/{$agent-id}/name");
	}

	template<> template<>
	void ServiceBuilderTestObject::test<5>()
	{
		LLSD test_block;
		test_block["service-builder"] = "/proc/{$proc}{%params}";
		mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);	
		LLSD data_map;
		data_map["proc"] = "do/something/useful";
		data_map["params"]["estate_id"] = 1;
		data_map["params"]["query"] = "public";
		std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
		ensure_equals(
			"two part URL Creation",
			test_url ,
			"/proc/do/something/useful?estate_id=1&query=public");
	}
}

