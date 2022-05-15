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
		ret_val.emplace_back(mscript::object::list());
		mscript::object::list& row_list = ret_val.back().listVal();
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

		L"msdb_sql_exec",
		L"msdb_sql_rows_affected",
		L"msdb_sql_last_inserted_id",

		L"msdb_4db_init",
		L"msdb_4db_close",

		L"msdb_4db_define",
		L"msdb_4db_undefine",

		L"msdb_4db_query",

		L"msdb_4db_delete",

		L"msdb_4db_drop",
		L"msdb_4db_reset",

		L"msdb_4db_get_schema",
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
				raiseError("Takes two parameters: a name for the database, and the path to the DB file");
			}

			std::wstring db_name = params[0].stringVal();
			std::wstring db_file_path = params[1].stringVal();

			if (g_db_conns.find(db_name) != g_db_conns.end())
				raiseWError(L"Database already initialized: " + db_name);

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
				raiseError("Takes the name of the database");
			}

			auto db_it = g_db_conns.find(params[0].stringVal());
			if (db_it != g_db_conns.end())
				g_db_conns.erase(db_it);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_sql_exec")
		{
			if
			(
				(params.size() != 2 && params.size() != 3)
				||
				params[0].type() != mscript::object::STRING
				||
				params[1].type() != mscript::object::STRING
				||
				(params.size() == 3 && params[2].type() != mscript::object::INDEX)
			)
			{
				raiseError("Takes the name of the database, the SQL query, and an optional index of query parameters");
			}

			auto sql_db = getSqldb(params[0].stringVal());
			std::wstring sql_query = params[1].stringVal();
			auto params_idx = 
				params.size() >= 3 
				? params[2].indexVal() 
				: mscript::object::index();
			fourdb::paramap query_params = convert(params_idx);
			
			auto reader = sql_db->execReader(sql_query, query_params);

			auto results = processDbReader(*reader);

			return mscript::module_utils::jsonStr(results);
		}
		else if (funcName == L"msdb_sql_rows_affected")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("Takes the database name");
			}

			auto sql_db = getSqldb(params[0].stringVal());
			int64_t rows_affected = sql_db->execScalarInt64(L"SELECT changes()").value();
			return mscript::module_utils::jsonStr(double(rows_affected));
		}
		else if (funcName == L"msdb_sql_last_inserted_id")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("Takes the database name");
			}

			auto sql_db = getSqldb(params[0].stringVal());
			int64_t last_inserted_id = sql_db->execScalarInt64(L"SELECT last_insert_rowid()").value();
			return mscript::module_utils::jsonStr(double(last_inserted_id));
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
				raiseError("Takes two parameters: a name for the context, and the path to the DB file");
			}

			std::wstring db_name = params[0].stringVal();
			std::wstring db_file_path = params[1].stringVal();

			if (g_contexts.find(db_name) != g_contexts.end())
				raiseWError(L"Context already initialized: " + db_name);

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
				raiseError("Takes the name of the context");
			}

			auto ctxt_it = g_contexts.find(params[0].stringVal());
			if (ctxt_it != g_contexts.end())
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
				raiseError("Takes four parameters: the name of the context, the table name, the key value, and an index of name-value pairs");
			}

			auto ctxt = get4db(params[0].stringVal());

			const std::wstring& table_name = params[1].stringVal();
			const fourdb::strnum key_value = convert(params[2]);
			const fourdb::paramap metadata = convert(params[3].indexVal());

			ctxt->define(table_name, key_value, metadata);

			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_undefine")
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
				params[3].type() != mscript::object::STRING
			)
			{
				raiseError("Takes four parameters: the name of the context, the table name, the key value, and the name of the metadata to remove");
			}

			auto ctxt = get4db(params[0].stringVal());

			const std::wstring& table_name = params[1].stringVal();
			const fourdb::strnum key_value = convert(params[2]);
			const std::wstring& metadata_name = params[3].stringVal();

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
				raiseError("Takes three parameters: the name of the context, the SQL query, and an index of name-value parameters");
			}

			auto ctxt = get4db(params[0].stringVal());

			fourdb::select sql_select = fourdb::sql::parse(params[1].stringVal());
			const fourdb::paramap query_params = convert(params[2].indexVal());
			for (const auto& param_it : query_params)
				sql_select.addParam(param_it.first, param_it.second);

			auto reader = ctxt->execQuery(sql_select);
			auto results = processDbReader(*reader);

			return mscript::module_utils::jsonStr(results);
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
				raiseError("Takes three parameters: the name of the context, the table name, and the key value");
			}

			auto ctxt = get4db(params[0].stringVal());

			const std::wstring& table_name = params[1].stringVal();
			const fourdb::strnum key_value = convert(params[2]);

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
				raiseError("Takes two parameters: the name of the context, and the table name");
			}

			auto ctxt = get4db(params[0].stringVal());
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
				raiseError("Takes the name of the context to reset");
			}

			auto ctxt = get4db(params[0].stringVal());
			ctxt->reset();
			return mscript::module_utils::jsonStr(mscript::object());
		}
		else if (funcName == L"msdb_4db_get_schema")
		{
			if
			(
				params.size() != 1
				||
				params[0].type() != mscript::object::STRING
			)
			{
				raiseError("Takes the name of the context to get the schema of");
			}

			auto ctxt = get4db(params[0].stringVal());
			const auto schema = ctxt->getSchema();
			mscript::object::index table_schema;
			for (const auto& tables_it : schema.vec())
			{
				const std::wstring& table_name = tables_it.first;
				mscript::object::list table_columns;
				table_columns.reserve(tables_it.second->size());
				for (const std::wstring& col : *tables_it.second)
					table_columns.push_back(col);
				table_schema.set(table_name, table_columns);
			}
			return mscript::module_utils::jsonStr(table_schema);
		}
		else
			raiseWError(L"Unknown function");
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
