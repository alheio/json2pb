#include "extensions.hpp"
#include "bin2ascii.hpp"
#include <jansson.h>

namespace j2pb 
{

std::unique_ptr<Extensions> ClassicExtensions::clone() const
{
	return std::make_unique<ClassicExtensions>(*this);
}

void ClassicExtensions::write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const
{
	if (nullptr != fd && fd->is_extension())
		json_object_set_new(root, fd->full_name().c_str(), jf);
}

bool ClassicExtensions::read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf)
{
	m_extensions.clear();
	
	if (nullptr != ref)
	{
		const google::protobuf::FieldDescriptor* fd = ref->FindKnownExtensionByName(jk);
		if (nullptr != fd)
			m_extensions.emplace_back(fd, jf);
	}
	
	return !m_extensions.empty();
}

std::unique_ptr<Extensions> OpenRTBExtensions::clone() const
{
	return std::make_unique<OpenRTBExtensions>(*this);
}

void OpenRTBExtensions::write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const
{
	if (!fd->is_extension())
		return;
	
	json_t* ext = json_object_get(root, "ext");
	if (nullptr == ext)
	{
		ext = json_object();
		json_object_set(root, "ext", ext);
	}
	else
		json_incref(ext);
	
	const std::string& fullName = fd->full_name();
	for (std::size_t pos = fullName.find('.'), prevPos = 0; pos != std::string::npos; pos = fullName.find('.', ++pos))
	{
		std::string token = fullName.substr(prevPos, pos - prevPos);
		if (!token.empty())
		{
			prevPos = pos + 1;
			
			json_t* innerObj = json_object_get(ext, token.c_str());
			if (nullptr == innerObj)
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

	json_object_set_new(ext, fd->name().c_str(), jf);
	json_decref(ext);
}

bool OpenRTBExtensions::read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf)
{
	// allow unknown openrtb extensions
	m_extensions.clear();
	
	bool rc = jk == "ext";
	
	if (rc && nullptr != jf && json_is_object(jf))
		readMore(ref, "", jf);
	
	return rc;
}

void OpenRTBExtensions::readMore(const google::protobuf::Reflection* ref, const std::string& name, json_t* jf)
{
	const char* dot = name.empty() ? "" : ".";
	
	for (void* i = json_object_iter(jf); nullptr != i; i = json_object_iter_next(jf, i))
	{
		const char* key = json_object_iter_key(i);
		json_t* value = json_object_iter_value(i);
		
		std::string fn = name + dot + key;
		const google::protobuf::FieldDescriptor* field = ref->FindKnownExtensionByName(fn);
		if (nullptr == field)
		{
			if (json_is_object(value))
				readMore(ref, fn, value);
		}
		else
		{
			m_extensions.emplace_back(field, value);
		}
	}
}

}
