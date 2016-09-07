#ifndef __JSON2PB_OPTIONS_HPP__
#define __JSON2PB_OPTIONS_HPP__


namespace j2pb
{
	class Options
	{
		bool m_enumAsNumber;
		
	public:
		Options() 
			: m_enumAsNumber(false)
		{
		}
		
		/**
		 * serialize protobuf enum as integer values instead of string representation
		 * defaul: enum as string
		 */
		Options& enumAsNumber(bool value) 
		{ 
			m_enumAsNumber = value;
			return *this; 
		}
		
		bool enumAsNumber() const { return m_enumAsNumber; }
	};
}


#endif