// /src/common/config.h
#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace minidb {

constexpr uint32_t PAGE_SIZE = 4096; // 4kb
constexpr uint32_t BUFFER_POOL_SIZE = 128; // 128 pages
constexpr uint8_t LRUK_REPLACER_K = 5;

constexpr uint32_t MAX_DATABASES = 2;
constexpr uint32_t MAX_TABLES = 64; // per db
constexpr uint32_t MAX_COLUMNS = 64; // per table
constexpr uint32_t CHAR_LENGTH = 127;

// Every database has a db file stored in the data directory with file name <db_name>.db
constexpr char db_dir[] = "/data/mini_db";
constexpr char catalog_file[] = "/data/catalog.db";

using page_id_t = uint32_t;
using slot_id_t = uint16_t;
using frame_id_t = uint32_t;
using database_id_t = uint32_t;
using table_id_t = uint32_t;
using column_id_t = uint32_t;
using index_id_t = uint32_t;

constexpr page_id_t INVALID_PAGE_ID = std::numeric_limits<page_id_t>::max();
constexpr frame_id_t INVALID_FRAME_ID = std::numeric_limits<frame_id_t>::max();

enum class PageType : uint8_t {
	INVALID_PAGE = 0,
	TABLE_PAGE, // regular data page
	INDEX_LEAF_PAGE,
	INDEX_INTERNAL_PAGE,
	CATALOG_PAGE,
};

enum class ColumnType : uint8_t {
	INVALID_COLUMN = 0,
	INT,
	FLOAT,
	CHAR
};

// macros

#define LOG(msg) \
	fprintf(stdout, "Log: %s\n", msg);

#define DISALLOW_COPY_AND_MOVE(cname)        \
	cname(const cname&) = delete;            \
	cname& operator=(const cname&) = delete; \
	cname(cname&&) = delete;                 \
	cname& operator=(cname&&) = delete;

} // namespace minidb