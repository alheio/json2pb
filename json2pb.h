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
		std::unique_ptr<Extensions> m_extensions;
		Options m_options;

	public:
		explicit Serializer(std::unique_ptr<Extensions> extensions);

		Serializer(std::unique_ptr<Extensions> extensions, Options options);

		Serializer(const Serializer& rhs);

		Serializer(Serializer&& rhs) = default;

		virtual ~Serializer() = default;

		Serializer& operator=(const Serializer& rhs) = delete;

		Serializer& operator=(Serializer&& rhs) = default;

		std::string toJson(const google::protobuf::Message &msg) const;
		std::string toJson(const google::protobuf::Message &msg, const Options& options) const;
		
		void toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg);
		void toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg, const Options& options);

		Options& options() { return m_options; }
		const Options& options() const { return m_options; }
		
	private:
		json_t* pb2json(const google::protobuf::Message& msg, const Options& options) const;
		json_t* field2json(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor *field, int index, const Options& options) const;
		
		void json2pb(google::protobuf::Message& msg, json_t* root, const Options& options) const;

		void jsonArrayOrField2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf, const Options& options) const;

		void json2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf, const Options& options) const;

		static void jsonFieldSetOrAdd(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, const std::string& value);
	};
}

#endif//__JSON2PB_H__
