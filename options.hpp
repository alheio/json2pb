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
		std::map<std::string, std::shared_ptr<const SerializationHook> > m_serializationHooks;

		/// default: false
		bool m_enumAsNumber;

		/// default: false
		bool m_deserializeIgnoreCase;

		/// default: false
		bool m_ignoreUnknownFields;

		/// default: true
		bool m_json2pbufImplicitCastToString;

	public:
		Options();

		Options(const Options& rhs);

		Options(Options&& rhs) = default;

		virtual ~Options() = default;

		Options& operator=(const Options& rhs);

		Options& operator=(Options&& rhs) = default;

		/**
		 * specify enum custom processing rule
		 *
		 * @param typeName - enum object typename in protobuf schema
		 * @param hook - custom rule
		 * @return *this
		 */
		Options& addEnumHook(const std::string& typeName, std::shared_ptr<SerializationHook> hook);

		/**
		 * json<->pbuf:
		 * serialize protobuf enum as integer values instead of string representation
		 * default: false
		 */
		Options& enumAsNumber(bool value);

		/**
		 * json->pbuf: ignore json keys register while json->pbuf conversion
		 * default: false
		 */
		Options& deserializeIgnoreCase(bool value);

		/**
		 * json->pbuf: allow unknow fields in json scheme
		 * default: false
		 */
		Options& ignoreUnknownFields(bool value);

		/**
		 * json->pbuf: implicit string cast from non-string json values to protobuf string scheme
		 * default: true
		 */
		Options& setJson2PbufImplicitCastToString(bool value);

		std::shared_ptr<const SerializationHook> getEnumHook(const std::string& typeName) const;

		bool enumAsNumber() const;

		bool deserializeIgnoreCase() const;

		bool ignoreUnknownFields() const;

		bool isJson2PbufImplicitCastToString() const;

	private:
		std::map<std::string, std::shared_ptr<const SerializationHook> > cloneHooks() const;
	};
}


#endif