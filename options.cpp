#include "options.hpp"

j2pb::Options::Options()
		: m_enumAsNumber(false)
		, m_deserializeIgnoreCase(false)
		, m_ignoreUnknownFields(false)
		, m_json2pbufImplicitCastToString(true)
{
}

j2pb::Options::Options(const Options& rhs)
		: m_serializationHooks(std::move(rhs.cloneHooks()))
		, m_enumAsNumber(rhs.m_enumAsNumber)
		, m_deserializeIgnoreCase(rhs.m_deserializeIgnoreCase)
		, m_ignoreUnknownFields(rhs.m_ignoreUnknownFields)
		, m_json2pbufImplicitCastToString(rhs.m_json2pbufImplicitCastToString)
{
}

j2pb::Options& j2pb::Options::operator=(const Options& rhs)
{
	if (this != &rhs)
	{
		m_serializationHooks = std::move(rhs.cloneHooks());
		m_enumAsNumber = rhs.m_enumAsNumber;
		m_deserializeIgnoreCase = rhs.m_deserializeIgnoreCase;
		m_ignoreUnknownFields = rhs.m_ignoreUnknownFields;
		m_json2pbufImplicitCastToString = rhs.m_json2pbufImplicitCastToString;
	}
	return *this;
}

j2pb::Options& j2pb::Options::enumAsNumber(bool value)
{
	m_enumAsNumber = value;
	return *this;
}

bool j2pb::Options::enumAsNumber() const
{
	return m_enumAsNumber;
}

j2pb::Options& j2pb::Options::addEnumHook(const std::string& typeName, std::shared_ptr<SerializationHook> hook)
{
	m_serializationHooks[typeName] = std::move(hook);
	return *this;
}

std::shared_ptr<const j2pb::SerializationHook> j2pb::Options::getEnumHook(const std::string& typeName) const
{
	auto it = m_serializationHooks.find(typeName);
	return it != m_serializationHooks.end() ? it->second : nullptr;
}

j2pb::Options& j2pb::Options::deserializeIgnoreCase(bool value)
{
	m_deserializeIgnoreCase = value;
	return *this;
}

bool j2pb::Options::deserializeIgnoreCase() const
{
	return m_deserializeIgnoreCase;
}

j2pb::Options& j2pb::Options::ignoreUnknownFields(bool value)
{
	m_ignoreUnknownFields = value;
	return *this;
}

bool j2pb::Options::ignoreUnknownFields() const
{
	return m_ignoreUnknownFields;
}

bool j2pb::Options::isJson2PbufImplicitCastToString() const
{
	return m_json2pbufImplicitCastToString;
}

/**
 * enable or disable implicit string cast from non-string json values to protobuf string scheme
 * default: enabled
 */
j2pb::Options& j2pb::Options::setJson2PbufImplicitCastToString(bool value)
{
	m_json2pbufImplicitCastToString = value;
	return *this;
}


std::map<std::string, std::shared_ptr<const j2pb::SerializationHook> > j2pb::Options::cloneHooks() const
{
	std::map<std::string, std::shared_ptr<const j2pb::SerializationHook> > rc;

	for (const auto& pair: m_serializationHooks)
		rc.emplace(std::make_pair(pair.first, pair.second->clone()));

	return rc;
}