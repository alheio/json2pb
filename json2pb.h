/*
 * Copyright (c) 2013 Pavel Shramov <shramov@mexmat.net>
 *
 * json2pb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef __JSON2PB_H__
#define __JSON2PB_H__

#include <google/protobuf/message.h>
#include <string>
#include <memory>
#include "options.hpp"
#include "extensions.hpp"

struct json_t;

namespace j2pb
{
	class Serializer
	{
		Options m_options;
		std::auto_ptr<Extensions> m_extensions;
		
	public:
		Serializer(Extensions* extensions);
		Serializer(const Serializer& rhs);
		
		std::string toJson(const google::protobuf::Message &msg) const;
		std::string toJson(const google::protobuf::Message &msg, const Options& options) const;
		
		void toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg);
		
		Options& options() { return m_options; }
		
	private:
		Serializer& operator=(const Serializer& rhs) const;
		
		json_t* pb2json(const google::protobuf::Message& msg, const Options& options) const;
		json_t* field2json(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor *field, size_t index, const Options& options) const;
		
		void json2pb(google::protobuf::Message& msg, json_t* root);
		void jsonArrayOrField2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf);
		void json2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf);
	};
}

#endif//__JSON2PB_H__
