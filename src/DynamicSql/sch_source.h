#ifndef _sqlbackend_sch_source_h_
#define _sqlbackend_sch_source_h_

namespace SqliteSchema {
#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>
#include <Sql/sch_source.h>
}
namespace MySqlSchema {
#define SCHEMADIALECT <MySql/MySqlSchema.h>
#include <Sql/sch_source.h>
}

// define global SqlIds
#define DOID(x) SqlId ADD_SCHEMA_PREFIX_CPP(x)(#x);
#include <plugin/sqlite3/Sqlite3Schema.h>

#endif
