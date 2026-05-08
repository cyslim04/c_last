#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <sqlite3.h>

#include "../include/sha256.h"

#include <ctype.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SERVER_PORT 8080
#define DB_PATH "data/trust_work.db"
#define DEMO_SEED_VERSION 4
#define MAX_REQUEST_SIZE 524288
#define MAX_RESPONSE_SIZE 262144
#define MAX_BODY_SIZE 262144

typedef struct {
  sqlite3 *db;
} AppContext;

typedef struct {
  char method[8];
  char path[512];
  char authorization[256];
  char body[MAX_BODY_SIZE];
} HttpRequest;

typedef struct {
  int id;
  char username[64];
  char name[128];
  char role[32];
  char status[32];
  char wallet_address[128];
  char email[128];
  char phone[64];
  char company[128];
  char bio[512];
  char avatar_path[256];
} CurrentUser;

typedef struct {
  int stage_no;
  char state[32];
  char latest_deliverable_status[64];
  char latest_deliverable_version[64];
  char latest_deliverable_name[256];
  char latest_approval_comment[512];
  char latest_stage_audit_decision[32];
  char latest_stage_audit_comment[512];
  int worklog_count;
  int deliverable_count;
  int evidence_count;
} StageSnapshot;

static void json_escape(const char *input, char *output, size_t size) {
  size_t i = 0;

  if (!input || size == 0) {
    if (size > 0) {
      output[0] = '\0';
    }
    return;
  }

  while (*input && i + 2 < size) {
    if (*input == '"' || *input == '\\') {
      output[i++] = '\\';
      output[i++] = *input++;
    } else if (*input == '\n') {
      output[i++] = '\\';
      output[i++] = 'n';
      input++;
    } else if (*input == '\r') {
      input++;
    } else {
      output[i++] = *input++;
    }
  }

  output[i] = '\0';
}

static int json_get_string(const char *json, const char *key, char *output, size_t size) {
  char pattern[80];
  char *start;
  char *cursor;
  size_t i = 0;

  if (!json || !key || !output || size == 0) {
    return 0;
  }

  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  start = strstr((char *) json, pattern);
  if (!start) {
    return 0;
  }

  cursor = strchr(start + (int) strlen(pattern), ':');
  if (!cursor) {
    return 0;
  }

  cursor++;
  while (*cursor && isspace((unsigned char) *cursor)) {
    cursor++;
  }

  if (*cursor != '"') {
    return 0;
  }

  cursor++;
  while (*cursor) {
    if (*cursor == '"' && cursor[-1] != '\\') {
      break;
    }

    if (*cursor == '\\' && cursor[1]) {
      cursor++;
    }

    if (i + 1 < size) {
      output[i++] = *cursor;
    }
    cursor++;
  }

  output[i] = '\0';
  return 1;
}

static int json_get_int(const char *json, const char *key, int *value) {
  char pattern[80];
  char *start;
  char *cursor;

  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  start = strstr((char *) json, pattern);
  if (!start) {
    return 0;
  }

  cursor = strchr(start + (int) strlen(pattern), ':');
  if (!cursor) {
    return 0;
  }

  cursor++;
  while (*cursor && isspace((unsigned char) *cursor)) {
    cursor++;
  }

  *value = (int) strtol(cursor, NULL, 10);
  return 1;
}

static int json_get_double(const char *json, const char *key, double *value) {
  char pattern[80];
  char *start;
  char *cursor;

  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  start = strstr((char *) json, pattern);
  if (!start) {
    return 0;
  }

  cursor = strchr(start + (int) strlen(pattern), ':');
  if (!cursor) {
    return 0;
  }

  cursor++;
  while (*cursor && isspace((unsigned char) *cursor)) {
    cursor++;
  }

  *value = strtod(cursor, NULL);
  return 1;
}

static int path_matches(const char *path, const char *base) {
  size_t base_len = strlen(base);
  return strncmp(path, base, base_len) == 0 && (path[base_len] == '\0' || path[base_len] == '?');
}

static int get_query_param(const char *path, const char *key, char *output, size_t size) {
  const char *query = strchr(path, '?');
  char pattern[64];
  const char *start;
  size_t i = 0;

  if (!query) {
    return 0;
  }

  query++;
  snprintf(pattern, sizeof(pattern), "%s=", key);
  start = strstr(query, pattern);
  if (!start) {
    return 0;
  }

  start += strlen(pattern);
  while (*start && *start != '&' && i + 1 < size) {
    output[i++] = *start++;
  }
  output[i] = '\0';
  return i > 0;
}

static int exec_sql(sqlite3 *db, const char *sql) {
  char *error = NULL;
  int rc = sqlite3_exec(db, sql, NULL, NULL, &error);

  if (rc != SQLITE_OK && error) {
    fprintf(stderr, "SQLite error: %s\n", error);
    sqlite3_free(error);
  }

  return rc;
}

static void exec_sql_ignore(sqlite3 *db, const char *sql) {
  char *error = NULL;
  int rc = sqlite3_exec(db, sql, NULL, NULL, &error);
  if (rc != SQLITE_OK && error) {
    sqlite3_free(error);
  }
}

static int scalar_int(sqlite3 *db, const char *sql) {
  sqlite3_stmt *stmt = NULL;
  int value = 0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
    value = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return value;
}

static int meta_int(sqlite3 *db, const char *key) {
  sqlite3_stmt *stmt = NULL;
  int value = 0;

  if (sqlite3_prepare_v2(db, "SELECT value FROM app_meta WHERE key = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      value = atoi((const char *) sqlite3_column_text(stmt, 0));
    }
  }

  sqlite3_finalize(stmt);
  return value;
}

static void set_meta_int(sqlite3 *db, const char *key, int value) {
  sqlite3_stmt *stmt = NULL;
  char buffer[32];

  snprintf(buffer, sizeof(buffer), "%d", value);
  if (sqlite3_prepare_v2(db, "INSERT INTO app_meta(key, value) VALUES (?, ?) ON CONFLICT(key) DO UPDATE SET value = excluded.value", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, buffer, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);
}

static void make_password_hash(const char *password, const char *salt, char output[65]) {
  char source[256];

  snprintf(source, sizeof(source), "%s:%s", password, salt);
  sha256_hex(source, output);
}

static int verify_password(const char *password, const char *salt, const char *hash) {
  char buffer[65];

  make_password_hash(password, salt, buffer);
  return strcmp(buffer, hash) == 0;
}

static void ensure_directories(void) {
  _mkdir("uploads");
  _mkdir("uploads\\avatars");
}

static void add_audit_log(sqlite3 *db, int actor_id, const char *action, const char *entity_type, int entity_id, const char *detail) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO audit_logs(actor_id, action, entity_type, entity_id, detail) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, actor_id);
    sqlite3_bind_text(stmt, 2, action, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, entity_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, entity_id);
    sqlite3_bind_text(stmt, 5, detail ? detail : "", -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

static void build_avatar_url(const char *avatar_path, char *output, size_t size) {
  if (!avatar_path || !avatar_path[0]) {
    output[0] = '\0';
    return;
  }

  snprintf(output, size, "http://localhost:%d/%s", SERVER_PORT, avatar_path);
}

static int load_user_by_id(sqlite3 *db, int user_id, CurrentUser *user) {
  sqlite3_stmt *stmt = NULL;
  int found = 0;

  memset(user, 0, sizeof(*user));

  if (sqlite3_prepare_v2(db, "SELECT id, username, name, role, status, wallet_address, email, phone, company, bio, avatar_path FROM users WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      user->id = sqlite3_column_int(stmt, 0);
      snprintf(user->username, sizeof(user->username), "%s", (const char *) sqlite3_column_text(stmt, 1));
      snprintf(user->name, sizeof(user->name), "%s", (const char *) sqlite3_column_text(stmt, 2));
      snprintf(user->role, sizeof(user->role), "%s", (const char *) sqlite3_column_text(stmt, 3));
      snprintf(user->status, sizeof(user->status), "%s", (const char *) sqlite3_column_text(stmt, 4));
      snprintf(user->wallet_address, sizeof(user->wallet_address), "%s", (const char *) sqlite3_column_text(stmt, 5));
      snprintf(user->email, sizeof(user->email), "%s", (const char *) sqlite3_column_text(stmt, 6));
      snprintf(user->phone, sizeof(user->phone), "%s", (const char *) sqlite3_column_text(stmt, 7));
      snprintf(user->company, sizeof(user->company), "%s", (const char *) sqlite3_column_text(stmt, 8));
      snprintf(user->bio, sizeof(user->bio), "%s", (const char *) sqlite3_column_text(stmt, 9));
      snprintf(user->avatar_path, sizeof(user->avatar_path), "%s", (const char *) sqlite3_column_text(stmt, 10));
      found = 1;
    }
  }

  sqlite3_finalize(stmt);
  return found;
}

static int current_user_id(const HttpRequest *request) {
  if (strncmp(request->authorization, "Bearer demo-token-", 18) != 0) {
    return 0;
  }

  return atoi(request->authorization + 18);
}

static int is_admin(const CurrentUser *user) {
  return strcmp(user->role, "admin") == 0;
}

static int is_developer(const CurrentUser *user) {
  return strcmp(user->role, "developer") == 0;
}

static int is_client_role(const CurrentUser *user) {
  return strcmp(user->role, "client") == 0;
}

static void send_raw_response(SOCKET client, int status_code, const char *status_text, const char *content_type, const unsigned char *body, int body_length) {
  char header[768];
  int header_length = snprintf(
    header,
    sizeof(header),
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: %s\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
    "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    status_code,
    status_text,
    content_type,
    body_length
  );

  send(client, header, header_length, 0);
  if (body_length > 0) {
    send(client, (const char *) body, body_length, 0);
  }
}

static void send_response(SOCKET client, int status_code, const char *status_text, const char *body) {
  send_raw_response(client, status_code, status_text, "application/json; charset=utf-8", (const unsigned char *) body, (int) strlen(body));
}

static void send_message(SOCKET client, int status_code, const char *status_text, const char *message) {
  char safe[512];
  char body[768];

  json_escape(message, safe, sizeof(safe));
  snprintf(body, sizeof(body), "{\"message\":\"%s\"}", safe);
  send_response(client, status_code, status_text, body);
}

static int ensure_authenticated(AppContext *app, SOCKET client, const HttpRequest *request, CurrentUser *user) {
  int user_id = current_user_id(request);

  if (!user_id || !load_user_by_id(app->db, user_id, user)) {
    send_message(client, 401, "Unauthorized", "未登录或会话已失效");
    return 0;
  }
  if (strcmp(user->status, "active") != 0) {
    send_message(client, 403, "Forbidden", "账号已停用");
    return 0;
  }

  return 1;
}

static const char *content_type_from_extension(const char *path) {
  const char *ext = strrchr(path, '.');

  if (!ext) {
    return "application/octet-stream";
  }

  if (_stricmp(ext, ".png") == 0) {
    return "image/png";
  }
  if (_stricmp(ext, ".jpg") == 0 || _stricmp(ext, ".jpeg") == 0) {
    return "image/jpeg";
  }
  if (_stricmp(ext, ".webp") == 0) {
    return "image/webp";
  }
  if (_stricmp(ext, ".gif") == 0) {
    return "image/gif";
  }

  return "application/octet-stream";
}

static void serve_upload(SOCKET client, const HttpRequest *request) {
  char request_path[512];
  char local_path[512];
  FILE *file = NULL;
  unsigned char *buffer = NULL;
  long length = 0;

  snprintf(request_path, sizeof(request_path), "%s", request->path);
  {
    char *query = strchr(request_path, '?');
    if (query) {
      *query = '\0';
    }
  }

  if (strstr(request_path, "..")) {
    send_message(client, 400, "Bad Request", "非法资源路径");
    return;
  }

  snprintf(local_path, sizeof(local_path), "%s", request_path + 1);
  file = fopen(local_path, "rb");
  if (!file) {
    send_message(client, 404, "Not Found", "资源不存在");
    return;
  }

  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);
  if (length <= 0) {
    fclose(file);
    send_message(client, 404, "Not Found", "资源不存在");
    return;
  }

  buffer = (unsigned char *) malloc((size_t) length);
  if (!buffer) {
    fclose(file);
    send_message(client, 500, "Internal Server Error", "读取资源失败");
    return;
  }

  fread(buffer, 1, (size_t) length, file);
  fclose(file);

  send_raw_response(client, 200, "OK", content_type_from_extension(local_path), buffer, (int) length);
  free(buffer);
}

static int recv_request(SOCKET client, HttpRequest *request) {
  char *buffer = NULL;
  int received = 0;
  int content_length = 0;
  char *headers_end;

  memset(request, 0, sizeof(*request));
  buffer = (char *) malloc(MAX_REQUEST_SIZE);
  if (!buffer) {
    return 0;
  }

  while (received < MAX_REQUEST_SIZE - 1) {
    int rc = recv(client, buffer + received, MAX_REQUEST_SIZE - 1 - received, 0);
    if (rc <= 0) {
      free(buffer);
      return 0;
    }

    received += rc;
    buffer[received] = '\0';
    headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end) {
      char *content_ptr = strstr(buffer, "Content-Length:");
      if (content_ptr) {
        content_length = atoi(content_ptr + 15);
      }
      if (received >= (int) ((headers_end + 4 - buffer) + content_length)) {
        break;
      }
    }
  }

  if (sscanf(buffer, "%7s %511s", request->method, request->path) != 2) {
    free(buffer);
    return 0;
  }

  {
    char *auth = strstr(buffer, "Authorization:");
    if (auth) {
      char *line_end = strstr(auth, "\r\n");
      auth += 14;
      while (*auth == ' ') {
        auth++;
      }
      if (line_end) {
        size_t len = (size_t) (line_end - auth);
        if (len >= sizeof(request->authorization)) {
          len = sizeof(request->authorization) - 1;
        }
        memcpy(request->authorization, auth, len);
        request->authorization[len] = '\0';
      }
    }
  }

  headers_end = strstr(buffer, "\r\n\r\n");
  if (headers_end && content_length > 0) {
    size_t body_len = (size_t) content_length;
    if (body_len >= sizeof(request->body)) {
      body_len = sizeof(request->body) - 1;
    }
    memcpy(request->body, headers_end + 4, body_len);
    request->body[body_len] = '\0';
  }

  free(buffer);
  return 1;
}

static int referenced_project_count(sqlite3 *db, int user_id) {
  sqlite3_stmt *stmt = NULL;
  int count = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM project_members WHERE user_id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
  }

  sqlite3_finalize(stmt);
  return count;
}

static void append_user_json(sqlite3 *db, int user_id, char *body, size_t size, size_t *off) {
  CurrentUser user;
  char name[256], username[128], role[64], status[64], wallet[128], email[128], phone[128], company[256], bio[1024], avatar_url[512], avatar[512];
  int reference_count = 0;

  if (!load_user_by_id(db, user_id, &user)) {
    *off += (size_t) snprintf(body + *off, size - *off, "null");
    return;
  }

  reference_count = referenced_project_count(db, user_id);
  json_escape(user.name, name, sizeof(name));
  json_escape(user.username, username, sizeof(username));
  json_escape(user.role, role, sizeof(role));
  json_escape(user.status, status, sizeof(status));
  json_escape(user.wallet_address, wallet, sizeof(wallet));
  json_escape(user.email, email, sizeof(email));
  json_escape(user.phone, phone, sizeof(phone));
  json_escape(user.company, company, sizeof(company));
  json_escape(user.bio, bio, sizeof(bio));
  build_avatar_url(user.avatar_path, avatar_url, sizeof(avatar_url));
  json_escape(avatar_url, avatar, sizeof(avatar));

  *off += (size_t) snprintf(
    body + *off,
    size - *off,
    "{\"id\":%d,\"username\":\"%s\",\"name\":\"%s\",\"role\":\"%s\",\"status\":\"%s\",\"walletAddress\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\",\"company\":\"%s\",\"bio\":\"%s\",\"avatarUrl\":\"%s\",\"referencedProjectCount\":%d,\"canDelete\":%d}",
    user.id,
    username,
    name,
    role,
    status,
    wallet,
    email,
    phone,
    company,
    bio,
    avatar,
    reference_count,
    reference_count == 0
  );
}

static void send_auth_payload(sqlite3 *db, SOCKET client, int user_id) {
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;

  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"token\":\"demo-token-%d\",\"user\":", user_id);
  append_user_json(db, user_id, body, sizeof(body), &off);
  snprintf(body + off, sizeof(body) - off, "}");
  send_response(client, 200, "OK", body);
}

static const char *quick_login_role_description(const char *role) {
  if (strcmp(role, "admin") == 0) {
    return "管理员演示账号";
  }
  if (strcmp(role, "developer") == 0) {
    return "开发者演示账号";
  }
  if (strcmp(role, "client") == 0) {
    return "客户演示账号";
  }
  return "角色演示账号";
}

static void ensure_schema(sqlite3 *db) {
  const char *schema =
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "username TEXT NOT NULL UNIQUE, "
    "name TEXT NOT NULL, "
    "role TEXT NOT NULL, "
    "status TEXT NOT NULL DEFAULT 'active', "
    "password_hash TEXT NOT NULL, "
    "password_salt TEXT NOT NULL, "
    "quick_login_password TEXT DEFAULT '', "
    "wallet_address TEXT DEFAULT '', "
    "email TEXT DEFAULT '', "
    "phone TEXT DEFAULT '', "
    "company TEXT DEFAULT '', "
    "bio TEXT DEFAULT '', "
    "avatar_path TEXT DEFAULT '', "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP, "
    "updated_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS wallet_bindings ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "user_id INTEGER NOT NULL, "
    "wallet_address TEXT NOT NULL, "
    "chain_id TEXT DEFAULT '', "
    "chain_name TEXT DEFAULT '', "
    "is_primary INTEGER DEFAULT 1, "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS projects ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "name TEXT NOT NULL, "
    "description TEXT DEFAULT '', "
    "status TEXT DEFAULT 'active', "
    "developer_id INTEGER NOT NULL, "
    "client_id INTEGER NOT NULL, "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS project_members ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "project_id INTEGER NOT NULL, "
    "user_id INTEGER NOT NULL, "
    "role TEXT NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS worklogs ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "project_id INTEGER NOT NULL, "
    "user_id INTEGER NOT NULL, "
    "stage_no INTEGER NOT NULL DEFAULT 1, "
    "work_date TEXT NOT NULL, "
    "hours REAL NOT NULL, "
    "task_description TEXT NOT NULL, "
    "digest TEXT NOT NULL, "
    "evidence_status TEXT DEFAULT 'pending', "
    "tx_hash TEXT DEFAULT '', "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS deliverables ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "project_id INTEGER NOT NULL, "
    "user_id INTEGER NOT NULL, "
    "stage_no INTEGER NOT NULL DEFAULT 1, "
    "version TEXT NOT NULL, "
    "file_name TEXT NOT NULL, "
    "summary TEXT DEFAULT '', "
    "file_hash TEXT NOT NULL, "
    "status TEXT DEFAULT 'uploaded', "
    "tx_hash TEXT DEFAULT '', "
    "confirmation_comment TEXT DEFAULT '', "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS evidence_records ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "business_type TEXT NOT NULL, "
    "business_id INTEGER NOT NULL, "
    "tx_hash TEXT NOT NULL, "
    "block_number INTEGER DEFAULT 0, "
    "status TEXT NOT NULL, "
    "comment TEXT DEFAULT '', "
    "actor_id INTEGER DEFAULT 0, "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS approval_records ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "deliverable_id INTEGER NOT NULL, "
    "client_id INTEGER NOT NULL, "
    "approved INTEGER NOT NULL, "
    "comment TEXT DEFAULT '', "
    "tx_hash TEXT DEFAULT '', "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS project_audits ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "project_id INTEGER NOT NULL, "
    "stage_no INTEGER DEFAULT 0, "
    "audit_type TEXT NOT NULL, "
    "decision TEXT NOT NULL, "
    "comment TEXT DEFAULT '', "
    "tx_hash TEXT DEFAULT '', "
    "actor_id INTEGER DEFAULT 0, "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS audit_logs ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "actor_id INTEGER DEFAULT 0, "
    "action TEXT NOT NULL, "
    "entity_type TEXT NOT NULL, "
    "entity_id INTEGER NOT NULL, "
    "detail TEXT DEFAULT '', "
    "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"
    "CREATE TABLE IF NOT EXISTS app_meta ("
    "key TEXT PRIMARY KEY, "
    "value TEXT NOT NULL"
    ");";

  exec_sql(db, schema);
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN email TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN phone TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN company TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN bio TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN avatar_path TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN updated_at TEXT DEFAULT CURRENT_TIMESTAMP");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN status TEXT NOT NULL DEFAULT 'active'");
  exec_sql_ignore(db, "ALTER TABLE users ADD COLUMN quick_login_password TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE wallet_bindings ADD COLUMN chain_id TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE wallet_bindings ADD COLUMN chain_name TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE worklogs ADD COLUMN stage_no INTEGER NOT NULL DEFAULT 1");
  exec_sql_ignore(db, "ALTER TABLE deliverables ADD COLUMN stage_no INTEGER NOT NULL DEFAULT 1");
  exec_sql_ignore(db, "ALTER TABLE deliverables ADD COLUMN confirmation_comment TEXT DEFAULT ''");
  exec_sql_ignore(db, "ALTER TABLE evidence_records ADD COLUMN actor_id INTEGER DEFAULT 0");
}

static int insert_user_account(sqlite3 *db, const char *username, const char *name, const char *role, const char *password, const char *email, const char *phone, const char *company, const char *bio, const char *wallet, const char *status, const char *salt_prefix) {
  sqlite3_stmt *stmt = NULL;
  char salt[64];
  char hash[65];

  snprintf(salt, sizeof(salt), "%s-%s", salt_prefix ? salt_prefix : "user", username);
  make_password_hash(password, salt, hash);

  if (sqlite3_prepare_v2(db, "INSERT INTO users(username, name, role, status, password_hash, password_salt, quick_login_password, wallet_address, email, phone, company, bio, avatar_path, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, '', CURRENT_TIMESTAMP)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, status ? status : "active", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, salt, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, password, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, wallet, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, company, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, bio, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return (int) sqlite3_last_insert_rowid(db);
}

static int insert_seed_user(sqlite3 *db, const char *username, const char *name, const char *role, const char *password, const char *email, const char *phone, const char *company, const char *bio, const char *wallet, const char *status) {
  return insert_user_account(db, username, name, role, password, email, phone, company, bio, wallet, status, "seed");
}

static void backfill_quick_login_password_for_known_user(sqlite3 *db, const char *username, const char *password) {
  sqlite3_stmt *stmt = NULL;
  sqlite3_stmt *update_stmt = NULL;
  int user_id = 0;
  const unsigned char *hash = NULL;
  const unsigned char *salt = NULL;
  const unsigned char *quick_login_password = NULL;

  if (sqlite3_prepare_v2(db, "SELECT id, password_hash, password_salt, quick_login_password FROM users WHERE username = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      quick_login_password = sqlite3_column_text(stmt, 3);
      hash = sqlite3_column_text(stmt, 1);
      salt = sqlite3_column_text(stmt, 2);
      if ((!quick_login_password || !quick_login_password[0]) &&
          hash &&
          salt &&
          verify_password(password, (const char *) salt, (const char *) hash)) {
        user_id = sqlite3_column_int(stmt, 0);
      }
    }
  }
  sqlite3_finalize(stmt);

  if (user_id > 0 && sqlite3_prepare_v2(db, "UPDATE users SET quick_login_password = ? WHERE id = ?", -1, &update_stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(update_stmt, 1, password, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(update_stmt, 2, user_id);
    sqlite3_step(update_stmt);
  }
  sqlite3_finalize(update_stmt);
}

static void backfill_known_quick_login_passwords(sqlite3 *db) {
  backfill_quick_login_password_for_known_user(db, "admin", "admin123");
  backfill_quick_login_password_for_known_user(db, "dev", "dev123");
  backfill_quick_login_password_for_known_user(db, "dev2", "dev123");
  backfill_quick_login_password_for_known_user(db, "client", "client123");
  backfill_quick_login_password_for_known_user(db, "client2", "client123");
}

static void insert_wallet_binding(sqlite3 *db, int user_id, const char *wallet, const char *chain_id, const char *chain_name, int is_primary) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO wallet_bindings(user_id, wallet_address, chain_id, chain_name, is_primary) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, wallet, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, chain_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, chain_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, is_primary);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

static int insert_project(sqlite3 *db, const char *name, const char *description, const char *status, int developer_id, int client_id) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO projects(name, description, status, developer_id, client_id) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, developer_id);
    sqlite3_bind_int(stmt, 5, client_id);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return (int) sqlite3_last_insert_rowid(db);
}

static void insert_project_member(sqlite3 *db, int project_id, int user_id, const char *role) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO project_members(project_id, user_id, role) VALUES (?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, role, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

static void build_worklog_digest(int project_id, int user_id, int stage_no, const char *work_date, double hours, const char *task_description, char output[65]) {
  char canonical[1024];

  snprintf(canonical, sizeof(canonical), "{\"projectId\":%d,\"userId\":%d,\"stageNo\":%d,\"workDate\":\"%s\",\"hours\":%.2f,\"taskDescription\":\"%s\"}", project_id, user_id, stage_no, work_date, hours, task_description);
  sha256_hex(canonical, output);
}

static int insert_worklog(sqlite3 *db, int project_id, int user_id, int stage_no, const char *work_date, double hours, const char *task_description, const char *status, const char *tx_hash) {
  sqlite3_stmt *stmt = NULL;
  char digest[65];

  build_worklog_digest(project_id, user_id, stage_no, work_date, hours, task_description, digest);
  if (sqlite3_prepare_v2(db, "INSERT INTO worklogs(project_id, user_id, stage_no, work_date, hours, task_description, digest, evidence_status, tx_hash) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, stage_no);
    sqlite3_bind_text(stmt, 4, work_date, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, hours);
    sqlite3_bind_text(stmt, 6, task_description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, digest, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, tx_hash, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return (int) sqlite3_last_insert_rowid(db);
}

static int insert_deliverable(sqlite3 *db, int project_id, int user_id, int stage_no, const char *version, const char *file_name, const char *summary, const char *file_hash, const char *status, const char *tx_hash, const char *confirmation_comment) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO deliverables(project_id, user_id, stage_no, version, file_name, summary, file_hash, status, tx_hash, confirmation_comment) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, stage_no);
    sqlite3_bind_text(stmt, 4, version, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, file_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, summary, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, file_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, tx_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, confirmation_comment, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return (int) sqlite3_last_insert_rowid(db);
}

static void insert_evidence(sqlite3 *db, const char *business_type, int business_id, const char *tx_hash, int block_number, const char *status, const char *comment, int actor_id) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO evidence_records(business_type, business_id, tx_hash, block_number, status, comment, actor_id) VALUES (?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, business_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, business_id);
    sqlite3_bind_text(stmt, 3, tx_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, block_number);
    sqlite3_bind_text(stmt, 5, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, comment, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, actor_id);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

static void insert_approval(sqlite3 *db, int deliverable_id, int client_id, int approved, const char *comment, const char *tx_hash) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO approval_records(deliverable_id, client_id, approved, comment, tx_hash) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, deliverable_id);
    sqlite3_bind_int(stmt, 2, client_id);
    sqlite3_bind_int(stmt, 3, approved);
    sqlite3_bind_text(stmt, 4, comment, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, tx_hash, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
}

static int insert_project_audit(sqlite3 *db, int project_id, int stage_no, const char *audit_type, const char *decision, const char *comment, const char *tx_hash, int actor_id) {
  sqlite3_stmt *stmt = NULL;

  if (sqlite3_prepare_v2(db, "INSERT INTO project_audits(project_id, stage_no, audit_type, decision, comment, tx_hash, actor_id) VALUES (?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    sqlite3_bind_text(stmt, 3, audit_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, decision, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, comment ? comment : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, tx_hash ? tx_hash : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, actor_id);
    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return (int) sqlite3_last_insert_rowid(db);
}

static void seed_demo_data(sqlite3 *db) {
  int admin_id;
  int developer_id;
  int developer_b_id;
  int client_id;
  int client_b_id;
  int project_final_ready_id;
  int project_stage_pending_id;
  int project_client_pending_id;
  int worklog_id;
  int deliverable_id;
  int project_audit_id;

  exec_sql(db, "BEGIN TRANSACTION");
  exec_sql(db, "DELETE FROM project_audits");
  exec_sql(db, "DELETE FROM approval_records");
  exec_sql(db, "DELETE FROM evidence_records");
  exec_sql(db, "DELETE FROM deliverables");
  exec_sql(db, "DELETE FROM worklogs");
  exec_sql(db, "DELETE FROM project_members");
  exec_sql(db, "DELETE FROM projects");
  exec_sql(db, "DELETE FROM wallet_bindings");
  exec_sql(db, "DELETE FROM audit_logs");
  exec_sql(db, "DELETE FROM users");
  exec_sql(db, "DELETE FROM sqlite_sequence WHERE name IN ('users','wallet_bindings','projects','project_members','worklogs','deliverables','evidence_records','approval_records','project_audits','audit_logs')");

  admin_id = insert_seed_user(db, "admin", "系统管理员", "admin", "admin123", "admin@trustwork.demo", "13800000001", "Trust Work Lab", "负责创建项目、管理成员和阶段/总审计闭环。", "", "active");
  developer_id = insert_seed_user(db, "dev", "林一帆", "developer", "dev123", "dev@trustwork.demo", "13800000002", "独立开发工作室", "负责可信工时平台项目的多阶段工时登记、交付与存证演示。", "0x4B0897b0513fdc7C541B6d9D7E929C4e5364D2dB", "active");
  developer_b_id = insert_seed_user(db, "dev2", "苏沐宸", "developer", "dev123", "dev2@trustwork.demo", "13800000004", "沐宸数字工坊", "负责第二项目的阶段交付、移动端联调与答辩演示。", "0x8626f6940E2eb28930eFb4CeF49B2d1F2C9C1199", "active");
  client_id = insert_seed_user(db, "client", "周岚", "client", "client123", "client@trustwork.demo", "13800000003", "北辰科技", "负责阶段确认交付并配合管理员完成阶段审计和总审计。", "0x583031D1113aD414F02576BD6afaBfb302140225", "active");
  client_b_id = insert_seed_user(db, "client2", "顾清禾", "client", "client123", "client2@trustwork.demo", "13800000005", "云桥创新", "负责第二项目的阶段确认与反馈。", "0xdD2FD4581271e230360230F9337D5c0430Bf44C0", "active");

  insert_wallet_binding(db, developer_id, "0x4B0897b0513fdc7C541B6d9D7E929C4e5364D2dB", "0xaa36a7", "Sepolia", 1);
  insert_wallet_binding(db, developer_id, "0x4B0897b0513fdc7C541B6d9D7E929C4e5364D2dC", "0x7a69", "Hardhat", 0);
  insert_wallet_binding(db, developer_b_id, "0x8626f6940E2eb28930eFb4CeF49B2d1F2C9C1199", "0xaa36a7", "Sepolia", 1);
  insert_wallet_binding(db, developer_b_id, "0xdD2FD4581271e230360230F9337D5c0430Bf44C1", "0x7a69", "Hardhat", 0);
  insert_wallet_binding(db, client_id, "0x583031D1113aD414F02576BD6afaBfb302140225", "0xaa36a7", "Sepolia", 1);
  insert_wallet_binding(db, client_b_id, "0xdD2FD4581271e230360230F9337D5c0430Bf44C0", "0xaa36a7", "Sepolia", 1);

  project_final_ready_id = insert_project(db, "企业官网 2.0 重构", "阶段 1 与阶段 2 均已完成管理员阶段审计，当前等待项目总审计。", "active", developer_id, client_id);
  project_stage_pending_id = insert_project(db, "可信工时平台答辩演示", "阶段 1 已闭环，阶段 2 已通过客户确认，等待管理员阶段审计。", "active", developer_id, client_id);
  project_client_pending_id = insert_project(db, "智慧园区小程序二期", "阶段 1 已完成存证，等待客户查看工时历史并确认本阶段交付。", "active", developer_b_id, client_b_id);

  insert_project_member(db, project_final_ready_id, developer_id, "developer");
  insert_project_member(db, project_final_ready_id, client_id, "client");
  insert_project_member(db, project_stage_pending_id, developer_id, "developer");
  insert_project_member(db, project_stage_pending_id, client_id, "client");
  insert_project_member(db, project_client_pending_id, developer_b_id, "developer");
  insert_project_member(db, project_client_pending_id, client_b_id, "client");

  worklog_id = insert_worklog(db, project_final_ready_id, developer_id, 1, "2026-04-03", 6.0, "完成官网首页框架与品牌视觉重构。", "confirmed", "0xwl10000000000000000000000000000000000000000000000000000000000001");
  insert_evidence(db, "worklog", worklog_id, "0xwl10000000000000000000000000000000000000000000000000000000000001", 110101, "confirmed", "阶段 1 工时摘要已写入演示链。", developer_id);
  worklog_id = insert_worklog(db, project_final_ready_id, developer_id, 1, "2026-04-04", 4.5, "补齐 CMS 接口联调与首屏交互动画。", "confirmed", "0xwl10000000000000000000000000000000000000000000000000000000000002");
  insert_evidence(db, "worklog", worklog_id, "0xwl10000000000000000000000000000000000000000000000000000000000002", 110102, "confirmed", "阶段 1 第二条工时已写入演示链。", developer_id);
  deliverable_id = insert_deliverable(db, project_final_ready_id, developer_id, 1, "v1.0.0", "homepage-stage1.zip", "官网第一阶段交付，包含首页、品牌页与基础 CMS 接口。", "1111111111111111111111111111111111111111111111111111111111111111", "approved", "0xdl10000000000000000000000000000000000000000000000000000000000001", "客户确认通过，允许进入下一阶段。");
  insert_evidence(db, "deliverable", deliverable_id, "0xdl10000000000000000000000000000000000000000000000000000000000001", 110201, "confirmed", "阶段 1 交付物已完成演示存证。", developer_id);
  insert_approval(db, deliverable_id, client_id, 1, "阶段 1 客户确认通过。", "0xcf10000000000000000000000000000000000000000000000000000000000001");
  insert_evidence(db, "deliverable_confirmation", deliverable_id, "0xcf10000000000000000000000000000000000000000000000000000000000001", 110202, "confirmed", "阶段 1 客户确认通过。", client_id);
  project_audit_id = insert_project_audit(db, project_final_ready_id, 1, "stage", "pass", "阶段 1 审计通过，可进入下一阶段。", "0xsa10000000000000000000000000000000000000000000000000000000000001", admin_id);
  insert_evidence(db, "stage_audit", project_audit_id, "0xsa10000000000000000000000000000000000000000000000000000000000001", 110203, "confirmed", "阶段 1 审计通过。", admin_id);

  worklog_id = insert_worklog(db, project_final_ready_id, developer_id, 2, "2026-04-08", 5.5, "完成官网活动页、报名表单与投放埋点。", "confirmed", "0xwl10000000000000000000000000000000000000000000000000000000000003");
  insert_evidence(db, "worklog", worklog_id, "0xwl10000000000000000000000000000000000000000000000000000000000003", 110301, "confirmed", "阶段 2 工时摘要已写入演示链。", developer_id);
  deliverable_id = insert_deliverable(db, project_final_ready_id, developer_id, 2, "v1.1.0", "homepage-stage2.zip", "官网第二阶段交付，补齐活动页、表单与投放分析。", "2222222222222222222222222222222222222222222222222222222222222222", "approved", "0xdl10000000000000000000000000000000000000000000000000000000000002", "客户确认通过，等待总审计。");
  insert_evidence(db, "deliverable", deliverable_id, "0xdl10000000000000000000000000000000000000000000000000000000000002", 110302, "confirmed", "阶段 2 交付物已完成演示存证。", developer_id);
  insert_approval(db, deliverable_id, client_id, 1, "阶段 2 客户确认通过。", "0xcf10000000000000000000000000000000000000000000000000000000000002");
  insert_evidence(db, "deliverable_confirmation", deliverable_id, "0xcf10000000000000000000000000000000000000000000000000000000000002", 110303, "confirmed", "阶段 2 客户确认通过。", client_id);
  project_audit_id = insert_project_audit(db, project_final_ready_id, 2, "stage", "pass", "阶段 2 审计通过，项目可进入总审计。", "0xsa10000000000000000000000000000000000000000000000000000000000002", admin_id);
  insert_evidence(db, "stage_audit", project_audit_id, "0xsa10000000000000000000000000000000000000000000000000000000000002", 110304, "confirmed", "阶段 2 审计通过。", admin_id);

  worklog_id = insert_worklog(db, project_stage_pending_id, developer_id, 1, "2026-04-09", 4.5, "完成平台首页统计模块与登录态修正。", "confirmed", "0xwl20000000000000000000000000000000000000000000000000000000000001");
  insert_evidence(db, "worklog", worklog_id, "0xwl20000000000000000000000000000000000000000000000000000000000001", 120101, "confirmed", "答辩项目阶段 1 工时已上链。", developer_id);
  deliverable_id = insert_deliverable(db, project_stage_pending_id, developer_id, 1, "v0.8.0", "trust-work-stage1.zip", "阶段 1 交付，完成仪表盘、路由与基础接口联通。", "3333333333333333333333333333333333333333333333333333333333333333", "approved", "0xdl20000000000000000000000000000000000000000000000000000000000001", "阶段 1 确认通过。");
  insert_evidence(db, "deliverable", deliverable_id, "0xdl20000000000000000000000000000000000000000000000000000000000001", 120102, "confirmed", "阶段 1 交付已存证。", developer_id);
  insert_approval(db, deliverable_id, client_id, 1, "阶段 1 客户确认通过。", "0xcf20000000000000000000000000000000000000000000000000000000000001");
  insert_evidence(db, "deliverable_confirmation", deliverable_id, "0xcf20000000000000000000000000000000000000000000000000000000000001", 120103, "confirmed", "阶段 1 客户确认通过。", client_id);
  project_audit_id = insert_project_audit(db, project_stage_pending_id, 1, "stage", "pass", "阶段 1 管理员审计通过。", "0xsa20000000000000000000000000000000000000000000000000000000000001", admin_id);
  insert_evidence(db, "stage_audit", project_audit_id, "0xsa20000000000000000000000000000000000000000000000000000000000001", 120104, "confirmed", "阶段 1 审计通过。", admin_id);

  worklog_id = insert_worklog(db, project_stage_pending_id, developer_id, 2, "2026-04-12", 5.0, "补齐个人中心、顶部壳层与成员选择器体验。", "confirmed", "0xwl20000000000000000000000000000000000000000000000000000000000002");
  insert_evidence(db, "worklog", worklog_id, "0xwl20000000000000000000000000000000000000000000000000000000000002", 120201, "confirmed", "阶段 2 工时已上链。", developer_id);
  worklog_id = insert_worklog(db, project_stage_pending_id, developer_id, 2, "2026-04-13", 3.5, "修正流程总览与审计页交互细节。", "pending", "");
  deliverable_id = insert_deliverable(db, project_stage_pending_id, developer_id, 2, "v1.0.0-beta", "trust-work-stage2.zip", "阶段 2 交付，个人中心、顶部壳层和表单选择器已完善。", "4444444444444444444444444444444444444444444444444444444444444444", "approved", "0xdl20000000000000000000000000000000000000000000000000000000000002", "客户确认通过，等待管理员阶段审计。");
  insert_evidence(db, "deliverable", deliverable_id, "0xdl20000000000000000000000000000000000000000000000000000000000002", 120202, "confirmed", "阶段 2 交付已存证。", developer_id);
  insert_approval(db, deliverable_id, client_id, 1, "阶段 2 客户确认通过，等待管理员阶段审计。", "0xcf20000000000000000000000000000000000000000000000000000000000002");
  insert_evidence(db, "deliverable_confirmation", deliverable_id, "0xcf20000000000000000000000000000000000000000000000000000000000002", 120203, "confirmed", "阶段 2 客户确认通过。", client_id);

  worklog_id = insert_worklog(db, project_client_pending_id, developer_b_id, 1, "2026-04-14", 6.5, "完成智慧园区预约流转、消息提醒与移动端适配。", "confirmed", "0xwl30000000000000000000000000000000000000000000000000000000000001");
  insert_evidence(db, "worklog", worklog_id, "0xwl30000000000000000000000000000000000000000000000000000000000001", 130101, "confirmed", "智慧园区项目阶段 1 工时已存证。", developer_b_id);
  worklog_id = insert_worklog(db, project_client_pending_id, developer_b_id, 1, "2026-04-15", 3.0, "补齐客户确认页文案与阶段展示逻辑。", "pending", "");
  deliverable_id = insert_deliverable(db, project_client_pending_id, developer_b_id, 1, "v0.9.2", "smart-park-stage1.zip", "阶段 1 交付，预约流转、消息提醒和客户确认页已可演示。", "5555555555555555555555555555555555555555555555555555555555555555", "notarized", "0xdl30000000000000000000000000000000000000000000000000000000000001", "");
  insert_evidence(db, "deliverable", deliverable_id, "0xdl30000000000000000000000000000000000000000000000000000000000001", 130102, "confirmed", "智慧园区项目阶段 1 交付已完成演示存证。", developer_b_id);

  add_audit_log(db, admin_id, "seed_reset", "app_meta", DEMO_SEED_VERSION, "重建阶段循环式答辩演示数据。");
  add_audit_log(db, admin_id, "create_project", "project", project_final_ready_id, "企业官网 2.0 重构");
  add_audit_log(db, admin_id, "create_project", "project", project_stage_pending_id, "可信工时平台答辩演示");
  add_audit_log(db, admin_id, "create_project", "project", project_client_pending_id, "智慧园区小程序二期");
  add_audit_log(db, developer_id, "bind_wallet", "wallet_binding", developer_id, "连接主开发者钱包");
  add_audit_log(db, developer_b_id, "bind_wallet", "wallet_binding", developer_b_id, "连接第二开发者钱包");
  add_audit_log(db, client_id, "approve_deliverable", "deliverable", project_stage_pending_id, "阶段确认通过");
  add_audit_log(db, admin_id, "stage_audit_pass", "project_stage", project_final_ready_id, "历史阶段审计通过");

  set_meta_int(db, "seed_version", DEMO_SEED_VERSION);
  exec_sql(db, "COMMIT");
}

static int init_database(sqlite3 **db) {
  if (sqlite3_open(DB_PATH, db) != SQLITE_OK) {
    return 0;
  }

  ensure_directories();
  ensure_schema(*db);
  backfill_known_quick_login_passwords(*db);

  if (meta_int(*db, "seed_version") != DEMO_SEED_VERSION ||
      scalar_int(*db, "SELECT COUNT(*) FROM users") < 3 ||
      scalar_int(*db, "SELECT COUNT(*) FROM projects") < 2) {
    seed_demo_data(*db);
  }

  return 1;
}

static int user_can_access_project(sqlite3 *db, int user_id, int project_id) {
  sqlite3_stmt *stmt = NULL;
  int allowed = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM project_members WHERE project_id = ? AND user_id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      allowed = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return allowed;
}

static int developer_owns_worklog(sqlite3 *db, int user_id, int worklog_id) {
  sqlite3_stmt *stmt = NULL;
  int allowed = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM worklogs WHERE id = ? AND user_id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, worklog_id);
    sqlite3_bind_int(stmt, 2, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      allowed = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return allowed;
}

static int client_owns_deliverable(sqlite3 *db, int user_id, int deliverable_id) {
  sqlite3_stmt *stmt = NULL;
  int allowed = 0;

  if (sqlite3_prepare_v2(db,
        "SELECT COUNT(*) FROM deliverables d "
        "JOIN projects p ON p.id = d.project_id "
        "WHERE d.id = ? AND p.client_id = ?",
        -1,
        &stmt,
        NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, deliverable_id);
    sqlite3_bind_int(stmt, 2, user_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      allowed = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return allowed;
}

static int project_exists(sqlite3 *db, int project_id) {
  sqlite3_stmt *stmt = NULL;
  int exists = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM projects WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      exists = sqlite3_column_int(stmt, 0) > 0;
    }
  }
  sqlite3_finalize(stmt);
  return exists;
}

static int user_matches_role_and_status(sqlite3 *db, int user_id, const char *role, const char *status) {
  sqlite3_stmt *stmt = NULL;
  int matched = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE id = ? AND role = ? AND status = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, role, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, status, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      matched = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return matched;
}

static int project_visible_to_user(sqlite3 *db, const CurrentUser *user, int project_id) {
  if (is_admin(user)) {
    return project_exists(db, project_id);
  }
  return user_can_access_project(db, user->id, project_id);
}

static int project_max_stage_no(sqlite3 *db, int project_id) {
  sqlite3_stmt *stmt = NULL;
  int max_stage = 0;

  if (sqlite3_prepare_v2(
        db,
        "SELECT COALESCE(MAX(stage_no), 0) FROM ("
        "SELECT stage_no FROM worklogs WHERE project_id = ? "
        "UNION ALL "
        "SELECT stage_no FROM deliverables WHERE project_id = ? "
        "UNION ALL "
        "SELECT stage_no FROM project_audits WHERE project_id = ? AND audit_type = 'stage'"
        ")",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, project_id);
    sqlite3_bind_int(stmt, 3, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      max_stage = sqlite3_column_int(stmt, 0);
    }
  }

  sqlite3_finalize(stmt);
  return max_stage;
}

static void load_stage_snapshot(sqlite3 *db, int project_id, int stage_no, StageSnapshot *snapshot) {
  sqlite3_stmt *stmt = NULL;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->stage_no = stage_no;
  snprintf(snapshot->state, sizeof(snapshot->state), "open");

  if (sqlite3_prepare_v2(
        db,
        "SELECT COUNT(*), COALESCE(MAX(work_date), '') FROM worklogs WHERE project_id = ? AND stage_no = ?",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      snapshot->worklog_count = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(
        db,
        "SELECT COUNT(*) FROM deliverables WHERE project_id = ? AND stage_no = ?",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      snapshot->deliverable_count = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(
        db,
        "SELECT COALESCE(d.status, ''), COALESCE(d.version, ''), COALESCE(d.file_name, ''), COALESCE(d.confirmation_comment, '') "
        "FROM deliverables d WHERE d.project_id = ? AND d.stage_no = ? ORDER BY d.id DESC LIMIT 1",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      snprintf(snapshot->latest_deliverable_status, sizeof(snapshot->latest_deliverable_status), "%s", (const char *) sqlite3_column_text(stmt, 0));
      snprintf(snapshot->latest_deliverable_version, sizeof(snapshot->latest_deliverable_version), "%s", (const char *) sqlite3_column_text(stmt, 1));
      snprintf(snapshot->latest_deliverable_name, sizeof(snapshot->latest_deliverable_name), "%s", (const char *) sqlite3_column_text(stmt, 2));
      snprintf(snapshot->latest_approval_comment, sizeof(snapshot->latest_approval_comment), "%s", (const char *) sqlite3_column_text(stmt, 3));
    }
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(
        db,
        "SELECT COALESCE(decision, ''), COALESCE(comment, '') FROM project_audits "
        "WHERE project_id = ? AND stage_no = ? AND audit_type = 'stage' ORDER BY id DESC LIMIT 1",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      snprintf(snapshot->latest_stage_audit_decision, sizeof(snapshot->latest_stage_audit_decision), "%s", (const char *) sqlite3_column_text(stmt, 0));
      snprintf(snapshot->latest_stage_audit_comment, sizeof(snapshot->latest_stage_audit_comment), "%s", (const char *) sqlite3_column_text(stmt, 1));
    }
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(
        db,
        "SELECT COUNT(*) FROM evidence_records e "
        "WHERE (e.business_type = 'worklog' AND e.business_id IN (SELECT id FROM worklogs WHERE project_id = ? AND stage_no = ?)) "
        "OR (e.business_type IN ('deliverable', 'deliverable_confirmation') AND e.business_id IN (SELECT id FROM deliverables WHERE project_id = ? AND stage_no = ?)) "
        "OR (e.business_type = 'stage_audit' AND e.business_id IN (SELECT id FROM project_audits WHERE project_id = ? AND stage_no = ? AND audit_type = 'stage'))",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    sqlite3_bind_int(stmt, 3, project_id);
    sqlite3_bind_int(stmt, 4, stage_no);
    sqlite3_bind_int(stmt, 5, project_id);
    sqlite3_bind_int(stmt, 6, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      snapshot->evidence_count = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);

  if (strcmp(snapshot->latest_stage_audit_decision, "pass") == 0) {
    snprintf(snapshot->state, sizeof(snapshot->state), "closed");
  } else if (strcmp(snapshot->latest_deliverable_status, "approved") == 0 && strcmp(snapshot->latest_stage_audit_decision, "reject") != 0) {
    snprintf(snapshot->state, sizeof(snapshot->state), "pending_stage_audit");
  } else {
    snprintf(snapshot->state, sizeof(snapshot->state), "open");
  }
}

static int project_current_stage_for_write(sqlite3 *db, int project_id, int *blocked_by_stage_audit) {
  int max_stage = project_max_stage_no(db, project_id);
  StageSnapshot snapshot;

  if (blocked_by_stage_audit) {
    *blocked_by_stage_audit = 0;
  }
  if (max_stage <= 0) {
    return 1;
  }

  load_stage_snapshot(db, project_id, max_stage, &snapshot);
  if (strcmp(snapshot.state, "closed") == 0) {
    return max_stage + 1;
  }
  if (strcmp(snapshot.state, "pending_stage_audit") == 0) {
    if (blocked_by_stage_audit) {
      *blocked_by_stage_audit = 1;
    }
    return max_stage;
  }
  return max_stage;
}

static int project_has_passed_stage(sqlite3 *db, int project_id) {
  sqlite3_stmt *stmt = NULL;
  int found = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM project_audits WHERE project_id = ? AND audit_type = 'stage' AND decision = 'pass'", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      found = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return found;
}

static int project_pending_stage_audit_count(sqlite3 *db, int project_id) {
  int max_stage = project_max_stage_no(db, project_id);
  StageSnapshot snapshot;

  if (max_stage <= 0) {
    return 0;
  }
  load_stage_snapshot(db, project_id, max_stage, &snapshot);
  return strcmp(snapshot.state, "pending_stage_audit") == 0 ? 1 : 0;
}

static int project_pending_client_stage_count(sqlite3 *db, int project_id) {
  int max_stage = project_max_stage_no(db, project_id);
  StageSnapshot snapshot;

  if (max_stage <= 0) {
    return 0;
  }
  load_stage_snapshot(db, project_id, max_stage, &snapshot);
  if (strcmp(snapshot.state, "open") != 0) {
    return 0;
  }
  if (strcmp(snapshot.latest_deliverable_status, "uploaded") == 0 || strcmp(snapshot.latest_deliverable_status, "notarized") == 0) {
    return 1;
  }
  return 0;
}

static int project_can_run_final_audit(sqlite3 *db, const char *project_status, int project_id) {
  int max_stage = project_max_stage_no(db, project_id);
  StageSnapshot snapshot;

  if (!project_status || strcmp(project_status, "active") != 0 || max_stage <= 0 || !project_has_passed_stage(db, project_id)) {
    return 0;
  }

  load_stage_snapshot(db, project_id, max_stage, &snapshot);
  return strcmp(snapshot.state, "closed") == 0;
}

static void build_final_audit_hint(sqlite3 *db, const char *project_status, int project_id, char *output, size_t size) {
  int max_stage = project_max_stage_no(db, project_id);
  StageSnapshot snapshot;

  if (!output || size == 0) {
    return;
  }

  if (project_status && strcmp(project_status, "completed") == 0) {
    snprintf(output, size, "该项目已完成最终审计归档");
    return;
  }
  if (project_status && strcmp(project_status, "disputed") == 0) {
    snprintf(output, size, "最终审计未通过，项目已进入争议处理");
    return;
  }
  if (max_stage <= 0) {
    snprintf(output, size, "项目尚未进入任何阶段");
    return;
  }

  load_stage_snapshot(db, project_id, max_stage, &snapshot);
  if (strcmp(snapshot.state, "pending_stage_audit") == 0) {
    snprintf(output, size, "阶段 %d 已通过客户确认，等待管理员阶段审计", snapshot.stage_no);
    return;
  }
  if (project_pending_client_stage_count(db, project_id) > 0) {
    snprintf(output, size, "阶段 %d 仍等待客户确认", snapshot.stage_no);
    return;
  }
  if (strcmp(snapshot.state, "closed") == 0 && project_has_passed_stage(db, project_id)) {
    snprintf(output, size, "所有阶段已关闭，可执行最终总审计");
    return;
  }
  snprintf(output, size, "当前仍有开放阶段，尚未满足最终总审计条件");
}

static int latest_deliverable_id_for_stage(sqlite3 *db, int project_id, int stage_no) {
  sqlite3_stmt *stmt = NULL;
  int deliverable_id = 0;

  if (sqlite3_prepare_v2(db, "SELECT id FROM deliverables WHERE project_id = ? AND stage_no = ? ORDER BY id DESC LIMIT 1", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, stage_no);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      deliverable_id = sqlite3_column_int(stmt, 0);
    }
  }

  sqlite3_finalize(stmt);
  return deliverable_id;
}

static int passed_stage_count(sqlite3 *db, int project_id) {
  sqlite3_stmt *stmt = NULL;
  int count = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(DISTINCT stage_no) FROM project_audits WHERE project_id = ? AND audit_type = 'stage' AND decision = 'pass'", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
  }

  sqlite3_finalize(stmt);
  return count;
}

static int select_default_project_id(sqlite3 *db, const CurrentUser *user) {
  sqlite3_stmt *stmt = NULL;
  int project_id = 0;

  if (is_admin(user)) {
    sqlite3_prepare_v2(db, "SELECT id FROM projects ORDER BY id DESC LIMIT 1", -1, &stmt, NULL);
  } else {
    sqlite3_prepare_v2(
      db,
      "SELECT p.id FROM projects p "
      "JOIN project_members pm ON pm.project_id = p.id "
      "WHERE pm.user_id = ? "
      "GROUP BY p.id "
      "ORDER BY p.id DESC LIMIT 1",
      -1,
      &stmt,
      NULL
    );
    sqlite3_bind_int(stmt, 1, user->id);
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    project_id = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return project_id;
}

static void append_global_process_step(
  char *body,
  size_t size,
  size_t *off,
  int first,
  const char *key,
  const char *title,
  const char *description,
  const char *actor,
  const char *state,
  const char *detail,
  const char *gate,
  int is_current
) {
  char safe_key[64], safe_title[128], safe_description[512], safe_actor[64], safe_state[32], safe_detail[512], safe_gate[256];
  json_escape(key, safe_key, sizeof(safe_key));
  json_escape(title, safe_title, sizeof(safe_title));
  json_escape(description, safe_description, sizeof(safe_description));
  json_escape(actor, safe_actor, sizeof(safe_actor));
  json_escape(state, safe_state, sizeof(safe_state));
  json_escape(detail, safe_detail, sizeof(safe_detail));
  json_escape(gate, safe_gate, sizeof(safe_gate));

  *off += (size_t) snprintf(
    body + *off,
    size - *off,
    "%s{\"key\":\"%s\",\"title\":\"%s\",\"description\":\"%s\",\"actor\":\"%s\",\"state\":\"%s\",\"detail\":\"%s\",\"gate\":\"%s\",\"isCurrent\":%d}",
    first ? "" : ",",
    safe_key,
    safe_title,
    safe_description,
    safe_actor,
    safe_state,
    safe_detail,
    safe_gate,
    is_current
  );
}

static void append_global_overview_lane(
  char *body,
  size_t size,
  size_t *off,
  int first,
  const char *role,
  const char *title,
  const char *summary
) {
  char safe_role[32], safe_title[64], safe_summary[256];

  json_escape(role, safe_role, sizeof(safe_role));
  json_escape(title, safe_title, sizeof(safe_title));
  json_escape(summary, safe_summary, sizeof(safe_summary));

  *off += (size_t) snprintf(
    body + *off,
    size - *off,
    "%s{\"role\":\"%s\",\"title\":\"%s\",\"summary\":\"%s\"}",
    first ? "" : ",",
    safe_role,
    safe_title,
    safe_summary
  );
}

static void append_global_overview_node(
  char *body,
  size_t size,
  size_t *off,
  int first,
  const char *key,
  const char *title,
  const char *actor,
  const char *kind,
  const char *tone,
  int is_current_focus,
  const char *summary,
  const char *detail
) {
  char safe_key[64], safe_title[128], safe_actor[32], safe_kind[32], safe_tone[32], safe_summary[256], safe_detail[512];

  json_escape(key, safe_key, sizeof(safe_key));
  json_escape(title, safe_title, sizeof(safe_title));
  json_escape(actor, safe_actor, sizeof(safe_actor));
  json_escape(kind, safe_kind, sizeof(safe_kind));
  json_escape(tone, safe_tone, sizeof(safe_tone));
  json_escape(summary, safe_summary, sizeof(safe_summary));
  json_escape(detail, safe_detail, sizeof(safe_detail));

  *off += (size_t) snprintf(
    body + *off,
    size - *off,
    "%s{\"key\":\"%s\",\"title\":\"%s\",\"actor\":\"%s\",\"kind\":\"%s\",\"tone\":\"%s\",\"isCurrentFocus\":%d,\"summary\":\"%s\",\"detail\":\"%s\"}",
    first ? "" : ",",
    safe_key,
    safe_title,
    safe_actor,
    safe_kind,
    safe_tone,
    is_current_focus,
    safe_summary,
    safe_detail
  );
}

static void append_global_overview_edge(
  char *body,
  size_t size,
  size_t *off,
  int first,
  const char *from,
  const char *to,
  const char *kind,
  const char *tone
) {
  char safe_from[64], safe_to[64], safe_kind[32], safe_tone[32];

  json_escape(from, safe_from, sizeof(safe_from));
  json_escape(to, safe_to, sizeof(safe_to));
  json_escape(kind, safe_kind, sizeof(safe_kind));
  json_escape(tone, safe_tone, sizeof(safe_tone));

  *off += (size_t) snprintf(
    body + *off,
    size - *off,
    "%s{\"from\":\"%s\",\"to\":\"%s\",\"kind\":\"%s\",\"tone\":\"%s\"}",
    first ? "" : ",",
    safe_from,
    safe_to,
    safe_kind,
    safe_tone
  );
}

static int stage_waiting_client_confirmation(const StageSnapshot *snapshot) {
  if (!snapshot) {
    return 0;
  }
  if (strcmp(snapshot->state, "open") != 0) {
    return 0;
  }
  return strcmp(snapshot->latest_deliverable_status, "uploaded") == 0 ||
         strcmp(snapshot->latest_deliverable_status, "notarized") == 0;
}

static int stage_needs_developer_revise(const StageSnapshot *snapshot) {
  if (!snapshot) {
    return 0;
  }
  if (strcmp(snapshot->state, "open") != 0) {
    return 0;
  }
  return strcmp(snapshot->latest_deliverable_status, "rejected") == 0 ||
         strcmp(snapshot->latest_stage_audit_decision, "reject") == 0;
}

static const char *stage_phase_label(const StageSnapshot *snapshot) {
  if (!snapshot) {
    return "尚未进入阶段";
  }
  if (strcmp(snapshot->state, "closed") == 0) {
    return "阶段闭环完成";
  }
  if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    return "等待阶段审计";
  }
  if (stage_waiting_client_confirmation(snapshot)) {
    return "等待客户确认";
  }
  return "开发登记中";
}

static const char *stage_current_owner(const StageSnapshot *snapshot) {
  if (!snapshot) {
    return "none";
  }
  if (strcmp(snapshot->state, "closed") == 0) {
    return "completed";
  }
  if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    return "admin";
  }
  if (stage_waiting_client_confirmation(snapshot)) {
    return "client";
  }
  return "developer";
}

static void build_stage_requirement_text(const StageSnapshot *snapshot, char *output, size_t size) {
  if (!output || size == 0) {
    return;
  }
  if (!snapshot) {
    snprintf(output, size, "项目尚未进入阶段循环。");
    return;
  }
  if (strcmp(snapshot->state, "closed") == 0) {
    snprintf(output, size, "本阶段已完成开发登记、客户确认与管理员阶段审计，当前只保留回溯查看能力。");
    return;
  }
  if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    snprintf(output, size, "客户已完成阶段确认，管理员通过后才能进入下一阶段。");
    return;
  }
  if (stage_waiting_client_confirmation(snapshot)) {
    snprintf(output, size, "客户必须先确认当前阶段交付，管理员阶段审计才会解锁。");
    return;
  }
  snprintf(output, size, "开发者需继续累计工时、交付物与证据，形成可确认的阶段交付。");
}

static void build_stage_next_action_text(const StageSnapshot *snapshot, int stage_no, char *output, size_t size) {
  if (!output || size == 0) {
    return;
  }
  if (!snapshot) {
    snprintf(output, size, "等待项目进入首个阶段。");
    return;
  }
  if (strcmp(snapshot->state, "closed") == 0) {
    snprintf(output, size, "如继续推进，下一次登记会自动进入新的阶段。");
    return;
  }
  if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    snprintf(output, size, "管理员执行阶段 %d 审计，并决定通过或驳回。", stage_no);
    return;
  }
  if (stage_waiting_client_confirmation(snapshot)) {
    snprintf(output, size, "客户查看阶段 %d 的交付与工时历史后，给出通过或驳回结论。", stage_no);
    return;
  }
  snprintf(output, size, "开发者继续补充阶段 %d 的工时、交付物，并在需要时发起存证。", stage_no);
}

static void build_stage_blocking_reason_text(const StageSnapshot *snapshot, int stage_no, char *output, size_t size) {
  if (!output || size == 0) {
    return;
  }
  if (!snapshot || strcmp(snapshot->state, "closed") == 0) {
    output[0] = '\0';
    return;
  }
  if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    snprintf(output, size, "阶段 %d 正等待管理员阶段审计。", stage_no);
    return;
  }
  if (stage_waiting_client_confirmation(snapshot)) {
    snprintf(output, size, "阶段 %d 正等待客户确认。", stage_no);
    return;
  }
  snprintf(output, size, "阶段 %d 仍由开发者持续登记与修订。", stage_no);
}

static void build_stage_role_state(
  const StageSnapshot *snapshot,
  const char *role,
  char *status,
  size_t status_size,
  char *summary,
  size_t summary_size
) {
  int waiting_client = stage_waiting_client_confirmation(snapshot);

  if (strcmp(role, "developer") == 0) {
    if (strcmp(snapshot->state, "closed") == 0) {
      snprintf(status, status_size, "completed");
      snprintf(summary, summary_size, "本阶段开发材料已闭环归档。");
    } else if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
      snprintf(status, status_size, "completed");
      snprintf(summary, summary_size, "开发者已完成本阶段交付，等待管理员阶段审计。");
    } else if (waiting_client) {
      snprintf(status, status_size, "completed");
      snprintf(summary, summary_size, "开发者已完成交付准备，当前等待客户确认。");
    } else {
      snprintf(status, status_size, "running");
      snprintf(summary, summary_size, "开发者仍在持续登记工时、交付物与证据。");
    }
    return;
  }

  if (strcmp(role, "client") == 0) {
    if (strcmp(snapshot->state, "closed") == 0 || strcmp(snapshot->state, "pending_stage_audit") == 0) {
      snprintf(status, status_size, "completed");
      snprintf(summary, summary_size, "客户已完成本阶段确认。");
    } else if (waiting_client) {
      snprintf(status, status_size, "running");
      snprintf(summary, summary_size, "客户当前需要查看阶段交付与工时历史。");
    } else {
      snprintf(status, status_size, "pending");
      snprintf(summary, summary_size, "等待开发者形成可确认交付后再进入客户确认。");
    }
    return;
  }

  if (strcmp(snapshot->state, "closed") == 0) {
    snprintf(status, status_size, "completed");
    snprintf(summary, summary_size, "管理员已完成本阶段审计。");
  } else if (strcmp(snapshot->state, "pending_stage_audit") == 0) {
    snprintf(status, status_size, "running");
    snprintf(summary, summary_size, "管理员当前需要对本阶段执行通过或驳回审计。");
  } else {
    snprintf(status, status_size, "locked");
    snprintf(summary, summary_size, "客户确认通过后，管理员阶段审计才会解锁。");
  }
}

static void append_stage_role_states(char *body, size_t size, size_t *off, const StageSnapshot *snapshot) {
  static const char *role_keys[3] = { "developer", "client", "admin" };
  static const char *role_titles[3] = { "开发者", "客户", "管理员" };
  int index;

  *off += (size_t) snprintf(body + *off, size - *off, "[");
  for (index = 0; index < 3; index++) {
    char status[32], summary[256], safe_status[32], safe_summary[512], safe_role[32], safe_title[64];

    build_stage_role_state(snapshot, role_keys[index], status, sizeof(status), summary, sizeof(summary));
    json_escape(role_keys[index], safe_role, sizeof(safe_role));
    json_escape(role_titles[index], safe_title, sizeof(safe_title));
    json_escape(status, safe_status, sizeof(safe_status));
    json_escape(summary, safe_summary, sizeof(safe_summary));

    *off += (size_t) snprintf(
      body + *off,
      size - *off,
      "%s{\"role\":\"%s\",\"title\":\"%s\",\"status\":\"%s\",\"summary\":\"%s\"}",
      index == 0 ? "" : ",",
      safe_role,
      safe_title,
      safe_status,
      safe_summary
    );
  }
  *off += (size_t) snprintf(body + *off, size - *off, "]");
}

static void handle_auth_quick_accounts(AppContext *app, SOCKET client) {
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"items\":[");
  if (sqlite3_prepare_v2(
        app->db,
        "SELECT id, role, username, name, quick_login_password, bio, company "
        "FROM users "
        "WHERE status = 'active' AND role IN ('admin','developer','client') AND COALESCE(quick_login_password, '') != '' "
        "ORDER BY CASE role WHEN 'admin' THEN 0 WHEN 'developer' THEN 1 WHEN 'client' THEN 2 ELSE 3 END, id ASC",
        -1,
        &stmt,
        NULL
      ) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *role = (const char *) sqlite3_column_text(stmt, 1);
      const char *username = (const char *) sqlite3_column_text(stmt, 2);
      const char *name = (const char *) sqlite3_column_text(stmt, 3);
      const char *password = (const char *) sqlite3_column_text(stmt, 4);
      const char *bio = (const char *) sqlite3_column_text(stmt, 5);
      const char *company = (const char *) sqlite3_column_text(stmt, 6);
      const char *display_name = (name && name[0]) ? name : username;
      const char *description = (bio && bio[0]) ? bio : ((company && company[0]) ? company : quick_login_role_description(role ? role : ""));
      char safe_role[64];
      char safe_display_name[256];
      char safe_username[128];
      char safe_password[128];
      char safe_description[1024];

      json_escape(role ? role : "", safe_role, sizeof(safe_role));
      json_escape(display_name ? display_name : "", safe_display_name, sizeof(safe_display_name));
      json_escape(username ? username : "", safe_username, sizeof(safe_username));
      json_escape(password ? password : "", safe_password, sizeof(safe_password));
      json_escape(description ? description : "", safe_description, sizeof(safe_description));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"id\":%d,\"role\":\"%s\",\"displayName\":\"%s\",\"username\":\"%s\",\"password\":\"%s\",\"description\":\"%s\"}",
        first ? "" : ",",
        sqlite3_column_int(stmt, 0),
        safe_role,
        safe_display_name,
        safe_username,
        safe_password,
        safe_description
      );
      first = 0;
    }
  }
  sqlite3_finalize(stmt);

  snprintf(body + off, sizeof(body) - off, "]}");
  send_response(client, 200, "OK", body);
}

static void handle_auth_login(AppContext *app, SOCKET client, const HttpRequest *request) {
  char username[64];
  char password[64];
  sqlite3_stmt *stmt = NULL;

  if (!json_get_string(request->body, "username", username, sizeof(username)) ||
      !json_get_string(request->body, "password", password, sizeof(password))) {
    send_message(client, 400, "Bad Request", "请输入用户名和密码");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "SELECT id, password_hash, password_salt, status FROM users WHERE username = ?", -1, &stmt, NULL) != SQLITE_OK) {
    send_message(client, 500, "Internal Server Error", "登录失败");
    return;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    send_message(client, 401, "Unauthorized", "账号不存在");
    return;
  }

  if (!verify_password(password, (const char *) sqlite3_column_text(stmt, 2), (const char *) sqlite3_column_text(stmt, 1))) {
    sqlite3_finalize(stmt);
    send_message(client, 401, "Unauthorized", "密码错误");
    return;
  }
  if (strcmp((const char *) sqlite3_column_text(stmt, 3), "active") != 0) {
    sqlite3_finalize(stmt);
    send_message(client, 403, "Forbidden", "账号已停用，请联系管理员");
    return;
  }

  send_auth_payload(app->db, client, sqlite3_column_int(stmt, 0));
  sqlite3_finalize(stmt);
}

static void handle_auth_me(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"user\":");
  append_user_json(app->db, user.id, body, sizeof(body), &off);
  snprintf(body + off, sizeof(body) - off, "}");
  send_response(client, 200, "OK", body);
}

static void handle_auth_profile(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char name[128], email[128], phone[64], company[128], bio[512];
  sqlite3_stmt *stmt = NULL;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (!json_get_string(request->body, "name", name, sizeof(name)) ||
      !json_get_string(request->body, "email", email, sizeof(email)) ||
      !json_get_string(request->body, "phone", phone, sizeof(phone)) ||
      !json_get_string(request->body, "company", company, sizeof(company)) ||
      !json_get_string(request->body, "bio", bio, sizeof(bio))) {
    send_message(client, 400, "Bad Request", "资料字段不完整");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET name = ?, email = ?, phone = ?, company = ?, bio = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, company, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, bio, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, user.id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user.id, "update_profile", "user", user.id, "更新个人资料");
  handle_auth_me(app, client, request);
}

static void handle_auth_password(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char current_password[64];
  char new_password[64];
  sqlite3_stmt *stmt = NULL;
  char hash[65];

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (!json_get_string(request->body, "currentPassword", current_password, sizeof(current_password)) ||
      !json_get_string(request->body, "newPassword", new_password, sizeof(new_password))) {
    send_message(client, 400, "Bad Request", "密码字段不完整");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "SELECT password_hash, password_salt FROM users WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user.id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      send_message(client, 404, "Not Found", "用户不存在");
      return;
    }

    if (!verify_password(current_password, (const char *) sqlite3_column_text(stmt, 1), (const char *) sqlite3_column_text(stmt, 0))) {
      sqlite3_finalize(stmt);
      send_message(client, 400, "Bad Request", "当前密码错误");
      return;
    }

    make_password_hash(new_password, (const char *) sqlite3_column_text(stmt, 1), hash);
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET password_hash = ?, quick_login_password = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_password, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, user.id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user.id, "change_password", "user", user.id, "修改登录密码");
  send_message(client, 200, "OK", "密码修改成功");
}

static unsigned char *base64_decode(const char *input, size_t *out_len) {
  const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned char table[256];
  size_t len = strlen(input);
  unsigned char *output;
  size_t i = 0;
  size_t j = 0;

  memset(table, 0x80, sizeof(table));
  for (i = 0; i < 64; i++) {
    table[(unsigned char) alphabet[i]] = (unsigned char) i;
  }
  table[(unsigned char) '='] = 0;

  output = (unsigned char *) malloc((len / 4 + 1) * 3);
  if (!output) {
    return NULL;
  }

  for (i = 0; i < len;) {
    unsigned char a, b, c, d;

    while (i < len && isspace((unsigned char) input[i])) {
      i++;
    }
    if (i >= len) {
      break;
    }

    a = table[(unsigned char) input[i++]];
    b = table[(unsigned char) input[i++]];
    c = table[(unsigned char) input[i++]];
    d = table[(unsigned char) input[i++]];

    if ((a | b | c | d) & 0x80) {
      free(output);
      return NULL;
    }

    output[j++] = (unsigned char) ((a << 2) | (b >> 4));
    if (input[i - 2] != '=') {
      output[j++] = (unsigned char) ((b << 4) | (c >> 2));
    }
    if (input[i - 1] != '=') {
      output[j++] = (unsigned char) ((c << 6) | d);
    }
  }

  *out_len = j;
  return output;
}

static const char *extension_from_upload(const char *file_name, const char *mime_type) {
  const char *ext = strrchr(file_name, '.');

  if (ext && (_stricmp(ext, ".png") == 0 || _stricmp(ext, ".jpg") == 0 || _stricmp(ext, ".jpeg") == 0 || _stricmp(ext, ".webp") == 0)) {
    return ext;
  }
  if (strcmp(mime_type, "image/png") == 0) {
    return ".png";
  }
  if (strcmp(mime_type, "image/webp") == 0) {
    return ".webp";
  }
  return ".jpg";
}

static void handle_auth_avatar(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char file_name[128];
  char mime_type[64];
  char content_base64[200000];
  const char *ext;
  unsigned char *binary;
  size_t binary_len = 0;
  char relative_path[256];
  char full_path[512];
  FILE *file = NULL;
  sqlite3_stmt *stmt = NULL;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (!json_get_string(request->body, "fileName", file_name, sizeof(file_name)) ||
      !json_get_string(request->body, "mimeType", mime_type, sizeof(mime_type)) ||
      !json_get_string(request->body, "contentBase64", content_base64, sizeof(content_base64))) {
    send_message(client, 400, "Bad Request", "头像上传参数不完整");
    return;
  }

  ext = extension_from_upload(file_name, mime_type);
  binary = base64_decode(content_base64, &binary_len);
  if (!binary || binary_len == 0) {
    send_message(client, 400, "Bad Request", "头像内容解析失败");
    free(binary);
    return;
  }

  snprintf(relative_path, sizeof(relative_path), "uploads/avatars/user_%d_%lld%s", user.id, (long long) time(NULL), ext);
  snprintf(full_path, sizeof(full_path), "%s", relative_path);

  file = fopen(full_path, "wb");
  if (!file) {
    free(binary);
    send_message(client, 500, "Internal Server Error", "头像保存失败");
    return;
  }

  fwrite(binary, 1, binary_len, file);
  fclose(file);
  free(binary);

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET avatar_path = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, relative_path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user.id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user.id, "upload_avatar", "user", user.id, relative_path);
  handle_auth_me(app, client, request);
}

static void handle_auth_logout(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  int user_id = current_user_id(request);

  if (user_id && load_user_by_id(app->db, user_id, &user)) {
    add_audit_log(app->db, user.id, "logout", "session", user.id, "主动退出登录");
  }

  send_message(client, 200, "OK", "已退出登录");
}

static void handle_bind_wallet(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char wallet[128];
  char chain_id[32];
  char chain_name[64];
  sqlite3_stmt *stmt = NULL;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (!json_get_string(request->body, "walletAddress", wallet, sizeof(wallet))) {
    send_message(client, 400, "Bad Request", "缺少钱包地址");
    return;
  }
  if (!json_get_string(request->body, "chainId", chain_id, sizeof(chain_id))) {
    chain_id[0] = '\0';
  }
  if (!json_get_string(request->body, "chainName", chain_name, sizeof(chain_name))) {
    chain_name[0] = '\0';
  }

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET wallet_address = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, wallet, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user.id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(app->db, "UPDATE wallet_bindings SET is_primary = 0 WHERE user_id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user.id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  insert_wallet_binding(app->db, user.id, wallet, chain_id, chain_name, 1);
  add_audit_log(app->db, user.id, "bind_wallet", "wallet_binding", user.id, wallet);
  handle_auth_me(app, client, request);
}

static int username_exists(sqlite3 *db, const char *username, int exclude_user_id) {
  sqlite3_stmt *stmt = NULL;
  int exists = 0;

  if (exclude_user_id > 0) {
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username = ? AND id != ?", -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 2, exclude_user_id);
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
      }
    }
  } else if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE username = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      exists = sqlite3_column_int(stmt, 0) > 0;
    }
  }

  sqlite3_finalize(stmt);
  return exists;
}

static void handle_users_get(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  char role_filter[32];
  char status_filter[32];
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以查看成员列表");
    return;
  }

  if (!get_query_param(request->path, "role", role_filter, sizeof(role_filter))) {
    role_filter[0] = '\0';
  }
  if (!get_query_param(request->path, "status", status_filter, sizeof(status_filter))) {
    snprintf(status_filter, sizeof(status_filter), "active");
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"items\":[");
  if (role_filter[0] && strcmp(status_filter, "all") != 0) {
    sqlite3_prepare_v2(app->db, "SELECT id FROM users WHERE role = ? AND status = ? ORDER BY id ASC", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, role_filter, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, status_filter, -1, SQLITE_TRANSIENT);
  } else if (role_filter[0]) {
    sqlite3_prepare_v2(app->db, "SELECT id FROM users WHERE role = ? ORDER BY id ASC", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, role_filter, -1, SQLITE_TRANSIENT);
  } else if (strcmp(status_filter, "all") != 0) {
    sqlite3_prepare_v2(app->db, "SELECT id FROM users WHERE role IN ('developer','client') AND status = ? ORDER BY id ASC", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, status_filter, -1, SQLITE_TRANSIENT);
  } else {
    sqlite3_prepare_v2(app->db, "SELECT id FROM users WHERE role IN ('developer','client') ORDER BY id ASC", -1, &stmt, NULL);
  }

  while (stmt && sqlite3_step(stmt) == SQLITE_ROW) {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "%s", first ? "" : ",");
    append_user_json(app->db, sqlite3_column_int(stmt, 0), body, sizeof(body), &off);
    first = 0;
  }
  sqlite3_finalize(stmt);

  snprintf(body + off, sizeof(body) - off, "]}");
  send_response(client, 200, "OK", body);
}

static void handle_users_create(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  char username[64], password[64], name[128], role[32], email[128], phone[64], company[128], bio[512];
  char detail[256];
  int member_id;
  char body[MAX_RESPONSE_SIZE];

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以新增成员");
    return;
  }

  name[0] = '\0';
  if (!json_get_string(request->body, "username", username, sizeof(username)) ||
      !json_get_string(request->body, "password", password, sizeof(password)) ||
      !json_get_string(request->body, "role", role, sizeof(role)) ||
      !json_get_string(request->body, "email", email, sizeof(email)) ||
      !json_get_string(request->body, "phone", phone, sizeof(phone)) ||
      !json_get_string(request->body, "company", company, sizeof(company)) ||
      !json_get_string(request->body, "bio", bio, sizeof(bio))) {
    send_message(client, 400, "Bad Request", "成员字段不完整");
    return;
  }
  if (strcmp(role, "developer") != 0 && strcmp(role, "client") != 0) {
    send_message(client, 400, "Bad Request", "只允许新增开发者或客户");
    return;
  }
  json_get_string(request->body, "name", name, sizeof(name));
  if (!name[0]) {
    snprintf(name, sizeof(name), "%s", username);
  }
  if (username_exists(app->db, username, 0)) {
    send_message(client, 400, "Bad Request", "用户名已存在");
    return;
  }

  member_id = insert_user_account(app->db, username, name, role, password, email, phone, company, bio, "", "active", "user");
  snprintf(detail, sizeof(detail), "%s/%s", role, username);
  add_audit_log(app->db, user->id, "create_user", "user", member_id, detail);
  snprintf(body, sizeof(body), "{\"message\":\"成员创建成功\",\"userId\":%d}", member_id);
  send_response(client, 200, "OK", body);
}

static void handle_users_update(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  int user_id = 0;
  char name[128], email[128], phone[64], company[128], bio[512];
  sqlite3_stmt *stmt = NULL;

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以编辑成员");
    return;
  }
  if (!json_get_int(request->body, "id", &user_id) ||
      !json_get_string(request->body, "name", name, sizeof(name)) ||
      !json_get_string(request->body, "email", email, sizeof(email)) ||
      !json_get_string(request->body, "phone", phone, sizeof(phone)) ||
      !json_get_string(request->body, "company", company, sizeof(company)) ||
      !json_get_string(request->body, "bio", bio, sizeof(bio))) {
    send_message(client, 400, "Bad Request", "编辑成员参数不完整");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET name = ?, email = ?, phone = ?, company = ?, bio = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND role IN ('developer','client')", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, company, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, bio, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, user_id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user->id, "update_user", "user", user_id, "管理员更新成员资料");
  send_message(client, 200, "OK", "成员资料已更新");
}

static void handle_user_password_reset(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  int user_id = 0;
  char new_password[64];
  sqlite3_stmt *stmt = NULL;
  char salt[64];
  char hash[65];

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以重置密码");
    return;
  }
  if (!json_get_int(request->body, "id", &user_id) ||
      !json_get_string(request->body, "newPassword", new_password, sizeof(new_password))) {
    send_message(client, 400, "Bad Request", "重置密码参数不完整");
    return;
  }

  snprintf(salt, sizeof(salt), "user-reset-%d", user_id);
  make_password_hash(new_password, salt, hash);
  if (sqlite3_prepare_v2(app->db, "UPDATE users SET password_hash = ?, password_salt = ?, quick_login_password = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND role IN ('developer','client')", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, salt, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, new_password, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, user_id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user->id, "reset_user_password", "user", user_id, "管理员重置成员密码");
  send_message(client, 200, "OK", "成员密码已重置");
}

static void handle_user_status(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  int user_id = 0;
  char status[32];
  sqlite3_stmt *stmt = NULL;

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以停用或启用成员");
    return;
  }
  if (!json_get_int(request->body, "id", &user_id) ||
      !json_get_string(request->body, "status", status, sizeof(status))) {
    send_message(client, 400, "Bad Request", "成员状态参数不完整");
    return;
  }
  if (strcmp(status, "active") != 0 && strcmp(status, "inactive") != 0) {
    send_message(client, 400, "Bad Request", "不支持的成员状态");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "UPDATE users SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND role IN ('developer','client')", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user->id, strcmp(status, "active") == 0 ? "enable_user" : "disable_user", "user", user_id, status);
  send_message(client, 200, "OK", strcmp(status, "active") == 0 ? "成员已启用" : "成员已停用");
}

static void handle_user_delete(AppContext *app, SOCKET client, const HttpRequest *request, const CurrentUser *user) {
  char id_buffer[32];
  int user_id = 0;
  sqlite3_stmt *stmt = NULL;

  if (!is_admin(user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以删除成员");
    return;
  }
  if (!get_query_param(request->path, "id", id_buffer, sizeof(id_buffer))) {
    send_message(client, 400, "Bad Request", "缺少成员 ID");
    return;
  }
  user_id = atoi(id_buffer);
  if (referenced_project_count(app->db, user_id) > 0) {
    send_message(client, 400, "Bad Request", "成员已被项目引用，只能停用");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "DELETE FROM users WHERE id = ? AND role IN ('developer','client')", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  add_audit_log(app->db, user->id, "delete_user", "user", user_id, "删除未被引用成员");
  send_message(client, 200, "OK", "成员已删除");
}

static void handle_projects(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (strcmp(request->method, "GET") == 0) {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"items\":[");

    if (is_admin(&user)) {
      sqlite3_prepare_v2(app->db,
        "SELECT p.id, p.name, p.description, p.status, p.developer_id, p.client_id, d.name, c.name "
        "FROM projects p "
        "JOIN users d ON d.id = p.developer_id "
        "JOIN users c ON c.id = p.client_id "
        "ORDER BY p.id DESC",
        -1,
        &stmt,
        NULL);
    } else {
      sqlite3_prepare_v2(app->db,
        "SELECT p.id, p.name, p.description, p.status, p.developer_id, p.client_id, d.name, c.name "
        "FROM projects p "
        "JOIN users d ON d.id = p.developer_id "
        "JOIN users c ON c.id = p.client_id "
        "JOIN project_members pm ON pm.project_id = p.id "
        "WHERE pm.user_id = ? "
        "GROUP BY p.id "
        "ORDER BY p.id DESC",
        -1,
        &stmt,
        NULL);
      sqlite3_bind_int(stmt, 1, user.id);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      char name[256], description[512], status[64], developer_name[128], client_name[128];
      json_escape((const char *) sqlite3_column_text(stmt, 1), name, sizeof(name));
      json_escape((const char *) sqlite3_column_text(stmt, 2), description, sizeof(description));
      json_escape((const char *) sqlite3_column_text(stmt, 3), status, sizeof(status));
      json_escape((const char *) sqlite3_column_text(stmt, 6), developer_name, sizeof(developer_name));
      json_escape((const char *) sqlite3_column_text(stmt, 7), client_name, sizeof(client_name));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"id\":%d,\"name\":\"%s\",\"description\":\"%s\",\"status\":\"%s\",\"developerId\":%d,\"clientId\":%d,\"developerName\":\"%s\",\"clientName\":\"%s\"}",
        first ? "" : ",",
        sqlite3_column_int(stmt, 0),
        name,
        description,
        status,
        sqlite3_column_int(stmt, 4),
        sqlite3_column_int(stmt, 5),
        developer_name,
        client_name
      );
      first = 0;
    }
    sqlite3_finalize(stmt);

    snprintf(body + off, sizeof(body) - off, "]}");
    send_response(client, 200, "OK", body);
    return;
  }

  if (!is_admin(&user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以创建项目");
    return;
  }

  {
    char name[128], description[512];
    int developer_id = 0;
    int client_id = 0;
    int project_id = 0;

    if (!json_get_string(request->body, "name", name, sizeof(name)) ||
        !json_get_string(request->body, "description", description, sizeof(description)) ||
        !json_get_int(request->body, "developerId", &developer_id) ||
        !json_get_int(request->body, "clientId", &client_id)) {
      send_message(client, 400, "Bad Request", "项目字段不完整");
      return;
    }
    if (!user_matches_role_and_status(app->db, developer_id, "developer", "active")) {
      send_message(client, 400, "Bad Request", "开发者不存在或已停用");
      return;
    }
    if (!user_matches_role_and_status(app->db, client_id, "client", "active")) {
      send_message(client, 400, "Bad Request", "客户不存在或已停用");
      return;
    }

    project_id = insert_project(app->db, name, description, "active", developer_id, client_id);
    insert_project_member(app->db, project_id, developer_id, "developer");
    insert_project_member(app->db, project_id, client_id, "client");
    add_audit_log(app->db, user.id, "create_project", "project", project_id, name);

    snprintf(body, sizeof(body), "{\"message\":\"项目创建成功\",\"projectId\":%d}", project_id);
    send_response(client, 200, "OK", body);
  }
}

static void handle_worklogs(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (strcmp(request->method, "GET") == 0) {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"items\":[");
    if (is_admin(&user)) {
      sqlite3_prepare_v2(app->db,
        "SELECT w.id, w.project_id, p.name, w.user_id, u.name, w.stage_no, w.work_date, w.hours, w.task_description, w.digest, w.evidence_status, w.tx_hash, w.created_at "
        "FROM worklogs w "
        "JOIN projects p ON p.id = w.project_id "
        "JOIN users u ON u.id = w.user_id "
        "ORDER BY w.id DESC",
        -1,
        &stmt,
        NULL);
    } else if (is_developer(&user)) {
      sqlite3_prepare_v2(app->db,
        "SELECT w.id, w.project_id, p.name, w.user_id, u.name, w.stage_no, w.work_date, w.hours, w.task_description, w.digest, w.evidence_status, w.tx_hash, w.created_at "
        "FROM worklogs w "
        "JOIN projects p ON p.id = w.project_id "
        "JOIN users u ON u.id = w.user_id "
        "WHERE w.user_id = ? "
        "ORDER BY w.id DESC",
        -1,
        &stmt,
        NULL);
      sqlite3_bind_int(stmt, 1, user.id);
    } else {
      sqlite3_prepare_v2(app->db,
        "SELECT w.id, w.project_id, p.name, w.user_id, u.name, w.stage_no, w.work_date, w.hours, w.task_description, w.digest, w.evidence_status, w.tx_hash, w.created_at "
        "FROM worklogs w "
        "JOIN projects p ON p.id = w.project_id "
        "JOIN users u ON u.id = w.user_id "
        "WHERE p.client_id = ? "
        "ORDER BY w.id DESC",
        -1,
        &stmt,
        NULL);
      sqlite3_bind_int(stmt, 1, user.id);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      char project_name[256], user_name[128], work_date[64], task[512], digest[128], status[64], tx_hash[128], created_at[64];
      json_escape((const char *) sqlite3_column_text(stmt, 2), project_name, sizeof(project_name));
      json_escape((const char *) sqlite3_column_text(stmt, 4), user_name, sizeof(user_name));
      json_escape((const char *) sqlite3_column_text(stmt, 6), work_date, sizeof(work_date));
      json_escape((const char *) sqlite3_column_text(stmt, 8), task, sizeof(task));
      json_escape((const char *) sqlite3_column_text(stmt, 9), digest, sizeof(digest));
      json_escape((const char *) sqlite3_column_text(stmt, 10), status, sizeof(status));
      json_escape((const char *) sqlite3_column_text(stmt, 11), tx_hash, sizeof(tx_hash));
      json_escape((const char *) sqlite3_column_text(stmt, 12), created_at, sizeof(created_at));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"id\":%d,\"projectId\":%d,\"projectName\":\"%s\",\"userId\":%d,\"userName\":\"%s\",\"stageNo\":%d,\"workDate\":\"%s\",\"hours\":%.2f,\"taskDescription\":\"%s\",\"digest\":\"%s\",\"evidenceStatus\":\"%s\",\"txHash\":\"%s\",\"createdAt\":\"%s\"}",
        first ? "" : ",",
        sqlite3_column_int(stmt, 0),
        sqlite3_column_int(stmt, 1),
        project_name,
        sqlite3_column_int(stmt, 3),
        user_name,
        sqlite3_column_int(stmt, 5),
        work_date,
        sqlite3_column_double(stmt, 7),
        task,
        digest,
        status,
        tx_hash,
        created_at
      );
      first = 0;
    }
    sqlite3_finalize(stmt);

    snprintf(body + off, sizeof(body) - off, "]}");
    send_response(client, 200, "OK", body);
    return;
  }

  if (!is_developer(&user)) {
    send_message(client, 403, "Forbidden", "只有开发者可以登记工时");
    return;
  }

  {
    int project_id = 0;
    int stage_no = 1;
    int blocked_by_stage_audit = 0;
    double hours = 0.0;
    char work_date[64];
    char task[512];
    char digest[65];
    int worklog_id = 0;

    if (!json_get_int(request->body, "projectId", &project_id) ||
        !json_get_double(request->body, "hours", &hours) ||
        !json_get_string(request->body, "workDate", work_date, sizeof(work_date)) ||
        !json_get_string(request->body, "taskDescription", task, sizeof(task))) {
      send_message(client, 400, "Bad Request", "工时字段不完整");
      return;
    }

    if (!user_can_access_project(app->db, user.id, project_id)) {
      send_message(client, 403, "Forbidden", "你无权在该项目下登记工时");
      return;
    }
    stage_no = project_current_stage_for_write(app->db, project_id, &blocked_by_stage_audit);
    if (blocked_by_stage_audit) {
      send_message(client, 400, "Bad Request", "当前阶段已通过客户确认，等待管理员阶段审计后才能继续登记");
      return;
    }

    worklog_id = insert_worklog(app->db, project_id, user.id, stage_no, work_date, hours, task, "pending", "");
    build_worklog_digest(project_id, user.id, stage_no, work_date, hours, task, digest);
    add_audit_log(app->db, user.id, "create_worklog", "worklog", worklog_id, task);

    snprintf(body, sizeof(body), "{\"message\":\"工时创建成功\",\"item\":{\"id\":%d,\"stageNo\":%d,\"digest\":\"%s\",\"evidenceStatus\":\"pending\"}}", worklog_id, stage_no, digest);
    send_response(client, 200, "OK", body);
  }
}

static void handle_deliverables(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (strcmp(request->method, "GET") == 0) {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"items\":[");

    if (is_admin(&user)) {
      sqlite3_prepare_v2(app->db,
        "SELECT d.id, d.project_id, p.name, d.user_id, u.name, d.stage_no, d.version, d.file_name, d.summary, d.file_hash, d.status, d.tx_hash, d.confirmation_comment, d.created_at "
        "FROM deliverables d "
        "JOIN projects p ON p.id = d.project_id "
        "JOIN users u ON u.id = d.user_id "
        "ORDER BY d.id DESC",
        -1,
        &stmt,
        NULL);
    } else if (is_developer(&user)) {
      sqlite3_prepare_v2(app->db,
        "SELECT d.id, d.project_id, p.name, d.user_id, u.name, d.stage_no, d.version, d.file_name, d.summary, d.file_hash, d.status, d.tx_hash, d.confirmation_comment, d.created_at "
        "FROM deliverables d "
        "JOIN projects p ON p.id = d.project_id "
        "JOIN users u ON u.id = d.user_id "
        "WHERE d.user_id = ? "
        "ORDER BY d.id DESC",
        -1,
        &stmt,
        NULL);
      sqlite3_bind_int(stmt, 1, user.id);
    } else {
      sqlite3_prepare_v2(app->db,
        "SELECT d.id, d.project_id, p.name, d.user_id, u.name, d.stage_no, d.version, d.file_name, d.summary, d.file_hash, d.status, d.tx_hash, d.confirmation_comment, d.created_at "
        "FROM deliverables d "
        "JOIN projects p ON p.id = d.project_id "
        "JOIN users u ON u.id = d.user_id "
        "WHERE p.client_id = ? "
        "ORDER BY d.id DESC",
        -1,
        &stmt,
        NULL);
      sqlite3_bind_int(stmt, 1, user.id);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      char project_name[256], user_name[128], version[64], file_name[256], summary[512], file_hash[128], status[64], tx_hash[128], confirmation_comment[512], created_at[64];
      json_escape((const char *) sqlite3_column_text(stmt, 2), project_name, sizeof(project_name));
      json_escape((const char *) sqlite3_column_text(stmt, 4), user_name, sizeof(user_name));
      json_escape((const char *) sqlite3_column_text(stmt, 6), version, sizeof(version));
      json_escape((const char *) sqlite3_column_text(stmt, 7), file_name, sizeof(file_name));
      json_escape((const char *) sqlite3_column_text(stmt, 8), summary, sizeof(summary));
      json_escape((const char *) sqlite3_column_text(stmt, 9), file_hash, sizeof(file_hash));
      json_escape((const char *) sqlite3_column_text(stmt, 10), status, sizeof(status));
      json_escape((const char *) sqlite3_column_text(stmt, 11), tx_hash, sizeof(tx_hash));
      json_escape((const char *) sqlite3_column_text(stmt, 12), confirmation_comment, sizeof(confirmation_comment));
      json_escape((const char *) sqlite3_column_text(stmt, 13), created_at, sizeof(created_at));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"id\":%d,\"projectId\":%d,\"projectName\":\"%s\",\"userId\":%d,\"userName\":\"%s\",\"stageNo\":%d,\"version\":\"%s\",\"fileName\":\"%s\",\"summary\":\"%s\",\"fileHash\":\"%s\",\"status\":\"%s\",\"txHash\":\"%s\",\"confirmationComment\":\"%s\",\"createdAt\":\"%s\"}",
        first ? "" : ",",
        sqlite3_column_int(stmt, 0),
        sqlite3_column_int(stmt, 1),
        project_name,
        sqlite3_column_int(stmt, 3),
        user_name,
        sqlite3_column_int(stmt, 5),
        version,
        file_name,
        summary,
        file_hash,
        status,
        tx_hash,
        confirmation_comment,
        created_at
      );
      first = 0;
    }
    sqlite3_finalize(stmt);

    snprintf(body + off, sizeof(body) - off, "]}");
    send_response(client, 200, "OK", body);
    return;
  }

  if (!is_developer(&user)) {
    send_message(client, 403, "Forbidden", "只有开发者可以登记交付物");
    return;
  }

  {
    int project_id = 0;
    int deliverable_id = 0;
    int stage_no = 1;
    int blocked_by_stage_audit = 0;
    char version[64];
    char file_name[256];
    char summary[512];
    char file_hash[128];

    if (!json_get_int(request->body, "projectId", &project_id) ||
        !json_get_string(request->body, "version", version, sizeof(version)) ||
        !json_get_string(request->body, "fileName", file_name, sizeof(file_name)) ||
        !json_get_string(request->body, "summary", summary, sizeof(summary)) ||
        !json_get_string(request->body, "fileHash", file_hash, sizeof(file_hash))) {
      send_message(client, 400, "Bad Request", "交付物字段不完整");
      return;
    }

    if (!user_can_access_project(app->db, user.id, project_id)) {
      send_message(client, 403, "Forbidden", "你无权在该项目下提交交付物");
      return;
    }
    stage_no = project_current_stage_for_write(app->db, project_id, &blocked_by_stage_audit);
    if (blocked_by_stage_audit) {
      send_message(client, 400, "Bad Request", "当前阶段已通过客户确认，等待管理员阶段审计后才能继续提交");
      return;
    }

    deliverable_id = insert_deliverable(app->db, project_id, user.id, stage_no, version, file_name, summary, file_hash, "uploaded", "", "");
    add_audit_log(app->db, user.id, "create_deliverable", "deliverable", deliverable_id, file_name);

    snprintf(body, sizeof(body), "{\"message\":\"交付物登记成功\",\"item\":{\"id\":%d,\"stageNo\":%d,\"status\":\"uploaded\"}}", deliverable_id, stage_no);
    send_response(client, 200, "OK", body);
  }
}

static void handle_evidence(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  int business_id = 0;
  int block_number = 0;
  char business_type[64];
  char tx_hash[128];
  char status[64];
  char comment[512];
  sqlite3_stmt *stmt = NULL;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (!json_get_string(request->body, "businessType", business_type, sizeof(business_type)) ||
      !json_get_int(request->body, "businessId", &business_id) ||
      !json_get_string(request->body, "txHash", tx_hash, sizeof(tx_hash)) ||
      !json_get_string(request->body, "status", status, sizeof(status))) {
    send_message(client, 400, "Bad Request", "存证参数不完整");
    return;
  }
  if (!json_get_int(request->body, "blockNumber", &block_number)) {
    block_number = 0;
  }
  if (!json_get_string(request->body, "comment", comment, sizeof(comment))) {
    comment[0] = '\0';
  }

  if (strcmp(business_type, "worklog") == 0) {
    if (!is_developer(&user) || !developer_owns_worklog(app->db, user.id, business_id)) {
      send_message(client, 403, "Forbidden", "你无权为该工时写入存证");
      return;
    }
    if (sqlite3_prepare_v2(app->db, "UPDATE worklogs SET tx_hash = ?, evidence_status = ? WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, tx_hash, -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, strcmp(status, "confirmed") == 0 ? "confirmed" : "failed", -1, SQLITE_STATIC);
      sqlite3_bind_int(stmt, 3, business_id);
      sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    add_audit_log(app->db, user.id, "write_worklog_evidence", "worklog", business_id, tx_hash);
  } else if (strcmp(business_type, "deliverable") == 0) {
    if (!is_developer(&user)) {
      send_message(client, 403, "Forbidden", "只有开发者可以提交交付物存证");
      return;
    }
    if (sqlite3_prepare_v2(app->db, "UPDATE deliverables SET tx_hash = ?, status = ?, confirmation_comment = '' WHERE id = ? AND user_id = ?", -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, tx_hash, -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, strcmp(status, "confirmed") == 0 ? "notarized" : "rejected", -1, SQLITE_STATIC);
      sqlite3_bind_int(stmt, 3, business_id);
      sqlite3_bind_int(stmt, 4, user.id);
      sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    add_audit_log(app->db, user.id, "write_deliverable_evidence", "deliverable", business_id, tx_hash);
  } else if (strcmp(business_type, "deliverable_confirmation") == 0) {
    if (!is_client_role(&user) || !client_owns_deliverable(app->db, user.id, business_id)) {
      send_message(client, 403, "Forbidden", "你无权确认该交付物");
      return;
    }

    insert_approval(app->db, business_id, user.id, strcmp(status, "confirmed") == 0 ? 1 : 0, comment, tx_hash);
    if (sqlite3_prepare_v2(app->db, "UPDATE deliverables SET tx_hash = ?, status = ?, confirmation_comment = ? WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, tx_hash, -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, strcmp(status, "confirmed") == 0 ? "approved" : "rejected", -1, SQLITE_STATIC);
      sqlite3_bind_text(stmt, 3, comment, -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 4, business_id);
      sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    add_audit_log(app->db, user.id, strcmp(status, "confirmed") == 0 ? "approve_deliverable" : "reject_deliverable", "deliverable", business_id, comment);
  } else {
    send_message(client, 400, "Bad Request", "不支持的业务类型");
    return;
  }

  insert_evidence(app->db, business_type, business_id, tx_hash, block_number, status, comment, user.id);
  send_message(client, 200, "OK", "存证回写成功");
}

static void handle_stage_audit(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  int project_id = 0;
  int stage_no = 0;
  int found = 0;
  char decision[16];
  char comment[512];
  char project_name[256] = "";
  char project_status[64] = "";
  char tx_hash[128];
  char detail[768];
  char body[MAX_RESPONSE_SIZE];
  sqlite3_stmt *stmt = NULL;
  int project_audit_id = 0;
  int latest_deliverable_id = 0;
  StageSnapshot snapshot;
  const char *message;
  const char *action;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }
  if (!is_admin(&user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以执行阶段审计");
    return;
  }

  if (!json_get_int(request->body, "projectId", &project_id) ||
      !json_get_int(request->body, "stageNo", &stage_no) ||
      !json_get_string(request->body, "decision", decision, sizeof(decision))) {
    send_message(client, 400, "Bad Request", "阶段审计参数不完整");
    return;
  }
  if (!json_get_string(request->body, "comment", comment, sizeof(comment))) {
    comment[0] = '\0';
  }
  if (strcmp(decision, "pass") != 0 && strcmp(decision, "reject") != 0) {
    send_message(client, 400, "Bad Request", "不支持的审计结论");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "SELECT name, status FROM projects WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      found = 1;
      snprintf(project_name, sizeof(project_name), "%s", (const char *) sqlite3_column_text(stmt, 0));
      snprintf(project_status, sizeof(project_status), "%s", (const char *) sqlite3_column_text(stmt, 1));
    }
  }
  sqlite3_finalize(stmt);

  if (!found) {
    send_message(client, 404, "Not Found", "待审项目不存在");
    return;
  }
  if (strcmp(project_status, "active") != 0) {
    send_message(client, 400, "Bad Request", "当前项目不可执行阶段审计");
    return;
  }

  load_stage_snapshot(app->db, project_id, stage_no, &snapshot);
  if (snapshot.stage_no <= 0 || snapshot.deliverable_count == 0) {
    send_message(client, 404, "Not Found", "待审阶段不存在");
    return;
  }
  if (strcmp(snapshot.state, "pending_stage_audit") != 0) {
    send_message(client, 400, "Bad Request", "当前阶段尚未进入待审计状态");
    return;
  }

  latest_deliverable_id = latest_deliverable_id_for_stage(app->db, project_id, stage_no);
  snprintf(tx_hash, sizeof(tx_hash), "0xsa%08x%04x%04x", (unsigned int) time(NULL), project_id, stage_no);
  project_audit_id = insert_project_audit(
    app->db,
    project_id,
    stage_no,
    "stage",
    decision,
    comment[0] ? comment : (strcmp(decision, "pass") == 0 ? "阶段审计通过。" : "阶段审计驳回。"),
    tx_hash,
    user.id
  );

  if (latest_deliverable_id > 0 && strcmp(decision, "reject") == 0) {
    if (sqlite3_prepare_v2(app->db, "UPDATE deliverables SET status = 'rejected', confirmation_comment = ? WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, comment[0] ? comment : "阶段审计驳回，需要继续修改。", -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 2, latest_deliverable_id);
      sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
  }

  insert_evidence(
    app->db,
    "stage_audit",
    project_audit_id,
    tx_hash,
    (int) time(NULL),
    strcmp(decision, "pass") == 0 ? "confirmed" : "failed",
    comment[0] ? comment : (strcmp(decision, "pass") == 0 ? "阶段审计通过。" : "阶段审计驳回。"),
    user.id
  );

  action = strcmp(decision, "pass") == 0 ? "stage_audit_pass" : "stage_audit_reject";
  message = strcmp(decision, "pass") == 0 ? "阶段审计已通过" : "阶段审计已驳回";
  snprintf(detail, sizeof(detail), "%s｜阶段 %d", project_name, stage_no);
  add_audit_log(app->db, user.id, action, "project_stage", project_id, detail);

  snprintf(body, sizeof(body), "{\"message\":\"%s\",\"projectId\":%d,\"stageNo\":%d,\"txHash\":\"%s\"}", message, project_id, stage_no, tx_hash);
  send_response(client, 200, "OK", body);
}

static void handle_final_audit(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  int project_id = 0;
  int found = 0;
  char decision[16];
  char comment[512];
  char project_name[256] = "";
  char project_status[64] = "";
  char detail[768];
  char body[MAX_RESPONSE_SIZE];
  sqlite3_stmt *stmt = NULL;
  int project_audit_id = 0;
  char tx_hash[128];
  const char *next_status;
  const char *action;
  const char *message;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }
  if (!is_admin(&user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以执行最终审计");
    return;
  }

  if ((!json_get_int(request->body, "projectId", &project_id) && !json_get_int(request->body, "targetId", &project_id)) ||
      !json_get_string(request->body, "decision", decision, sizeof(decision))) {
    send_message(client, 400, "Bad Request", "最终审计参数不完整");
    return;
  }
  if (!json_get_string(request->body, "comment", comment, sizeof(comment))) {
    comment[0] = '\0';
  }
  if (strcmp(decision, "pass") != 0 && strcmp(decision, "reject") != 0) {
    send_message(client, 400, "Bad Request", "不支持的审计结论");
    return;
  }

  if (sqlite3_prepare_v2(app->db, "SELECT name, status FROM projects WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      found = 1;
      snprintf(project_name, sizeof(project_name), "%s", (const char *) sqlite3_column_text(stmt, 0));
      snprintf(project_status, sizeof(project_status), "%s", (const char *) sqlite3_column_text(stmt, 1));
    }
  }
  sqlite3_finalize(stmt);

  if (!found) {
    send_message(client, 404, "Not Found", "待审项目不存在");
    return;
  }

  if (!project_can_run_final_audit(app->db, project_status, project_id)) {
    char hint[256];
    build_final_audit_hint(app->db, project_status, project_id, hint, sizeof(hint));
    send_message(client, 400, "Bad Request", hint);
    return;
  }

  next_status = strcmp(decision, "pass") == 0 ? "completed" : "disputed";
  action = strcmp(decision, "pass") == 0 ? "final_audit_pass" : "final_audit_reject";
  message = strcmp(decision, "pass") == 0 ? "最终审计已通过" : "最终审计已驳回";
  snprintf(tx_hash, sizeof(tx_hash), "0xfa%08x%04x", (unsigned int) time(NULL), project_id);

  if (sqlite3_prepare_v2(app->db, "UPDATE projects SET status = ? WHERE id = ?", -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, next_status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, project_id);
    sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);

  if (comment[0]) {
    snprintf(detail, sizeof(detail), "%s｜%s", project_name, comment);
  } else {
    snprintf(detail, sizeof(detail), "%s", project_name);
  }
  project_audit_id = insert_project_audit(
    app->db,
    project_id,
    0,
    "final",
    decision,
    comment[0] ? comment : (strcmp(decision, "pass") == 0 ? "项目总审计通过。" : "项目总审计驳回。"),
    tx_hash,
    user.id
  );
  insert_evidence(
    app->db,
    "final_audit",
    project_audit_id,
    tx_hash,
    (int) time(NULL),
    strcmp(decision, "pass") == 0 ? "confirmed" : "failed",
    comment[0] ? comment : (strcmp(decision, "pass") == 0 ? "项目总审计通过。" : "项目总审计驳回。"),
    user.id
  );
  add_audit_log(app->db, user.id, action, "project", project_id, detail);

  snprintf(body, sizeof(body), "{\"message\":\"%s\",\"projectId\":%d,\"status\":\"%s\",\"txHash\":\"%s\"}", message, project_id, next_status, tx_hash);
  send_response(client, 200, "OK", body);
}

static void handle_process_flow(AppContext *app, SOCKET client, const HttpRequest *request) {
  static const char *global_keys[6] = {
    "project_setup",
    "wallet_ready",
    "stage_records",
    "client_stage_confirm",
    "admin_stage_audit",
    "final_audit",
  };
  static const char *global_titles[6] = {
    "管理员创建项目并确定成员",
    "开发者完成首次钱包绑定",
    "开发者在当前阶段持续登记工时与交付物",
    "客户确认当前阶段交付并查看工时历史",
    "管理员执行当前阶段审计",
    "管理员执行项目总审计并归档",
  };
  static const char *global_descriptions[6] = {
    "项目创建时即确定开发者与客户，不再拆成独立成员分配步骤。",
    "钱包绑定作为项目启动准备动作，为后续存证提供身份信息。",
    "同一阶段内允许累计多条工时和多次交付，保留完整历史记录。",
    "客户以交付物为主进行确认，同时查看该阶段的全部工时历史。",
    "客户通过后，管理员对该阶段执行一次阶段审计，若驳回则继续使用同一阶段修订。",
    "所有阶段关闭后，管理员执行最终总审计，决定项目归档或争议状态。",
  };
  static const char *global_actors[6] = {
    "admin",
    "developer",
    "developer",
    "client",
    "admin",
    "admin",
  };
  static const char *global_gates[6] = {
    "项目已创建且已配置开发者与客户。",
    "开发者已完成首次钱包绑定。",
    "当前阶段已形成工时、交付与证据沉淀。",
    "客户已确认当前阶段交付。",
    "管理员已完成当前阶段审计。",
    "所有阶段已关闭且满足项目总审计条件。",
  };
  static const char *overview_lane_roles[3] = { "admin", "developer", "client" };
  static const char *overview_lane_titles[3] = { "管理员", "开发者", "客户" };
  static const char *overview_lane_summaries[3] = {
    "立项、阶段审计与总审计",
    "登记工时、提交交付与补充修订",
    "确认交付、查看工时与反馈"
  };
  static const char *overview_node_keys[7] = {
    "project_setup",
    "wallet_ready",
    "developer_log_submit",
    "client_review",
    "developer_revise",
    "admin_stage_audit",
    "final_audit"
  };
  static const char *overview_node_titles[7] = {
    "项目创建",
    "钱包准备",
    "登记工时与交付",
    "客户确认",
    "补充修订",
    "阶段审计",
    "最终总审计"
  };
  static const char *overview_node_actors[7] = {
    "admin",
    "developer",
    "developer",
    "client",
    "developer",
    "admin",
    "admin"
  };
  static const char *overview_node_kinds[7] = {
    "setup",
    "setup",
    "loop",
    "loop",
    "loop",
    "audit",
    "audit"
  };
  CurrentUser user;
  CurrentUser developer_user;
  CurrentUser client_user;
  StageSnapshot current_snapshot;
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;
  int selected_project_id = 0;
  char project_param[32];
  int project_exists_flag = 0;
  int developer_id = 0;
  int client_id = 0;
  char project_name[256] = "";
  char project_status[64] = "";
  char developer_name[128] = "";
  char client_name[128] = "";
  char hint_title[128] = "阶段循环式流程总览";
  char hint_summary[512] = "查看项目立项、开发准备、阶段循环与最终总审计的整体状态。";
  int max_stage = 0;
  int pending_client_count = 0;
  int passed_stage_total = 0;
  int final_eligible = 0;
  int wallet_ready = 0;
  int project_active = 0;
  int current_step_index = 0;
  const char *overview_focus_key = "project_setup";
  int step_index;

  memset(&developer_user, 0, sizeof(developer_user));
  memset(&client_user, 0, sizeof(client_user));
  memset(&current_snapshot, 0, sizeof(current_snapshot));

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (get_query_param(request->path, "projectId", project_param, sizeof(project_param))) {
    selected_project_id = atoi(project_param);
  }
  if (!selected_project_id || !project_visible_to_user(app->db, &user, selected_project_id)) {
    selected_project_id = select_default_project_id(app->db, &user);
  }
  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"globalHint\":{\"title\":\"%s\",\"summary\":\"%s\"},\"projectOptions\":[", hint_title, hint_summary);
  first = 1;
  if (is_admin(&user)) {
    sqlite3_prepare_v2(
      app->db,
      "SELECT p.id, p.name, p.status, d.name, c.name "
      "FROM projects p "
      "JOIN users d ON d.id = p.developer_id "
      "JOIN users c ON c.id = p.client_id "
      "ORDER BY p.id DESC",
      -1,
      &stmt,
      NULL
    );
  } else {
    sqlite3_prepare_v2(
      app->db,
      "SELECT p.id, p.name, p.status, d.name, c.name "
      "FROM projects p "
      "JOIN project_members pm ON pm.project_id = p.id "
      "JOIN users d ON d.id = p.developer_id "
      "JOIN users c ON c.id = p.client_id "
      "WHERE pm.user_id = ? "
      "GROUP BY p.id "
      "ORDER BY p.id DESC",
      -1,
      &stmt,
      NULL
    );
    sqlite3_bind_int(stmt, 1, user.id);
  }
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char safe_name[256], safe_status[64], safe_developer_name[128], safe_client_name[128];
    json_escape((const char *) sqlite3_column_text(stmt, 1), safe_name, sizeof(safe_name));
    json_escape((const char *) sqlite3_column_text(stmt, 2), safe_status, sizeof(safe_status));
    json_escape((const char *) sqlite3_column_text(stmt, 3), safe_developer_name, sizeof(safe_developer_name));
    json_escape((const char *) sqlite3_column_text(stmt, 4), safe_client_name, sizeof(safe_client_name));
    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\",\"developerName\":\"%s\",\"clientName\":\"%s\"}",
      first ? "" : ",",
      sqlite3_column_int(stmt, 0),
      safe_name,
      safe_status,
      safe_developer_name,
      safe_client_name
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"selectedProject\":");
  if (selected_project_id && project_visible_to_user(app->db, &user, selected_project_id)) {
    sqlite3_prepare_v2(
      app->db,
      "SELECT p.id, p.name, p.status, p.developer_id, p.client_id, d.name, c.name "
      "FROM projects p "
      "JOIN users d ON d.id = p.developer_id "
      "JOIN users c ON c.id = p.client_id "
      "WHERE p.id = ?",
      -1,
      &stmt,
      NULL
    );
    sqlite3_bind_int(stmt, 1, selected_project_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      char safe_name[256], safe_status[64], safe_developer_name[128], safe_client_name[128];
      project_exists_flag = 1;
      snprintf(project_name, sizeof(project_name), "%s", (const char *) sqlite3_column_text(stmt, 1));
      snprintf(project_status, sizeof(project_status), "%s", (const char *) sqlite3_column_text(stmt, 2));
      developer_id = sqlite3_column_int(stmt, 3);
      client_id = sqlite3_column_int(stmt, 4);
      snprintf(developer_name, sizeof(developer_name), "%s", (const char *) sqlite3_column_text(stmt, 5));
      snprintf(client_name, sizeof(client_name), "%s", (const char *) sqlite3_column_text(stmt, 6));
      load_user_by_id(app->db, developer_id, &developer_user);
      load_user_by_id(app->db, client_id, &client_user);
      json_escape(project_name, safe_name, sizeof(safe_name));
      json_escape(project_status, safe_status, sizeof(safe_status));
      json_escape(developer_name, safe_developer_name, sizeof(safe_developer_name));
      json_escape(client_name, safe_client_name, sizeof(safe_client_name));
      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\",\"developerName\":\"%s\",\"clientName\":\"%s\",\"developer\":",
        selected_project_id,
        safe_name,
        safe_status,
        safe_developer_name,
        safe_client_name
      );
      append_user_json(app->db, developer_id, body, sizeof(body), &off);
      off += (size_t) snprintf(body + off, sizeof(body) - off, ",\"client\":");
      append_user_json(app->db, client_id, body, sizeof(body), &off);
      off += (size_t) snprintf(body + off, sizeof(body) - off, "}");
    } else {
      off += (size_t) snprintf(body + off, sizeof(body) - off, "null");
    }
    sqlite3_finalize(stmt);
  } else {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "null");
  }

  if (project_exists_flag) {
    max_stage = project_max_stage_no(app->db, selected_project_id);
    pending_client_count = project_pending_client_stage_count(app->db, selected_project_id);
    passed_stage_total = passed_stage_count(app->db, selected_project_id);
    project_active = strcmp(project_status, "active") == 0;
    wallet_ready = developer_user.wallet_address[0] != '\0';
    final_eligible = project_can_run_final_audit(app->db, project_status, selected_project_id);
    if (max_stage > 0) {
      load_stage_snapshot(app->db, selected_project_id, max_stage, &current_snapshot);
    }
  }

  if (!project_exists_flag) {
    current_step_index = 0;
  } else if (!wallet_ready) {
    current_step_index = 1;
  } else if (strcmp(project_status, "completed") == 0 || strcmp(project_status, "disputed") == 0 || final_eligible) {
    current_step_index = 5;
  } else if (max_stage <= 0) {
    current_step_index = 2;
  } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
    current_step_index = 4;
  } else if (pending_client_count > 0) {
    current_step_index = 3;
  } else {
    current_step_index = 2;
  }

  if (!project_exists_flag) {
    overview_focus_key = "project_setup";
  } else if (!wallet_ready) {
    overview_focus_key = "wallet_ready";
  } else if (strcmp(project_status, "completed") == 0 || strcmp(project_status, "disputed") == 0 || final_eligible) {
    overview_focus_key = "final_audit";
  } else if (max_stage <= 0) {
    overview_focus_key = "developer_log_submit";
  } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
    overview_focus_key = "admin_stage_audit";
  } else if (stage_needs_developer_revise(&current_snapshot)) {
    overview_focus_key = "developer_revise";
  } else if (stage_waiting_client_confirmation(&current_snapshot)) {
    overview_focus_key = "client_review";
  } else {
    overview_focus_key = "developer_log_submit";
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, ",\"globalOverview\":{");
  off += (size_t) snprintf(body + off, sizeof(body) - off, "\"lanes\":[");
  first = 1;
  for (step_index = 0; step_index < 3; step_index++) {
    append_global_overview_lane(
      body,
      sizeof(body),
      &off,
      first,
      overview_lane_roles[step_index],
      overview_lane_titles[step_index],
      overview_lane_summaries[step_index]
    );
    first = 0;
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"nodes\":[");
  first = 1;
  for (step_index = 0; step_index < 7; step_index++) {
    const char *node_key = overview_node_keys[step_index];
    const char *node_tone = "neutral";
    char node_summary[256];
    char node_detail[512];
    int is_focus = strcmp(node_key, overview_focus_key) == 0;

    node_summary[0] = '\0';
    node_detail[0] = '\0';

    if (strcmp(node_key, "project_setup") == 0) {
      snprintf(node_summary, sizeof(node_summary), "创建项目并绑定开发者、客户。");
      if (!project_exists_flag) {
        snprintf(node_detail, sizeof(node_detail), "当前还没有可展示项目，整体流程会从项目创建开始。");
      } else {
        node_tone = "available";
        snprintf(node_detail, sizeof(node_detail), "当前项目“%s”已完成立项与成员绑定，后续流程围绕钱包准备和阶段循环推进。", project_name);
      }
    } else if (strcmp(node_key, "wallet_ready") == 0) {
      snprintf(node_summary, sizeof(node_summary), "开发者完成首次钱包绑定。");
      if (!project_exists_flag) {
        snprintf(node_detail, sizeof(node_detail), "需要先创建项目并确定开发者后，钱包准备才会进入流程。");
      } else if (!wallet_ready) {
        snprintf(node_detail, sizeof(node_detail), "当前开发者尚未完成首次钱包绑定，流程焦点停留在钱包准备。");
      } else {
        node_tone = "available";
        snprintf(node_detail, sizeof(node_detail), "钱包准备已完成，后续流程会进入阶段循环。");
      }
    } else if (strcmp(node_key, "developer_log_submit") == 0) {
      snprintf(node_summary, sizeof(node_summary), "登记工时并提交阶段交付。");
      if (!project_exists_flag || !wallet_ready) {
        snprintf(node_detail, sizeof(node_detail), "需要先完成项目创建和钱包准备，开发者才会进入工时与交付登记。");
      } else {
        node_tone = "available";
        if (max_stage <= 0) {
          snprintf(node_detail, sizeof(node_detail), "项目已准备完毕，开发者下一步会从这里开始登记首轮工时与交付。");
        } else if (stage_needs_developer_revise(&current_snapshot)) {
          snprintf(node_detail, sizeof(node_detail), "当前阶段已收到驳回反馈，开发者会继续补充工时、交付物或证据后再次提交。");
        } else {
          snprintf(node_detail, sizeof(node_detail), "在每个开放阶段中，开发者都会持续登记工时、提交交付并补充证据。");
        }
      }
    } else if (strcmp(node_key, "client_review") == 0) {
      snprintf(node_summary, sizeof(node_summary), "客户核验交付与工时历史。");
      if (!project_exists_flag || !wallet_ready || max_stage <= 0) {
        snprintf(node_detail, sizeof(node_detail), "当前还没有可供客户确认的阶段交付。");
      } else {
        node_tone = "available";
        if (stage_waiting_client_confirmation(&current_snapshot)) {
          snprintf(node_detail, sizeof(node_detail), "最近交付已形成，客户当前应在这里确认或驳回。");
        } else {
          snprintf(node_detail, sizeof(node_detail), "每当开发者形成可确认交付后，客户都会在这里核验工时与交付。");
        }
      }
    } else if (strcmp(node_key, "developer_revise") == 0) {
      snprintf(node_summary, sizeof(node_summary), "驳回后补充修订并再次提交。");
      if (!project_exists_flag || !wallet_ready || max_stage <= 0) {
        snprintf(node_detail, sizeof(node_detail), "当客户或阶段审计驳回时，开发者会回到这里继续补充修订。");
      } else {
        node_tone = "available";
        if (stage_needs_developer_revise(&current_snapshot)) {
          snprintf(node_detail, sizeof(node_detail), "最近交付或阶段结果已被驳回，开发者当前应在这里补充修订后再次提交。");
        } else {
          snprintf(node_detail, sizeof(node_detail), "如果客户或管理员驳回当前阶段，开发者会在这里继续修订并回到客户确认。");
        }
      }
    } else if (strcmp(node_key, "admin_stage_audit") == 0) {
      snprintf(node_summary, sizeof(node_summary), "客户通过后进入管理员阶段审计。");
      if (!project_exists_flag || max_stage <= 0) {
        snprintf(node_detail, sizeof(node_detail), "当前还没有可进入阶段审计的阶段。");
      } else {
        node_tone = "available";
        if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
          snprintf(node_detail, sizeof(node_detail), "客户确认已完成，管理员当前会在这里执行阶段审计。");
        } else {
          snprintf(node_detail, sizeof(node_detail), "客户通过后，流程会进入管理员阶段审计，并决定是否闭环当前阶段。");
        }
      }
    } else {
      snprintf(node_summary, sizeof(node_summary), "所有阶段闭环后执行项目总审计。");
      if (!project_exists_flag) {
        snprintf(node_detail, sizeof(node_detail), "当前没有可进入最终总审计的项目。");
      } else if (strcmp(project_status, "completed") == 0) {
        node_tone = "available";
        snprintf(node_detail, sizeof(node_detail), "该项目已经完成最终总审计归档，整体流程在这里收束。");
      } else if (strcmp(project_status, "disputed") == 0) {
        node_tone = "available";
        snprintf(node_detail, sizeof(node_detail), "该项目在最终总审计后进入争议状态，整体流程同样在这里收束。");
      } else if (final_eligible) {
        node_tone = "available";
        snprintf(node_detail, sizeof(node_detail), "所有阶段已闭环，管理员当前可以执行完整项目总审计。");
      } else {
        snprintf(node_detail, sizeof(node_detail), "只有所有阶段都关闭后，流程才会进入最终总审计。");
      }
    }

    if (is_focus) {
      node_tone = "focus";
    }

    append_global_overview_node(
      body,
      sizeof(body),
      &off,
      first,
      node_key,
      overview_node_titles[step_index],
      overview_node_actors[step_index],
      overview_node_kinds[step_index],
      node_tone,
      is_focus,
      node_summary,
      node_detail
    );
    first = 0;
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"edges\":[");
  first = 1;
  {
    static const char *overview_edge_from[8] = {
      "project_setup",
      "wallet_ready",
      "developer_log_submit",
      "client_review",
      "developer_revise",
      "client_review",
      "admin_stage_audit",
      "admin_stage_audit"
    };
    static const char *overview_edge_to[8] = {
      "wallet_ready",
      "developer_log_submit",
      "client_review",
      "developer_revise",
      "client_review",
      "admin_stage_audit",
      "developer_revise",
      "final_audit"
    };
    static const char *overview_edge_kind[8] = {
      "forward",
      "forward",
      "forward",
      "rework",
      "rework",
      "approval",
      "rework",
      "forward"
    };
    int edge_index;
    for (edge_index = 0; edge_index < 8; edge_index++) {
      const char *edge_tone =
        strcmp(overview_edge_from[edge_index], overview_focus_key) == 0 ||
        strcmp(overview_edge_to[edge_index], overview_focus_key) == 0
          ? "focus"
          : "neutral";

      append_global_overview_edge(
        body,
        sizeof(body),
        &off,
        first,
        overview_edge_from[edge_index],
        overview_edge_to[edge_index],
        overview_edge_kind[edge_index],
        edge_tone
      );
      first = 0;
    }
  }
  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"focusKey\":\"%s\"}", overview_focus_key);

  off += (size_t) snprintf(body + off, sizeof(body) - off, ",\"globalSteps\":[");
  for (step_index = 0; step_index < 6; step_index++) {
    char runtime_state[32] = "locked";
    char runtime_detail[512];
    const char *runtime_gate = global_gates[step_index];

    runtime_detail[0] = '\0';
    if (step_index == 0) {
      if (project_exists_flag) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "当前项目“%s”已完成成员绑定，后续阶段循环可以直接推进。", project_name);
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "当前还没有可展示项目，流程会从项目创建开始。");
      }
    } else if (step_index == 1) {
      if (!project_exists_flag) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "需要先创建项目并确定开发者后，钱包准备才有意义。");
      } else if (wallet_ready) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "开发者已完成钱包绑定，可继续进行阶段存证。");
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "开发者尚未完成首次钱包绑定，链上存证准备仍未就绪。");
      }
    } else if (step_index == 2) {
      if (!project_exists_flag || !wallet_ready) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "需要先完成项目创建和钱包准备，才能进入阶段登记。");
      } else if (max_stage <= 0) {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "项目已准备完毕，等待开发者创建第一个阶段记录。");
      } else if (pending_client_count > 0 || strcmp(current_snapshot.state, "pending_stage_audit") == 0 || strcmp(current_snapshot.state, "closed") == 0) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "阶段 %d 已形成完整开发材料，可以交由后续角色处理。", current_snapshot.stage_no);
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "开发者当前仍在阶段 %d 持续登记工时、交付物与证据。", current_snapshot.stage_no);
      }
    } else if (step_index == 3) {
      if (!project_exists_flag || !wallet_ready || max_stage <= 0) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "开发者尚未形成可供客户确认的阶段交付。");
      } else if (pending_client_count > 0) {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "阶段 %d 的交付已准备完毕，当前正等待客户确认。", current_snapshot.stage_no);
      } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0 || strcmp(current_snapshot.state, "closed") == 0 || final_eligible || strcmp(project_status, "completed") == 0 || strcmp(project_status, "disputed") == 0) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "客户已完成最近阶段确认，流程可以继续推进。");
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "当前阶段仍由开发者持续登记，客户确认尚未解锁。");
      }
    } else if (step_index == 4) {
      if (!project_exists_flag || max_stage <= 0) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "尚未形成可审计阶段。");
      } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "客户已通过阶段 %d，管理员当前需要执行阶段审计。", current_snapshot.stage_no);
      } else if (strcmp(current_snapshot.state, "closed") == 0 && passed_stage_total > 0) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "最近阶段审计已通过，阶段闭环已完成。");
      } else if (pending_client_count > 0) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "管理员阶段审计仍锁定，因为阶段 %d 还在等待客户确认。", current_snapshot.stage_no);
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "当前阶段还未进入待审计状态。");
      }
    } else {
      char final_summary[512];
      build_final_audit_hint(app->db, project_status, selected_project_id, final_summary, sizeof(final_summary));
      if (!project_exists_flag) {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "当前没有可进入最终总审计的项目。");
      } else if (strcmp(project_status, "completed") == 0) {
        snprintf(runtime_state, sizeof(runtime_state), "completed");
        snprintf(runtime_detail, sizeof(runtime_detail), "该项目已完成最终总审计归档。");
      } else if (strcmp(project_status, "disputed") == 0) {
        snprintf(runtime_state, sizeof(runtime_state), "failed");
        snprintf(runtime_detail, sizeof(runtime_detail), "最终总审计未通过，项目已进入争议状态。");
      } else if (final_eligible) {
        snprintf(runtime_state, sizeof(runtime_state), "running");
        snprintf(runtime_detail, sizeof(runtime_detail), "所有阶段已关闭，管理员当前可以执行完整项目审核。");
      } else {
        snprintf(runtime_state, sizeof(runtime_state), "locked");
        snprintf(runtime_detail, sizeof(runtime_detail), "%s", final_summary);
      }
    }

    append_global_process_step(
      body,
      sizeof(body),
      &off,
      step_index == 0,
      global_keys[step_index],
      global_titles[step_index],
      global_descriptions[step_index],
      global_actors[step_index],
      runtime_state,
      runtime_detail,
      runtime_gate,
      step_index == current_step_index
    );
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"roleProgress\":[");
  first = 1;
  {
    int role_index;
    for (role_index = 0; role_index < 3; role_index++) {
      const char *role_key = role_index == 0 ? "developer" : (role_index == 1 ? "client" : "admin");
      char title[128], status[32], summary[512], next_action[512], blocked_by[512];
      char safe_role[32], safe_title[128], safe_status[32], safe_summary[512], safe_next_action[512], safe_blocked_by[512];
      int stage_no = 0;

      title[0] = '\0';
      status[0] = '\0';
      summary[0] = '\0';
      next_action[0] = '\0';
      blocked_by[0] = '\0';

      if (!project_exists_flag) {
        stage_no = 0;
        snprintf(title, sizeof(title), "%s", role_index == 0 ? "开发者" : (role_index == 1 ? "客户" : "管理员"));
        snprintf(status, sizeof(status), "pending");
        snprintf(summary, sizeof(summary), "当前没有可展示项目。");
        snprintf(next_action, sizeof(next_action), "等待项目进入流程总览。");
      } else if (role_index == 0) {
        stage_no = max_stage > 0 ? max_stage : 1;
        snprintf(title, sizeof(title), "开发者 · %s", developer_name[0] ? developer_name : "未分配");
        if (strcmp(project_status, "completed") == 0 || strcmp(project_status, "disputed") == 0) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "所有阶段开发动作已结束，项目已离开日常推进状态。");
          snprintf(next_action, sizeof(next_action), "如需继续推进，需要新建或恢复项目流程。");
        } else if (!wallet_ready) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "开发者尚未完成首次钱包绑定。");
          snprintf(next_action, sizeof(next_action), "先完成钱包绑定，再进入阶段登记。");
        } else if (max_stage <= 0) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "项目已准备完毕，等待创建第一个阶段记录。");
          snprintf(next_action, sizeof(next_action), "开始登记工时或提交首个交付物。");
        } else if (strcmp(current_snapshot.state, "open") == 0 && !stage_waiting_client_confirmation(&current_snapshot)) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "开发者当前仍在阶段 %d 持续登记与修订。", current_snapshot.stage_no);
          snprintf(next_action, sizeof(next_action), "继续补充工时、交付物与阶段证据。");
        } else if (pending_client_count > 0) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "阶段 %d 的开发材料已准备完成，当前等待客户确认。", current_snapshot.stage_no);
          snprintf(next_action, sizeof(next_action), "如客户驳回，则继续在当前阶段修订。");
          snprintf(blocked_by, sizeof(blocked_by), "客户待确认阶段 %d", current_snapshot.stage_no);
        } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "客户已通过阶段 %d，当前等待管理员阶段审计。", current_snapshot.stage_no);
          snprintf(next_action, sizeof(next_action), "等待阶段审计结论，必要时根据驳回意见继续修订。");
          snprintf(blocked_by, sizeof(blocked_by), "管理员待阶段审计阶段 %d", current_snapshot.stage_no);
        } else {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "所有已开启阶段都已开发完成，等待最终总审计。");
          snprintf(next_action, sizeof(next_action), "等待管理员执行项目总审计。");
        }
      } else if (role_index == 1) {
        stage_no = max_stage;
        snprintf(title, sizeof(title), "客户 · %s", client_name[0] ? client_name : "未分配");
        if (strcmp(project_status, "completed") == 0) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "客户确认流程已全部结束，项目已完成归档。");
          snprintf(next_action, sizeof(next_action), "当前只保留历史回溯查看。");
        } else if (strcmp(project_status, "disputed") == 0) {
          snprintf(status, sizeof(status), "failed");
          snprintf(summary, sizeof(summary), "项目已进入争议状态，客户确认流程已结束。");
          snprintf(next_action, sizeof(next_action), "等待争议处理结果。");
        } else if (max_stage <= 0) {
          snprintf(status, sizeof(status), "locked");
          snprintf(summary, sizeof(summary), "项目尚未形成可确认阶段。");
          snprintf(next_action, sizeof(next_action), "等待开发者完成首轮阶段材料。");
          snprintf(blocked_by, sizeof(blocked_by), "开发者尚未提交可确认交付");
        } else if (pending_client_count > 0) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "阶段 %d 当前由客户执行确认。", current_snapshot.stage_no);
          snprintf(next_action, sizeof(next_action), "查看本阶段交付与工时历史后作出通过或驳回。");
        } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0 || strcmp(current_snapshot.state, "closed") == 0 || final_eligible) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "客户已完成最近阶段确认。");
          snprintf(next_action, sizeof(next_action), "等待后续阶段开启或进入最终总审计。");
        } else {
          snprintf(status, sizeof(status), "locked");
          snprintf(summary, sizeof(summary), "开发者仍在准备当前阶段交付。");
          snprintf(next_action, sizeof(next_action), "等待形成可确认交付后再进入客户确认。");
          snprintf(blocked_by, sizeof(blocked_by), "开发者未完成当前阶段交付");
        }
      } else {
        stage_no = max_stage;
        snprintf(title, sizeof(title), "管理员 · 审计总控");
        if (strcmp(project_status, "completed") == 0) {
          snprintf(status, sizeof(status), "completed");
          snprintf(summary, sizeof(summary), "该项目已完成最终总审计归档。");
          snprintf(next_action, sizeof(next_action), "当前只保留项目回溯与留痕查看能力。");
        } else if (strcmp(project_status, "disputed") == 0) {
          snprintf(status, sizeof(status), "failed");
          snprintf(summary, sizeof(summary), "最终总审计未通过，项目已进入争议状态。");
          snprintf(next_action, sizeof(next_action), "根据争议处理流程继续跟进。");
        } else if (max_stage <= 0) {
          snprintf(status, sizeof(status), "locked");
          snprintf(summary, sizeof(summary), "尚未形成可审计阶段。");
          snprintf(next_action, sizeof(next_action), "等待开发者和客户推动项目进入阶段闭环。");
          snprintf(blocked_by, sizeof(blocked_by), "项目尚未进入阶段循环");
        } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "阶段 %d 已准备好，管理员当前可执行阶段审计。", current_snapshot.stage_no);
          snprintf(next_action, sizeof(next_action), "执行通过或驳回审计，决定是否进入下一阶段。");
        } else if (final_eligible) {
          snprintf(status, sizeof(status), "running");
          snprintf(summary, sizeof(summary), "所有阶段已关闭，项目满足最终总审计条件。");
          snprintf(next_action, sizeof(next_action), "执行完整项目审核并决定归档或争议状态。");
        } else if (pending_client_count > 0) {
          snprintf(status, sizeof(status), "locked");
          snprintf(summary, sizeof(summary), "当前阶段仍在等待客户确认。");
          snprintf(next_action, sizeof(next_action), "等待客户通过后再执行阶段审计。");
          snprintf(blocked_by, sizeof(blocked_by), "客户尚未确认阶段 %d", current_snapshot.stage_no);
        } else {
          snprintf(status, sizeof(status), "locked");
          snprintf(summary, sizeof(summary), "开发者仍在持续推进当前阶段。");
          snprintf(next_action, sizeof(next_action), "等待当前阶段形成可审计状态。");
          snprintf(blocked_by, sizeof(blocked_by), "开发者未完成阶段 %d", current_snapshot.stage_no);
        }
      }

      json_escape(role_key, safe_role, sizeof(safe_role));
      json_escape(title, safe_title, sizeof(safe_title));
      json_escape(status, safe_status, sizeof(safe_status));
      json_escape(summary, safe_summary, sizeof(safe_summary));
      json_escape(next_action, safe_next_action, sizeof(safe_next_action));
      json_escape(blocked_by, safe_blocked_by, sizeof(safe_blocked_by));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"role\":\"%s\",\"title\":\"%s\",\"stageNo\":%d,\"status\":\"%s\",\"summary\":\"%s\",\"nextAction\":\"%s\",\"blockedBy\":\"%s\"}",
        first ? "" : ",",
        safe_role,
        safe_title,
        stage_no,
        safe_status,
        safe_summary,
        safe_next_action,
        safe_blocked_by
      );
      first = 0;
    }
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"stageSummaries\":[");
  if (project_exists_flag) {
    first = 1;
    for (step_index = 1; step_index <= max_stage; step_index++) {
      StageSnapshot snapshot;
      char safe_state[32], safe_version[64], safe_name[256], safe_summary[768], safe_approval[512], safe_audit_decision[32], safe_audit_comment[512];
      char phase[64], owner[32], requirement[256], next_action[256], blocking_reason[256];
      char safe_phase[128], safe_owner[64], safe_requirement[512], safe_next_action[512], safe_blocking_reason[512];
      char summary[768];
      int is_current;

      load_stage_snapshot(app->db, selected_project_id, step_index, &snapshot);
      is_current = step_index == max_stage;
      if (strcmp(snapshot.state, "closed") == 0) {
        snprintf(summary, sizeof(summary), "阶段 %d 已完成客户确认与管理员阶段审计。", snapshot.stage_no);
      } else if (strcmp(snapshot.state, "pending_stage_audit") == 0) {
        snprintf(summary, sizeof(summary), "阶段 %d 的客户确认已经完成，当前等待管理员阶段审计。", snapshot.stage_no);
      } else if (stage_waiting_client_confirmation(&snapshot)) {
        snprintf(summary, sizeof(summary), "阶段 %d 的开发材料已齐备，当前等待客户确认。", snapshot.stage_no);
      } else {
        snprintf(summary, sizeof(summary), "阶段 %d 仍在持续登记工时、交付与证据。", snapshot.stage_no);
      }

      snprintf(phase, sizeof(phase), "%s", stage_phase_label(&snapshot));
      snprintf(owner, sizeof(owner), "%s", stage_current_owner(&snapshot));
      build_stage_requirement_text(&snapshot, requirement, sizeof(requirement));
      build_stage_next_action_text(&snapshot, snapshot.stage_no, next_action, sizeof(next_action));
      build_stage_blocking_reason_text(&snapshot, snapshot.stage_no, blocking_reason, sizeof(blocking_reason));

      json_escape(snapshot.state, safe_state, sizeof(safe_state));
      json_escape(snapshot.latest_deliverable_version, safe_version, sizeof(safe_version));
      json_escape(snapshot.latest_deliverable_name, safe_name, sizeof(safe_name));
      json_escape(snapshot.latest_approval_comment, safe_approval, sizeof(safe_approval));
      json_escape(snapshot.latest_stage_audit_decision, safe_audit_decision, sizeof(safe_audit_decision));
      json_escape(snapshot.latest_stage_audit_comment, safe_audit_comment, sizeof(safe_audit_comment));
      json_escape(summary, safe_summary, sizeof(safe_summary));
      json_escape(phase, safe_phase, sizeof(safe_phase));
      json_escape(owner, safe_owner, sizeof(safe_owner));
      json_escape(requirement, safe_requirement, sizeof(safe_requirement));
      json_escape(next_action, safe_next_action, sizeof(safe_next_action));
      json_escape(blocking_reason, safe_blocking_reason, sizeof(safe_blocking_reason));

      off += (size_t) snprintf(
        body + off,
        sizeof(body) - off,
        "%s{\"stageNo\":%d,\"status\":\"%s\",\"worklogCount\":%d,\"deliverableCount\":%d,\"evidenceCount\":%d,\"latestDeliverableVersion\":\"%s\",\"latestDeliverableName\":\"%s\",\"approvalComment\":\"%s\",\"stageAuditDecision\":\"%s\",\"stageAuditComment\":\"%s\",\"summary\":\"%s\",\"isCurrent\":%d,\"phase\":\"%s\",\"currentOwner\":\"%s\",\"requirement\":\"%s\",\"nextAction\":\"%s\",\"blockingReason\":\"%s\",\"roleStates\":",
        first ? "" : ",",
        snapshot.stage_no,
        safe_state,
        snapshot.worklog_count,
        snapshot.deliverable_count,
        snapshot.evidence_count,
        safe_version,
        safe_name,
        safe_approval,
        safe_audit_decision,
        safe_audit_comment,
        safe_summary,
        is_current,
        safe_phase,
        safe_owner,
        safe_requirement,
        safe_next_action,
        safe_blocking_reason
      );
      append_stage_role_states(body, sizeof(body), &off, &snapshot);
      off += (size_t) snprintf(body + off, sizeof(body) - off, "}");
      first = 0;
    }
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"currentStage\":");
  if (project_exists_flag && max_stage > 0) {
    char safe_state[32], safe_summary[768], safe_phase[128], safe_owner[64], safe_requirement[512], safe_next_action[512], safe_blocking_reason[512];
    char summary[768], phase[64], owner[32], requirement[256], next_action[256], blocking_reason[256];

    if (strcmp(current_snapshot.state, "closed") == 0) {
      snprintf(summary, sizeof(summary), "最近阶段已闭环，下一次登记会自动进入新的阶段。");
    } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
      snprintf(summary, sizeof(summary), "最近阶段已通过客户确认，当前等待管理员阶段审计。");
    } else if (stage_waiting_client_confirmation(&current_snapshot)) {
      snprintf(summary, sizeof(summary), "最近阶段已形成交付，当前等待客户确认。");
    } else {
      snprintf(summary, sizeof(summary), "最近阶段仍在持续登记工时与交付物。");
    }

    snprintf(phase, sizeof(phase), "%s", stage_phase_label(&current_snapshot));
    snprintf(owner, sizeof(owner), "%s", stage_current_owner(&current_snapshot));
    build_stage_requirement_text(&current_snapshot, requirement, sizeof(requirement));
    build_stage_next_action_text(&current_snapshot, current_snapshot.stage_no, next_action, sizeof(next_action));
    build_stage_blocking_reason_text(&current_snapshot, current_snapshot.stage_no, blocking_reason, sizeof(blocking_reason));

    json_escape(current_snapshot.state, safe_state, sizeof(safe_state));
    json_escape(summary, safe_summary, sizeof(safe_summary));
    json_escape(phase, safe_phase, sizeof(safe_phase));
    json_escape(owner, safe_owner, sizeof(safe_owner));
    json_escape(requirement, safe_requirement, sizeof(safe_requirement));
    json_escape(next_action, safe_next_action, sizeof(safe_next_action));
    json_escape(blocking_reason, safe_blocking_reason, sizeof(safe_blocking_reason));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "{\"stageNo\":%d,\"status\":\"%s\",\"summary\":\"%s\",\"isCurrent\":1,\"phase\":\"%s\",\"currentOwner\":\"%s\",\"requirement\":\"%s\",\"nextAction\":\"%s\",\"blockingReason\":\"%s\",\"roleStates\":",
      current_snapshot.stage_no,
      safe_state,
      safe_summary,
      safe_phase,
      safe_owner,
      safe_requirement,
      safe_next_action,
      safe_blocking_reason
    );
    append_stage_role_states(body, sizeof(body), &off, &current_snapshot);
    off += (size_t) snprintf(body + off, sizeof(body) - off, "}");
  } else {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "null");
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, ",\"finalAuditStatus\":");
  if (project_exists_flag) {
    char final_summary[512], safe_summary[512], safe_status[32], safe_blocking_reason[512];
    char final_status[32], blocking_reason[512];
    int blocking_stage_no = 0;
    int requirement_has_active_project = project_active;
    int requirement_has_passed_stage = passed_stage_total > 0;
    int requirement_all_closed = max_stage > 0 && strcmp(current_snapshot.state, "closed") == 0;

    build_final_audit_hint(app->db, project_status, selected_project_id, final_summary, sizeof(final_summary));
    if (strcmp(project_status, "completed") == 0) {
      snprintf(final_status, sizeof(final_status), "completed");
        blocking_reason[0] = '\0';
    } else if (strcmp(project_status, "disputed") == 0) {
      snprintf(final_status, sizeof(final_status), "failed");
      snprintf(blocking_reason, sizeof(blocking_reason), "项目已经进入争议状态，无法再次执行最终总审计。");
    } else if (final_eligible) {
      snprintf(final_status, sizeof(final_status), "running");
        blocking_reason[0] = '\0';
    } else {
      snprintf(final_status, sizeof(final_status), "locked");
      if (max_stage <= 0) {
        snprintf(blocking_reason, sizeof(blocking_reason), "项目尚未进入任何阶段，最终总审计尚未解锁。");
      } else if (strcmp(current_snapshot.state, "pending_stage_audit") == 0) {
        blocking_stage_no = current_snapshot.stage_no;
        snprintf(blocking_reason, sizeof(blocking_reason), "阶段 %d 仍在等待管理员阶段审计。", current_snapshot.stage_no);
      } else if (pending_client_count > 0) {
        blocking_stage_no = current_snapshot.stage_no;
        snprintf(blocking_reason, sizeof(blocking_reason), "阶段 %d 仍在等待客户确认。", current_snapshot.stage_no);
      } else {
        blocking_stage_no = current_snapshot.stage_no;
        snprintf(blocking_reason, sizeof(blocking_reason), "当前仍有开放阶段，尚未满足最终总审计条件。");
      }
    }

    json_escape(final_summary, safe_summary, sizeof(safe_summary));
    json_escape(final_status, safe_status, sizeof(safe_status));
    json_escape(blocking_reason, safe_blocking_reason, sizeof(safe_blocking_reason));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "{\"eligible\":%d,\"status\":\"%s\",\"summary\":\"%s\",\"passedStageCount\":%d,\"blockingReason\":\"%s\",\"blockingStageNo\":%d,\"requirements\":[",
      final_eligible,
      safe_status,
      safe_summary,
      passed_stage_total,
      safe_blocking_reason,
      blocking_stage_no
    );
    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "{\"label\":\"项目仍处于进行中\",\"met\":%d},"
      "{\"label\":\"至少完成一个已闭环阶段\",\"met\":%d},"
      "{\"label\":\"当前不存在待客户确认或待阶段审计阶段\",\"met\":%d}"
      "]}",
      requirement_has_active_project,
      requirement_has_passed_stage,
      requirement_all_closed
    );
  } else {
    off += (size_t) snprintf(body + off, sizeof(body) - off, "null");
  }

  snprintf(body + off, sizeof(body) - off, "}");
  send_response(client, 200, "OK", body);
}

static void handle_dashboard(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  sqlite3_stmt *stmt = NULL;
  int first = 1;
  int metric_a = 0;
  int metric_b = 0;
  int metric_c = 0;
  int metric_d = 0;
  const char *headline = "";
  const char *subline = "";

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }

  if (is_admin(&user)) {
    sqlite3_prepare_v2(app->db, "SELECT id, status FROM projects ORDER BY id DESC", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int project_id = sqlite3_column_int(stmt, 0);
      const char *project_status = (const char *) sqlite3_column_text(stmt, 1);
      metric_a++;
      metric_b += project_pending_client_stage_count(app->db, project_id);
      metric_c += project_pending_stage_audit_count(app->db, project_id);
      metric_d += project_can_run_final_audit(app->db, project_status, project_id);
    }
    sqlite3_finalize(stmt);
    headline = "管理员阶段总览";
    subline = "查看待客户确认、待阶段审计与待总审计项目。";
  } else if (is_developer(&user)) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM project_members WHERE user_id = %d", user.id);
    metric_a = scalar_int(app->db, sql);
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM worklogs WHERE user_id = %d", user.id);
    metric_b = scalar_int(app->db, sql);
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM deliverables WHERE user_id = %d", user.id);
    metric_c = scalar_int(app->db, sql);
    sqlite3_prepare_v2(
      app->db,
      "SELECT DISTINCT p.id FROM projects p "
      "JOIN project_members pm ON pm.project_id = p.id "
      "WHERE pm.user_id = ? ORDER BY p.id DESC",
      -1,
      &stmt,
      NULL
    );
    sqlite3_bind_int(stmt, 1, user.id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int blocked_by_stage_audit = 0;
      project_current_stage_for_write(app->db, sqlite3_column_int(stmt, 0), &blocked_by_stage_audit);
      metric_d += blocked_by_stage_audit ? 1 : 0;
    }
    sqlite3_finalize(stmt);
    headline = "开发者阶段工作台";
    subline = "聚焦当前阶段的工时、交付、客户确认与管理员阶段审计状态。";
  } else {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM project_members WHERE user_id = %d", user.id);
    metric_a = scalar_int(app->db, sql);
    sqlite3_prepare_v2(app->db, "SELECT id FROM projects WHERE client_id = ? ORDER BY id DESC", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user.id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int project_id = sqlite3_column_int(stmt, 0);
      metric_b += project_pending_client_stage_count(app->db, project_id);
      metric_c += passed_stage_count(app->db, project_id);
      metric_d += project_pending_stage_audit_count(app->db, project_id);
    }
    sqlite3_finalize(stmt);
    headline = "客户阶段确认面板";
    subline = "查看待确认阶段，结合工时历史判断是否通过当前阶段交付。";
  }

  off += (size_t) snprintf(
    body + off,
    sizeof(body) - off,
    "{\"headline\":\"%s\",\"subline\":\"%s\",\"metrics\":["
    "{\"label\":\"%s\",\"value\":%d,\"description\":\"%s\"},"
    "{\"label\":\"%s\",\"value\":%d,\"description\":\"%s\"},"
    "{\"label\":\"%s\",\"value\":%d,\"description\":\"%s\"},"
    "{\"label\":\"%s\",\"value\":%d,\"description\":\"%s\"}"
    "],\"pendingEvidence\":[",
    headline,
    subline,
    "项目数",
    metric_a,
    is_admin(&user) ? "当前系统内项目数量" : (is_developer(&user) ? "当前开发者可见项目数" : "当前客户可见项目数"),
    is_admin(&user) ? "待客户确认阶段" : (is_developer(&user) ? "我的工时" : "待确认阶段"),
    metric_b,
    is_admin(&user) ? "仍等待客户确认的阶段数量" : (is_developer(&user) ? "当前账号的工时记录总数" : "仍待客户确认的阶段数量"),
    is_admin(&user) ? "待阶段审计" : (is_developer(&user) ? "我的交付物" : "已通过阶段"),
    metric_c,
    is_admin(&user) ? "客户已通过、等待管理员阶段审计的阶段数量" : (is_developer(&user) ? "当前账号的交付物总数" : "已经完成管理员阶段审计的阶段数"),
    is_admin(&user) ? "待总审计" : (is_developer(&user) ? "待阶段审计项目" : "待管理员阶段审计"),
    metric_d,
    is_admin(&user) ? "当前已满足总审计条件的活跃项目数" : (is_developer(&user) ? "当前被管理员阶段审计阻塞的项目数" : "客户已通过、等待管理员阶段审计的阶段数")
  );

  if (is_client_role(&user)) {
    sqlite3_prepare_v2(app->db,
      "SELECT d.id, p.name, d.version, d.status, d.summary "
      "FROM deliverables d JOIN projects p ON p.id = d.project_id "
      "WHERE p.client_id = ? AND d.status IN ('uploaded','notarized') "
      "ORDER BY d.id DESC LIMIT 6",
      -1,
      &stmt,
      NULL);
    sqlite3_bind_int(stmt, 1, user.id);
  } else if (is_developer(&user)) {
    sqlite3_prepare_v2(app->db,
      "SELECT w.id, p.name, w.task_description, w.evidence_status, w.work_date "
      "FROM worklogs w JOIN projects p ON p.id = w.project_id "
      "WHERE w.user_id = ? "
      "ORDER BY w.id DESC LIMIT 6",
      -1,
      &stmt,
      NULL);
    sqlite3_bind_int(stmt, 1, user.id);
  } else {
    sqlite3_prepare_v2(app->db,
      "SELECT d.id, p.name, d.version, d.status, d.summary "
      "FROM deliverables d JOIN projects p ON p.id = d.project_id "
      "WHERE d.status IN ('uploaded','notarized','approved') "
      "ORDER BY d.id DESC LIMIT 6",
      -1,
      &stmt,
      NULL);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char project_name[256], title[256], status[64], description[512];
    json_escape((const char *) sqlite3_column_text(stmt, 1), project_name, sizeof(project_name));
    json_escape((const char *) sqlite3_column_text(stmt, 2), title, sizeof(title));
    json_escape((const char *) sqlite3_column_text(stmt, 3), status, sizeof(status));
    json_escape((const char *) sqlite3_column_text(stmt, 4), description, sizeof(description));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"projectName\":\"%s\",\"title\":\"%s\",\"status\":\"%s\",\"description\":\"%s\"}",
      first ? "" : ",",
      sqlite3_column_int(stmt, 0),
      project_name,
      title,
      status,
      description
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"recentTransactions\":[");
  first = 1;

  if (is_admin(&user)) {
    sqlite3_prepare_v2(app->db, "SELECT e.id, e.business_type, e.tx_hash, e.status, e.created_at FROM evidence_records e ORDER BY e.id DESC LIMIT 8", -1, &stmt, NULL);
  } else if (is_developer(&user)) {
    sqlite3_prepare_v2(app->db, "SELECT e.id, e.business_type, e.tx_hash, e.status, e.created_at FROM evidence_records e WHERE e.actor_id = ? ORDER BY e.id DESC LIMIT 8", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user.id);
  } else {
    sqlite3_prepare_v2(app->db, "SELECT e.id, e.business_type, e.tx_hash, e.status, e.created_at FROM evidence_records e WHERE e.actor_id = ? OR e.business_type = 'deliverable_confirmation' ORDER BY e.id DESC LIMIT 8", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user.id);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char type[64], tx_hash[128], status[64], created_at[64];
    json_escape((const char *) sqlite3_column_text(stmt, 1), type, sizeof(type));
    json_escape((const char *) sqlite3_column_text(stmt, 2), tx_hash, sizeof(tx_hash));
    json_escape((const char *) sqlite3_column_text(stmt, 3), status, sizeof(status));
    json_escape((const char *) sqlite3_column_text(stmt, 4), created_at, sizeof(created_at));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"type\":\"%s\",\"txHash\":\"%s\",\"status\":\"%s\",\"createdAt\":\"%s\"}",
      first ? "" : ",",
      sqlite3_column_int(stmt, 0),
      type,
      tx_hash,
      status,
      created_at
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  snprintf(body + off, sizeof(body) - off, "]}");
  send_response(client, 200, "OK", body);
}

static void handle_audit(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;
  sqlite3_stmt *stmt = NULL;
  char body[MAX_RESPONSE_SIZE];
  size_t off = 0;
  int first = 1;

  if (!ensure_authenticated(app, client, request, &user)) {
    return;
  }
  if (!is_admin(&user)) {
    send_message(client, 403, "Forbidden", "只有管理员可以查看审计数据");
    return;
  }

  off += (size_t) snprintf(body + off, sizeof(body) - off, "{\"transactions\":[");
  sqlite3_prepare_v2(app->db, "SELECT id, business_type, tx_hash, status, comment, created_at FROM evidence_records ORDER BY id DESC LIMIT 12", -1, &stmt, NULL);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char type[64], tx_hash[128], status[64], comment[512], created_at[64];
    json_escape((const char *) sqlite3_column_text(stmt, 1), type, sizeof(type));
    json_escape((const char *) sqlite3_column_text(stmt, 2), tx_hash, sizeof(tx_hash));
    json_escape((const char *) sqlite3_column_text(stmt, 3), status, sizeof(status));
    json_escape((const char *) sqlite3_column_text(stmt, 4), comment, sizeof(comment));
    json_escape((const char *) sqlite3_column_text(stmt, 5), created_at, sizeof(created_at));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"type\":\"%s\",\"txHash\":\"%s\",\"status\":\"%s\",\"comment\":\"%s\",\"createdAt\":\"%s\"}",
      first ? "" : ",",
      sqlite3_column_int(stmt, 0),
      type,
      tx_hash,
      status,
      comment,
      created_at
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"logs\":[");
  first = 1;
  sqlite3_prepare_v2(app->db, "SELECT a.id, COALESCE(u.name, '系统'), a.action, a.entity_type, a.detail, a.created_at FROM audit_logs a LEFT JOIN users u ON u.id = a.actor_id ORDER BY a.id DESC LIMIT 14", -1, &stmt, NULL);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char actor_name[128], action[128], entity_type[64], detail[512], created_at[64];
    json_escape((const char *) sqlite3_column_text(stmt, 1), actor_name, sizeof(actor_name));
    json_escape((const char *) sqlite3_column_text(stmt, 2), action, sizeof(action));
    json_escape((const char *) sqlite3_column_text(stmt, 3), entity_type, sizeof(entity_type));
    json_escape((const char *) sqlite3_column_text(stmt, 4), detail, sizeof(detail));
    json_escape((const char *) sqlite3_column_text(stmt, 5), created_at, sizeof(created_at));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"actorName\":\"%s\",\"action\":\"%s\",\"entityType\":\"%s\",\"detail\":\"%s\",\"createdAt\":\"%s\"}",
      first ? "" : ",",
      sqlite3_column_int(stmt, 0),
      actor_name,
      action,
      entity_type,
      detail,
      created_at
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"stageQueue\":[");
  first = 1;
  sqlite3_prepare_v2(app->db, "SELECT id, name, status FROM projects WHERE status = 'active' ORDER BY id DESC", -1, &stmt, NULL);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int project_id = sqlite3_column_int(stmt, 0);
    StageSnapshot snapshot;
    char safe_name[256], safe_version[128], safe_file_name[256], safe_comment[512];
    int max_stage = project_max_stage_no(app->db, project_id);

    if (max_stage <= 0) {
      continue;
    }
    load_stage_snapshot(app->db, project_id, max_stage, &snapshot);
    if (strcmp(snapshot.state, "pending_stage_audit") != 0) {
      continue;
    }

    json_escape((const char *) sqlite3_column_text(stmt, 1), safe_name, sizeof(safe_name));
    json_escape(snapshot.latest_deliverable_version, safe_version, sizeof(safe_version));
    json_escape(snapshot.latest_deliverable_name, safe_file_name, sizeof(safe_file_name));
    json_escape(snapshot.latest_approval_comment, safe_comment, sizeof(safe_comment));
    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"projectId\":%d,\"projectName\":\"%s\",\"stageNo\":%d,\"worklogCount\":%d,\"deliverableCount\":%d,\"latestDeliverableVersion\":\"%s\",\"latestDeliverableName\":\"%s\",\"approvalComment\":\"%s\"}",
      first ? "" : ",",
      project_id,
      safe_name,
      snapshot.stage_no,
      snapshot.worklog_count,
      snapshot.deliverable_count,
      safe_version,
      safe_file_name,
      safe_comment
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"finalQueue\":[");
  first = 1;
  sqlite3_prepare_v2(app->db, "SELECT id, name, status FROM projects WHERE status = 'active' ORDER BY id DESC", -1, &stmt, NULL);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int project_id = sqlite3_column_int(stmt, 0);
    const char *project_status = (const char *) sqlite3_column_text(stmt, 2);
    char safe_name[256], safe_hint[256], final_audit_hint[256];
    int max_stage = project_max_stage_no(app->db, project_id);

    if (!project_can_run_final_audit(app->db, project_status, project_id)) {
      continue;
    }
    json_escape((const char *) sqlite3_column_text(stmt, 1), safe_name, sizeof(safe_name));
    build_final_audit_hint(app->db, project_status, project_id, final_audit_hint, sizeof(final_audit_hint));
    json_escape(final_audit_hint, safe_hint, sizeof(safe_hint));
    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"projectId\":%d,\"projectName\":\"%s\",\"passedStageCount\":%d,\"lastStageNo\":%d,\"summary\":\"%s\"}",
      first ? "" : ",",
      project_id,
      safe_name,
      passed_stage_count(app->db, project_id),
      max_stage,
      safe_hint
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  off += (size_t) snprintf(body + off, sizeof(body) - off, "],\"projects\":[");
  first = 1;
  sqlite3_prepare_v2(
    app->db,
    "SELECT p.id, p.name, p.status, COUNT(DISTINCT w.id), COUNT(DISTINCT d.id) "
    "FROM projects p "
    "LEFT JOIN worklogs w ON w.project_id = p.id "
    "LEFT JOIN deliverables d ON d.project_id = p.id "
    "GROUP BY p.id "
    "ORDER BY p.id DESC",
    -1,
    &stmt,
    NULL
  );
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int project_id = sqlite3_column_int(stmt, 0);
    const char *project_status = (const char *) sqlite3_column_text(stmt, 2);
    int max_stage = project_max_stage_no(app->db, project_id);
    StageSnapshot snapshot;
    char safe_name[256], safe_status[64], final_audit_hint[256], safe_hint[256], safe_stage_state[32];

    memset(&snapshot, 0, sizeof(snapshot));
    if (max_stage > 0) {
      load_stage_snapshot(app->db, project_id, max_stage, &snapshot);
    }
    json_escape((const char *) sqlite3_column_text(stmt, 1), safe_name, sizeof(safe_name));
    json_escape(project_status, safe_status, sizeof(safe_status));
    build_final_audit_hint(app->db, project_status, project_id, final_audit_hint, sizeof(final_audit_hint));
    json_escape(final_audit_hint, safe_hint, sizeof(safe_hint));
    json_escape(max_stage > 0 ? snapshot.state : "none", safe_stage_state, sizeof(safe_stage_state));

    off += (size_t) snprintf(
      body + off,
      sizeof(body) - off,
      "%s{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\",\"worklogCount\":%d,\"deliverableCount\":%d,\"stageCount\":%d,\"passedStageCount\":%d,\"pendingStageAuditCount\":%d,\"pendingClientStageCount\":%d,\"currentStageNo\":%d,\"currentStageState\":\"%s\",\"finalAuditEligible\":%d,\"finalAuditHint\":\"%s\"}",
      first ? "" : ",",
      project_id,
      safe_name,
      safe_status,
      sqlite3_column_int(stmt, 3),
      sqlite3_column_int(stmt, 4),
      max_stage,
      passed_stage_count(app->db, project_id),
      project_pending_stage_audit_count(app->db, project_id),
      project_pending_client_stage_count(app->db, project_id),
      max_stage,
      safe_stage_state,
      project_can_run_final_audit(app->db, project_status, project_id),
      safe_hint
    );
    first = 0;
  }
  sqlite3_finalize(stmt);

  snprintf(body + off, sizeof(body) - off, "]}");
  send_response(client, 200, "OK", body);
}

static void route_request(AppContext *app, SOCKET client, const HttpRequest *request) {
  CurrentUser user;

  if (strcmp(request->method, "OPTIONS") == 0) {
    send_message(client, 200, "OK", "ok");
  } else if (strcmp(request->method, "GET") == 0 && strncmp(request->path, "/uploads/", 9) == 0) {
    serve_upload(client, request);
  } else if (strcmp(request->path, "/api/health") == 0 && strcmp(request->method, "GET") == 0) {
    send_message(client, 200, "OK", "backend running");
  } else if (strcmp(request->path, "/api/auth/login") == 0 && strcmp(request->method, "POST") == 0) {
    handle_auth_login(app, client, request);
  } else if (strcmp(request->path, "/api/auth/quick-accounts") == 0 && strcmp(request->method, "GET") == 0) {
    handle_auth_quick_accounts(app, client);
  } else if (strcmp(request->path, "/api/auth/me") == 0 && strcmp(request->method, "GET") == 0) {
    handle_auth_me(app, client, request);
  } else if (strcmp(request->path, "/api/auth/profile") == 0 && strcmp(request->method, "PUT") == 0) {
    handle_auth_profile(app, client, request);
  } else if (strcmp(request->path, "/api/auth/password") == 0 && strcmp(request->method, "PUT") == 0) {
    handle_auth_password(app, client, request);
  } else if (strcmp(request->path, "/api/auth/avatar") == 0 && strcmp(request->method, "POST") == 0) {
    handle_auth_avatar(app, client, request);
  } else if (strcmp(request->path, "/api/auth/logout") == 0 && strcmp(request->method, "POST") == 0) {
    handle_auth_logout(app, client, request);
  } else if (strcmp(request->path, "/api/auth/wallet") == 0 && strcmp(request->method, "POST") == 0) {
    handle_bind_wallet(app, client, request);
  } else if (strcmp(request->path, "/api/users/password/reset") == 0 && strcmp(request->method, "POST") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_user_password_reset(app, client, request, &user);
  } else if (strcmp(request->path, "/api/users/status") == 0 && strcmp(request->method, "POST") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_user_status(app, client, request, &user);
  } else if (path_matches(request->path, "/api/users") && strcmp(request->method, "GET") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_users_get(app, client, request, &user);
  } else if (strcmp(request->path, "/api/users") == 0 && strcmp(request->method, "POST") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_users_create(app, client, request, &user);
  } else if (strcmp(request->path, "/api/users") == 0 && strcmp(request->method, "PUT") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_users_update(app, client, request, &user);
  } else if (path_matches(request->path, "/api/users") && strcmp(request->method, "DELETE") == 0) {
    if (!ensure_authenticated(app, client, request, &user)) {
      return;
    }
    handle_user_delete(app, client, request, &user);
  } else if (strcmp(request->path, "/api/projects") == 0) {
    handle_projects(app, client, request);
  } else if (strcmp(request->path, "/api/worklogs") == 0) {
    handle_worklogs(app, client, request);
  } else if (strcmp(request->path, "/api/deliverables") == 0) {
    handle_deliverables(app, client, request);
  } else if (strcmp(request->path, "/api/evidence") == 0 && strcmp(request->method, "POST") == 0) {
    handle_evidence(app, client, request);
  } else if (path_matches(request->path, "/api/process-flow") && strcmp(request->method, "GET") == 0) {
    handle_process_flow(app, client, request);
  } else if ((strcmp(request->path, "/api/dashboard") == 0 || strcmp(request->path, "/api/admin/overview") == 0) && strcmp(request->method, "GET") == 0) {
    handle_dashboard(app, client, request);
  } else if (strcmp(request->path, "/api/audit/stage") == 0 && strcmp(request->method, "POST") == 0) {
    handle_stage_audit(app, client, request);
  } else if (strcmp(request->path, "/api/audit/final") == 0 && strcmp(request->method, "POST") == 0) {
    handle_final_audit(app, client, request);
  } else if (strcmp(request->path, "/api/audit") == 0 && strcmp(request->method, "GET") == 0) {
    handle_audit(app, client, request);
  } else {
    send_message(client, 404, "Not Found", "接口不存在");
  }
}

int main(void) {
  WSADATA wsa_data;
  SOCKET server_socket;
  struct sockaddr_in server_addr;
  AppContext app;

  if (!init_database(&app.db)) {
    fprintf(stderr, "Failed to initialize database\n");
    return 1;
  }

  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    sqlite3_close(app.db);
    return 1;
  }

  server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket == INVALID_SOCKET) {
    fprintf(stderr, "Failed to create socket\n");
    sqlite3_close(app.db);
    WSACleanup();
    return 1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR ||
      listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
    fprintf(stderr, "Failed to bind/listen on port %d\n", SERVER_PORT);
    closesocket(server_socket);
    sqlite3_close(app.db);
    WSACleanup();
    return 1;
  }

  printf("Trust Work backend running at http://localhost:%d\n", SERVER_PORT);
  while (1) {
    SOCKET client = accept(server_socket, NULL, NULL);
    if (client != INVALID_SOCKET) {
      HttpRequest request;
      if (recv_request(client, &request)) {
        route_request(&app, client, &request);
      } else {
        send_message(client, 400, "Bad Request", "请求格式错误");
      }
      closesocket(client);
    }
  }

  closesocket(server_socket);
  sqlite3_close(app.db);
  WSACleanup();
  return 0;
}
