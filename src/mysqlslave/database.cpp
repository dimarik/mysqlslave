#include <mysqlslave/database.hpp>
#include <fstream>

namespace mysql {

/* 
 * ========================================= CRow
 * ========================================================
 */	
	
const CValue& CRow::operator[](const std::string& name) const 
{
	if (_table)
	{
		const CColumnDesc* colDesc = _table->find_column(name);
		if (colDesc)
		{
			try 
			{ 
				return _data.at(colDesc->get_position());
			}
			catch (...) 
			{
				;
			}
		}
	}
	return _null_value;
}
	
int CTable::tune(uint8_t* data, size_t size, const CFormatDescriptionLogEvent& fmt)
{
	if (_tuned) return 0;

	int rc;
	if ((rc = CTableMapLogEvent::tune(data, size, fmt)) == 0 && CTableMapLogEvent::is_valid())
	{
		_row.set_table(this);
		_row.resize(_column_count);
		uint8_t* type = _column_types;
		for (unsigned int i = 0; i < _column_count; ++i)
		{
			_row[i].pos(i);
			_row[i]._type = (CValue::EColumnType) * type++;
		}
		_tuned = true;
	}
	
	return rc;
}

int CTable::update(CRowLogEvent& rlev)
{
	const uint8_t* pfields;
	size_t len;
	uint64_t nullfields_mask;
	
	if( !rlev.is_valid() || _table_id != rlev._table_id )
		return -1;
	
	pfields = rlev.rows_data();
	len = rlev.rows_len();
	
	_new_rows.clear();
	_rows.clear();
	while( len > 0 )
	{
		nullfields_mask = ~rlev.build_column_mask(&pfields, &len, rlev.used_columns_1bit_count());
		update_row(_row, &pfields, &len, rlev._ncolumns, rlev.used_columns_mask(), nullfields_mask);
		if (len > 0)
		{
			_rows.push_back(_row);
			if( rlev.get_type_code() == UPDATE_ROWS_EVENT )
			{
				nullfields_mask = ~rlev.build_column_mask(&pfields, &len, rlev.used_columns_afterimage_1bit_count());
				update_row(_row, &pfields, &len, rlev._ncolumns, rlev.used_columns_afterimage_mask(), nullfields_mask);
				_new_rows.push_back(_row);
			}
		}
		else
		{
			_new_rows.push_back(_row);
		}
	}
		
	return len == 0 ? 0 : -1;
}

int CTable::update_row(CRow& row, const uint8_t** pdata, size_t* len, uint64_t ncolumns, uint64_t usedcolumns_mask, uint64_t nullfields_mask)
{
	CValue::EColumnType type;
	uint32_t metadata;
	uint32_t length;
	uint8_t* pmetadata;
	uint64_t bit;
	
	ssize_t size = *len;
	const uint8_t* data = *pdata;
	pmetadata = _metadata;
	
/*VDEBUG_CHUNK(	
	{
	char buf[64];
	sprintf(buf, "row_%d.row", (int)::time(NULL));
	std::ofstream f(buf, std::ofstream::binary);
	f.write((char*)*pdata, *len);
	f.flush();
	f.close();
	}
)*/
	bit = 0x01;
	for (uint64_t i = 0; i < ncolumns; ++i)
	{
		if (size <= 0)
		{
			row[i].reset();
			continue;
		} // NOTE: achtung; exception is here
		
		type = (CValue::EColumnType)*(_column_types + i);
		switch (CValue::calc_metadata_size(type))
		{
			case 0:
			{
				metadata = 0;
				break;
			}
			case 1:
			{
				metadata = *pmetadata++;
				break;
			}
			case 2:
			{
				metadata = *(uint16_t*)pmetadata;
				pmetadata += 2;
				break;
			}
			default:
				// хз, ненадежно, конечно, но метаданные по протоколу типа не должны быть длиннее 2 байт
				metadata = 0; 
		}
		
		row[i].reset();
		if (usedcolumns_mask & nullfields_mask & bit)
		{
			length = CValue::calc_field_size(type, data, metadata);
			row[i].tune(type, data, metadata, length);
			data += length;
			size -= length;
		}
			
		bit <<= 1;
	}
	*pdata = data;
	*len = size < 0 ? 0 : size;
	
	return size >= 0 ? 0 : -1;
}

void CTable::add_column(const std::string& column_name, int column_pos, const std::string& column_type)
{
	_data[column_name].update(column_pos, column_type);
}

}

