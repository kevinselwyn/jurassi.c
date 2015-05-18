#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>

#define TITLE   "Jurassic Park, System Security Interface"
#define VERSION "Version 4.0.5, Alpha E"

static int read_audio(char **wav, char *filename, char *boundary) {
	int rc = 0, boundaries = 0, i = 0, l = 0;
	char *exec = NULL, *cmd_str = NULL, *cmd_which = NULL;
	size_t file_size = 0, boundary_offset = 0, boundary_size = 0, wav_size = 0;
	FILE *file = NULL, *cmd = NULL;

	if (!filename || strlen(filename) == 0) {
		printf("No filename provided\n");

		rc = 1;
		goto cleanup;
	}

	if (strncmp(filename, ".", 1) != 0 && strncmp(filename, "/", 1) != 0) {
		cmd_str = malloc(sizeof(char) * (7 + strlen(filename)));
		strcpy(cmd_str, "which ");
		strcat(cmd_str, filename);

		cmd = popen(cmd_str, "r");
		cmd_which = malloc(sizeof(char) * 100);
		(void)fread(cmd_which, 1, 100, cmd);

		for (i = (int)strlen(cmd_which), l = 0; i >= l; i--) {
			if (cmd_which[i] == '\n') {
				cmd_which[i] = '\0';
				break;
			}
		}

		filename = cmd_which;
	}

	file = fopen(filename, "rb");

	(void)fseek(file, 0, SEEK_END);
	file_size = (size_t)ftell(file);
	(void)fseek(file, 0, SEEK_SET);

	if (file_size == 0) {
		printf("File is empty\n");

		rc = 1;
		goto cleanup;
	}

	exec = malloc(sizeof(char) * file_size + 1);

	if (!exec) {
		printf("Memory error\n");

		rc = 1;
		goto cleanup;
	}

	if (fread(exec, 1, file_size, file) != file_size) {
		printf("Could not read %s\n", filename);

		rc = 1;
		goto cleanup;
	}

	boundary_size = strlen(boundary);

	for (i = 0, l = (int)file_size; i < l; i++) {
		if (strncmp(exec + i, boundary, boundary_size) == 0) {
			boundaries++;
		}

		if (boundaries == 2) {
			boundary_offset = i + boundary_size;
			break;
		}
	}

	wav_size = file_size - boundary_offset;
	*wav = malloc(sizeof(char) * wav_size);
	memcpy(*wav, exec + boundary_offset, wav_size);

cleanup:
	if (file) {
		(void)fclose(file);
	}

	if (cmd) {
		(void)pclose(cmd);
	}

	if (cmd_str) {
		free(cmd_str);
	}

	if (cmd_which) {
		free(cmd_which);
	}

	return rc;
}

static int play_audio(char *wav, void (*func)()) {
	int rc = 0, i = 0, l = 0;
	int header_size = 44, default_driver = 0, file_size = 0, buffer_count = 0;
	size_t BUF_SIZE = 4096, leftover_bytes = 0;
	char *buffer = NULL;
	ao_device *device;
	ao_sample_format format;
	struct wav_header{
		int format;
		int channels;
		int frequency;
		int bytesInData;
	};

	struct wav_header header;

	if (strncmp(wav, "RIFF", 4) != 0) {
		rc = 1;
		goto cleanup;
	}

	if (strncmp(wav + 8, "WAVEfmt ", 8) != 0) {
		rc = 1;
		goto cleanup;
	}

	header.format = (int)wav[16];
	header.channels = (int)wav[22];
	header.frequency = (int)wav[24] + ((int)wav[25] << 8) + ((int)wav[26] << 16) + ((int)wav[27] << 24);

	if (strncmp(wav + 36, "data", 4) != 0) {
		rc = 1;
		goto cleanup;
	}

	header.bytesInData = (int)wav[40] + ((int)wav[41] << 8) + ((int)wav[42] << 16) + ((int)wav[43] << 24);

	ao_initialize();
	default_driver = ao_default_driver_id();

	memset(&format, 0, sizeof(format));
	format.bits = header.format;
	format.channels = header.channels;
	format.rate = header.frequency;
	format.byte_format = AO_FMT_LITTLE;

	device = ao_open_live(default_driver, &format, NULL);

	if (device == NULL) {
		rc = 1;
		goto cleanup;
	}

	file_size = header.bytesInData;
	buffer_count = (int)(file_size / BUF_SIZE);

	for (i = 0, l = buffer_count; i < l; ++i) {
		(void)ao_play(device, wav + header_size + (i * BUF_SIZE), (uint_32)BUF_SIZE);
		(*func)();
	}

	leftover_bytes = (size_t)(file_size % BUF_SIZE);
	buffer = malloc(sizeof(char) * BUF_SIZE);
	memcpy(buffer, wav + header_size + (i * BUF_SIZE), leftover_bytes);
	memset(buffer + leftover_bytes, 0, BUF_SIZE - leftover_bytes);
	(void)ao_play(device, buffer, (uint_32)BUF_SIZE);

	(void)ao_close(device);
	ao_shutdown();

cleanup:
	if (buffer) {
		free(buffer);
	}

	return rc;
}

static void tick() {
	printf("YOU DIDN'T SAY THE MAGIC WORD!\n");
}

int main(int argc, char *argv[]) {
	int tries = 0, repeat = 1;
	size_t length = 0;
	char *line = NULL, *wav = NULL;

	(void)read_audio(&wav, argv[0], "magic.wav");

	(void)system("clear");

	printf("%s\n", TITLE);
	printf("%s\n", VERSION);
	printf("Ready...\n");

	line = malloc(sizeof(char) * 100);

	while (tries < 3) {
		printf("> ");
		(void)fgets(line, 100, stdin);

		if (strlen(line) == 1) {
			continue;
		}

		if (strncmp("access", line, 6) == 0) {
			if (tries < 2) {
				printf("access: PERMISSION DENIED.\n");
			}

			tries++;
		} else {
			length = strlen(line);
			line[length - 1] = '\0';

			printf("%s: command not found\n", line);
		}
	}

	printf("access: PERMISSION DENIED....and....\n");

	(void)read_audio(&wav, argv[0], "magic.wav");

	while (repeat == 1) {
		(void)play_audio(wav, &tick);
	}

	return 0;
}