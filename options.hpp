#ifndef __JSON2PB_OPTIONS_HPP__
#define __JSON2PB_OPTIONS_HPP__

#include <string>
#include <map>
#include <memory>

#include  "hooks.hpp"

namespace j2pb
{
	class Options
	{
		bool m_enumAsNumber;
		
		typedef std::map<std::string, std::shared_ptr<const SerializationHook> > hooks_t;
		hooks_t m_serializationHooks;
		
		bool m_deserializeIgnoreCase;
		
	public:
		Options() 
			: m_enumAsNumber(false)
			, m_deserializeIgnoreCase(false)
		{
		}
		
		/**
		 * serialize protobuf enum as integer values instead of string representation
		 * defaul: enum as string
		 */
		Options& enumAsNumber(bool value) 
		{ 
			m_enumAsNumber = value;
			return *this; 
		}
		
		bool enumAsNumber() const { return m_enumAsNumber; }
		
		Options& addEnumHook(const std::string& typeName, std::shared_ptr<SerializationHook> hook)
		{
			m_serializationHooks[typeName] = hook;
			return *this;
		}
		
		const SerializationHook* getEnumHook(const std::string& typeName) const
		{
			hooks_t::const_iterator it = m_serializationHooks.find(typeName);
			return it != m_serializationHooks.end() ? it->second.get() : NULL;
		}
		
		Options& deserializeIgnoreCase(bool value)
		{
			m_deserializeIgnoreCase = value;
			return *this;
		}
		
		bool deserializeIgnoreCase() const { return m_deserializeIgnoreCase; }
	};
}


#endif