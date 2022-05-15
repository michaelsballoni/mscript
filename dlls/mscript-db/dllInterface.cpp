#include "pch.h"

// One thing at a time
std::mutex g_mutex;

// Global database access object storage
// Applications start with a name and a path to the DB file on disk,
// then refer to that database by name after that
std::unordered_map<std::wstring, std::shared_ptr<fourdb::ctxt>> g_contexts;
std::unordered_map<std::wstring, std::shared_ptr<fourdb::db>> g_db_conns;

//
// CONVERSION ROUTINES
//
static fourdb::strnum convert(const mscript::object& obj)
{
	switch (obj.type())
	{
	case mscript::object::NOTHING:
		return double(0.0);

	case mscript::object::STRING:
		return obj.stringVal();

	case mscript::object::NUMBER:
		return obj.numberVal();

	case mscript::object::BOOL:
		return obj.boolVal() ? 1.0 : 0.0;

	default:
		raiseError("Invalid object type for conversion to 4db: " + mscript::num2str(int(obj.type())));
	}

}

static mscript::object convert(const fourdb::strnum& obj)
{
	if (obj.isStr())
		return obj.str();
	else
		return obj.num();
}

static fourdb::paramap convert(const mscript::object::index idx)
{
	fourdb::paramap ret_val;
	for (const auto& it : idx.vec())
		ret_val.insert({ it.first.toString(), convert(it.second) });
	return ret_val;
}

// Given a 4db context name, get the context object
static std::shared_ptr<fourdb::ctxt> get4db(const std::wstring& name)
{
	const auto& it = g_contexts.find(name);
	if (it == g_contexts.end())
		raiseWError(L"4db not found: " + name + L" - call msdb_4db_init first");
	return it->second;
}

// Given a SQL DB context name, get the DB object
static std::shared_ptr<fourdb::db> getSqldb(const std::wstring& name)
{
	const auto& it = g_db_conns.find(name);
	if (it == g_db_conns.end())
		raiseWError(L"SQL DB not found: " + name + L" - call msdb_sql_init first");
	return it->second;
}

// Given a DB reader, return a list of lists, 
// column header names in the first list, row data in the following lists
static mscript::object::list processDbReader(fourdb::dbreader& reader)
{
	mscript::object::list ret_val;
	ret_val.push_back(mscript::object::list()); // column names

	unsigned col_count = reader.getColCount();
	ret_val[0].listVal().reserve(col_count);
	for (unsigned c = 0; c < col_count; ++c)
		ret_val[0].listVal().push_back(reader.getColName(c));

	while (reader.read())
	{
		mscript::object::list row_list;
		row_list.reserve(col_count);
		for (unsigned c = 0; c < col_count; ++c)
		{
			bool is_null = false;
			fourdb::strnum val = reader.getStrNum(c, is_null);
			row_list.push_back(is_null ? mscript::object() : convert(val));
		}
	}

	return ret_val;
}

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"msdb_sql_init",
		L"msdb_sql_close",
		L"msdb_sql_query",

		L"msdb_4db_init",
		L"msdb_4db_close",

		L"msdb_4db_define",
		L"msdb_4db_undefine",

		L"msdb_4db_query",

		L"msdb_4db_delete",

		L"msdb_4db_drop",
		L"msdb_4db_reset",

		L"msdb_4db_getschema",
	};
	return mscript::module_utils::getExports(exports);
}

void mscript_FreeString(wchar_t* str)
{
	delete[] str;
}

wchar_t* mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson)
{
	try
	{
		std::wstring funcName = functionName;
		auto params = mscript::module_utils::getParams(parametersJson);

		std::unique_lock ctx_lock(g_mutex);

		if (funcName == L"msdb_sql_init")
		{
			if
			(
				params.size() != 2
				||
				params[0].type() != mscript::object::STRING
				|| 
				params[1].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_sql_init takes two parameters: a name for the database, and the path to the DB file");
			}

			std::wstring db_name = params[0].stringVal();
			std::wstring db_file_path = params[1].stringVal();

			if (g_db_conns.find(db_file_path) != g_db_conns.end())
				raiseWError(L"msdb_sql_init: database already initialized: " + db_name);

			auto new_db = std::make_shared<fourdb::db>(mscript::toNarrowStr(db_file_path));
			g_db_conns.insert({ db_name, new_db });

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_sql_close")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_sql_close takes the name of the database");
			}

			std::wstring db_name = params[0].stringVal();

			auto db_it = g_db_conns.find(db_name);
			if (db_it == g_db_conns.end())
				raiseWError(L"msdb_sql_close: database not open: " + db_name);

			g_db_conns.erase(db_it);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_sql_query")
		{
			if
			(
				params.size() != 3
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				||
				params[2].type() != mscript::object::INDEX
			)
			{
				raiseError("msdb_sql_query takes three parameters: the name of the database, the SQL query, and an index of query parameters");
			}

			std::wstring db_name = params[0].stringVal();
			std::wstring sql_query = params[1].stringVal();
			fourdb::paramap query_params = convert(params[2].indexVal());
			
			auto sql_db = getSqldb(db_name);
			auto reader = sql_db->execReader(sql_query, query_params);

			auto results = processDbReader(*reader);

			return mscript::module_utils::jsonStr(results);
		}
		else if (funcName == L"msdb_4db_init")
		{
			if
			(
				params.size() != 2 
				|| 
				params[0].type() != mscript::object::STRING 
				|| 
				params[1].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_init takes two parameters: a name for the context, and the path to the DB file");
			}

			std::wstring db_name = params[0].stringVal();
			std::wstring db_file_path = params[1].stringVal();

			if (g_contexts.find(db_name) != g_contexts.end())
				raiseWError(L"msdb_4db_init: context already initialized: " + db_name);

			auto ctxt_ptr = std::make_shared<fourdb::ctxt>(mscript::toNarrowStr(db_file_path));
			g_contexts.insert({ db_name, ctxt_ptr });

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_close")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_close takes the name of the context");
			}

			std::wstring db_name = params[0].stringVal();

			auto ctxt_it = g_contexts.find(db_name);
			if (ctxt_it == g_contexts.end())
				raiseWError(L"msdb_4db_close: context not open: " + db_name);

			g_contexts.erase(ctxt_it);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_define")
		{
			if
			(
				params.size() != 4
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				//|| object
				//params[2].type() != mscript::object::STRING
				||
				params[3].type() != mscript::object::INDEX
			)
			{
				raiseError("msdb_4db_define takes four parameters: the name of the context, the table name, the key value, and an index of name-value pairs");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			const std::wstring& table_name = params[1].stringVal();
			fourdb::strnum key_value = convert(params[2]);
			const fourdb::paramap metadata = convert(params[2].indexVal());

			ctxt->define(table_name, key_value, metadata);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_undefine")
		{
			if
			(
				params.size() != 3
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				//|| object
				//params[2].type() != mscript::object::STRING
				||
				params[3].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_undefine takes four parameters: the name of the context, the table name, the key value, and the name of the metadata to remove");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			const std::wstring& table_name = params[1].stringVal();
			fourdb::strnum key_value = convert(params[2]);
			const std::wstring& metadata_name = params[1].stringVal();

			ctxt->undefine(table_name, key_value, metadata_name);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_query")
		{
			if
			(
				params.size() != 3
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				||
				params[2].type() != mscript::object::INDEX
			)
			{
				raiseError("msdb_4db_query takes three parameters: the name of the context, the SQL query, and an index of name-value parameters");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			const std::wstring& sql_query = params[1].stringVal();
			const fourdb::paramap query_params = convert(params[2].indexVal());

			fourdb::select sql_select = fourdb::sql::parse(sql_query);
			for (const auto& param_it : query_params)
				sql_select.addParam(param_it.first, param_it.second);

			auto db_reader = ctxt->execQuery(sql_select);

			mscript::object::list ret_val;
			ret_val.push_back(mscript::object::list());

			unsigned col_count = db_reader->getColCount();
			ret_val[0].listVal().reserve(col_count);
			for (unsigned c = 0; c < col_count; ++c)
				ret_val[0].listVal().push_back(db_reader->getColName(c));

			while (db_reader->read())
			{
				mscript::object::list row_idx;
				row_idx.reserve(col_count);
				for (unsigned c = 0; c < col_count; ++c)
				{
					bool is_null = false;
					fourdb::strnum val = db_reader->getStrNum(c, is_null);
					row_idx.push_back(is_null ? mscript::object() : convert(val));
				}
			}

			return mscript::module_utils::jsonStr(ret_val);
		}
		else if (funcName == L"msdb_4db_delete")
		{
			if
			(
				params.size() != 3
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				//|| object
				//params[2].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_delete takes three parameters: the name of the context, the table name, and the key value");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			const std::wstring& table_name = params[1].stringVal();
			fourdb::strnum key_value = convert(params[2]);

			ctxt->deleteRow(table_name, key_value);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_drop")
		{
			if
			(
				params.size() != 2
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_drop takes two parameters: the name of the context, and the table name");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			const std::wstring& table_name = params[1].stringVal();

			ctxt->drop(table_name);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_reset")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_reset takes the name of the context to reset");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);

			ctxt->reset();

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_getschema")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("msdb_4db_getschema takes the name of the context to get the schema of");
			}

			const std::wstring& db_name = params[0].stringVal();
			auto ctxt = get4db(db_name);
			auto schema = ctxt->getSchema();
			std::vector<std::wstring> lines;
			for (const auto& tables_it : schema.vec())
			{
				const std::wstring& table_name = tables_it.first;
				const auto& column_names = *tables_it.second;
				std::wstring line = 
					table_name + 
					L": " + 
					mscript::join(column_names, L", ");
				lines.push_back(line);
			}
			std::wstring schema_str = mscript::join(lines, L"\n");
			return mscript::module_utils::jsonStr(schema_str);
		}
		else
			raiseWError(L"msdb: unknown function: " + funcName);
	}
	catch (const mscript::user_exception& exp)
	{
		return mscript::module_utils::errorStr(functionName, exp);
	}
	catch (const std::exception& exp)
	{
		return mscript::module_utils::errorStr(functionName, exp);
	}
	catch (...)
	{
		return nullptr;
	}
}
