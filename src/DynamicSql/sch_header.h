#ifndef _sqlbackend_sch_header_h_
#define _sqlbackend_sch_header_h_

namespace SqliteSchema {
#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>
#include <Sql/sch_header.h>
}
namespace MySqlSchema {
#define SCHEMADIALECT <MySql/MySqlSchema.h>
#include <Sql/sch_header.h>
}

// declare global SqlIds
#define DOID(x) extern SqlId ADD_SCHEMA_PREFIX_CPP(x);
#include <plugin/sqlite3/Sqlite3Schema.h>

#endif
