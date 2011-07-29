#ifndef __MYSQL_COLUMNDESC_HPP
#define __MYSQL_COLUMNDESC_HPP

#include <vector>
#include <string>

namespace mysql {

class CColumnDesc
{
public:
	typedef std::vector<std::string> TVariants;
	CColumnDesc()
		: _position(0)
	{
	}
	
	CColumnDesc(size_t pos, const std::string& field_type)
		: _position(pos)
	{
		update(pos, field_type);
	}
	
	void update(size_t pos, const std::string& field_type)
	{
		_position = pos;
		_variants.clear();
		if (!field_type.empty() && (field_type.compare(0, 4, "enum") == 0 || field_type.compare(0, 3, "set") == 0))
		{
			size_t pos1 = 0, pos2 = 0;
			while (1)
			{
				pos1 = field_type.find('\'',pos1);
				if (pos1 == std::string::npos) break;

				pos2 = field_type.find('\'',++pos1);
				if (pos2 == std::string::npos) break;

				_variants.push_back(field_type.substr(pos1, pos2 - pos1));
				pos1 = pos2 + 1;
			}
		}
	}

	size_t get_position() const
	{
		return _position;
	}
	
	TVariants get_variants() const
	{
		return _variants;
	}

private:
	size_t _position;
	TVariants _variants;
};

}

#endif

