// src/storage/disk_manager.h

/**
 * Disk manager reads the relevant pages from disk and writes the pages to disk by the request of BPM
 * The pages of disk in bpm are called frames, Eventually they are owned and operated by BPM
 *
 * Many databases reduce the involvement of OS memory mapping, as it may not flush the pages to disk when requested(no-force)
 * fsync() forces the OS to flush any dirty pages for that file out to stable storage to attain force policy(durability)
 * The kernel still arbitrates when your flush actually hits the device (and the drive’s firmware still schedules the physical head movement).
 * If you really want to minimize OS involvement you need to:
 *   - Open the file with O_DIRECT (bypass page cache).
 *   - Use a kernel‐bypass userspace driver (e.g. SPDK) so you talk NVMe/DAS devices directly.
 *   - But in all cases the hardware’s own firmware will still schedule incoming commands on the platter or flash chips.
 *
 * For MiniDB, we will use a simple file I/O interface to read and write pages, We shift to fsync() in future for writing pages.
 *
 * WritePage    - overwrite (or extend) an existing page
 * ReadPage     - load a page’s contents (scan)
 * AllocatePage - assign a fresh page ID (create a new page) into .db file
 * DeallocatePage - mark a page as free (generally removing entire page from disk is done using a background thread)
 *
 * Memory refresher
 * - Memory is a sequence of bytes, each byte has an address.
 * - A page is a fixed-size block of memory, typically 4KB or 8KB.
 * - To read a page, seek to that memory pointer where page starts and read PAGE_SIZE bytes
 *
 * - Physical vs Logical Representation
 *   * In‐memory structs may have padding/alignment; on‐disk layouts must be packed and byte‐order safe.
 *   * Always define explicit on‐disk headers (e.g., fixed‐size page header) and serialize/deserialize fields.
 *
 * - Pages and Page IDs
 *   * Logical page_id_t in BufferPool → index into frame table.
 *   * Disk offset calculation: offset = page_id * PAGE_SIZE.
 *   * To fetch: fstream.seekg(offset), read PAGE_SIZE bytes into buffer.
 *   * To write: populate in‐memory buffer, fstream.seekp(offset), write PAGE_SIZE.
 *
 * - Handling Byte Data
 *   * Use std::memcpy or manual bit‐shifts to pack/unpack fields.
 *   * For strings or variable data, store length prefix, then raw bytes.
 */

#pragma once

#include <fstream>
#include <string>

#include "common/config.h"

namespace minidb {
namespace storage {
	class DiskManager {
	public:
		explicit DiskManager(const std::string& db_file);
		~DiskManager();

		void ReadPage(page_id_t page_id, char* page_data);
		void WritePage(page_id_t page_id, const char* page_data);
		page_id_t AllocatePage();
		void DeallocatePage(page_id_t page_id);
		uint32_t GetNumberOfPages() const;

	private:
		std::string db_file_name; // independent for a database instance
		std::fstream db_io_; // file stream
		page_id_t next_page_id_; // next available page ID for allocation
		long long file_size_; // size of the database file

		DISALLOW_COPY_AND_MOVE(DiskManager);
	};

} // namespace storage
} // namespace minidb