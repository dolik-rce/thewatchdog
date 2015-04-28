#include "Client.h"

NAMESPACE_UPP

template<>
void Jsonize(JsonIO& io, HttpCookie& var) {
	io("id", var.id)("value", var.value)("domain", var.domain)("path", var.path)("raw", var.raw);
};

END_UPP_NAMESPACE

CookieFile::CookieFile(String filename, bool always) {
	Load();
}
CookieFile::~CookieFile() {
	Store();
}

void CookieFile::SetFilename(String fn) {
	filename = fn;
	Load();
}

void CookieFile::Load() {
	if(filename.GetCount())
		LoadFromJsonFile(cookies, filename);
}

void CookieFile::Store() const {
	if(filename.GetCount())
		StoreAsJsonFile(cookies, filename);
}

void CookieFile::StoreCookies(const HttpRequest& req) {
	if (always) Load();
	const VectorMap<String, HttpCookie>& c = req.GetHttpHeader().cookies;
	for(int i = 0; i < c.GetCount(); i++)
		cookies.Add(c.GetKey(i), c[i]);
	if (always) Store();
}

void CookieFile::LoadCookies(HttpRequest& req) {
	if (always) Load();
	for(int i = 0; i < cookies.GetCount(); i++)
		req.Cookie(cookies[i]);
	if (always) Store();
}
