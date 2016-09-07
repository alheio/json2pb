/*
 * Copyright (c) 2013 Pavel Shramov <shramov@mexmat.net>
 *
 * json2pb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "json2pb.h"
#include <jansson.h>
#include "bin2ascii.h"
#include "exceptions.hpp"

using namespace google::protobuf;

struct json_autoptr {
	json_t * ptr;
	json_autoptr(json_t *json) : ptr(json) {}
	~json_autoptr() { if (ptr) json_decref(ptr); }
	json_t * release() { json_t *tmp = ptr; ptr = 0; return tmp; }
};

namespace j2pb 
{

int json_dump_std_string(const char *buf, size_t size, void *data)
{
	std::string *s = (std::string *) data;
	s->append(buf, size);
	return 0;
}

Serializer::Serializer(Extensions* extensions)
	: m_extensions(extensions)
{
}


Serializer::Serializer(const Serializer& rhs)
	: m_extensions(rhs.m_extensions->clone())
{
}


std::string Serializer::toJson(const google::protobuf::Message& msg) const
{
	return toJson(msg, m_options);
}

std::string Serializer::toJson(const Message& msg, const Options& options) const
{
	std::string r;

	json_t *root = pb2json(msg, options);
	json_autoptr _auto(root);
	json_dump_callback(root, json_dump_std_string, &r, 0);
	return r;
}


void Serializer::toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg)
{
	json_t *root;
	json_error_t error;

	root = json_loadb(buf, size, 0, &error);

	if (!root)
		throw j2pb_error(std::string("Load failed: ") + error.text);

	json_autoptr _auto(root);

	if (!json_is_object(root))
		throw j2pb_error("Malformed JSON: not an object");

	json2pb(msg, root);
}

json_t* Serializer::pb2json(const google::protobuf::Message& msg, const Options& options) const
{
	const Descriptor *d = msg.GetDescriptor();
	const Reflection *ref = msg.GetReflection();
	
	if (!d || !ref) return 0;

	json_t *root = json_object();
	json_autoptr _auto(root);

	std::vector<const FieldDescriptor *> fields;
	ref->ListFields(msg, &fields);
	
	json_t* ext = 0;

	for (size_t i = 0; i != fields.size(); i++)
	{
		const FieldDescriptor *field = fields[i];

		json_t *jf = 0;
		if(field->is_repeated()) {
			size_t count = ref->FieldSize(msg, field);
			if (!count) continue;

			json_autoptr array(json_array());
			for (size_t j = 0; j < count; j++)
				json_array_append_new(array.ptr, field2json(msg, field, j, options));
			jf = array.release();
		} else if (ref->HasField(msg, field))
			jf = field2json(msg, field, 0, options);
		else
			continue;

		if (field->is_extension())
		{
			m_extensions->write(field, root, jf);
		}
		else
		{
			json_object_set_new(root, field->name().c_str(), jf);
		}
		
	}
	return _auto.release();
}

json_t* Serializer::field2json(const Message& msg, const FieldDescriptor* field, size_t index, const Options& options) const
{
	const Reflection *ref = msg.GetReflection();
	const bool repeated = field->is_repeated();
	json_t *jf = 0;
	switch (field->cpp_type())
	{
#define _CONVERT(type, ctype, fmt, sfunc, afunc)		\
		case FieldDescriptor::type: {			\
			const ctype value = (repeated)?		\
				ref->afunc(msg, field, index):	\
				ref->sfunc(msg, field);		\
			jf = fmt(value);			\
			break;					\
		}

		_CONVERT(CPPTYPE_DOUBLE, double, json_real, GetDouble, GetRepeatedDouble);
		_CONVERT(CPPTYPE_FLOAT, double, json_real, GetFloat, GetRepeatedFloat);
		_CONVERT(CPPTYPE_INT64, json_int_t, json_integer, GetInt64, GetRepeatedInt64);
		_CONVERT(CPPTYPE_UINT64, json_int_t, json_integer, GetUInt64, GetRepeatedUInt64);
		_CONVERT(CPPTYPE_INT32, json_int_t, json_integer, GetInt32, GetRepeatedInt32);
		_CONVERT(CPPTYPE_UINT32, json_int_t, json_integer, GetUInt32, GetRepeatedUInt32);
		_CONVERT(CPPTYPE_BOOL, bool, json_boolean, GetBool, GetRepeatedBool);
#undef _CONVERT
		case FieldDescriptor::CPPTYPE_STRING: {
			std::string scratch;
			const std::string &value = (repeated)?
				ref->GetRepeatedStringReference(msg, field, index, &scratch):
				ref->GetStringReference(msg, field, &scratch);
			if (field->type() == FieldDescriptor::TYPE_BYTES)
				jf = json_string(b64_encode(value).c_str());
			else
				jf = json_string(value.c_str());
			break;
		}
		case FieldDescriptor::CPPTYPE_MESSAGE: {
			const Message& mf = (repeated)?
				ref->GetRepeatedMessage(msg, field, index):
				ref->GetMessage(msg, field);
			jf = pb2json(mf, options);
			break;
		}
		case FieldDescriptor::CPPTYPE_ENUM: {
			const EnumValueDescriptor* ef = (repeated)? 
				ref->GetRepeatedEnum(msg, field, index):
				ref->GetEnum(msg, field);
			
			jf = options.enumAsNumber() ? json_integer(ef->number()) : json_string(ef->name().c_str());
			break;
		}
		default:
			break;
	}
	if (!jf) throw j2pb_error(field, "Fail to convert to json");
	return jf;
}

void Serializer::json2pb(google::protobuf::Message& msg, json_t* root)
{
	const Descriptor *d = msg.GetDescriptor();
	const Reflection *ref = msg.GetReflection();
	if (!d || !ref) throw j2pb_error("No descriptor or reflection");
	
	for (void* i = json_object_iter(root); i; i = json_object_iter_next(root, i))
	{
		const char *name = json_object_iter_key(i);
		json_t *jf = json_object_iter_value(i);

		const FieldDescriptor *field = d->FindFieldByName(name);
		if (NULL != field)
		{
			jsonArrayOrField2field(msg, field, jf);
		}
		else
		{
			if (m_extensions->read(ref, name, jf))
			{
				for (std::size_t j = 0; j<m_extensions->size(); ++j)
				{
					Extensions::extension_t ext = m_extensions->get(j);
					jsonArrayOrField2field(msg, ext.first, ext.second);
				}
			}
			else
				throw j2pb_error("Unknown field: " + std::string(name));
		}
	}
}

void Serializer::jsonArrayOrField2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf)
{
	if (field->is_repeated()) 
	{
		if (!json_is_array(jf))
			throw j2pb_error(field, "Not array");
		
		for (size_t j = 0, sz = json_array_size(jf); j < sz; j++)
			json2field(msg, field, json_array_get(jf, j));
	} 
	else 
		json2field(msg, field, jf);
}

void Serializer::json2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf)
{
	const Reflection *ref = msg.GetReflection();
	const bool repeated = field->is_repeated();
	json_error_t error;
	
	if (!field->is_required() && json_is_null(jf))
		return;

	switch (field->cpp_type())
	{
#define _SET_OR_ADD(sfunc, afunc, value)			\
		do {						\
			if (repeated)				\
				ref->afunc(&msg, field, value);	\
			else					\
				ref->sfunc(&msg, field, value);	\
		} while (0)

#define _CONVERT(type, ctype, fmt, sfunc, afunc) 		\
		case FieldDescriptor::type: {			\
			ctype value;				\
			int r = json_unpack_ex(jf, &error, JSON_STRICT, fmt, &value); \
			if (r) throw j2pb_error(field, std::string("Failed to unpack: ") + error.text); \
			_SET_OR_ADD(sfunc, afunc, value);	\
			break;					\
		}

		_CONVERT(CPPTYPE_DOUBLE, double, "F", SetDouble, AddDouble);
		_CONVERT(CPPTYPE_FLOAT, double, "F", SetFloat, AddFloat);
		_CONVERT(CPPTYPE_INT64, json_int_t, "I", SetInt64, AddInt64);
		_CONVERT(CPPTYPE_UINT64, json_int_t, "I", SetUInt64, AddUInt64);
		_CONVERT(CPPTYPE_INT32, json_int_t, "I", SetInt32, AddInt32);
		_CONVERT(CPPTYPE_UINT32, json_int_t, "I", SetUInt32, AddUInt32);
		_CONVERT(CPPTYPE_BOOL, int, "b", SetBool, AddBool);

		case FieldDescriptor::CPPTYPE_STRING: {
			if (!json_is_string(jf))
				throw j2pb_error(field, "Not a string");
			const char * value = json_string_value(jf);
			if(field->type() == FieldDescriptor::TYPE_BYTES)
				_SET_OR_ADD(SetString, AddString, b64_decode(value));
			else
				_SET_OR_ADD(SetString, AddString, value);
			break;
		}
		case FieldDescriptor::CPPTYPE_MESSAGE: {
			Message *mf = (repeated)?
				ref->AddMessage(&msg, field):
				ref->MutableMessage(&msg, field);
			json2pb(*mf, jf);
			break;
		}
		case FieldDescriptor::CPPTYPE_ENUM: {
			const EnumDescriptor *ed = field->enum_type();
			const EnumValueDescriptor *ev = 0;
			if (json_is_integer(jf)) {
				ev = ed->FindValueByNumber(json_integer_value(jf));
			} else if (json_is_string(jf)) {
				ev = ed->FindValueByName(json_string_value(jf));
			} else
				throw j2pb_error(field, "Not an integer or string");
			if (!ev)
				throw j2pb_error(field, "Enum value not found");
			_SET_OR_ADD(SetEnum, AddEnum, ev);
			break;
		}
		default:
			break;
	}
}

}
