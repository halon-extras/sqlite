#include "query.hpp"
#include "sqlite/sqlite.hpp"
#include "argument.hpp"
#include "utils.hpp"
#include <string>
#include <stdexcept>
#include <variant>
#include <climits>

std::shared_ptr<SQLiteConnection> get_connection(SQLite* ptr)
{
	std::unique_lock<std::mutex> ul(ptr->profile->mtx);
	ptr->profile->cv.wait(ul, [&]() { return !ptr->profile->pool.empty() || ptr->profile->pool_added < ptr->profile->pool_size; });

	std::shared_ptr<SQLiteConnection> connection;

	if (!ptr->profile->pool.empty())
	{
		// Reuse existing connection
		connection = ptr->profile->pool.front();
		ptr->profile->pool.erase(ptr->profile->pool.begin());
	}
	else
	{
		// Open new connection
		sqlite3* db = nullptr;
		unsigned int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
		if (ptr->profile->readonly)
			flags = SQLITE_OPEN_READONLY;
		if (sqlite3_open_v2(ptr->profile->filename.c_str(), &db, flags, nullptr) != SQLITE_OK)
		{
			sqlite3_close(db);
			throw std::runtime_error(sqlite3_errmsg(db));
		}

		if (ptr->profile->busy_timeout != 0)
			sqlite3_busy_timeout(db, ptr->profile->busy_timeout);
		else
			sqlite3_busy_timeout(db, INT_MAX);

		if (ptr->profile->foreign_keys)
		{
			char* err = nullptr;
			if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err) != SQLITE_OK)
			{
				std::string msg = err ? err : "Failed to enable foreign_keys";
				sqlite3_free(err);
				sqlite3_close(db);
				throw std::runtime_error(msg);
			}
		}

		connection = std::make_shared<SQLiteConnection>();
		connection->db = db;
		++ptr->profile->pool_added;
	}

	return connection;
}

sqlite3_stmt* exec_prepared(SQLite* ptr, std::shared_ptr<SQLiteConnection>& connection, const std::string& query, std::vector<Param>& params)
{
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(connection->db, query.data(), static_cast<int>(query.size()), &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error(sqlite3_errmsg(connection->db));

	for (int i = 0; i < (int)params.size(); i++)
	{
		const Param& x = params[i];
		std::visit([stmt, i](const auto& val) {
			using T = std::decay_t<decltype(val)>;
			if constexpr (std::is_same_v<T, std::string>)
			{
				if (is_valid_utf8(val.data(), val.size()))
					sqlite3_bind_text(stmt, i + 1, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
				else
					sqlite3_bind_blob(stmt, i + 1, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
			}
			else if constexpr (std::is_same_v<T, double>)
				sqlite3_bind_double(stmt, i + 1, val);
			else if constexpr (std::is_same_v<T, bool>)
				sqlite3_bind_int(stmt, i + 1, val ? 1 : 0);
			else if constexpr (std::is_same_v<T, nullptr_t>)
				sqlite3_bind_null(stmt, i + 1);
		},
				   x);
	}

	return stmt;
}

void SQLite_query(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	SQLite* ptr = (SQLite*)HalonMTA_hsl_object_ptr_get(hhc);

	// Parse arguments
	std::string query;
	if (!parse_hsl_argument_as_string(hhc, args, 0, true, query))
		return;

	std::vector<std::variant<std::nullptr_t, std::string, double, bool>> params;
	if (!parse_hsl_argument_as_params(hhc, args, 1, false, params))
		return;

	// Get connection
	std::shared_ptr<SQLiteConnection> connection;
	try
	{
		connection = get_connection(ptr);
	}
	catch (std::exception const& err)
	{
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
		return;
	}

	// Execute prepared statement
	sqlite3_stmt* stmt = nullptr;
	try
	{
		stmt = exec_prepared(ptr, connection, query, params);
	}
	catch (std::exception const& err)
	{
		sqlite3_finalize(stmt);

		// Put the connection back into the pool
		ptr->profile->mtx.lock();
		ptr->profile->pool.push_back(connection);
		ptr->profile->mtx.unlock();
		ptr->profile->cv.notify_one();

		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
		return;
	}

	// Set HSL response
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);
	int idx = 0;
	int column_count = sqlite3_column_count(stmt);
	while (true)
	{
		int rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			HalonHSLValue *k1, *v1;
			HalonMTA_hsl_value_array_add(ret, &k1, &v1);

			double i = idx;
			HalonMTA_hsl_value_set(k1, HALONMTA_HSL_TYPE_NUMBER, &i, 0);
			HalonMTA_hsl_value_set(v1, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);

			for (int c = 0; c < column_count; ++c)
			{
				const char* column_name = sqlite3_column_name(stmt, c);
				int column_type = sqlite3_column_type(stmt, c);

				HalonHSLValue *k2, *v2;
				HalonMTA_hsl_value_array_add(v1, &k2, &v2);
				HalonMTA_hsl_value_set(k2, HALONMTA_HSL_TYPE_STRING, column_name, 0);

				switch (column_type)
				{
					case SQLITE_TEXT:
					{
						const unsigned char* value = sqlite3_column_text(stmt, c);
						HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_STRING, value, 0);
						break;
					}
					case SQLITE_INTEGER:
					{
						std::string value = std::to_string(static_cast<long long>(sqlite3_column_int64(stmt, c)));
						HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_STRING, value.c_str(), 0);
						break;
					}
					case SQLITE_FLOAT:
					{
						double value = sqlite3_column_double(stmt, c);
						HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NUMBER, &value, 0);
						break;
					}
					case SQLITE_BLOB:
					{
						const void* blob = sqlite3_column_blob(stmt, c);
						int bytes = sqlite3_column_bytes(stmt, c);
						const char* value = static_cast<const char*>(blob);
						HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_STRING, value, bytes);
						break;
					}
					case SQLITE_NULL:
					{
						HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NONE, nullptr, 0);
						break;
					}
				}
			}
		}
		else if (rc == SQLITE_DONE)
		{
			break;
		}
		else
		{
			sqlite3_finalize(stmt);

			// Put the connection back into the pool
			ptr->profile->mtx.lock();
			ptr->profile->pool.push_back(connection);
			ptr->profile->mtx.unlock();
			ptr->profile->cv.notify_one();

			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, sqlite3_errmsg(connection->db), 0);
			return;
		}

		++idx;
	}

	sqlite3_finalize(stmt);

	// Put the connection back into the pool
	ptr->profile->mtx.lock();
	ptr->profile->pool.push_back(connection);
	ptr->profile->mtx.unlock();
	ptr->profile->cv.notify_one();
}
