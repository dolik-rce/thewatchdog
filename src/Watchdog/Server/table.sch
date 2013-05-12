TABLE_(SESSION)
	STRING_ (SID, 32)       PRIMARY_KEY
	BLOB_   (SDATA)
	TIME_   (STIME)
END_TABLE

TABLE_(CLIENT)
	INT_    (ID)            PRIMARY_KEY AUTO_INCREMENT
	STRING_ (NAME, 200)     INDEX NOT_NULL UNIQUE
	STRING_ (PASSWORD, 36)  NOT_NULL
	STRING_ (DESCR, 1024)   NOT_NULL
	STRING_ (SRC, 1024)     NOT_NULL
	TIME_   (LAST_ACTIVITY)
	TIME_   (LAST_WORK)     INDEX
	STRING_ (SALT, 4)
END_TABLE

TABLE_(AUTH)
	INT_    (AUTH_ID)       PRIMARY_KEY AUTO_INCREMENT
	STRING_ (NONCE, 32)     NOT_NULL
	TIME_   (VALID)         NOT_NULL
END_TABLE

TABLE_(WORK)
	INT_    (REVISION)      INDEX UNIQUE NOT_NULL
	TIME_   (DT)            NOT_NULL
	STRING_ (AUTHOR, 200)   NOT_NULL
	STRING_ (MSG, 1024)     NOT_NULL SQLDEFAULT("") 
	STRING_ (PATH, 1024)    NOT_NULL
END_TABLE

TABLE_(RESULT)
	INT_    (REV)           INDEX NOT_NULL
	INT_    (CLIENT_ID)     INDEX NOT_NULL DUAL_UNIQUE(REV, CLIENT_ID)
	TIME_   (START)         INDEX NOT_NULL
	TIME_   (FINISHED)
	INT_    (DURATION)
	INT_    (STATUS)        NOT_NULL SQLDEFAULT(0)
	INT_    (TEST_COUNT)    NOT_NULL SQLDEFAULT(1)
	INT_    (ERR_COUNT)     NOT_NULL SQLDEFAULT(0)
	INT_    (FAIL_COUNT)    NOT_NULL SQLDEFAULT(0)
	INT_    (SKIP_COUNT)    NOT_NULL SQLDEFAULT(0)
END_TABLE

TABLE_(MAIL)
	STRING_ (EMAIL, 1024)   NOT_NULL
	STRING_ (FILTER, 1024)  NOT_NULL
	STRING_ (TOKEN, 32)     NOT_NULL
	INT_    (UNIQ)          NOT_NULL UNIQUE
	TIME_   (CTIME)         NOT_NULL
	BOOL_   (ACTIVE)        NOT_NULL SQLDEFAULT(0)
END_TABLE