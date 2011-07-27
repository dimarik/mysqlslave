#ifndef MYSQL_DATABASE_H
#define MYSQL_DATABASE_H


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

#include <string>
#include <vector>
#include <list>
#include <map>

#include "logevent.h"
#include "value.h"
#include "columndesc.hpp"

namespace mysql {

struct CCaseIgnorer
{ 
	bool operator() (const std::string &s1, const std::string &s2) const
	{
		return strcasecmp(s1.c_str(), s2.c_str()) > 0;	
	}
};

class CTable;

class CRow
{
public:
	CRow()
		: _table(0)
	{
	}

	const CValue& operator[](const std::string& name) const;
	const CValue& operator[](size_t i) const
	{
		return _data[i];
	}
	CValue& operator[](size_t i)
	{
		return _data[i];
	}

	void resize(size_t sz)
	{
		_data.resize(sz);
	}

	void set_table(CTable* table)
	{
		_table = table;
	}

protected:
	CTable *_table;
	CValue _null_value;
	std::vector<CValue> _data;
};



class CTable : public CTableMapLogEvent
{
public:
	typedef std::list<CRow> TRows;

	const CColumnDesc* find_column(const std::string& s) const
	{
		data_type::const_iterator it = _data.find(s);
		if (it == _data.end()) return 0;
		return &(it->second);
	}
private:
	typedef std::map<std::string, CColumnDesc, CCaseIgnorer> data_type;
	data_type _data;

	
public:
	CTable()
		: _tuned(false)
	{
	}
	virtual ~CTable() throw()
	{
	}
	
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	virtual bool is_valid() const
	{
		return _tuned;
	}
	
	const TRows& get_rows() const
	{
		return _rows;
	}
	const TRows& get_new_rows() const
	{
		return _new_rows;
	}
	
	void add_column(const std::string& column_name, int column_pos, const std::string& column_type);
	
	int update(CRowLogEvent &rlev);
protected:
	int update_row(CRow &row, const uint8_t **pdata, size_t *len, uint64_t ncolumns, uint64_t usedcolumns_mask, uint64_t nullfields_mask);

protected:
	CRow _row;
	TRows _rows;
	TRows _new_rows;
	bool _tuned;
};	


class CDatabase
{
	typedef std::map<std::string, CTable, CCaseIgnorer> data_type;
	data_type _data;
public:
	CTable* get_table(const std::string& s)
	{
		data_type::iterator it = _data.find(s);
		if (it == _data.end()) return 0;
		return &(it->second);
	}

	const CTable* get_table(const std::string& s) const
	{
		data_type::const_iterator it = _data.find(s);
		if (it == _data.end()) return 0;
		return &(it->second);
	}

	void set_table(const std::string& s, CTable& t)
	{
		_data[s] = t;
	}
};


}

#endif // DATABASE_H
