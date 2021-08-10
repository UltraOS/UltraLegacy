#pragma once

long create_process(const char* path);
long create_thread(void* entrypoint, void* arg);

void exit_process(long code);
void exit_thread(long code);

void sleep(long nanoseconds);
