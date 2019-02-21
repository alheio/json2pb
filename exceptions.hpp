#ifndef __JSON2PB_EXCEPTIONS__HPP__
#define __JSON2PB_EXCEPTIONS__HPP__

#include <stdexcept>
#include <google/protobuf/descriptor.h>

namespace j2pb
{
	class j2pb_error : public std::runtime_error
	{
	public:
		explicit j2pb_error(const char* arg)
			: runtime_error(arg)
		{
		}

		explicit j2pb_error(const std::string& arg) noexcept(false)
			: runtime_error(arg)
		{
		}

		j2pb_error(const google::protobuf::FieldDescriptor *field, const std::string &e)
			: runtime_error(field->name() + ": " + e)
		{
		}
	};
}

#endif
