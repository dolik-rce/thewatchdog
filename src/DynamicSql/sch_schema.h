#ifndef _sqlbackend_sch_schema_h_
#define _sqlbackend_sch_schema_h_

namespace SqliteSchema {
#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>
#include <Sql/sch_schema.h>
#undef SCHEMADIALECT
}
namespace MySqlSchema {
#define SCHEMADIALECT <MySql/MySqlSchema.h>
#include <Sql/sch_schema.h>
#undef SCHEMADIALECT
}

void All_Tables(SqlSchema& sch, int dialect) {
	switch(dialect) {
		case MY_SQL: MySqlSchema::All_Tables(sch); break;
		case SQLITE3: SqliteSchema::All_Tables(sch); break;
		default: NEVER();
	}
}

#endif
