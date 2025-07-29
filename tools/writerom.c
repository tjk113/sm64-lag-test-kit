#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRIF(cond, args...) {\
	if (cond) {\
		fprintf(stderr, args);\
		exit(EXIT_FAILURE);\
	}\
}

#define READ(dst, size, count) ERRIF((fread(dst, size, count, fp) != count), "Failed to read from \"%s\".\n", filename)
#define WRITE(src, size, count) ERRIF((fwrite(src, size, count, fp) != count), "Failed to write to \"%s\".\n", filename)
#define WRITE_ALIGN(src, size, count, mod) {\
	WRITE(src, size, count);\
	size_t offset = align(size*count, mod);\
	if (offset > 0) {\
		ERRIF((fseek(fp, offset, SEEK_CUR) != 0), "Failed to write to \"%s\".\n", filename);\
	}\
}

struct BlockData {
	uint32_t addr;
	uint32_t size;
};

struct RecordingData {
	uint16_t blocks_length;
	uint16_t inputs_length;
	uint8_t warp_params[16];
	struct BlockData *block_data;
	size_t data_size;
	void *state_data;
	void *inputs_data;
};


char* allocAndCopyString(char *src, size_t len) {
	char *dst = (char*)malloc(sizeof(char)*(len + 1));
	ERRIF((dst == NULL), "String allocation failed.\n");
	strncpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}

uint32_t swapEndian(uint32_t v) {
	return ((v << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | (v >> 24));
}

uint16_t swapEndianShort(uint16_t v) {
	return ((v << 8) | (v >> 8));
}

size_t align(size_t v, size_t m) {
	size_t r = v % m;
	if (r != 0) {
		return m - r;
	}
	return 0;
}


int getFiles(char **rec_filenames, char **rom_filename_ptr, int max_recs) {
	int num_recs = 0;
	char *rom_filename = NULL;

	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			char *filename = dir->d_name;
			size_t len = strnlen(filename, 128);
			if ((len == 128) || (len < 4)) {
				continue;
			}
			char *ext = filename + len - 4;
			if (strncmp(ext, ".z64", 4) == 0) {
				ERRIF((rom_filename != NULL), "Multiple ROMs found in directory.\n");
				rom_filename = allocAndCopyString(filename, len);
			} else if (strncmp(ext, ".dat", 4) == 0) {
				ERRIF((num_recs == max_recs), "Exceeded max recordings (%d).\n", max_recs);
				printf("Found recording: \"%s\".\n", filename);
				rec_filenames[num_recs++] = allocAndCopyString(filename, len);
			}
		}
	}

	*rom_filename_ptr = rom_filename;
	return num_recs;
}

struct RecordingData loadRecording(const char *filename) {
	struct RecordingData rec;

	FILE *fp = fopen(filename, "rb");
	ERRIF((fp == NULL), "Failed to read file \"%s\".\n", filename);

	READ(rec.warp_params, 1, 13);

	READ(&rec.blocks_length, 2, 1);
	ERRIF((rec.blocks_length == 0), "Invalid blocks length.\n");
	
	rec.block_data = (struct BlockData*)malloc(sizeof(struct BlockData)*rec.blocks_length);
	ERRIF((rec.block_data == NULL), "Failed to allocate block data.\n");
	READ(rec.block_data, 4, 2*rec.blocks_length);

	rec.data_size = 0;
	for (int i = 0; i < rec.blocks_length; i++) {
		rec.data_size += rec.block_data[i].size;
	}
	ERRIF((rec.data_size == 0), "Invalid data size.\n");

	rec.state_data = malloc(rec.data_size);
	ERRIF((rec.state_data == NULL), "Failed to allocate state data.\n");
	READ(rec.state_data, 1, rec.data_size);

	READ(&rec.inputs_length, 2, 1);
	ERRIF((rec.inputs_length == 0), "Invalid inputs length.\n");

	size_t inputs_size = 8*rec.inputs_length;
	rec.inputs_data = malloc(inputs_size);
	ERRIF((rec.inputs_data == NULL), "Failed to allocate inputs data.\n");

	READ(rec.inputs_data, 1, inputs_size);

	fclose(fp);
	return rec;
}


size_t getDataSize(struct RecordingData *recs, int num_recs) {
	size_t size = 32*num_recs;
	for (int i = 0; i < num_recs; i++) {
		size_t data_size = recs[i].data_size;
		size_t aligned_data_size = data_size + align(data_size, 4);
		size += 8*recs[i].blocks_length + aligned_data_size + 8*recs[i].inputs_length;
	}
	return size;
}

void seekDataStart(FILE *fp, uint32_t marker) {
	uint32_t v = ~marker;
	while (v != marker) {
		ERRIF((fread(&v, 4, 1, fp) != 1), "Could not find data start in ROM.\n");
	}
}

uint32_t writeRecordingHeader(const char *filename, FILE *fp, struct RecordingData rec, uint32_t cur_data_ptr) {
	uint16_t blocks_length_s = swapEndianShort(rec.blocks_length);
	WRITE(&blocks_length_s, 2, 1);
	uint16_t inputs_length_s = swapEndianShort(rec.inputs_length);
	WRITE(&inputs_length_s, 2, 1);
	WRITE(&rec.warp_params, 16, 1);

	uint32_t mem_blocks_ptr = swapEndian(cur_data_ptr);
	WRITE(&mem_blocks_ptr, 4, 1);
	cur_data_ptr += 8*rec.blocks_length;

	uint32_t state_data_ptr = swapEndian(cur_data_ptr);
	WRITE(&state_data_ptr, 4, 1);
	cur_data_ptr += rec.data_size;
	cur_data_ptr += align(rec.data_size, 4);

	uint32_t inputs_data_ptr = swapEndian(cur_data_ptr);
	WRITE(&inputs_data_ptr, 4, 1);
	cur_data_ptr += 8*rec.inputs_length;

	return cur_data_ptr;
}

void writeRecordingData(const char *filename, FILE *fp, struct RecordingData rec) {
	for (int i = 0; i < rec.blocks_length; i++) {
		uint32_t addr_s = swapEndian(rec.block_data[i].addr);
		WRITE(&addr_s, 4, 1);
		uint32_t size_s = swapEndian(rec.block_data[i].size);
		WRITE(&size_s, 4, 1);
	}
	WRITE_ALIGN(rec.state_data, 1, rec.data_size, 4);
	WRITE(rec.inputs_data, 1, 8*rec.inputs_length);
}

void updateROM(const char *filename, struct RecordingData *recs, int num_recs, size_t data_size) {
	FILE *fp = fopen(filename, "rb+");
	ERRIF((fp == NULL), "Failed to open ROM \"%s\".\n", filename);

	seekDataStart(fp, 0xC0B1835A);
	uint32_t data_start_addr, max_data_size;
	READ(&data_start_addr, 4, 1);
	data_start_addr = swapEndian(data_start_addr);
	READ(&max_data_size, 4, 1);
	max_data_size = swapEndian(max_data_size);
	ERRIF((data_size > max_data_size), "Recording data too large (%d/%d).\n", data_size, max_data_size);

	fseek(fp, 0, SEEK_CUR);
	uint32_t num_recs_s = swapEndian(num_recs);
	WRITE(&num_recs_s, 4, 1);

	uint32_t cur_data_ptr = data_start_addr + 32*num_recs;
	for (int i = 0; i < num_recs; i++) {
		cur_data_ptr = writeRecordingHeader(filename, fp, recs[i], cur_data_ptr);
	}

	for (int i = 0; i < num_recs; i++) {
		writeRecordingData(filename, fp, recs[i]);
	}

	fclose(fp);
}


#define MAX_RECS 128
int main() {
	char *rec_filenames[MAX_RECS];
	char *rom_filename;

	int num_recs = getFiles(rec_filenames, &rom_filename, MAX_RECS);
	ERRIF((rom_filename == NULL), "Could not find ROM in current directory.\n");
	ERRIF((num_recs == 0), "No recordings found in current directory.\n");

	struct RecordingData recs[MAX_RECS];
	for (int i = 0; i < num_recs; i++) {
		recs[i] = loadRecording(rec_filenames[i]);
	}

	size_t data_size = getDataSize(recs, num_recs);
	updateROM(rom_filename, recs, num_recs, data_size);
	printf("Successfully written ROM.\n");
}