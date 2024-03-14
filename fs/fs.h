#include<time.h>

//�����_�� ��ũ��


#define route(str, pwd) ((*str == '/') ? absolute_route_inode_return(str) : relative_route_inode_return(str, pwd))

typedef struct { //���ۺ�
	unsigned long long int
		inode1, inode2, //64 * 2 = 128
		datablock1, datablock2, datablock3, datablock4; //64 * 4 = 256
} superblock;

typedef struct { //���̳��
	unsigned char db_mask; //�����ͺ��� ��뿩�θ� ��Ÿ���� ����ũ, 8��Ʈ�� �����Ǿ� ��Ʈ �ϳ��� �����ͺ� �ϳ�
	unsigned long long int //�����ͺ�, �� �����ͺ��� �ּҸ� ����
		db1 : 8, db2 : 8, db3 : 8, db4 : 8,
		db5 : 8, db6 : 8, db7 : 8, db8 : 8;
	_Bool is_dir; //�ش� ������ ���͸����� ����, 1 == ���͸�
	unsigned char single_idb; //�δ��̷�Ʈ ���� �ּҸ� ����
	_Bool is_idb; //�ش� ������ �δ��̷�Ʈ ���� ��������� ����
	struct tm time; //�ð�����
	unsigned int file_size; //�ش� ������ ũ�� ����
} inode;

/*
typedef struct { //���͸��� ���� ������ �̸��� ���̳������ (�����ͺ� �ϳ��� �� 32�� ��)
	char name1, name2, name3, name4, name5, name6, name7; //�����̸�
	unsigned char inode; //���̳��
} name_inode;
*/


//�ʱ�ȭ
void setup();

//���̳��
void inode_timeset(unsigned char inode);
int inode_alloc(unsigned int size);
void inode_free(unsigned char inode);
int find_empty_inode();
int is_inode_empty(int n);
void myinode(int inode);
//�����ͺ�
int db_alloc();
void db_free(unsigned char db);
int find_empty_db();
int is_db_empty(int n);
void db_print(unsigned char db);
void idb_print(unsigned char single_idb);

//���͸�
int dir_alloc(unsigned char pwd);
int dir_search(unsigned char pwd, char* name);
void dir_arr(unsigned char pwd);

//�ڵ�� ������ ����
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

//���� ����
void realfile_read(char* name, char** buffer);
void realfile_write(char* name, char* str);

//��ɾ�
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
//��ɾ� ������ �����ϴ� �Լ�
char* now_dir(unsigned char);
int absolute_route_inode_return(char*);
int relative_route_inode_return(char*, unsigned char);
int my_strncmp(const char* a, const char* b, int n);




//void** datablock = (void*)calloc(256, sizeof(void*));
//*(datablock + i) = (name_inode*)malloc(256);
//*(datablock + i)->(������)