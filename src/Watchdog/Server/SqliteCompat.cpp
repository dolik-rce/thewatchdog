#include "Sql.h"
#include <plugin/pcre/Pcre.h>

namespace {
	T_SQLITE_DLL* sSqlite = NULL;
}

extern "C" {
void sqlite_md5(sqlite3_context *context, int argc, sqlite3_value **argv) {
	String md5 = MD5StringS((const char*)sSqlite->sqlite3_value_text(argv[0]));
	sSqlite->sqlite3_result_text(context, md5.Begin(), md5.GetCount(), SQLITE_TRANSIENT);
	return;
}
void sqlite_concat(sqlite3_context *context, int argc, sqlite3_value **argv) {
	if(argc==0) {
		sSqlite->sqlite3_result_null(context);
		return;
	}
	String s;
	for(int i = 0; i < argc; i++)
		s.Cat((const char*)sSqlite->sqlite3_value_text(argv[i]));
	sSqlite->sqlite3_result_text(context, s.Begin(), s.GetCount(), SQLITE_TRANSIENT);
}
void sqlite_if(sqlite3_context *context, int argc, sqlite3_value **argv) {
	bool cond = sSqlite->sqlite3_value_int(argv[0]);
	if(cond)
		sSqlite->sqlite3_result_value(context, argv[1]); 
	else
		sSqlite->sqlite3_result_value(context, argv[2]); 
}

// Regexp function is based on public domain code by Alexey Tourbin <at@altlinux.org>
struct cache_entry {
	char *s;
	pcre *p;
	pcre_extra *e;
	cache_entry() : s(NULL), p(NULL), e(NULL) {}
};

#ifndef CACHE_SIZE
#define CACHE_SIZE 16
#endif

static
void sqlite_regexp(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	const char *re, *str;
	pcre *p;
	pcre_extra *e;
	
	ASSERT(argc == 2);
	
	re = (const char *) sSqlite->sqlite3_value_text(argv[0]);
	if (!re) {
		sSqlite->sqlite3_result_error(ctx, "no regexp", -1);
		return;
	}
	
	str = (const char *) sSqlite->sqlite3_value_text(argv[1]);
	if (!str) {
		sSqlite->sqlite3_result_error(ctx, "no string", -1);
		return;
	}
	
	/* simple LRU cache */
	{
		int i;
		int found = 0;
		cache_entry *cache = (cache_entry*)sSqlite->sqlite3_user_data(ctx);
		
		ASSERT(cache);
		
		for (i = 0; i < CACHE_SIZE && cache[i].s; i++){
			if (strcmp(re, cache[i].s) == 0) {
				found = 1;
				break;
			}
		}
		if (found) {
			if (i > 0) {
				cache_entry c = cache[i];
				memmove(cache + 1, cache, i * sizeof(cache_entry));
				cache[0] = c;
			}
		}
		else {
			cache_entry c;
			const char *err;
			int pos;
			c.p = pcre_compile(re, 0, &err, &pos, NULL);
			if (!c.p) {
				String e2 = Format("%s: %s (offset %d)", re, err, pos);
				sSqlite->sqlite3_result_error(ctx, e2.Begin(), -1);
				return;
			}
			c.e = pcre_study(c.p, 0, &err);
			c.s = strdup(re);
			if (!c.s) {
				sSqlite->sqlite3_result_error(ctx, "strdup: ENOMEM", -1);
				pcre_free(c.p);
				pcre_free(c.e);
				return;
			}
			i = CACHE_SIZE - 1;
			if (cache[i].s) {
				free(cache[i].s);
				ASSERT(cache[i].p);
				pcre_free(cache[i].p);
				pcre_free(cache[i].e);
			}
			memmove(cache + 1, cache, i * sizeof(cache_entry));
			cache[0] = c;
		}
		p = cache[0].p;
		e = cache[0].e;
	}
	
	{
		int rc;
		ASSERT(p);
		rc = pcre_exec(p, e, str, strlen(str), 0, 0, NULL, 0);
		sSqlite->sqlite3_result_int(ctx, rc >= 0);
		return;
	}
}

}

struct SqlCache : public Buffer<cache_entry> {
	SqlCache() : Buffer<cache_entry>(CACHE_SIZE) {}
};

void AddSqliteCompatibilityFunctions(DynamicSqlSession& dsql){
	::sqlite3* db = (::sqlite3*)(Sqlite3Session::sqlite3*)dsql.GetSqlite3Session();
	sSqlite = &dsql.GetSqliteDli();
	sSqlite->sqlite3_create_function(db, "md5", 1, SQLITE_ANY, 0, (void*)sqlite_md5, 0, 0);
	sSqlite->sqlite3_create_function(db, "concat", -1, SQLITE_ANY, 0, (void*)sqlite_concat, 0, 0);
	sSqlite->sqlite3_create_function(db, "if", 3, SQLITE_ANY, 0, (void*)sqlite_if, 0, 0);
	SqlCache& cache = Single<SqlCache>();
	sSqlite->sqlite3_create_function(db, "REGEXP", 2, SQLITE_UTF8, ~cache, (void*)sqlite_regexp, 0, 0);
}
