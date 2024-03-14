#include<time.h>

//디버그_용 매크로


#define route(str, pwd) ((*str == '/') ? absolute_route_inode_return(str) : relative_route_inode_return(str, pwd))

typedef struct { //슈퍼블럭
	unsigned long long int
		inode1, inode2, //64 * 2 = 128
		datablock1, datablock2, datablock3, datablock4; //64 * 4 = 256
} superblock;

typedef struct { //아이노드
	unsigned char db_mask; //데이터블럭의 사용여부를 나타내는 마스크, 8비트로 구성되어 비트 하나당 데이터블럭 하나
	unsigned long long int //데이터블럭, 각 데이터블럭의 주소를 저장
		db1 : 8, db2 : 8, db3 : 8, db4 : 8,
		db5 : 8, db6 : 8, db7 : 8, db8 : 8;
	_Bool is_dir; //해당 파일이 디렉터리인지 저장, 1 == 디렉터리
	unsigned char single_idb; //인다이렉트 블럭의 주소를 저장
	_Bool is_idb; //해당 파일의 인다이렉트 블럭이 사용중인지 저장
	struct tm time; //시간정보
	unsigned int file_size; //해당 파일의 크기 저장
} inode;

/*
typedef struct { //디렉터리에 들어가는 파일의 이름과 아이노드저장 (데이터블럭 하나에 총 32개 들어감)
	char name1, name2, name3, name4, name5, name6, name7; //파일이름
	unsigned char inode; //아이노드
} name_inode;
*/


//초기화
void setup();

//아이노드
void inode_timeset(unsigned char inode);
int inode_alloc(unsigned int size);
void inode_free(unsigned char inode);
int find_empty_inode();
int is_inode_empty(int n);
void myinode(int inode);
//데이터블럭
int db_alloc();
void db_free(unsigned char db);
int find_empty_db();
int is_db_empty(int n);
void db_print(unsigned char db);
void idb_print(unsigned char single_idb);

//디렉터리
int dir_alloc(unsigned char pwd);
int dir_search(unsigned char pwd, char* name);
void dir_arr(unsigned char pwd);

//코드상에 구현된 파일
void file_write(unsigned char inode, char* str);
void idb_write(unsigned char single_idb, char* str);
void file_add(unsigned char pwd, char* name);
int file_search(unsigned char pwd, char* name);
void file_empty(unsigned char inode);
void file_rewrite(unsigned char inode, char* str);
void file_read(unsigned char inode, char** buffer);
void idb_read(unsigned char single_idb, char** buffer);

//myfs
void fs_save();
void fs_load();

//실제 파일
void realfile_read(char* name, char** buffer);
void realfile_write(char* name, char* str);

//명령어
void myrmdir(unsigned char pwd, unsigned char target_inode);
void myrm(unsigned char pwd, unsigned char target_inode);
void mymv_dir(unsigned char pwd, unsigned char target_inode, unsigned char dir_inode);
void mymv_file(unsigned char pwd, unsigned char target_inode, char* str);
void myls(unsigned char pwd);
void myls_target(unsigned char pwd, unsigned char inode);
void mycd(unsigned char* pwd, char* name);
void mycat(unsigned char inode);
void mymkdir(unsigned char pwd, const char* name);
void mymkfs(unsigned char* pwd);
void mycpto(unsigned char target_inode, char* host_dest);
void mycpfrom(char* host_name, unsigned char dest_inode);
void mycp(unsigned char pwd, unsigned char target_inode, char* name);
void mytree(unsigned char pwd, int cnt);
void mydatablock(unsigned char db);
void print_inode_db(void);
void myshowfile(int start, int end, unsigned char file_inode);
//명령어 구현을 보조하는 함수
char* now_dir(unsigned char);
int absolute_route_inode_return(char*);
int relative_route_inode_return(char*, unsigned char);
int my_strncmp(const char* a, const char* b, int n);




//void** datablock = (void*)calloc(256, sizeof(void*));
//*(datablock + i) = (name_inode*)malloc(256);
//*(datablock + i)->(데이터)