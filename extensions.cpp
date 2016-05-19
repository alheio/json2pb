#include "extensions.hpp"
#include "bin2ascii.h"
#include <jansson.h>

namespace j2pb 
{
void ClassicExtensions::write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const
{
	if (NULL != fd && fd->is_extension())
		json_object_set_new(root, fd->full_name().c_str(), jf);
}

bool ClassicExtensions::read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf)
{
	m_extensions.clear();
	
	if (NULL != ref)
	{
		const google::protobuf::FieldDescriptor* fd = ref->FindKnownExtensionByName(jk);
		if (NULL != fd)
			m_extensions.push_back(std::make_pair(fd, jf));
	}
	
	return !m_extensions.empty();
}

void OpenRTBExtensions::write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const
{
	if (!fd->is_extension())
		return;
	
	json_t* ext = json_object_get(root, "ext");
	if (NULL == ext)
	{
		ext = json_object();
		json_object_set(root, "ext", ext);
	}
	else
		json_incref(ext);
	
	const std::string& fullName = fd->full_name();
	bool hasTokens = false;
	for (std::size_t pos = fullName.find('.'), prevPos = 0; pos != std::string::npos; pos = fullName.find('.', ++pos))
	{
		std::string token = fullName.substr(prevPos, pos - prevPos);
		if (!token.empty())
		{
			hasTokens = true;
			prevPos = pos + 1;
			
			json_t* innerObj = json_object_get(ext, token.c_str());
			if (NULL == innerObj)
			{
				innerObj = json_object();
				json_object_set(ext, token.c_str(), innerObj);
			}
			else
				json_incref(innerObj);
			
			json_decref(ext);
			ext = innerObj;
		}
	}
	
	if (hasTokens)
		json_object_set_new(ext, fd->name().c_str(), jf);
	else
		json_decref(jf);
	
	json_decref(ext);
}

bool OpenRTBExtensions::read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf)
{
	m_extensions.clear();
	
	if (jk == "ext" && NULL != jf && json_is_object(jf))
		readMore(ref, "", jf);
	
	return !m_extensions.empty();
}

void OpenRTBExtensions::readMore(const google::protobuf::Reflection* ref, const std::string& name, json_t* jf)
{
	const char* dot = name.empty() ? "" : ".";
	
	for (void* i = json_object_iter(jf); NULL != i; i = json_object_iter_next(jf, i))
	{
		const char* key = json_object_iter_key(i);
		json_t* value = json_object_iter_value(i);
		
		std::string fn = name + dot + key;
		const google::protobuf::FieldDescriptor* field = ref->FindKnownExtensionByName(fn);
		if (NULL == field)
		{
			if (json_is_object(value))
				readMore(ref, fn, value);
		}
		else
		{
			m_extensions.push_back(std::make_pair(field, value));
		}
	}
}

}
