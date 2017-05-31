#define BOOST_TEST_MODULE testSerializationHook
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "json2pb.h"
#include "options.hpp"
#include "test.pb.h"

BOOST_AUTO_TEST_CASE(testHook)
{
	j2pb::Serializer serializer(new j2pb::OpenRTBExtensions());
	
	serializer.options().addEnumHook("json2pb.test.ValuesEnum", std::make_shared<j2pb::ChrReplaceSerializationHook>('_','-'));
	
	json2pb::test::EnumMessage msg;
	
	msg.mutable_enums()->Add(json2pb::test::VALUE_1);
	msg.mutable_enums()->Add(json2pb::test::VALUE_2);
	
	BOOST_TEST_MESSAGE("msg: " << msg.DebugString());
	std::string json = serializer.toJson(msg);
	BOOST_TEST_MESSAGE("json:" << serializer.toJson(msg));
	
	json2pb::test::EnumMessage msg2;
	serializer.toProtobuf(json.c_str(), json.length(), msg2);
	
	BOOST_CHECK(2 == msg2.enums_size());
	BOOST_CHECK(json2pb::test::VALUE_1 == msg2.enums(0));
	BOOST_CHECK(json2pb::test::VALUE_2 == msg2.enums(1));
}
