#pragma once

void *ei_malloc(size_t size);
void *ei_calloc(size_t nmemb, size_t size);
void ei_free(void *ptr);

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif