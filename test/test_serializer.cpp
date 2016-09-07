#define BOOST_TEST_MODULE testSerializer
#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>
#include <memory>
#include <string.h>

#include "json2pb.h"
#include "bin2ascii.h"
#include "test_ext.pb.h"

struct Fixture
{
	Fixture& me;
	std::string m_json;
	std::auto_ptr<j2pb::Serializer> m_serializer;
	
	Fixture(j2pb::Serializer* serializer, const std::string& path)
		: me(*this)
		, m_serializer(serializer)
	{
		GOOGLE_PROTOBUF_VERIFY_VERSION;
		std::ifstream f(path.c_str());
		m_json.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	}
	
	~Fixture()
	{
	}
};

struct FixtureClassic : public Fixture
{
	FixtureClassic() 
		: Fixture(new j2pb::Serializer(new j2pb::ClassicExtensions()), "test.json")
	{
	}
};

struct FixtureOrtb : public Fixture
{
	FixtureOrtb() 
		: Fixture(new j2pb::Serializer(new j2pb::OpenRTBExtensions()), "test_ortb.json")
	{
	}
};

typedef boost::mpl::vector<FixtureClassic, FixtureOrtb> Fixtures;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(testFields, T, Fixtures, T)
{
	BOOST_TEST_MESSAGE("=== toProtobuf ===");
	
	Fixture& f = T::me;
	json2pb::test::ComplexMessage msg;
	
    BOOST_TEST_MESSAGE("json: " << f.m_json);
	
	f.m_serializer->toProtobuf(f.m_json.c_str(), f.m_json.length(), msg);
	BOOST_TEST_MESSAGE("msg: " << msg.DebugString());
	
	BOOST_CHECK(msg.has__str() && msg._str() == "b");
	BOOST_CHECK(msg.has__bin() && 0 == ::memcmp(msg._bin().data(), b64_decode("0a0a0a0a").data(), msg._bin().size()));
	BOOST_CHECK(msg.has__bool() && msg._bool());
	BOOST_CHECK(msg.has__float() && (1.0f - msg._float()) < 0.000000001 );
	BOOST_CHECK(4 == msg._int_size() && 10 == msg._int(0) && 20 == msg._int(1) && 30 == msg._int(2) && 40 == msg._int(3));
	BOOST_CHECK(2 == msg._enum_size() && json2pb::test::ComplexMessage::VALUE1 == msg._enum(0) && json2pb::test::ComplexMessage::VALUE2 == msg._enum(1));
	BOOST_CHECK(2 == msg.str_list_size() && "v0" == msg.str_list(0) && "v1" == msg.str_list(1));
	
	BOOST_CHECK(msg.has_sub());
	const json2pb::test::ComplexMessage::SubMessage& sub = msg.sub();
	
	BOOST_CHECK(sub.has_field() && "subfield" == sub.field());
	BOOST_CHECK(sub.has_field() && "subfield" == sub.field());
	BOOST_CHECK(2 == sub.echo_size() 
		&& sub.echo(0).has_text() && "first" == sub.echo(0).text() 
		&& sub.echo(1).has_text() && "second" == sub.echo(1).text());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(testExtensions, T, Fixtures, T)
{
	BOOST_TEST_MESSAGE("=== extensions ===");
	
	Fixture& f = T::me;
	json2pb::test::ComplexMessage msg;
	
	f.m_serializer->toProtobuf(f.m_json.c_str(), f.m_json.length(), msg);

	{
		BOOST_CHECK(msg.HasExtension(json2pb::test::e_bool) && msg.GetExtension(json2pb::test::e_bool));
		BOOST_CHECK(msg.HasExtension(json2pb::test::e_int) && 500 == msg.GetExtension(json2pb::test::e_int));
		
		BOOST_CHECK(msg.HasExtension(company::structure));
		const company::Structure& structure = msg.GetExtension(company::structure);
		BOOST_CHECK(structure.has_id() && 200 == structure.id());
	}
	
	{
		BOOST_CHECK(msg.has_sub());
		const json2pb::test::ComplexMessage::SubMessage& sub = msg.sub();
		
		BOOST_CHECK(sub.HasExtension(company::id) && 100 == sub.GetExtension(company::id));
		BOOST_CHECK(sub.HasExtension(company::sub_structure));
		const company::Structure& structure = sub.GetExtension(company::sub_structure);
		BOOST_CHECK(structure.has_id() && 101 == structure.id());
	}
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(testSerializations, T, Fixtures, T)
{
	BOOST_TEST_MESSAGE("=== serializations ===");
	
	Fixture& f = T::me;
	json2pb::test::ComplexMessage msg1, msg2;
	std::string str1, str2;
	
	f.m_serializer->toProtobuf(f.m_json.c_str(), f.m_json.length(), msg1);
	str1 = f.m_serializer->toJson(msg1);
	
	BOOST_TEST_MESSAGE(str1);
	f.m_serializer->toProtobuf(str1.c_str(), str1.length(), msg2);

	msg1.SerializeToString(&str1);
	msg2.SerializeToString(&str2);
	
	BOOST_CHECK(str1 == str2);
}
