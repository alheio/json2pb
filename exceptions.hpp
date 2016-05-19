#ifndef __JSON2PB_EXCEPTIONS__HPP__
#define __JSON2PB_EXCEPTIONS__HPP__

#include <stdexcept>
#include <google/protobuf/message.h>

namespace j2pb
{
	class j2pb_error : public std::exception 
	{
	std::string _error;
	public:
		j2pb_error(const std::string &e) : _error(e) {}
		j2pb_error(const google::protobuf::FieldDescriptor *field, const std::string &e) : _error(field->name() + ": " + e) {}
		virtual ~j2pb_error() throw() {};

		virtual const char *what() const throw () { return _error.c_str(); };
	};
}

#endif
