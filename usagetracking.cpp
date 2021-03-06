#include "usagetracking.h"
#include <QDebug>

static int num_files_open=0;
static int64_t num_bytes_allocated=0;
static int malloc_count=0;
static int num_bytes_read=0;
static int num_bytes_written=0;

FILE *jfopen(const char *path,const char *mode) {
	//printf("jfopen.\n");
	FILE *F=fopen(path,mode);
	if (!F) return 0;
	num_files_open++;
	return F;
}

void jfclose(FILE *F) {
	//printf("jfclose.\n");
	if (!F) return;
	fclose(F);
	num_files_open--;
}

int jfread(void *data,size_t sz,int num,FILE *F) {
	int ret=fread(data,sz,num,F);
	num_bytes_read+=ret;
	return ret;
}

int jfwrite(void *data,size_t sz,int num,FILE *F) {
	int ret=fwrite(data,sz,num,F);
	num_bytes_written+=ret;
	return ret;
}

int jnumfilesopen() {
	return num_files_open;
}

void *jmalloc(size_t num_bytes) {
	//printf("jmalloc %d.\n",(int)num_bytes);
	if (!num_bytes) return 0;
	void *ret=malloc(num_bytes+8); //add some space to track the number of bytes
	int32_t *tmp=(int32_t *)ret;
	tmp[0]=num_bytes;
	num_bytes_allocated+=num_bytes;
	malloc_count++;
	return (void *)(((unsigned char *)ret)+8);
}

void jfree(void *ptr) {
	//printf("jfree.\n");
	if (!ptr) return;
	int64_t *tmp=(int64_t *)((unsigned char *)ptr-8);
	int64_t num_bytes=tmp[0];
	free((void *)tmp);
	num_bytes_allocated-=num_bytes;
	malloc_count--;
}
int jmalloccount() {
	return malloc_count;
}

int64_t jbytesallocated() {
	return num_bytes_allocated;
}

int jnumbytesread() {
	return num_bytes_read;
}

int jnumbyteswritten() {
	return num_bytes_written;
}
