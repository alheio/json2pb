#ifndef __JSON2PB_EXTENSIONS__HPP__
#define __JSON2PB_EXTENSIONS__HPP__

#include <google/protobuf/message.h>
#include <string>
#include <vector>
#include <memory>

struct json_t;

namespace j2pb
{
	class Extensions
	{
	public:
		typedef std::pair<const google::protobuf::FieldDescriptor*, json_t*> extension_t;
		
	protected:
		std::vector<extension_t> m_extensions;
		
	public:
		virtual ~Extensions() = default;
		virtual std::unique_ptr<Extensions> clone() const = 0;
		virtual void write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const = 0;
		virtual bool read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf) = 0;
		
	public:
		std::size_t size() const { return m_extensions.size(); };
		const extension_t& get(std::size_t pos) const { return m_extensions[pos]; }
	};
	

	class ClassicExtensions : public Extensions
	{
	public:
		std::unique_ptr<Extensions> clone() const override;
		void write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const override;
		bool read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf) override;
	};
	
	
	class OpenRTBExtensions : public Extensions
	{
	public:
		std::unique_ptr<Extensions> clone() const override;
		void write(const google::protobuf::FieldDescriptor* fd, json_t* root, json_t* jf) const override;
		bool read(const google::protobuf::Reflection* ref, const std::string& jk, json_t* jf) override;
		
	private:
		void readMore(const google::protobuf::Reflection* ref, const std::string& name, json_t* jf);
	};
}

#endif
