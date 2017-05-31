#include "hooks.hpp"
#include <algorithm>

j2pb::ChrReplaceSerializationHook::ChrReplaceSerializationHook(char from, char to)
	: m_from(from)
	, m_to(to)
{
}

std::string j2pb::ChrReplaceSerializationHook::preSerialize(const std::string& value) const
{
	std::string rc(value);
	std::replace(rc.begin(),rc.end(), m_from, m_to);
	return rc;
}

std::string j2pb::ChrReplaceSerializationHook::preDeserialize(const std::string& value) const
{
	std::string rc(value);
	std::replace(rc.begin(),rc.end(), m_to, m_from);
	return rc;
}

