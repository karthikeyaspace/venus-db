// /src/common/config.h
#pragma once

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace venus {

constexpr uint32_t PAGE_SIZE = 4096; // 4kb
constexpr uint32_t MAX_BUFFER_POOL_SIZE = 128; // 128 pages
constexpr uint8_t LRUK_REPLACER_K = 5;

constexpr uint32_t MAX_DATABASES = 5;
constexpr uint32_t MAX_TABLES = 64; // per db
constexpr uint32_t MAX_COLUMNS = 64; // per table
constexpr uint32_t MAX_CHAR_LENGTH = 32;

// Every database has a db file stored in the data directory with file name <db_name>.db
constexpr const char* db_dir = "/mnt/d/KARTHIKEYA/PROJECTS/YO2/venus-db/data";

using page_id_t = uint32_t;
using slot_id_t = uint16_t;
using frame_id_t = uint32_t;
using database_id_t = uint32_t;
using table_id_t = uint32_t;
using column_id_t = uint32_t;
using index_id_t = uint32_t;

constexpr table_id_t INVALID_TABLE_ID = std::numeric_limits<table_id_t>::max();
constexpr page_id_t INVALID_PAGE_ID = std::numeric_limits<page_id_t>::max();
constexpr frame_id_t INVALID_FRAME_ID = std::numeric_limits<frame_id_t>::max();

constexpr page_id_t MASTER_TABLES_PAGE_ID = 0;
constexpr page_id_t MASTER_COLUMNS_PAGE_ID = 1;
constexpr page_id_t MASTER_INDEXES_PAGE_ID = 2;
constexpr page_id_t FIRST_USABLE_PAGE_ID = 3;

constexpr const char* MASTER_TABLES_NAME = "master_tables";
constexpr const char* MASTER_COLUMNS_NAME = "master_columns";
constexpr const char* MASTER_INDEXES_NAME = "master_indexes";

// macros

#define LOG(msg)                                   \
	do {                                           \
		std::cout << "[Log] " << msg << std::endl; \
	} while (0)

#define DISALLOW_COPY_AND_MOVE(cname)        \
	cname(const cname&) = delete;            \
	cname& operator=(const cname&) = delete; \
	cname(cname&&) = delete;                 \
	cname& operator=(cname&&) = delete;

} // namespace venus