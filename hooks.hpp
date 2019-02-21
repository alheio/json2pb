#ifndef __JSON2PB_HOOKS_HPP__
#define __JSON2PB_HOOKS_HPP__

#include <memory>
#include <string>

namespace j2pb
{
	class SerializationHook
	{
	public:
		virtual ~SerializationHook() = default;

		virtual std::unique_ptr<SerializationHook> clone() const = 0;

		virtual std::string preSerialize(const std::string& value) const = 0;

		virtual std::string preDeserialize(const std::string& value) const = 0;
	};
	
	
	class ChrReplaceSerializationHook : public SerializationHook
	{
		char m_from;
		char m_to;

	public:
		ChrReplaceSerializationHook(char from, char to);

		std::unique_ptr<SerializationHook> clone() const override;

		std::string preSerialize(const std::string& value) const override;

		std::string preDeserialize(const std::string& value) const override;

		char getCharFrom() const { return m_from; }

		char getCharTo() const { return m_to; }
	};
}
#endif
