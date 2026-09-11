#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include "retranslation_format.h"

/* C Runtime declarations from msvcrt */
int __cdecl _vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
int __cdecl _snprintf(char *buffer, size_t count, const char *format, ...);
int __cdecl memcmp(const void *buf1, const void *buf2, size_t count);
void* __cdecl memcpy(void *dest, const void *src, size_t count);
int __cdecl strcmp(const char *string1, const char *string2);
int __cdecl _stricmp(const char *string1, const char *string2);
char* __cdecl strrchr(const char *str, int c);
char* __cdecl strstr(const char *str, const char *strSearch);

/* ========================================================================= */
/* Logging                                                                   */
/* ========================================================================= */
static HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_LogCs;

static void InitLog(const char* dir) {
    InitializeCriticalSection(&g_LogCs);
    char logPath[MAX_PATH];
    _snprintf(logPath, sizeof(logPath), "%s\\retranslation_hook.log", dir);
    g_hLogFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static void Log(const char* fmt, ...) {
    if (g_hLogFile == INVALID_HANDLE_VALUE) return;
    EnterCriticalSection(&g_LogCs);
    char buf[1024];
    va_list va;
    va_start(va, fmt);
    int len = _vsnprintf(buf, sizeof(buf) - 1, fmt, va);
    va_end(va);
    if (len > 0) {
        DWORD written = 0;
        WriteFile(g_hLogFile, buf, (DWORD)len, &written, NULL);
        FlushFileBuffers(g_hLogFile);
    }
    LeaveCriticalSection(&g_LogCs);
}

/* ========================================================================= */
/* System version.dll Export Forwarding                                      */
/* ========================================================================= */
static HMODULE g_hRealVersion = NULL;

static void EnsureRealVersion(void) {
    if (g_hRealVersion) return;
    char sysDir[MAX_PATH];
    if (GetSystemDirectoryA(sysDir, MAX_PATH) > 0) {
        char fullPath[MAX_PATH];
        _snprintf(fullPath, sizeof(fullPath), "%s\\version.dll", sysDir);
        g_hRealVersion = LoadLibraryA(fullPath);
    }
    if (!g_hRealVersion) {
        /* Fallback for Wine / Proton */
        g_hRealVersion = LoadLibraryA("C:\\windows\\system32\\version.dll");
    }
}

#define FORWARD_EXPORT(ret_t, name, params, args) \
    ret_t WINAPI name params { \
        EnsureRealVersion(); \
        typedef ret_t (WINAPI *pfn_t) params; \
        pfn_t pfn = (pfn_t)GetProcAddress(g_hRealVersion, #name); \
        if (pfn) return pfn args; \
        return (ret_t)0; \
    }

FORWARD_EXPORT(BOOL, GetFileVersionInfoA, (LPCSTR f, DWORD h, DWORD l, LPVOID d), (f, h, l, d))
FORWARD_EXPORT(BOOL, GetFileVersionInfoByHandle, (DWORD h, LPCWSTR f, LPVOID d, DWORD l), (h, f, d, l))
FORWARD_EXPORT(BOOL, GetFileVersionInfoExA, (DWORD flags, LPCSTR f, DWORD h, DWORD l, LPVOID d), (flags, f, h, l, d))
FORWARD_EXPORT(BOOL, GetFileVersionInfoExW, (DWORD flags, LPCWSTR f, DWORD h, DWORD l, LPVOID d), (flags, f, h, l, d))
FORWARD_EXPORT(DWORD, GetFileVersionInfoSizeA, (LPCSTR f, LPDWORD h), (f, h))
FORWARD_EXPORT(DWORD, GetFileVersionInfoSizeExA, (DWORD flags, LPCSTR f, LPDWORD h), (flags, f, h))
FORWARD_EXPORT(DWORD, GetFileVersionInfoSizeExW, (DWORD flags, LPCWSTR f, LPDWORD h), (flags, f, h))
FORWARD_EXPORT(DWORD, GetFileVersionInfoSizeW, (LPCWSTR f, LPDWORD h), (f, h))
FORWARD_EXPORT(BOOL, GetFileVersionInfoW, (LPCWSTR f, DWORD h, DWORD l, LPVOID d), (f, h, l, d))
FORWARD_EXPORT(DWORD, VerFindFileA, (DWORD u, LPCSTR a, LPCSTR b, LPCSTR c, LPSTR d, PUINT e, LPSTR f, PUINT g), (u, a, b, c, d, e, f, g))
FORWARD_EXPORT(DWORD, VerFindFileW, (DWORD u, LPCWSTR a, LPCWSTR b, LPCWSTR c, LPWSTR d, PUINT e, LPWSTR f, PUINT g), (u, a, b, c, d, e, f, g))
FORWARD_EXPORT(DWORD, VerInstallFileA, (DWORD u, LPCSTR a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPSTR f, PUINT g), (u, a, b, c, d, e, f, g))
FORWARD_EXPORT(DWORD, VerInstallFileW, (DWORD u, LPCWSTR a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPWSTR f, PUINT g), (u, a, b, c, d, e, f, g))
FORWARD_EXPORT(DWORD, VerLanguageNameA, (DWORD lang, LPSTR buf, DWORD size), (lang, buf, size))
FORWARD_EXPORT(DWORD, VerLanguageNameW, (DWORD lang, LPWSTR buf, DWORD size), (lang, buf, size))
FORWARD_EXPORT(BOOL, VerQueryValueA, (LPCVOID block, LPCSTR sub, LPVOID *buf, PUINT len), (block, sub, buf, len))
FORWARD_EXPORT(BOOL, VerQueryValueW, (LPCVOID block, LPCWSTR sub, LPVOID *buf, PUINT len), (block, sub, buf, len))

/* ========================================================================= */
/* Translation Resource Loader (retranslation.dat)                           */
/* ========================================================================= */
static char g_DllDir[MAX_PATH] = {0};
static const IFTRHeader* g_Header = NULL;
static const IFTREntry* g_Entries = NULL;
static const char* g_StringPool = NULL;
static uint8_t* g_DatBuffer = NULL;

static bool LoadRetranslationDat(void) {
    char datPath[MAX_PATH];
    _snprintf(datPath, sizeof(datPath), "%s\\retranslation.dat", g_DllDir);
    Log("Opening retranslation resource: %s\n", datPath);

    HANDLE hFile = CreateFileA(datPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        Log("Error: Could not open %s (error=%lu)\n", datPath, GetLastError());
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize < sizeof(IFTRHeader)) {
        Log("Error: retranslation.dat is too small (%lu bytes)\n", fileSize);
        CloseHandle(hFile);
        return false;
    }

    g_DatBuffer = (uint8_t*)VirtualAlloc(NULL, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!g_DatBuffer) {
        Log("Error: Failed to allocate %lu bytes for retranslation.dat\n", fileSize);
        CloseHandle(hFile);
        return false;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, g_DatBuffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        Log("Error: Failed to read retranslation.dat completely\n");
        VirtualFree(g_DatBuffer, 0, MEM_RELEASE);
        g_DatBuffer = NULL;
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    g_Header = (const IFTRHeader*)g_DatBuffer;
    if (memcmp(g_Header->magic, "IFTR", 4) != 0 || g_Header->version != 1) {
        Log("Error: Invalid IFTR magic or version in retranslation.dat\n");
        VirtualFree(g_DatBuffer, 0, MEM_RELEASE);
        g_DatBuffer = NULL;
        return false;
    }

    g_Entries = (const IFTREntry*)(g_DatBuffer + sizeof(IFTRHeader));
    g_StringPool = (const char*)(g_DatBuffer + sizeof(IFTRHeader) + g_Header->entry_count * sizeof(IFTREntry));

    Log("Successfully loaded retranslation.dat: %u entries, %u bytes string pool\n",
        g_Header->entry_count, g_Header->pool_size);
    return true;
}

static const IFTREntry* FindTranslation(uint16_t scene_id, uint16_t line_id) {
    if (!g_Header || !g_Entries) return NULL;
    int low = 0;
    int high = (int)g_Header->entry_count - 1;
    uint32_t target = ((uint32_t)scene_id << 16) | line_id;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        uint32_t mid_val = ((uint32_t)g_Entries[mid].scene_id << 16) | g_Entries[mid].line_id;
        if (mid_val == target) return &g_Entries[mid];
        if (mid_val < target) low = mid + 1;
        else high = mid - 1;
    }
    return NULL;
}

static bool ParseId(const wchar_t* wchars, int wlen, uint16_t* out_scene, uint16_t* out_line) {
    if (!wchars || wlen < 3) return false;
    int hyphen = -1;
    for (int i = 0; i < wlen; i++) {
        if (wchars[i] == L'-') {
            hyphen = i;
            break;
        }
    }
    if (hyphen <= 0 || hyphen >= wlen - 1) return false;
    uint32_t s = 0, l = 0;
    for (int i = 0; i < hyphen; i++) {
        if (wchars[i] < L'0' || wchars[i] > L'9') return false;
        s = s * 10 + (wchars[i] - L'0');
    }
    for (int i = hyphen + 1; i < wlen; i++) {
        if (wchars[i] < L'0' || wchars[i] > L'9') return false;
        l = l * 10 + (wchars[i] - L'0');
    }
    if (s > 65535 || l > 65535) return false;
    *out_scene = (uint16_t)s;
    *out_line = (uint16_t)l;
    return true;
}

/* ========================================================================= */
/* IL2CPP Declarations & Function Pointers                                    */
/* ========================================================================= */
typedef void* (*pfn_il2cpp_domain_get_t)(void);
typedef void** (*pfn_il2cpp_domain_get_assemblies_t)(void* domain, size_t* size);
typedef void* (*pfn_il2cpp_assembly_get_image_t)(void* assembly);
typedef const char* (*pfn_il2cpp_image_get_name_t)(void* image);
typedef void* (*pfn_il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
typedef const char* (*pfn_il2cpp_class_get_name_t)(void* klass);
typedef const char* (*pfn_il2cpp_class_get_namespace_t)(void* klass);
typedef void* (*pfn_il2cpp_class_get_type_t)(void* klass);
typedef void* (*pfn_il2cpp_type_get_object_t)(void* type);
typedef void* (*pfn_il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
typedef void* (*pfn_il2cpp_class_get_field_from_name_t)(void* klass, const char* name);
typedef void (*pfn_il2cpp_field_get_value_t)(void* obj, void* field, void* value);
typedef void (*pfn_il2cpp_field_set_value_t)(void* obj, void* field, void* value);
typedef void* (*pfn_il2cpp_string_new_len_t)(const char* str, uint32_t len);
typedef const wchar_t* (*pfn_il2cpp_string_chars_t)(void* str);
typedef int32_t (*pfn_il2cpp_string_length_t)(void* str);
typedef int32_t (*pfn_il2cpp_array_length_t)(void* array);
typedef void* (*pfn_il2cpp_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);

typedef void* (*pfn_il2cpp_object_get_class_t)(void* obj);
typedef void* (*pfn_il2cpp_class_get_element_class_t)(void* klass);
typedef bool (*pfn_il2cpp_class_is_valuetype_t)(void* klass);
typedef int32_t (*pfn_il2cpp_class_value_size_t)(void* klass, uint32_t* align);
typedef int32_t (*pfn_il2cpp_class_instance_size_t)(void* klass);
typedef int32_t (*pfn_il2cpp_array_element_size_t)(void* klass);
typedef void* (*pfn_il2cpp_class_get_fields_t)(void* klass, void** iter);
typedef const char* (*pfn_il2cpp_field_get_name_t)(void* field);
typedef size_t (*pfn_il2cpp_field_get_offset_t)(void* field);
typedef void* (*pfn_il2cpp_field_get_type_t)(void* field);
typedef const char* (*pfn_il2cpp_type_get_name_t)(void* type);

static pfn_il2cpp_domain_get_t p_il2cpp_domain_get = NULL;
static pfn_il2cpp_domain_get_assemblies_t p_il2cpp_domain_get_assemblies = NULL;
static pfn_il2cpp_assembly_get_image_t p_il2cpp_assembly_get_image = NULL;
static pfn_il2cpp_image_get_name_t p_il2cpp_image_get_name = NULL;
static pfn_il2cpp_class_from_name_t p_il2cpp_class_from_name = NULL;
static pfn_il2cpp_class_get_name_t p_il2cpp_class_get_name = NULL;
static pfn_il2cpp_class_get_namespace_t p_il2cpp_class_get_namespace = NULL;
static pfn_il2cpp_class_get_type_t p_il2cpp_class_get_type = NULL;
static pfn_il2cpp_type_get_object_t p_il2cpp_type_get_object = NULL;
static pfn_il2cpp_class_get_method_from_name_t p_il2cpp_class_get_method_from_name = NULL;
static pfn_il2cpp_class_get_field_from_name_t p_il2cpp_class_get_field_from_name = NULL;
static pfn_il2cpp_field_get_value_t p_il2cpp_field_get_value = NULL;
static pfn_il2cpp_field_set_value_t p_il2cpp_field_set_value = NULL;
static pfn_il2cpp_string_new_len_t p_il2cpp_string_new_len = NULL;
static pfn_il2cpp_string_chars_t p_il2cpp_string_chars = NULL;
static pfn_il2cpp_string_length_t p_il2cpp_string_length = NULL;
static pfn_il2cpp_array_length_t p_il2cpp_array_length = NULL;
static pfn_il2cpp_runtime_invoke_t orig_il2cpp_runtime_invoke = NULL;

static pfn_il2cpp_object_get_class_t p_il2cpp_object_get_class = NULL;
static pfn_il2cpp_class_get_element_class_t p_il2cpp_class_get_element_class = NULL;
static pfn_il2cpp_class_is_valuetype_t p_il2cpp_class_is_valuetype = NULL;
static pfn_il2cpp_class_value_size_t p_il2cpp_class_value_size = NULL;
static pfn_il2cpp_class_instance_size_t p_il2cpp_class_instance_size = NULL;
static pfn_il2cpp_array_element_size_t p_il2cpp_array_element_size = NULL;
static pfn_il2cpp_class_get_fields_t p_il2cpp_class_get_fields = NULL;
static pfn_il2cpp_field_get_name_t p_il2cpp_field_get_name = NULL;
static pfn_il2cpp_field_get_offset_t p_il2cpp_field_get_offset = NULL;
static pfn_il2cpp_field_get_type_t p_il2cpp_field_get_type = NULL;
static pfn_il2cpp_type_get_name_t p_il2cpp_type_get_name = NULL;

static volatile bool g_RetranslationApplied = false;
static void* g_LastPatchedStoryInstance = NULL;
static DWORD g_LastScanTick = 0;
static void* g_FindObjectsMethod = NULL;
static void* g_StoryTypeObject = NULL;

/* ========================================================================= */
/* In-Memory Story Translation Injector                                      */
/* ========================================================================= */
static bool PatchStoryTranslationDetails(void* story_obj) {
    if (!story_obj) return false;
    if (story_obj == g_LastPatchedStoryInstance) return true;

    void* klass = p_il2cpp_object_get_class ? p_il2cpp_object_get_class(story_obj) : *(void**)story_obj;
    if (!klass) return false;

    void* trans_field = p_il2cpp_class_get_field_from_name(klass, "Translations");
    if (!trans_field) {
        Log("Warning: 'Translations' field not found on StoryTranslationDetails\n");
        return false;
    }

    void* list_obj = NULL;
    p_il2cpp_field_get_value(story_obj, trans_field, &list_obj);
    if (!list_obj) return false;

    void* list_class = p_il2cpp_object_get_class ? p_il2cpp_object_get_class(list_obj) : *(void**)list_obj;
    void* items_field = p_il2cpp_class_get_field_from_name(list_class, "_items");
    void* size_field = p_il2cpp_class_get_field_from_name(list_class, "_size");
    if (!items_field || !size_field) {
        Log("Warning: _items or _size field not found on List<T>\n");
        return false;
    }

    void* array_obj = NULL;
    int32_t list_size = 0;
    p_il2cpp_field_get_value(list_obj, items_field, &array_obj);
    p_il2cpp_field_get_value(list_obj, size_field, &list_size);

    if (!array_obj || list_size <= 0) return false;

    Log("Found Translations list with %d items. Inspecting element type...\n", list_size);

    void* array_class = p_il2cpp_object_get_class ? p_il2cpp_object_get_class(array_obj) : *(void**)array_obj;
    void* item_class = p_il2cpp_class_get_element_class ? p_il2cpp_class_get_element_class(array_class) : NULL;

    if (!item_class) {
        Log("Error: Could not determine item_class from array_class\n");
        return false;
    }

    const char* item_ns = p_il2cpp_class_get_namespace ? p_il2cpp_class_get_namespace(item_class) : "";
    const char* item_name = p_il2cpp_class_get_name ? p_il2cpp_class_get_name(item_class) : "";
    bool is_val = p_il2cpp_class_is_valuetype ? p_il2cpp_class_is_valuetype(item_class) : false;
    int32_t val_size = (is_val && p_il2cpp_class_value_size) ? p_il2cpp_class_value_size(item_class, NULL) : 0;
    int32_t inst_size = p_il2cpp_class_instance_size ? p_il2cpp_class_instance_size(item_class) : 0;
    int32_t arr_elem_size = p_il2cpp_array_element_size ? p_il2cpp_array_element_size(array_class) : 0;

    Log("Item class: '%s.%s', is_valuetype=%d, val_size=%d, inst_size=%d, arr_elem_size=%d\n",
        item_ns, item_name, (int)is_val, val_size, inst_size, arr_elem_size);

    /* Enumerate all fields of item_class */
    void* iter = NULL;
    void* field = NULL;
    void* id_field = NULL;
    void* eng_field = NULL;
    size_t id_f_offset = 0;
    size_t eng_f_offset = 0;

    if (p_il2cpp_class_get_fields) {
        while ((field = p_il2cpp_class_get_fields(item_class, &iter)) != NULL) {
            const char* fname = p_il2cpp_field_get_name ? p_il2cpp_field_get_name(field) : "";
            size_t foff = p_il2cpp_field_get_offset ? p_il2cpp_field_get_offset(field) : 0;
            void* ftype = p_il2cpp_field_get_type ? p_il2cpp_field_get_type(field) : NULL;
            const char* ftname = (ftype && p_il2cpp_type_get_name) ? p_il2cpp_type_get_name(ftype) : "";
            Log("  Field: '%s' (type: '%s', offset: %zu / 0x%zx)\n", fname, ftname, foff, foff);

            if (_stricmp(fname, "Id") == 0 || strstr(fname, "<Id>") != NULL) {
                id_field = field;
                id_f_offset = foff;
            }
            if (_stricmp(fname, "English") == 0 || strstr(fname, "<English>") != NULL) {
                eng_field = field;
                eng_f_offset = foff;
            }
        }
    }

    if (!id_field || !eng_field) {
        Log("Error: Could not identify Id or English field on item_class\n");
        return false;
    }

    /* Determine element stride and field offsets in array memory */
    size_t stride = is_val ? (size_t)(val_size > 0 ? val_size : (arr_elem_size > 0 ? arr_elem_size : 48)) : sizeof(void*);
    if (stride == 0) stride = sizeof(void*);

    size_t actual_id_offset = id_f_offset;
    size_t actual_eng_offset = eng_f_offset;

    if (is_val) {
        bool verified = false;
        size_t test_offsets[4];
        int test_count = 0;
        if (id_f_offset >= 16) test_offsets[test_count++] = id_f_offset - 16;
        test_offsets[test_count++] = id_f_offset;
        test_offsets[test_count++] = 0;
        test_offsets[test_count++] = 8;

        for (int t = 0; t < test_count; t++) {
            size_t try_off = test_offsets[t];
            char* item0 = ((char*)array_obj + 32);
            if (IsBadReadPtr(item0 + try_off, sizeof(void*))) continue;

            void* test_id_str = *(void**)(item0 + try_off);
            if (!test_id_str || IsBadReadPtr(test_id_str, 24)) continue;

            const wchar_t* wchars = p_il2cpp_string_chars ? p_il2cpp_string_chars(test_id_str) : NULL;
            int wlen = p_il2cpp_string_length ? p_il2cpp_string_length(test_id_str) : 0;
            uint16_t ts = 0, tl = 0;
            if (wchars && ParseId(wchars, wlen, &ts, &tl)) {
                actual_id_offset = try_off;
                if (id_f_offset >= 16 && try_off == id_f_offset - 16 && eng_f_offset >= 16) {
                    actual_eng_offset = eng_f_offset - 16;
                } else if (try_off == id_f_offset) {
                    actual_eng_offset = eng_f_offset;
                } else {
                    actual_eng_offset = try_off + 8;
                }
                char id_sample[16] = {0};
                for (int c = 0; c < (wlen < 15 ? wlen : 15); c++) id_sample[c] = (char)wchars[c];
                Log("Verified valuetype Id offset: %zu (sample: '%s'), English offset: %zu\n",
                    actual_id_offset, id_sample, actual_eng_offset);
                verified = true;
                break;
            }
        }

        if (!verified) {
            actual_id_offset = (id_f_offset >= 16) ? (id_f_offset - 16) : id_f_offset;
            actual_eng_offset = (eng_f_offset >= 16) ? (eng_f_offset - 16) : eng_f_offset;
            Log("Warning: Could not verify Id string at trial offsets. Using fallback offset %zu / %zu\n",
                actual_id_offset, actual_eng_offset);
        }
    } else {
        /* Reference type: elements array contains pointers to heap objects */
        void** obj_ptrs = (void**)((char*)array_obj + 32);
        void* item0 = obj_ptrs[0];
        if (item0 && !IsBadReadPtr(item0, sizeof(void*) * 4)) {
            void* test_id_str = *(void**)((char*)item0 + id_f_offset);
            if (test_id_str && !IsBadReadPtr(test_id_str, 24)) {
                const wchar_t* wchars = p_il2cpp_string_chars ? p_il2cpp_string_chars(test_id_str) : NULL;
                int wlen = p_il2cpp_string_length ? p_il2cpp_string_length(test_id_str) : 0;
                uint16_t ts = 0, tl = 0;
                if (wchars && ParseId(wchars, wlen, &ts, &tl)) {
                    char id_sample[16] = {0};
                    for (int c = 0; c < (wlen < 15 ? wlen : 15); c++) id_sample[c] = (char)wchars[c];
                    Log("Verified reference type Id offset: %zu (sample: '%s'), English offset: %zu\n",
                        actual_id_offset, id_sample, actual_eng_offset);
                }
            }
        }
    }

    Log("Starting translation injection over %d entries (stride=%zu, id_off=%zu, eng_off=%zu)...\n",
        list_size, stride, actual_id_offset, actual_eng_offset);

    int patched = 0;
    int matched = 0;

    for (int i = 0; i < list_size; i++) {
        char* item = NULL;
        if (is_val) {
            item = ((char*)array_obj + 32) + (size_t)i * stride;
        } else {
            void** obj_ptrs = (void**)((char*)array_obj + 32);
            item = (char*)obj_ptrs[i];
        }
        if (!item || IsBadReadPtr(item, actual_eng_offset + sizeof(void*))) continue;

        void* id_str = *(void**)(item + actual_id_offset);
        if (!id_str || IsBadReadPtr(id_str, 24)) continue;

        const wchar_t* wchars = p_il2cpp_string_chars(id_str);
        int wlen = p_il2cpp_string_length(id_str);
        uint16_t scene = 0, line = 0;
        if (!ParseId(wchars, wlen, &scene, &line)) continue;

        const IFTREntry* entry = FindTranslation(scene, line);
        if (entry) {
            matched++;
            const char* new_text = g_StringPool + entry->str_offset;
            void* new_str = p_il2cpp_string_new_len(new_text, entry->str_len);
            if (new_str) {
                *(void**)(item + actual_eng_offset) = new_str;
                patched++;
                if (patched <= 3 || patched == 18015) {
                    char preview[96];
                    int plen = entry->str_len > 60 ? 60 : entry->str_len;
                    memcpy(preview, new_text, plen);
                    preview[plen] = '\0';
                    for (int p = 0; p < plen; p++) {
                        if (preview[p] == '\n' || preview[p] == '\r') preview[p] = ' ';
                    }
                    Log("  Patched [%03u-%03u]: \"%s...\"\n", scene, line, preview);
                }
            }
        }
    }

    Log("=========================================================\n");
    Log("[SUCCESS] IN-MEMORY RETRANSLATION ACTIVE!\n");
    Log("Successfully matched: %d / %d\n", matched, list_size);
    Log("Successfully patched: %d / %d dialogue lines in RAM.\n", patched, list_size);
    Log("=========================================================\n");

    g_LastPatchedStoryInstance = story_obj;
    g_RetranslationApplied = true;
    return true;
}

/* ========================================================================= */
/* Periodic Scanner via Main Thread                                          */
/* ========================================================================= */
static void CheckForStoryTranslationDetailsOnMainThread(void) {
    DWORD now = GetTickCount();
    DWORD interval = (g_RetranslationApplied && g_LastPatchedStoryInstance) ? 3000 : 500;
    if (now - g_LastScanTick < interval) return;
    g_LastScanTick = now;

    if (!g_FindObjectsMethod || !g_StoryTypeObject) {
        void* domain = p_il2cpp_domain_get();
        if (!domain) return;
        size_t num_assemblies = 0;
        void** assemblies = p_il2cpp_domain_get_assemblies(domain, &num_assemblies);
        if (!assemblies || num_assemblies == 0) return;

        void* core_image = NULL;
        void* game_data_image = NULL;
        for (size_t i = 0; i < num_assemblies; i++) {
            void* img = p_il2cpp_assembly_get_image(assemblies[i]);
            if (!img) continue;
            const char* name = p_il2cpp_image_get_name(img);
            if (!name) continue;
            if (strcmp(name, "UnityEngine.CoreModule.dll") == 0 || strcmp(name, "UnityEngine.CoreModule") == 0) {
                core_image = img;
            }
            if (strcmp(name, "Game.Data.dll") == 0 || strcmp(name, "Game.Data") == 0) {
                game_data_image = img;
            }
        }

        if (!core_image || !game_data_image) return;

        void* res_class = p_il2cpp_class_from_name(core_image, "UnityEngine", "Resources");
        if (res_class) {
            g_FindObjectsMethod = p_il2cpp_class_get_method_from_name(res_class, "FindObjectsOfTypeAll", 1);
        }

        void* story_class = p_il2cpp_class_from_name(game_data_image, "ifapp.Game.Data.Story", "StoryTranslationDetails");
        if (story_class) {
            void* story_type = p_il2cpp_class_get_type(story_class);
            if (story_type) {
                g_StoryTypeObject = p_il2cpp_type_get_object(story_type);
            }
        }
    }

    if (!g_FindObjectsMethod || !g_StoryTypeObject) return;

    void* args[1] = { g_StoryTypeObject };
    void* exc = NULL;
    void* arr_obj = orig_il2cpp_runtime_invoke(g_FindObjectsMethod, NULL, args, &exc);
    if (!arr_obj || exc) return;

    int32_t len = p_il2cpp_array_length(arr_obj);
    if (len > 0) {
        void** items = (void**)((char*)arr_obj + 32);
        for (int32_t k = 0; k < len; k++) {
            void* story_instance = items[k];
            if (story_instance) {
                PatchStoryTranslationDetails(story_instance);
            }
        }
    }
}

/* ========================================================================= */
/* Detour: il2cpp_runtime_invoke                                             */
/* ========================================================================= */
static void* Detour_il2cpp_runtime_invoke(void* method, void* obj, void** params, void** exc) {
    CheckForStoryTranslationDetailsOnMainThread();
    return orig_il2cpp_runtime_invoke(method, obj, params, exc);
}

/* ========================================================================= */
/* Hook Installation Worker Thread                                           */
/* ========================================================================= */
static DWORD WINAPI HookThread(LPVOID lpParam) {
    Log("--- In Falsus English Retranslation Hook Initializing ---\n");

    if (!LoadRetranslationDat()) {
        Log("Fatal: Aborting hook initialization due to missing resource.\n");
        return 1;
    }

    HMODULE hGameAssembly = NULL;
    for (int i = 0; i < 300; i++) { /* Wait up to 30 seconds */
        hGameAssembly = GetModuleHandleA("GameAssembly.dll");
        if (hGameAssembly) break;
        Sleep(100);
    }

    if (!hGameAssembly) {
        Log("Error: GameAssembly.dll not found after 30s timeout.\n");
        return 1;
    }

    Log("Found GameAssembly.dll at %p\n", (void*)hGameAssembly);

    /* Resolve IL2CPP exports */
    p_il2cpp_domain_get = (pfn_il2cpp_domain_get_t)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    p_il2cpp_domain_get_assemblies = (pfn_il2cpp_domain_get_assemblies_t)GetProcAddress(hGameAssembly, "il2cpp_domain_get_assemblies");
    p_il2cpp_assembly_get_image = (pfn_il2cpp_assembly_get_image_t)GetProcAddress(hGameAssembly, "il2cpp_assembly_get_image");
    p_il2cpp_image_get_name = (pfn_il2cpp_image_get_name_t)GetProcAddress(hGameAssembly, "il2cpp_image_get_name");
    p_il2cpp_class_from_name = (pfn_il2cpp_class_from_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_from_name");
    p_il2cpp_class_get_name = (pfn_il2cpp_class_get_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_name");
    p_il2cpp_class_get_namespace = (pfn_il2cpp_class_get_namespace_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_namespace");
    p_il2cpp_class_get_type = (pfn_il2cpp_class_get_type_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_type");
    p_il2cpp_type_get_object = (pfn_il2cpp_type_get_object_t)GetProcAddress(hGameAssembly, "il2cpp_type_get_object");
    p_il2cpp_class_get_method_from_name = (pfn_il2cpp_class_get_method_from_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_method_from_name");
    p_il2cpp_class_get_field_from_name = (pfn_il2cpp_class_get_field_from_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_field_from_name");
    p_il2cpp_field_get_value = (pfn_il2cpp_field_get_value_t)GetProcAddress(hGameAssembly, "il2cpp_field_get_value");
    p_il2cpp_field_set_value = (pfn_il2cpp_field_set_value_t)GetProcAddress(hGameAssembly, "il2cpp_field_set_value");
    p_il2cpp_string_new_len = (pfn_il2cpp_string_new_len_t)GetProcAddress(hGameAssembly, "il2cpp_string_new_len");
    p_il2cpp_string_chars = (pfn_il2cpp_string_chars_t)GetProcAddress(hGameAssembly, "il2cpp_string_chars");
    p_il2cpp_string_length = (pfn_il2cpp_string_length_t)GetProcAddress(hGameAssembly, "il2cpp_string_length");
    p_il2cpp_array_length = (pfn_il2cpp_array_length_t)GetProcAddress(hGameAssembly, "il2cpp_array_length");

    p_il2cpp_object_get_class = (pfn_il2cpp_object_get_class_t)GetProcAddress(hGameAssembly, "il2cpp_object_get_class");
    p_il2cpp_class_get_element_class = (pfn_il2cpp_class_get_element_class_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_element_class");
    p_il2cpp_class_is_valuetype = (pfn_il2cpp_class_is_valuetype_t)GetProcAddress(hGameAssembly, "il2cpp_class_is_valuetype");
    p_il2cpp_class_value_size = (pfn_il2cpp_class_value_size_t)GetProcAddress(hGameAssembly, "il2cpp_class_value_size");
    p_il2cpp_class_instance_size = (pfn_il2cpp_class_instance_size_t)GetProcAddress(hGameAssembly, "il2cpp_class_instance_size");
    p_il2cpp_array_element_size = (pfn_il2cpp_array_element_size_t)GetProcAddress(hGameAssembly, "il2cpp_array_element_size");
    p_il2cpp_class_get_fields = (pfn_il2cpp_class_get_fields_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_fields");
    p_il2cpp_field_get_name = (pfn_il2cpp_field_get_name_t)GetProcAddress(hGameAssembly, "il2cpp_field_get_name");
    p_il2cpp_field_get_offset = (pfn_il2cpp_field_get_offset_t)GetProcAddress(hGameAssembly, "il2cpp_field_get_offset");
    p_il2cpp_field_get_type = (pfn_il2cpp_field_get_type_t)GetProcAddress(hGameAssembly, "il2cpp_field_get_type");
    p_il2cpp_type_get_name = (pfn_il2cpp_type_get_name_t)GetProcAddress(hGameAssembly, "il2cpp_type_get_name");

    uint8_t* pExport_invoke = (uint8_t*)GetProcAddress(hGameAssembly, "il2cpp_runtime_invoke");
    if (!pExport_invoke) {
        Log("Error: Could not resolve il2cpp_runtime_invoke\n");
        return 1;
    }

    Log("Resolved il2cpp_runtime_invoke at %p\n", pExport_invoke);

    /* Resolve original function from the 5-byte relative jump stub in export table */
    if (pExport_invoke[0] == 0xE9) {
        int32_t rel = *(int32_t*)(pExport_invoke + 1);
        orig_il2cpp_runtime_invoke = (pfn_il2cpp_runtime_invoke_t)(pExport_invoke + 5 + rel);
        Log("Resolved original invoke target at %p\n", (void*)orig_il2cpp_runtime_invoke);
    } else {
        Log("Warning: il2cpp_runtime_invoke does not start with E9 jump, direct call used\n");
        orig_il2cpp_runtime_invoke = (pfn_il2cpp_runtime_invoke_t)pExport_invoke;
    }

    /* Install 14-byte 64-bit absolute jump hook at export address:
       FF 25 00 00 00 00 [8-byte 64-bit address] */
    DWORD oldProtect = 0;
    if (VirtualProtect(pExport_invoke, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pExport_invoke[0] = 0xFF;
        pExport_invoke[1] = 0x25;
        pExport_invoke[2] = 0x00;
        pExport_invoke[3] = 0x00;
        pExport_invoke[4] = 0x00;
        pExport_invoke[5] = 0x00;
        uint64_t targetAddr = (uint64_t)Detour_il2cpp_runtime_invoke;
        memcpy(pExport_invoke + 6, &targetAddr, 8);
        pExport_invoke[14] = 0x90; /* nop */
        pExport_invoke[15] = 0x90; /* nop */
        VirtualProtect(pExport_invoke, 16, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), pExport_invoke, 16);
        Log("Successfully hooked il2cpp_runtime_invoke!\n");
    } else {
        Log("Error: VirtualProtect failed to set PAGE_EXECUTE_READWRITE on export (err=%lu)\n", GetLastError());
        return 1;
    }

    Log("Hook active. Main thread scanning scheduled for StoryTranslationDetails...\n");
    return 0;
}

/* ========================================================================= */
/* DllMain                                                                   */
/* ========================================================================= */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* exeName = strrchr(exePath, '\\');
        if (!exeName) exeName = exePath; else exeName++;

        /* Ignore helper processes like UnityCrashHandler64 */
        if (_stricmp(exeName, "infalsus.exe") != 0 && _stricmp(exeName, "test_loader.exe") != 0) {
            return TRUE;
        }

        /* Determine DLL directory */
        GetModuleFileNameA(hinstDLL, g_DllDir, MAX_PATH);
        char* lastSlash = strrchr(g_DllDir, '\\');
        if (lastSlash) *lastSlash = '\0';

        InitLog(g_DllDir);
        Log("=== In Falsus Retranslation Hook Attached (PID: %lu) ===\n", GetCurrentProcessId());
        Log("Executable: %s\n", exePath);
        Log("Directory:  %s\n", g_DllDir);

        CreateThread(NULL, 0, HookThread, NULL, 0, NULL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_DatBuffer) {
            VirtualFree(g_DatBuffer, 0, MEM_RELEASE);
            g_DatBuffer = NULL;
        }
        if (g_hRealVersion) {
            FreeLibrary(g_hRealVersion);
            g_hRealVersion = NULL;
        }
        if (g_hLogFile != INVALID_HANDLE_VALUE) {
            CloseHandle(g_hLogFile);
            g_hLogFile = INVALID_HANDLE_VALUE;
        }
        DeleteCriticalSection(&g_LogCs);
    }
    return TRUE;
}
