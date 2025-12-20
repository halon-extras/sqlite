#ifndef SQLITE_HPP_
#define SQLITE_HPP_

#include <HalonMTA.h>
#include <mutex>
#include <memory>
#include <vector>
#include <condition_variable>
#include <sqlite3.h>
#include <string>

struct SQLiteConnection
{
	sqlite3* db = nullptr;

	~SQLiteConnection()
	{
		if (db)
			sqlite3_close(db);
	}
};

struct SQLiteProfile
{
	std::string filename;
	std::vector<std::shared_ptr<SQLiteConnection>> pool;
	unsigned int pool_size = 1;
	unsigned int pool_added = 0;
	std::mutex mtx;
	std::condition_variable cv;
	bool foreign_keys = false;
	bool readonly = false;
	unsigned int busy_timeout = 0;
};

class SQLite
{
  public:
	std::shared_ptr<SQLiteProfile> profile;
};

void SQLite_constructor(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret);
void SQLite_free(void* ptr);

#endif
