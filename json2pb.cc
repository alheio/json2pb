/*
 * Copyright (c) 2013 Pavel Shramov <shramov@mexmat.net>
 *
 * json2pb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "json2pb.h"
#include <utility>
#include <algorithm>
#include <jansson.h>
#include <cctype>
#include <cstring>
#include "exceptions.hpp"
#include "bin2ascii.hpp"

using namespace google::protobuf;

struct json_autoptr {
	json_t * ptr;
	explicit json_autoptr(json_t *json) : ptr(json) {}
	~json_autoptr() { if (ptr) json_decref(ptr); }
	json_t * release() { json_t *tmp = ptr; ptr = nullptr; return tmp; }
};

namespace j2pb 
{

int json_dump_std_string(const char *buf, size_t size, void *data)
{
	auto *s = (std::string *) data;
	s->append(buf, size);
	return 0;
}

Serializer::Serializer(std::unique_ptr<Extensions> extensions)
	: m_extensions(std::move(extensions))
{
}

Serializer::Serializer(std::unique_ptr<Extensions> extensions, Options options)
	: m_extensions(std::move(extensions))
	, m_options(std::move(options))
{
}

Serializer::Serializer(const Serializer& rhs)
	: m_extensions(rhs.m_extensions->clone())
	, m_options(rhs.m_options)
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
	json_dump_callback(root, json_dump_std_string, &r, JSON_REAL_PRECISION(options.jsonRealPrecision()));
	return r;
}


void Serializer::toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg)
{
	toProtobuf(buf, size, msg, m_options);
}

void Serializer::toProtobuf(const char *buf, size_t size, google::protobuf::Message &msg, const Options& options)
{
	json_t *root;
	json_error_t error;

	root = json_loadb(buf, size, 0, &error);

	if (!root)
		throw j2pb_error(std::string("Load failed: ") + error.text);

	json_autoptr _auto(root);

	if (!json_is_object(root))
		throw j2pb_error("Malformed JSON: not an object");

	json2pb(msg, root, options);
}

json_t* Serializer::pb2json(const google::protobuf::Message& msg, const Options& options) const
{
	const Descriptor *d = msg.GetDescriptor();
	const Reflection *ref = msg.GetReflection();
	
	if (!d || !ref) return nullptr;

	json_t *root = json_object();
	json_autoptr _auto(root);

	std::vector<const FieldDescriptor *> fields;
	fields.reserve(16);

	ref->ListFields(msg, &fields);
	
	json_t* ext = nullptr;

	for (int i = 0; i != fields.size(); i++)
	{
		const FieldDescriptor *field = fields[i];

		json_t *jf = nullptr;
		if(field->is_repeated()) {
			auto count = ref->FieldSize(msg, field);
			if (!count) continue;

			json_autoptr array(json_array());
			for (int j = 0; j < count; j++)
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

json_t* Serializer::field2json(const Message& msg, const FieldDescriptor* field, int index, const Options& options) const
{
	const Reflection *ref = msg.GetReflection();
	const bool repeated = field->is_repeated();
	json_t *jf = nullptr;
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
				jf = json_string(Bin2ASCII::b64_encode(value).c_str());
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
			const EnumValueDescriptor* ef = (repeated) ? ref->GetRepeatedEnum(msg, field, index) : ref->GetEnum(msg, field);
			if (!options.enumAsNumber())
			{
				auto hook = options.getEnumHook(ef->type()->full_name());
				jf = hook ? json_string(hook->preSerialize(ef->name()).c_str()) : json_string(ef->name().c_str());
			}
			else
			{
				jf = json_integer(ef->number());
			}
			break;
		}
		default:
			break;
	}
	if (!jf) throw j2pb_error(field, "Fail to convert to json");
	return jf;
}

std::string toLowercase(const std::string& value)
{
	std::string rc;
	rc.resize(value.size());
	std::transform(value.begin(), value.end(), rc.begin(), ::tolower);
	return rc;
}

void Serializer::json2pb(google::protobuf::Message& msg, json_t* root, const Options& options) const
{
	const Descriptor *d = msg.GetDescriptor();
	const Reflection *ref = msg.GetReflection();
	if (!d || !ref) throw j2pb_error("No descriptor or reflection");
	
	for (void* i = json_object_iter(root); i; i = json_object_iter_next(root, i))
	{
		const char *name = json_object_iter_key(i);
		json_t *jf = json_object_iter_value(i);

		const FieldDescriptor *field;
		
		if (m_options.deserializeIgnoreCase())
		{
			field = d->FindFieldByLowercaseName(toLowercase(name));
		}
		else
		{
			field = d->FindFieldByName(name);
		}
		
		if (nullptr != field)
		{
			jsonArrayOrField2field(msg, field, jf, options);
		}
		else
		{
			if (m_extensions->read(ref, name, jf))
			{
				for (std::size_t j = 0; j<m_extensions->size(); ++j)
				{
					Extensions::extension_t ext = m_extensions->get(j);
					jsonArrayOrField2field(msg, ext.first, ext.second, options);
				}
			}
			else if (!m_options.ignoreUnknownFields())
			{
				throw j2pb_error("Unknown field: " + std::string(name));
			}
		}
	}
}

void Serializer::jsonArrayOrField2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf, const Options& options) const
{
	if (field->is_repeated()) 
	{
		if (!json_is_array(jf))
			throw j2pb_error(field, "Not array");

		for (size_t j = 0, sz = json_array_size(jf); j < sz; j++)
			json2field(msg, field, json_array_get(jf, j), options);
	} 
	else
		json2field(msg, field, jf, options);
}

void Serializer::json2field(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, json_t* jf, const Options& options) const
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
			{
				if (!options.isJson2PbufImplicitCastToString())
					throw j2pb_error(field, "Not a string");

				std::unique_ptr<char[]> value(json_dumps(jf, JSON_ENCODE_ANY|JSON_COMPACT));
				if (!value)
					throw j2pb_error(field, "implicit string cast failed");

				jsonFieldSetOrAdd(msg, field, value.get());
			}
			else
			{
				const char* value = json_string_value(jf);

				jsonFieldSetOrAdd(msg, field, value);
			}

			break;
		}
		case FieldDescriptor::CPPTYPE_MESSAGE: {
			Message *mf = (repeated)?
				ref->AddMessage(&msg, field):
				ref->MutableMessage(&msg, field);
			json2pb(*mf, jf, options);
			break;
		}
		case FieldDescriptor::CPPTYPE_ENUM: {
			const EnumDescriptor *ed = field->enum_type();
			const EnumValueDescriptor *ev = nullptr;
			if (json_is_integer(jf)) {
				ev = ed->FindValueByNumber(json_integer_value(jf));
			} else if (json_is_string(jf)) {
				const char* valueName = json_string_value(jf);
				if (nullptr == valueName || '\0' == *valueName)
					break; // ignore enum empty strings
				auto hook = m_options.getEnumHook(ed->full_name());
				ev = hook ? ed->FindValueByName(hook->preDeserialize(valueName)) : ed->FindValueByName(valueName);
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

void Serializer::jsonFieldSetOrAdd(google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, const std::string& value)
{
	const auto * ref = msg.GetReflection();

	if (field->is_repeated())
	{
		ref->AddString(&msg, field, field->type() == FieldDescriptor::TYPE_BYTES ? Bin2ASCII::b64_decode(value) : value);
	}
	else
	{
		ref->SetString(&msg, field, field->type() == FieldDescriptor::TYPE_BYTES ? Bin2ASCII::b64_decode(value) : value);
	}
}

}
