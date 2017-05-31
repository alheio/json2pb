#ifndef __JSON2PB_HOOKS_HPP__
#define __JSON2PB_HOOKS_HPP__

#include <string>

namespace j2pb
{
	class SerializationHook
	{
	public:
		virtual ~SerializationHook() {}
		virtual std::string preSerialize(const std::string& value) const = 0;
		virtual std::string preDeserialize(const std::string& value) const = 0;
	};
	
	
	class ChrReplaceSerializationHook : public SerializationHook
	{
		char m_from;
		char m_to;
	public:
		ChrReplaceSerializationHook(char from, char to);
		virtual std::string preSerialize(const std::string& value) const;
		virtual std::string preDeserialize(const std::string& value) const;
	};
}
#endif
