#include <windows.h>
#include <winhttp.h>

#include <stdio.h>
#include <string.h>

#include "../../include/chain_rpc.h"

static int utf8_to_wide(const char *input, wchar_t *output, int output_count) {
  int converted;

  if (!input || !output || output_count <= 0) {
    return 0;
  }
  converted = MultiByteToWideChar(CP_UTF8, 0, input, -1, output, output_count);
  return converted > 0;
}

int chain_rpc_post(const char *url, const char *body, char *response, size_t response_size) {
  wchar_t wide_url[2048];
  wchar_t host[512];
  wchar_t path[1536];
  wchar_t extra[512];
  URL_COMPONENTS parts;
  HINTERNET session = NULL;
  HINTERNET connection = NULL;
  HINTERNET request = NULL;
  DWORD flags = 0;
  DWORD bytes_available = 0;
  size_t total = 0;
  int ok = 0;

  if (!url || !body || !response || response_size < 2 || !utf8_to_wide(url, wide_url, (int) (sizeof(wide_url) / sizeof(wide_url[0])))) {
    return 0;
  }

  memset(&parts, 0, sizeof(parts));
  parts.dwStructSize = sizeof(parts);
  parts.lpszHostName = host;
  parts.dwHostNameLength = (DWORD) (sizeof(host) / sizeof(host[0]));
  parts.lpszUrlPath = path;
  parts.dwUrlPathLength = (DWORD) (sizeof(path) / sizeof(path[0]));
  parts.lpszExtraInfo = extra;
  parts.dwExtraInfoLength = (DWORD) (sizeof(extra) / sizeof(extra[0]));
  parts.dwSchemeLength = (DWORD) -1;
  if (!WinHttpCrackUrl(wide_url, 0, 0, &parts) || parts.dwHostNameLength == 0) {
    return 0;
  }
  if (parts.dwHostNameLength >= sizeof(host) / sizeof(host[0]) ||
      parts.dwUrlPathLength >= sizeof(path) / sizeof(path[0]) ||
      parts.dwExtraInfoLength >= sizeof(extra) / sizeof(extra[0])) {
    return 0;
  }
  host[parts.dwHostNameLength] = L'\0';
  path[parts.dwUrlPathLength] = L'\0';
  extra[parts.dwExtraInfoLength] = L'\0';
  if (parts.nScheme == INTERNET_SCHEME_HTTPS) {
    flags = WINHTTP_FLAG_SECURE;
  } else if (parts.nScheme != INTERNET_SCHEME_HTTP) {
    return 0;
  }

  if (parts.dwUrlPathLength == 0) {
    path[0] = L'/';
    path[1] = L'\0';
  }
  if (parts.dwExtraInfoLength > 0 && wcslen(path) + wcslen(extra) < (sizeof(path) / sizeof(path[0])) - 1) {
    wcscat(path, extra);
  }

  session = WinHttpOpen(L"TrustWorkBackend/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) {
    goto cleanup;
  }
  WinHttpSetTimeouts(session, 5000, 5000, 8000, 12000);
  connection = WinHttpConnect(session, host, parts.nPort, 0);
  if (!connection) {
    goto cleanup;
  }
  request = WinHttpOpenRequest(connection, L"POST", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!request) {
    goto cleanup;
  }
  if (!WinHttpSendRequest(
        request,
        L"Content-Type: application/json\r\n",
        (DWORD) -1,
        (LPVOID) body,
        (DWORD) strlen(body),
        (DWORD) strlen(body),
        0
      ) || !WinHttpReceiveResponse(request, NULL)) {
    goto cleanup;
  }

  while (WinHttpQueryDataAvailable(request, &bytes_available) && bytes_available > 0) {
    DWORD bytes_read = 0;
    DWORD chunk = bytes_available;
    if (total + chunk >= response_size) {
      chunk = (DWORD) (response_size - total - 1);
    }
    if (chunk == 0 || !WinHttpReadData(request, response + total, chunk, &bytes_read)) {
      goto cleanup;
    }
    total += bytes_read;
    response[total] = '\0';
    if (chunk < bytes_available || total + 1 >= response_size) {
      goto cleanup;
    }
  }
  response[total] = '\0';
  ok = total > 0;

cleanup:
  if (request) {
    WinHttpCloseHandle(request);
  }
  if (connection) {
    WinHttpCloseHandle(connection);
  }
  if (session) {
    WinHttpCloseHandle(session);
  }
  if (!ok) {
    response[0] = '\0';
  }
  return ok;
}
