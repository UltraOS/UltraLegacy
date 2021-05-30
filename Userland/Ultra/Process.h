#pragma once

long create_process(const char* path);
long create_thread(void* entrypoint, void* arg);

void process_exit(long code);
