#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"fs.h"

#define DEBUG printf("DEBUG\n")
_Bool debug_mode;
#define DEBUGMOD if(debug_mode == 1)

superblock super = {0, 0, 0, 0, 0, 0}; //���ۺ�(0���� �ʱ�ȭ)
inode* inode_list; //���̳��迭�� ������
char** datablock; //�����ͺ��迭�� ������

void setup() { //�ʱ� ���Ͻý��� ����, 128���� ���̳�� ����, 256���� �����ͺ��� ������ ����, ��Ʈ���͸� ����
	inode_list = (inode*)calloc(128, sizeof(inode)); //���̳�� 128�� �Ҵ�
	datablock = (char**)calloc(256, sizeof(char*)); //�����ͺ� 256�� �Ҵ�

	//��Ʈ���͸� ����
	dir_alloc(0);
	(inode_list + 0)->is_dir = 1;

	debug_mode = 0;
}

void inode_timeset(unsigned char inode) { //���̳���� �ð� ����
	time_t now = time(NULL);
	(inode_list + inode)->time = *localtime(&now);

	DEBUGMOD printf("inode_timeset : time set finish\n");

	return;
}

int find_empty_inode() { //�� inode ��ȣ����
	unsigned long long int mask = (unsigned long long int)1 << 63;

	for (int i = 0; i < 64; i++) { //0-63
		if ((super.inode1 & mask) == 0) {
			return i;
		}
		else {
			mask >>= 1;
		}
	}

	mask = (unsigned long long int)1 << 63;
	for (int i = 0; i < 63; i++) { //64 - 126
		if ((super.inode2 & mask) == 0) {
			return 64 + i;
		}
		else {
			mask >>= 1;
		}
	}

	printf("no empty inode\n"); //����
	return -1;
}

int find_empty_db() { //�� db ��ȣ����
	unsigned long long int mask = (unsigned long long int)1 << 63;

	for (int i = 0; i < 64; i++) { //0-63
		if ((super.datablock1 & mask) == 0) {
			DEBUGMOD printf("find_empty_db : found empty db %d\n", i);
			return i;
		}
		else {
			mask >>= 1;
		}
	}

	mask = (unsigned long long int)1 << 63;
	for (int i = 0; i < 64; i++) { //64 - 127
		if ((super.datablock2 & mask) == 0) {
			DEBUGMOD printf("find_empty_db : found empty db %d\n", 64 + i);
			return 64 + i;
		}
		else {
			mask >>= 1;
		}
	}

	mask = (unsigned long long int)1 << 63;
	for (int i = 0; i < 64; i++) { //127 - 191
		if ((super.datablock3 & mask) == 0) {
			DEBUGMOD printf("find_empty_db : found empty db %d\n", 128 + i);
			return 128 + i;
		}
		else {
			mask >>= 1;
		}
	}

	mask = (unsigned long long int)1 << 63;
	for (int i = 0; i < 63; i++) { //192 - 254
		if ((super.datablock4 & mask) == 0) {
			DEBUGMOD printf("find_empty_db : found empty db %d\n", 192 + i);
			return 192 + i;
		}
		else {
			mask >>= 1;
		}
	}

	printf("no empty db\n"); //����
	return -1;
}

int db_alloc() { //�����ͺ� �Ҵ� // ex : (inode_list + n)->db1 = db_alloc();
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ
	int n = find_empty_db(); //����� ã��
	if (n == -1) {
		printf("not alloced\n");
		return -1;
	}

	if (n >= 0 && n < 64) {
		super.datablock1 |= (mask >> n);
	}
	else if (n >= 64 && n < 128) {
		super.datablock2 |= (mask >> (n - 64));
	}
	else if (n >= 128 && n < 192) {
		super.datablock3 |= (mask >> (n - 128));
	}
	else if (n >= 192 && n < 256) {
		super.datablock4 |= (mask >> (n - 192));
	}
	else {
		printf("not alloced\n");
		return -1;
	}

	*(datablock + n) = (char*)calloc(256, 1);

	DEBUGMOD printf("db_alloc : db %d alloced\n", n);
	return n;


}


int idb_alloc(unsigned int size) { //���̳�带 ���� �δ��̷�Ʈ�� �Ҵ�, �δ��̷�Ʈ ���� �����ͺ��� �ϳ��� �ڸ��� ������

	int add = db_alloc(); //�����ͺ� �Ҵ�

	DEBUGMOD
	printf("allocating idb\n");

	int db_num = (size / 256) + (size % 256 != 0); //�ʿ��� db����

	(*(*(datablock + add) + 0)) = db_num - 128; //�δ��̷�Ʈ ���� ù��° ĭ�� �δ��̷�Ʈ �� ������ �����ͺ� ����, char�� -���� �����ϹǷ� 128���� �����ߴٰ� 128 ���ؼ� ����

	for (int i = 0; i < db_num; i++) { //�����ͺ��� unsigned char �� �ƴ϶� char �̹Ƿ� 0-255�� ǥ���ϱ� ���� -128�� �� ���� ����, �ٽ� �о�ö��� +128
		(*(*(datablock + add) + i + 1)) = db_alloc() - 128;
	}

	return add;
}

//���͸��� �ƴ� ���̳�� �Ҵ�
//inode �Ҵ� // ex : (inode_list + n)->single_idb = inode_alloc(~, ~);
int inode_alloc(unsigned int size) { // size : ������ ũ�� (����Ʈ����)
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ
	int n = find_empty_inode(); //����� ã��

	inode_timeset(n);

	if (n == -1) {
		printf("not alloced\n");
		return -1;
	}

	//���̳�� ����Ʈ ����, ���͸����� ����
	if (n >= 0 && n < 64) {
		super.inode1 |= (mask >> n);
		(inode_list + n)->is_dir = 0;
	}
	else if (n >= 64 && n < 128) {
		super.inode2 |= (mask >> (n - 64));
		(inode_list + n)->is_dir = 0;
	}
	else {
		printf("not alloced\n");
		return -1;
	}

	(inode_list + n)->db_mask = 0;
	//�δٷ�Ʈ�� ��
	(inode_list + n)->is_idb = 0;

	//������ ũ�� ����
	(inode_list + n)->file_size = size;

	int db_num = (size / 256) + (size % 256 != 0); //�ʿ��� db����

	if (db_num > 8) { //�ʿ��� �����ͺ��� 8�� �ʰ� (indirect �ʿ�)

		//db8�� �Ҵ�
		(inode_list + n)->db1 = db_alloc();
		(inode_list + n)->db2 = db_alloc();
		(inode_list + n)->db3 = db_alloc();
		(inode_list + n)->db4 = db_alloc();
		(inode_list + n)->db5 = db_alloc();
		(inode_list + n)->db6 = db_alloc();
		(inode_list + n)->db7 = db_alloc();
		(inode_list + n)->db8 = db_alloc();

		(inode_list + n)->db_mask = 0xff;


		//8���� �Ҵ��ϰ� ���� �����͸� �δ��̷�Ʈ���� �ٽ��Ҵ�(�� ���ڶ�� �ű⼭ �� �ݺ�)
		(inode_list + n)->single_idb = idb_alloc(size - 8 * 256);
		(inode_list + n)->is_idb = 1;
	}
	else if (db_num <= 8) { //�ʿ��� �����ͺ��� 8�� ���� (indirect ���ʿ�)
		switch (db_num) { //�����ͺ� 1-8�� �Ҵ�
		case 8:
			(inode_list + n)->db8 = db_alloc();
			(inode_list + n)->db_mask += 0x1;
		case 7:
			(inode_list + n)->db7 = db_alloc();
			(inode_list + n)->db_mask += 0x2;
		case 6:
			(inode_list + n)->db6 = db_alloc();
			(inode_list + n)->db_mask += 0x4;
		case 5:
			(inode_list + n)->db5 = db_alloc();
			(inode_list + n)->db_mask += 0x8;
		case 4:
			(inode_list + n)->db4 = db_alloc();
			(inode_list + n)->db_mask += 0x10;
		case 3:
			(inode_list + n)->db3 = db_alloc();
			(inode_list + n)->db_mask += 0x20;
		case 2:
			(inode_list + n)->db2 = db_alloc();
			(inode_list + n)->db_mask += 0x40;
		case 1:
			(inode_list + n)->db1 = db_alloc();
			(inode_list + n)->db_mask += 0x80;
		case 0:
			break;
		}
	}

	return n;

	
}

//ex : db_free((inode_list + n)->db1);
//!!! : db_free((inode_liat + n)->db1);,����� db_mask�� ������ ��
void db_free(unsigned char db) { //�ش� ��ȣ�� db�� �Ҵ������ϰ� ���ۺ��� ������
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ

	free(*(datablock + db)); //�����ͺ� �Ҵ� ����

	//���ۺ� ����
	if (db >= 0 && db < 64) {
		super.datablock1 &= ~(mask >> db);
	}
	else if (db >= 64 && db < 128) {
		super.datablock2 &= ~(mask >> (db - 64));
	}
	else if (db >= 128 && db < 192) {
		super.datablock3 &= ~(mask >> (db - 128));
	}
	else if (db >= 192 && db < 256) {
		super.datablock4 &= ~(mask >> (db - 192));
	}
}

void idb_free(unsigned char single_idb) { //�� �Լ��� �δ��̷�Ʈ�� ����, �� �Լ��� ȣ���ϸ鼭 �۵���
	int db_num = *(*(datablock + single_idb) + 0);

	for (int i = 0; i < db_num + 128; i++) {
		DEBUGMOD printf("idb_free : db_%d free\n", *(*(datablock + single_idb) + i) + 128);
		db_free(*(*(datablock + single_idb) + i + 1) + 128);
	}

	db_free(single_idb);
}

void inode_free(unsigned char inode) { //�ش� ��ȣ�� ���̳�带 �Ҵ������ϱ����� ���ۺ��� ������, ���� �����ͺ����� �ڵ����� �Ҵ�����(���� �ƴϿ��µ� �ٲ�)
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ

	//���ۺ� ����
	if (inode >= 0 && inode < 64) {
		super.inode1 &= ~(mask >> inode);
	}
	else if (inode >= 64 && inode < 128) {
		super.inode2 &= ~(mask >> (inode - 64));
	}

	if (!is_db_empty((inode_list + inode)->db1) && ((inode_list + inode)->db_mask & (unsigned char)0x80) != 0) { //�ش� db�� ������� �ʴٸ�
		db_free((inode_list + inode)->db1); //�ش� db�Ҵ�����
		(inode_list + inode)->db_mask -= 0x80;
		DEBUGMOD printf("%d ", (inode_list + inode)->db1);
	}
	if (!is_db_empty((inode_list + inode)->db2) && ((inode_list + inode)->db_mask & (unsigned char)0x40) != 0) {
		db_free((inode_list + inode)->db2);
		(inode_list + inode)->db_mask -= 0x40;
		DEBUGMOD printf("%d ", (inode_list + inode)->db2);
	}
	if (!is_db_empty((inode_list + inode)->db3) && ((inode_list + inode)->db_mask & (unsigned char)0x20) != 0) {
		db_free((inode_list + inode)->db3);
		(inode_list + inode)->db_mask -= 0x20;
		DEBUGMOD printf("%d ", (inode_list + inode)->db3);
	}
	if (!is_db_empty((inode_list + inode)->db4) && ((inode_list + inode)->db_mask & (unsigned char)0x10) != 0) {
		db_free((inode_list + inode)->db4);
		(inode_list + inode)->db_mask -= 0x10;
		DEBUGMOD printf("%d ", (inode_list + inode)->db4);
	}
	if (!is_db_empty((inode_list + inode)->db5) && ((inode_list + inode)->db_mask & (unsigned char)0x8) != 0) {
		db_free((inode_list + inode)->db5);
		(inode_list + inode)->db_mask -= 0x8;
		DEBUGMOD printf("%d ", (inode_list + inode)->db5);
	}
	if (!is_db_empty((inode_list + inode)->db6) && ((inode_list + inode)->db_mask & (unsigned char)0x4) != 0) {
		db_free((inode_list + inode)->db6);
		(inode_list + inode)->db_mask -= 0x4;
		DEBUGMOD printf("%d ", (inode_list + inode)->db6);
	}
	if (!is_db_empty((inode_list + inode)->db7) && ((inode_list + inode)->db_mask & (unsigned char)0x2) != 0) {
		db_free((inode_list + inode)->db7);
		(inode_list + inode)->db_mask -= 0x2;
		DEBUGMOD printf("%d ", (inode_list + inode)->db7);
	}
	if (!is_db_empty((inode_list + inode)->db8) && ((inode_list + inode)->db_mask & (unsigned char)0x1) != 0) {
		db_free((inode_list + inode)->db8);
		(inode_list + inode)->db_mask -= 0x1;
		DEBUGMOD printf("%d ", (inode_list + inode)->db8);
	}
	DEBUGMOD printf("\n");

	if ((inode_list + inode)->is_idb == 1) { //�δ��̷�Ʈ ���� ������� �ʴٸ�
		idb_free((inode_list + inode)->single_idb); //�ش� ���̳�� �Ҵ�����
		(inode_list + inode)->is_idb = 0; //�ش� �δ��̷�Ʈ ���� �Ҵ������ ǥ��

		DEBUGMOD printf("inode_free : single_idb unalloced\n");
	}

}

//���͸��� ���̳�� �Ҵ�
//�����ͺ��� 0-6�̸� + 7���̳��, 8-14�̸�, 15���̳�� ...
int dir_alloc(unsigned char pwd) { //pwd : �θ���͸�
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ
	int n = find_empty_inode(); //����� ã��
	if (n == -1) {
		printf("not alloced\n");
		return -1;
	}

	inode_timeset(n);
	(inode_list + n)->file_size = 256;

	//���̳�� ����Ʈ ����, ���͸����� ����
	if (n >= 0 && n < 64) {
		super.inode1 |= (mask >> n);
		(inode_list + n)->is_dir = 1;
	}
	else if (n >= 64 && n < 128) {
		super.inode2 |= (mask >> (n - 64));
		(inode_list + n)->is_dir = 1;
	}
	else {
		printf("not alloced\n");
		return -1;
	}

	(inode_list + n)->db1 = db_alloc(); //���͸����� �ϳ��� �����ͺ��� �Ҵ�
	(inode_list + n)->db_mask = 0x80;

	(inode_list + n)->is_idb = 0;;

	//���͸� �����ͺ��� �۵���� :
	//0, 8, 16... ��° �� : �ش� ������ ���̳���ȣ ����, -1�� �Ҵ�� ���̳�尡 ���ٴ� ��
	//1-7, 9-15, 17-23...��° �� : 0, 8, 16...��° ���� ���̳�忡 �ش��ϴ� ������ �̸�����, �� 7����
	//�������� �ι��ڸ� �뷮�������� ���������Ƿ� �����ͺ��� ���Ͽ� %s�� ������ ���X
	for (int i = 0; i < 256; i++) {
		*(*(datablock + (inode_list + n)->db1) + i) = '\0'; //�ش� �����ͺ� ���θ� '\0'�� �ʱ�ȭ
		if (i % 8 == 0) {
			*(*(datablock + (inode_list + n)->db1) + i) = -1; //0, 8, 16...��° �� -1�� �ʱ�ȭ
		}
	}

	*(*(datablock + (inode_list + n)->db1) + 0) = n; //����ο��� '.'�� ���� ���͸��� �ǹ�, . == �� ���͸��� ���̳���ȣ
	*(*(datablock + (inode_list + n)->db1) + 1) = '.';

	*(*(datablock + (inode_list + n)->db1) + 8) = pwd; //����ο��� ".."�� �θ� ���͸��� �ǹ�, .. == �θ� ���͸� ���̳���ȣ
	sprintf((*(datablock + (inode_list + n)->db1) + 9), "..");

	/*
	* (0 == '\0' == NULL)
	* �ش� �����ͺ� ���� : 
	* [n]['.'][0][0][0][0][0][0]
	* [pwd]['.']['.'][0][0][0][0][0]
	* [-1][0][0][0][0][0][0][0]
	* [-1][0][0][0][0][0][0][0]
	...
	*/

	return n;
}

int dir_search(unsigned char pwd, char* name) { //pwd : Ȯ���ϰ���� ���͸��� �����ϴ� ���͸��� ���̳��, name : ã�����ϴ� ������ �̸�
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd���� ���� �˻������ �Ǵ� ���͸��ּ� ����

	int len = 0;
	for (int i = 0; 8 * i < 256; i++) {

		if (*(p + (8 * i)) != -1) { //�̸��� ���� ���� Ȯ��, 8n��° ���� -1�̶�� ������ ���°��̹Ƿ� ����

			len = 0; //�̸��� ���� ��
			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1��° ������ 7���ڰ� �ǰų� '\0'�� ���Ë����� len++, ������ �̸� ��������
				len++;
			}

			//strncmp(name, (p + (8 * i) + 1), len) == 0
			if (strlen(name) == len && my_strncmp(name, (p + (8 * i) + 1), len)) { //�� ������ �̸��� ���̿� name�� ���̰� ������ Ȯ�� 
				if (!(inode_list + *(p + (8 * i)))->is_dir) { //�� ������ ���͸��������� Ȯ��
					return -1; //�ش� ������ �̸��� ������ ���͸��� �ƴ�
				}
				return *(p + (8 * i)); //�̸��� name�� ���͸����� �߰�, �� ������ ���̳�尪 ����
			}
		}
	}

	return -2; //�ش� �̸��� ������ �������� ����
}

void dir_arr(unsigned char pwd) { //���͸� ���θ� ���������� ������
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd���� ���� �˻������ �Ǵ� ���͸��ּ� ����
	unsigned char* arr;
	unsigned char tmp;
	char** name_arr;
	char* name_tmp;
	int cnt = 0, cnt_2 = 0;

	for (int i = 2; 8 * i < 256; i++) { //������ ���� ����
		if (*(p + (8 * i)) != -1) {
			cnt++;
		}
	}
	DEBUGMOD printf("dir_arr : cnt = %d\n", cnt);

	arr = malloc(sizeof(unsigned char) * cnt);
	name_arr = malloc(sizeof(char*) * cnt);

	//���ϸ�� �а� ����
	cnt = 0;
	for (int i = 2; 8 * i < 256; i++) {
		if (*(p + (8 * i)) != -1) { //���� �߰�
			*(arr + cnt) = *(p + (8 * i)); //���̳�� ����
			DEBUGMOD printf("dir_arr : inode copied, %d\n", *(p + (8 * i)));
			*(p + (8 * i)) = -1;
			*(name_arr + cnt) = calloc(8, sizeof(char));

			DEBUGMOD printf("dir_arr : name copy start : ");
			for (int j = 1;j < 8 && *(p + (8 * i) + j) != '\0'; j++) { //�����̸� ����
				*(*(name_arr + cnt) + j - 1) = *(p + (8 * i) + j);
				DEBUGMOD printf("%c", *(p + (8 * i) + j));
				*(p + (8 * i) + j) = '\0';
			}
			DEBUGMOD printf("\ndir_arr : name copied\n");

			cnt++;
		}
	}

	//�о�� ���ϸ�� ������ ����
	for (int j = 0; j < cnt - 1; j++) {
		for (int i = 0; i < cnt - 1; i++) {
			if (strcmp(*(name_arr + i), *(name_arr + i + 1)) > 0) { //������ ��
				tmp = *(arr + i);
				strcpy(name_tmp, *(name_arr + i));

				*(arr + i) = *(arr + i + 1);
				strcpy(*(name_arr + i), *(name_arr + i + 1));

				*(arr + i + 1) = tmp;
				strcpy(*(name_arr + i + 1), name_tmp);

				DEBUGMOD printf("dir_add : swap done, %d, %d\n", j, i);
			}
		}
	}

	//���ĵ� ���� �ٽ� ���͸��� �Է�
	cnt_2 = 0;
	for (int i = 2; 8 * i < 256 && cnt_2 < cnt; i++) {
		if (*(p + (8 * i)) == -1) { //����� �߰�
			*(p + (8 * i)) = *(arr + cnt_2); //���̳�� �ٿ��ֱ�
			DEBUGMOD printf("dir_arr : inode pasted, %d\n", *(p + (8 * i)));

			DEBUGMOD printf("dir_arr : name paste start : ");
			for (int j = 1;j < 8 && *(*(name_arr + cnt_2) + j - 1) != '\0'; j++) { //�̸� �ٿ��ֱ�
				*(p + (8 * i) + j) = *(*(name_arr + cnt_2) + j - 1);
				DEBUGMOD printf("%c", *(p + (8 * i) + j));
			}
			DEBUGMOD printf("\ndir_arr : name pasted\n");

			cnt_2++;
		}
	}

	for (int i = 0; i < cnt - 1; i++) { //�Ҵ�����
		free(*(name_arr + i));
	}
	free(arr);
	free(name_arr);
}

int is_inode_empty(int n) { //�ش� ��ȣ�� ���̳�尡 �� �������� ���ۺ��� ���� Ȯ��
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ
	if (n < 0) {
		printf("input integer that >= 0\n");
		return -1;
	}

	mask >>= (n % 64);

	if (n >= 0 && n < 64) {
		return ((super.inode1 & mask) == 0);
	}
	else if (n >= 64 && n < 128) {
		return ((super.inode2 & mask) == 0);
	}
}

int is_db_empty(int n) { //�ش� ��ȣ�� db�� �� �������� ���ۺ��� ���� Ȯ��
	unsigned long long int mask = (unsigned long long int)1 << 63; //����ũ
	if (n < 0) {
		printf("input integer that >= 0\n");
		return -1;
	}

	mask >>= (n % 64);

	if (n >= 0 && n < 64) {
		return (super.datablock1 & mask) == 0;
	}
	else if (n >= 64 && n < 128) {
		return (super.datablock2 & mask) == 0;
	}
	else if (n >= 128 && n < 192) {
		return (super.datablock3 & mask) == 0;
	}
	else if (n >= 192 && n < 256) {
		return (super.datablock4 & mask) == 0;
	}
}

//�ϳ��� �����ͺ��� ���Ͽ� 256�ڸ� ��� ����ϰų� '\0'�� ���ö����� ���
void db_print(unsigned char db) {
	char c;
	for (int i = 0; i < 256 && *(*(datablock + db) + i) != '\0'; i++)
		putchar(*(*(datablock + db) + i));
}

void idb_print(unsigned char single_idb) { //�� �Լ��� idb����

	int db_num = *(*(datablock + single_idb) + 0) + 128;

	for (int i = 0; i < db_num; i++) {
		DEBUGMOD printf("idb_print : printing db_%d\n", *(*(datablock + single_idb) + i + 1) + 128);
		db_print(*(*(datablock + single_idb) + i + 1) + 128);
	}

	printf("\n");
}

//�ش� ��ũ�δ� file_write���� ����, �Լ��Ʒ��� #undef�� �����Ǿ�����
#define db_set(n) \
for (int i = 0; i < ((strlen(str + (n - 1) * 256 + 1) < 256) ? strlen(str + (n - 1) * 256) + 1 : 256); i++) {\
	*(*(datablock + (inode_list + inode)->db##n) + i) = *(str + (n - 1) * 256 + i);}
//���ڿ��� ������ db�� ����, db�� �̹� ����� �Ҵ�Ǿ��ִٰ� �����ϰ� ����ǹǷ� db_alloc()���� ������ ������ŭ �Ҵ����ִ� ���� �ʿ�
//file_rewrite()���� �̹� �� �Ǿ�����
void file_write(unsigned char inode, char* str) {
	DEBUGMOD
	printf("file_write : string length : %lu\n", strlen(str));

	int db_num = (strlen(str)) / 256 + ((strlen(str)) % 256 != 0); //���ڿ� ������ ���� �ʿ��� db�� ���� ���

	if (db_num <= 8) {
		switch (db_num) {
		case 8:
			db_set(8)
		case 7:
			db_set(7)
		case 6:
			db_set(6)
		case 5:
			db_set(5)
		case 4:
			db_set(4)
		case 3:
			db_set(3)
		case 2:
			db_set(2)
		case 1:
			db_set(1)
		case 0:
			break;
		}
	}
	else {
		db_set(8)
		db_set(7)
		db_set(6)
		db_set(5)
		db_set(4)
		db_set(3)
		db_set(2)
		db_set(1);

		db_num -= 8;

		//���� ���ڿ� ����Լ��� ����
		//(inode_list + inode)->single_idb = idb_alloc(db_num * 256);
		DEBUGMOD printf("file_write : idb_write start\n");
		idb_write((inode_list + inode)->single_idb, (str + 8 * 256));
		(inode_list + inode)->is_idb = 1;
	}
}
#undef db_set

#define db_set(n) \
for (int i = 0; i < ((strlen(str + (n - 1) * 256 + 1) < 256) ? strlen(str + (n - 1) * 256) + 1 : 256); i++) {\
	*(*(datablock + (*(*(datablock + single_idb) + j) + 128) ) + i) = *(str + (n - 1) * 256 + i);}
void idb_write(unsigned char single_idb, char* str) { //�δ��̷�Ʈ ���� �о ���� �����ͺ��� ���ڿ� ����, �ַ� file_write���� ����
	int db_num = *(*(datablock + single_idb) + 0) + 128;
	DEBUGMOD{
		printf("idb_write : db_num = %d\n", db_num); 
		printf("start with %c\n", *str);
	}

	for (int j = 0; j < db_num; j++) {
		DEBUGMOD printf("idb_write : db_%d wrote\n", *(*(datablock + single_idb) + j + 1) + 128);
		db_set(j + 1);
	}
}
#undef db_set

//�ش� ���͸��� ������� ���� ����, dir_add()�� ���� ����
void file_add(unsigned char pwd, char* name) { //pwd : ������ ������ ���͸��� ���̳��, name : ������ ������ �̸�
	if (strlen(name) > 7) { //�����̸� 7��������
		printf("filename is too long\n");
		return ;
	}
	for (int i = 0; *(name + i) != '\0'; i++) {
		switch (*(name + i)) {
		case '/':
			printf("filename contains impossible text (ex : '/')\n");
			return;
		}
	}

	char tmp;

	char* p = *(datablock + (inode_list + pwd)->db1); //p = ���� ���͸�(== ������ �߰��� ���͸�) �� ���̳���� �����ͺ�

	/*if (file_search(pwd, name) >= 0) {
		printf("%s already exist\n", name);
		return;
	}*/

	for (int i = 0; 8 * i < 256; i++) { //�����ͺ��� 0, 8, 16...��° �� Ȯ�� �� �� ���� �˻�
		if (*(p + (8 * i)) == -1) { //-1�� ��������� �ǹ�
			if((8 * i) + 8 < 256)
				tmp = *(p + (8 * i) + 8);

			*(p + (8 * i)) = inode_alloc(0); //�ش� ������� �� ������ ���̳�� �Ҵ�
			sprintf(p + (8 * i) + 1, "%s", name);

			if ((8 * i) + 8 < 256)
				*(p + (8 * i) + 8) = tmp; 
			return;
		}
	}

	printf("this directory is full, 1 directory can contain 32 files\n"); //32ĭ�� �̹� ����á�ٸ� �����ź�

	return;
}

//�ش� ���͸����� ���ϴ� �̸��� ���� ã��, �ִٸ� �� ������ ���̳�帮��, ������ -2, -1����
int file_search(unsigned char pwd, char* name) {

	char* p = *(datablock + (inode_list + pwd)->db1); //pwd���� ���� �˻������ �Ǵ� ���͸��ּ� ����

	int len = 0;
	for (int i = 0; 8 * i < 256; i++) {

		if (*(p + (8 * i)) != -1) { //�̸��� ���� ���� Ȯ��, 8n��° ���� -1�̶�� ������ ���°��̹Ƿ� ����

			len = 0; //�̸��� ���� ��
			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1��° ������ 7���ڰ� �ǰų� '\0'�� ���Ë����� len++, ������ �̸� ��������
				len++;
			}
			DEBUGMOD printf("file_search : len = %d\n", len);

			//strncmp(name, (p + (8 * i) + 1), len) == 0
			DEBUGMOD printf("file_search : %d\n", my_strncmp(name, (p + (8 * i) + 1), len));
			if (strlen(name) == len && my_strncmp(name, (p + (8 * i) + 1), len)) { //�� ������ �̸��� ���̿� name�� ���̰� ������ Ȯ�� 
				if ((inode_list + *(p + (8 * i)))->is_dir) { //�� ������ �Ϲ��������� Ȯ��
					return -1; //�ش� ������ �̸��� ������ �Ϲ������� �ƴ�
				}
				return *(p + (8 * i)); //�̸��� name�� �Ϲ����� �߰�, �� ������ ���̳�尪 ����
			}
		}
	}

	return -2; //�ش� �̸��� ������ �������� ����
}

//������ ������ ���� ���������� db�� �Ҵ�������, ������ ���븸 �������� ������ü�� ������
void file_empty(unsigned char inode) {

	if (!is_db_empty((inode_list + inode)->db1) && ((inode_list + inode)->db_mask & (unsigned char)0x80) != 0){ //�ش� db�� ������� �ʴٸ�
		db_free((inode_list + inode)->db1); //�ش� db�Ҵ�����
		(inode_list + inode)->db_mask -= 0x80; //���̳���� db_mask ����
	}
	if (!is_db_empty((inode_list + inode)->db2) && ((inode_list + inode)->db_mask & (unsigned char)0x40) != 0) {
		db_free((inode_list + inode)->db2);
		(inode_list + inode)->db_mask -= 0x40;
	}
	if (!is_db_empty((inode_list + inode)->db3) && ((inode_list + inode)->db_mask & (unsigned char)0x20) != 0) {
		db_free((inode_list + inode)->db3);
		(inode_list + inode)->db_mask -= 0x20;
	}
	if (!is_db_empty((inode_list + inode)->db4) && ((inode_list + inode)->db_mask & (unsigned char)0x10) != 0) {
		db_free((inode_list + inode)->db4);
		(inode_list + inode)->db_mask -= 0x10;
	}
	if (!is_db_empty((inode_list + inode)->db5) && ((inode_list + inode)->db_mask & (unsigned char)0x8) != 0) {
		db_free((inode_list + inode)->db5);
		(inode_list + inode)->db_mask -= 0x8;
	}
	if (!is_db_empty((inode_list + inode)->db6) && ((inode_list + inode)->db_mask & (unsigned char)0x4) != 0) {
		db_free((inode_list + inode)->db6);
		(inode_list + inode)->db_mask -= 0x4;
	}
	if (!is_db_empty((inode_list + inode)->db7) && ((inode_list + inode)->db_mask & (unsigned char)0x2) != 0) {
		db_free((inode_list + inode)->db7);
		(inode_list + inode)->db_mask -= 0x2;
	}
	if (!is_db_empty((inode_list + inode)->db8) && ((inode_list + inode)->db_mask & (unsigned char)0x1) != 0) {
		db_free((inode_list + inode)->db8);
		(inode_list + inode)->db_mask -= 0x1;
	}

	if ((inode_list + inode)->is_idb == 1) { //�δ��̷�Ʈ ���� ������� �ʴٸ�
		idb_free((inode_list + inode)->single_idb); //�ش� �δ��̷�Ʈ ���� ������ ���� ���Ͻ���
		(inode_list + inode)->is_idb = 0; //�ش� �δ��̷�Ʈ ���� ������ NULL�� ����
	}

	(inode_list + inode)->file_size = 0;

}

//�ش� ���̳���� ������ ������ �����, str�� �����ϱ����� �ʿ��� db�� ������ ��� �� �Ҵ��ϰ� ������ ������
void file_rewrite(unsigned char inode, char* str) {

	file_empty(inode); //���� ���ۼ� ������ �����ͺ� �����

	int db_num = ((strlen(str)) / 256) + ((strlen(str)) % 256 != 0); //�ʿ��� db����

	if (db_num > 8) { //�ʿ��� �����ͺ��� 8�� �ʰ� (indirect �ʿ�)

		//db8�� �Ҵ�
		(inode_list + inode)->db1 = db_alloc();
		(inode_list + inode)->db2 = db_alloc();
		(inode_list + inode)->db3 = db_alloc();
		(inode_list + inode)->db4 = db_alloc();
		(inode_list + inode)->db5 = db_alloc();
		(inode_list + inode)->db6 = db_alloc();
		(inode_list + inode)->db7 = db_alloc();
		(inode_list + inode)->db8 = db_alloc();

		(inode_list + inode)->db_mask = 0xff;

		//8���� �Ҵ��ϰ� ���� �����͸� �δ��̷�Ʈ���� �ٽ��Ҵ�(�� ���ڶ�� �ű⼭ �� �ݺ�)
		(inode_list + inode)->single_idb = idb_alloc((strlen(str)) - 8 * 256);
		(inode_list + inode)->is_idb = 1;
	}
	else if (db_num <= 8) { //�ʿ��� �����ͺ��� 8�� ���� (indirect ���ʿ�)
		switch (db_num) { //�����ͺ� 1-8�� �Ҵ�
		case 8:
			(inode_list + inode)->db8 = db_alloc();
			(inode_list + inode)->db_mask += 0x1;
		case 7:
			(inode_list + inode)->db7 = db_alloc();
			(inode_list + inode)->db_mask += 0x2;
		case 6:
			(inode_list + inode)->db6 = db_alloc();
			(inode_list + inode)->db_mask += 0x4;
		case 5:
			(inode_list + inode)->db5 = db_alloc();
			(inode_list + inode)->db_mask += 0x8;
		case 4:
			(inode_list + inode)->db4 = db_alloc();
			(inode_list + inode)->db_mask += 0x10;
		case 3:
			(inode_list + inode)->db3 = db_alloc();
			(inode_list + inode)->db_mask += 0x20;
		case 2:
			(inode_list + inode)->db2 = db_alloc();
			(inode_list + inode)->db_mask += 0x40;
		case 1:
			(inode_list + inode)->db1 = db_alloc();
			(inode_list + inode)->db_mask += 0x80;
			break;
		}
	}

	file_write(inode, str); //���ο� ���� �ۼ�
	(inode_list + inode)->file_size = strlen(str);
}

void file_read(unsigned char inode, char** buffer) { //������ ������ �о ���ۿ� ����, ���۸� �Ҵ������� ���Ҵ�
	int db_num = ((inode_list + inode)->file_size / 256) + ((inode_list + inode)->file_size % 256 != 0); //�����ͺ��� ���� ���

	free(*buffer); //������ ���Ҵ� ���� ���۸� �Ҵ�������
	*buffer = calloc((inode_list + inode)->file_size, sizeof(char)); //������ ũ�⸸ŭ ���� ���Ҵ�

	if (db_num <= 8) { //�����ͺ��� 8�� ����
		switch (db_num) {
		case 8:
			strcat(*buffer, *(datablock + (inode_list + inode)->db8)); //�����ͺ��� �о���ۿ� ������
		case 7:
			strcat(*buffer, *(datablock + (inode_list + inode)->db7));
		case 6:
			strcat(*buffer, *(datablock + (inode_list + inode)->db6));
		case 5:
			strcat(*buffer, *(datablock + (inode_list + inode)->db5));
		case 4:
			strcat(*buffer, *(datablock + (inode_list + inode)->db4));
		case 3:
			strcat(*buffer, *(datablock + (inode_list + inode)->db3));
		case 2:
			strcat(*buffer, *(datablock + (inode_list + inode)->db2));
		case 1:
			strcat(*buffer, *(datablock + (inode_list + inode)->db1));
		case 0:
			break;
		}
	}
	else { //�����ͺ��� 8�� �ʰ�
		strcat(*buffer, *(datablock + (inode_list + inode)->db8));
		strcat(*buffer, *(datablock + (inode_list + inode)->db7));
		strcat(*buffer, *(datablock + (inode_list + inode)->db6));
		strcat(*buffer, *(datablock + (inode_list + inode)->db5));
		strcat(*buffer, *(datablock + (inode_list + inode)->db4));
		strcat(*buffer, *(datablock + (inode_list + inode)->db3));
		strcat(*buffer, *(datablock + (inode_list + inode)->db2));
		strcat(*buffer, *(datablock + (inode_list + inode)->db1));

		//���� ���ڿ� �δ��̷�Ʈ ���� ����
		idb_read((inode_list + inode)->single_idb, buffer);
		DEBUGMOD printf("file_read : idb_read finished\n");
	}
}

void idb_read(unsigned char single_idb, char** buffer) { //�δ��̷�Ʈ ���� �о��, ���� file_read�� ���ӵǾ� ���� ������ ����
	int db_num = *(*(datablock + single_idb) + 0) + 128;

	for (int i = 0; i < db_num; i++) {
		strcat(*buffer, *(datablock + *(*(datablock + single_idb) + i + 1) + 128));
		DEBUGMOD printf("idb_read : (%d/%d) strcat db_%d", i, db_num, *(*(datablock + single_idb) + i + 1) + 128);
	}
}

//���� ���Ͻý����� ���¸� myfs�� ����
void fs_save() {
	FILE* myfs = fopen("myfs", "wb");
	if (myfs == NULL) { //������ �ҷ����� ���ߴٸ�
		printf("failed to open myfs, fs not saved\n");
		return;
	}

	fwrite(&super, sizeof(super), 1, myfs); //���ۺ� ����
	DEBUGMOD
	printf("superblock saved\n");

	fwrite(inode_list, sizeof(inode), 128, myfs); //���̳�� ����

	DEBUGMOD
	printf("inode_list saved\n");

	//�δ��̷�Ʈ ���� ��������� �ٲ�鼭 ������
	//�δ��̷�Ʈ�� ����
	//idb* current_idb;
	//for (int i = 0; i < 128; i++) {
	//	if (!is_inode_empty(i) && (inode_list + i)->single_idb != NULL) { //i��° ���̳�尡 ������̸� idb�� ������̶��
	//		current_idb = (inode_list + i)->single_idb;
	//		while (current_idb != NULL) { //idb�� ��� ���󰡸� ����
	//			fwrite(current_idb, sizeof(idb), 1, myfs);
	//			current_idb = current_idb->single_idb;

	//			DEBUGMOD
	//			printf("single indirect block of inode%d saved\n", i);
	//		}
	//	}
	//}

	for (int i = 0; i < 256; i++) {
		if (!is_db_empty(i)) { //i��° db�� ������̶��
			fwrite(*(datablock + i), sizeof(char), 256, myfs); //�ش� db����

			DEBUGMOD
			printf("datablock%d saved\n", i);
		}
	}

	fclose(myfs);
	DEBUGMOD printf("fs_save : myfs closed\n");
}

//myfs�κ��� ���Ͻý����� ���� �о��
void fs_load() {
	FILE* myfs = fopen("myfs", "rb");
	if (myfs == NULL) { //������ �ҷ����� ���ߴٸ�
		printf("failed to open myfs, fs not loaded\n");
		return;
	}

	fread(&super, sizeof(super), 1, myfs); //���ۺ� �ҷ���

	DEBUGMOD
	printf("superblock loaded\n");

	fread(inode_list, sizeof(inode), 128, myfs); //���̳�� ����Ʈ �ҷ���

	DEBUGMOD
	printf("inode_list loaded\n");

	//�δ��̷�Ʈ ���� ��������� �ٲ�鼭 ������
	//idb�ҷ�����
	//idb** current_idb;
	//for (int i = 0; i < 128; i++) {
	//	if (!is_inode_empty(i) && (inode_list + i)->single_idb != NULL) { //i��° ���̳�尡 ������̸� idb�� ������̶��
	//		(inode_list + i)->single_idb = malloc(sizeof(idb)); //�ش� idb�� �޸��Ҵ�
	//		current_idb = &((inode_list + i)->single_idb);
	//		fread(*current_idb, sizeof(idb), 1, myfs); //�ش� idb�� ���� �о����
	//		current_idb = &((*current_idb)->single_idb); //���� idb�� ����

	//		DEBUGMOD
	//		printf("single indirect block of inode%d loaded\n", i);

	//		while (*current_idb != NULL) { //���� ������ idb�� ���Ͽ� NULL�� ���ö����� �ݺ�
	//			*current_idb = malloc(sizeof(idb));
	//			fread(*current_idb, sizeof(idb), 1, myfs);
	//			current_idb = &((*current_idb)->single_idb);

	//			DEBUGMOD
	//			printf("single indirect block of inode%d loaded\n", i);
	//		}
	//		
	//	}
	//}

	//�����ͺ� �о����
	for (int i = 0; i < 256; i++) {
		if (!is_db_empty(i)) { //i��° db�� ������̶��
			*(datablock + i) = (char*)calloc(256, 1); //�޸��Ҵ�
			fread(*(datablock + i), sizeof(char), 256, myfs); //������ �о����

			DEBUGMOD
			printf("datablock%d loaded\n", i);
		}
	}

	fclose(myfs);
}

//���� ������ �о ���ۿ� ��������
void realfile_read(char* name, char** buffer) { //���۰� �Ҵ������� �� �ٽ� �Ҵ��, file_read�� ����
	FILE* file = fopen(name, "r"); //���� ����
	if (file == NULL) { //���� ���� ����
		printf("failed to open %s, function didn't worked\n", name);
		*buffer = NULL;
		return;
	}

	int len = 0;
	char c; 
	while ((c = getc(file)) != EOF) { //���� ���� ����
		len++;
	}

	DEBUGMOD
	printf("file length : %d\n", len);

	free(*buffer);
	*buffer = calloc(len, sizeof(char)); //������ ���̸�ŭ �޸��Ҵ�
	rewind(file); //������ Ŀ�� ó������


	for (int i = 0; i < len && (c = getc(file)) != EOF; i++) { //���ۿ� ���� ���� ����

		*(*buffer + i) = c;
	}

	fclose(file); //���� ����
	return;
}

void realfile_write(char* name, char* str) {
	FILE* file = fopen(name, "w"); //���� ����
	if (file == NULL) { //���� ���� ����
		printf("failed to open %s, function didn't worked\n", name);
		return;
	}

	for (int i = 0; *(str + i) != '\0'; i++) { //���Ͽ� ���� �Է��ϱ�
		putc(*(str + i), file);
	}

	fclose(file); //���� ����
}





void myrmdir(unsigned char pwd, unsigned char target_inode) { //���͸� ����

	if (target_inode == pwd) { //���� ���͸� ���� �Ұ���
		printf("can't remove current directory\n");
		return;
	}

	for (int i = 2; 8 * i < 256; i++) { //Ÿ�� ���͸��� ������".", ".."�� ������ ������ �����ϴ��� Ȯ��
		if (*(*(datablock + (inode_list + target_inode)->db1) + 8 * i) != -1) {
			printf("that directory is not an empty directory\n");
			return;
		}
	}

	DEBUGMOD
		printf("myrmdir : %d is empty\n", target_inode);

	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //Ÿ�� ���͸��� ���� ���͸����� Ÿ�� ���͸��� ���̳�� Ž��
			DEBUGMOD
				printf("myrmdir : found target\n");

			*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1; //���� ���͸����� �ش� ���̳����� -1�� ��ü
			for (int j = 1; j < 8; j++) { //�̸� '\0'�� ��ü
				*(*(datablock + (inode_list + pwd)->db1) + i * 8 + j) = '\0';
			}
			DEBUGMOD
				printf("myrmdir : target removed\n");

			inode_free(target_inode); //�ش� ���̳�� �Ҵ� ����
		}
	}
}

void myrm(unsigned char pwd, unsigned char target_inode) { //���� ����

	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //��� ���̳���� ���� Ž��
			DEBUGMOD
				printf("myrm : found target : %d\n", *(*(datablock + (inode_list + pwd)->db1) + i * 8));

			*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1;
			for (int j = 1; j < 8; j++) { //�̸� ��ü '\0'�� ��ü
				*(*(datablock + (inode_list + pwd)->db1) + i * 8 + j) = '\0';
			}
			DEBUGMOD
				printf("myrm : target removed from direcoty\n");

			inode_free(target_inode); //������ ���̳�� �Ҵ� ����, ���� �����ͺ��� �Ҵ������� 
			DEBUGMOD
				printf("myrm : target removed from filesystem\n");
		}
	}
}

void mymv_dir(unsigned char pwd, unsigned char target_inode, unsigned char dir_inode) { //��� ������ ��ġ�� �ٸ� ���͸��� �ű�
	char* name = (char*)malloc(7);
	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //��� ���̳���� ���� Ž��
			DEBUGMOD
				printf("mymv_dir : found target from directory, ready to copy\n");

			for (int j = 1; j < 8; j++) { //������ �̸� ����
				*(name + j - 1) = *(*(datablock + (inode_list + pwd)->db1) + i * 8 + j);

				DEBUGMOD
					printf("%c", *(*(datablock + (inode_list + pwd)->db1) + i * 8 + j));
			}
			DEBUGMOD
				printf("mymv_dir : target name copied %s\n", name);
		}
	}

	for (int i = 0; 8 * i < 256; i++) {
		if (*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i) == -1) {//�Ű��� ���͸����� ����� Ž��
			DEBUGMOD
				printf("mymv_dir : empty place found\n");

			*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i) = target_inode; //����� ���̳�� �ٿ��ֱ�
			DEBUGMOD
				printf("mymv_dir : pasted inode %d\n", target_inode);

			for (int j = 1; j < 8 && *(name + j - 1) != '\0'; j++) { //��� ���͸��� ������ �̸� �ٿ��ֱ�
				*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i + j) = *(name + j - 1);
			}
			DEBUGMOD
				printf("mymv_dir : name of %s pasted to direcory\n", name);

			for (int i = 0; i * 8 < 256; i++) {
				if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //������ ���͸����� ���� ���� ���� ����

					DEBUGMOD
						printf("mymv_dir : found target from directory, ready to remove\n");

					*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1; //���͸��� �ش� ���̳�尪 -1�� ��ü
					for (int j = 1; j < 8; j++) { //�̸��� 7����Ʈ ��ü '\0'�� ��ü
						*(*(datablock + (inode_list + pwd)->db1) + i * 8 + j) = '\0';
					}

					DEBUGMOD
						printf("mymv_dir : target removed from direcoty\n");
				}
			}
				
			free(name);
			return;
		}
	}

	printf("there is no empty place in directory\n"); //��� ���͸��� ����� ����
	free(name);
	return;
}

void mymv_file(unsigned char pwd, unsigned char target_inode, char* str) { //������ �̸��� ����
	if (file_search(pwd, str) != -2) { //������ �̹� ������
		printf("%s is already exist\n", str);
		return;
	}
	if (strlen(str) > 7) { //�����̸� 7��������
		printf("filename is too long\n");
		return;
	}
	for (int i = 0; *(str + i) != '\0'; i++) { //�̸��� �Ұ����� ���� ����
		switch (*(str + i)) {
		case '/':
			printf("filename contains impossible text (ex : '/')\n");
			return;
		}
	}
	

	DEBUGMOD
		printf("mymv_file : start change name to %s\n", str);

	for (int i = 0; 8 * i < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + 8 * i) == target_inode) { //���͸����� �̸��� �ٲٰ��� �ϴ� ������ ���̳�� Ž��
			DEBUGMOD
				printf("mymv : found target\n");

			for (int j = 0; (j < ((strlen(str) + 1 < 7) ? strlen(str) + 1 : 7)); j++) { //�̸� ����
				DEBUGMOD
					printf("mymv : '%c' to '%c' (i = %d, j = %d)\n", *(*(datablock + (inode_list + pwd)->db1) + 8 * i + j + 1), *(str + j), i, j);

				*(*(datablock + (inode_list + pwd)->db1) + 8 * i + j + 1) = *(str + j);
			}

			DEBUGMOD
				printf("mymv : target name changed to %s\n", str);

			return;
		}
	}
}

//�� �߰��Ϸ�
//�ش� ���͸��� ���ϵ� ���
void myls(unsigned char pwd) { //pwd : Ȯ���ϰ���� ���͸��� ���̳��
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd���� ���� Ȯ���ϰ���� ���͸��ּ� ����
	struct tm tm;

	dir_arr(pwd); //������� ���� ���� ������

	for (int i = 0; (8 * i) < 256; i++) { //0, 8, 16 ...��° �� Ȯ�� �� ������� ���� ���� �˻�
		if (*(p + (8 * i)) != -1) { //-1�� ��������� �ǹ�

			DEBUGMOD
				printf("myls : printing data of inode %d\n", *(p + (8 * i)));

			tm = (inode_list + *(p + (8 * i)))->time;
			printf(" %4d/%2d/%2d ",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //�� ������ ���̳�� ǥ��(���߿� ������ �� ����)
			printf("%2d:%2d:%2d ", tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("%9s ", (inode_list + *(p + (8 * i)))->is_dir ? "directory" : "file");
			printf("%u ", *(p + (8 * i)) + 1);
			printf("%u bytes ", (inode_list + *(p + (8 * i)))->file_size);

			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //�� ������ �̸� ���
				printf("%c", *(p + (8 * i) + j)); //������ �ι��ڸ� ǥ������ �ʱ⶧���� %s�� ��� �Ұ���
			}
			
			printf("\n");
		}
	}
}

void myls_target(unsigned char pwd, unsigned char inode) { //pwd : Ȯ���ϰ���� ������ ���̳�� (myls�� ���, �� �ϳ��� �Ϲ����ϸ��� ������� ����Ǳ����� ����)
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd���� ���� Ȯ���ϰ���� ���͸��ּ� ����
	struct tm tm;

	for (int i = 0; (8 * i) < 256; i++) { //0, 8, 16 ...��° �� Ȯ�� �� ������� ���� ���� �˻�
		if (*(p + (8 * i)) == inode) { //-1�� ��������� �ǹ�

			DEBUGMOD
				printf("myls_target : printing data of inode %d\n", *(p + (8 * i)));

			tm = (inode_list + *(p + (8 * i)))->time;
			printf(" %4d/%2d/%2d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //�� ������ ���̳�� ǥ��(���߿� ������ �� ����)
			printf("%2d:%2d:%2d ", tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("%9s ", (inode_list + *(p + (8 * i)))->is_dir ? "directory" : "file");
			printf("%u ", *(p + (8 * i)) + 1);
			printf("%u bytes ", (inode_list + *(p + (8 * i)))->file_size);

			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //�� ������ �̸� ���
				printf("%c", *(p + (8 * i) + j)); //������ �ι��ڸ� ǥ������ �ʱ⶧���� %s�� ��� �Ұ���
			}

			printf("\n");
		}
	}
}

//�� �߰��Ϸ�
void mycpto(unsigned char target_inode, char* host_dest) { //������ ���Ͻý��� ������ ���� ��ǻ�ͷ� ����
	char* str = NULL;
	file_read(target_inode, &str); //���� ���� �о��
	realfile_write(host_dest, str); //���� ���� ����
	free(str);
}

//�� �߰��Ϸ�
void mycpfrom(char* host_name, unsigned char dest_inode) { //���� ������ ���� ���Ͻý������� �о����
	char* str = NULL;
	realfile_read(host_name, &str); //���� �о��
	DEBUGMOD
		printf("mycpfrom : reading realfile finished\n");

	if (str == NULL) { //����
		printf("there is no such file in host pc %s\n", host_name);
		return;
	}

	file_rewrite(dest_inode, str); //������ ����� ���Ͽ� ��������
	DEBUGMOD
		printf("mycpfrom : file_size : %u\n", (inode_list + dest_inode)->file_size);
	DEBUGMOD
		printf("mycpfrom : writing file finished\n");

	free(str);
}

void mycp(unsigned char pwd, unsigned char target_inode, char* name) { //���� ����
	char* str = NULL;
	file_read(target_inode, &str); //���ϳ����� �о str����
	file_add(pwd, name); //�� ���� �߰�
	file_rewrite(file_search(pwd, name), str); //�� ���Ͽ� ���� ����
	free(str);
}

//�� �߰��Ϸ�
//name�̶�� �̸��� ���͸��� ã�� �ش� ���͸��� ������ ����
void mycd(unsigned char* pwd, char* name) { //pwd : ������ �ϴ� ���͸��� �������͸��� ���̳��, name : ������ �ϴ� ���͸��� ���̳��

	int n = dir_search(*pwd, name); //pwd���� name�� �ش��ϴ� ���͸� �˻�
	switch (n) {
	case -2: //�׷� ���� ����
		printf("no such file or directory\n");
		return;
	case -1: //���͸��� �ƴ�
		printf("%s is not a directory\n", name);
		return;
	default: //�ش� ���͸��� �̵�
		*pwd = n;
		return;
	}
}

//�� �߰��Ϸ�
void mycat(unsigned char inode) { //�ش� ��ȣ�� ���̳���� �����ͺ��� �о��, ���͸� ���̳�带 ������δ� ���������� �۵����� ����
	if (is_inode_empty(inode)) {
		printf("%d is empty inode\n", inode);
		return;
	}

	//������� ���� �����ͺ� ���
	if (!is_db_empty((inode_list + inode)->db1) && ((inode_list + inode)->db_mask & (unsigned char)0x80) != 0)
		db_print((inode_list + inode)->db1);
	if (!is_db_empty((inode_list + inode)->db2) && ((inode_list + inode)->db_mask & (unsigned char)0x40) != 0)
		db_print((inode_list + inode)->db2);
	if (!is_db_empty((inode_list + inode)->db3) && ((inode_list + inode)->db_mask & (unsigned char)0x20) != 0)
		db_print((inode_list + inode)->db3);
	if (!is_db_empty((inode_list + inode)->db4) && ((inode_list + inode)->db_mask & (unsigned char)0x10) != 0)
		db_print((inode_list + inode)->db4);
	if (!is_db_empty((inode_list + inode)->db5) && ((inode_list + inode)->db_mask & (unsigned char)0x8) != 0)
		db_print((inode_list + inode)->db5);
	if (!is_db_empty((inode_list + inode)->db6) && ((inode_list + inode)->db_mask & (unsigned char)0x4) != 0)
		db_print((inode_list + inode)->db6);
	if (!is_db_empty((inode_list + inode)->db7) && ((inode_list + inode)->db_mask & (unsigned char)0x2) != 0)
		db_print((inode_list + inode)->db7);
	if (!is_db_empty((inode_list + inode)->db8) && ((inode_list + inode)->db_mask & (unsigned char)0x1) != 0)
		db_print((inode_list + inode)->db8);

	if ((inode_list + inode)->is_idb == 1) { //�δ��̷�Ʈ �� �о���� ���
		idb_print((inode_list + inode)->single_idb);
	}

	printf("\n");
}

//�� �߰��Ϸ�
void mymkdir(unsigned char pwd, const char* name) {
	char tmp;

	if (strlen(name) > 7) { //�����̸� 7��������
		printf("filename is too long\n");
		return;
	}


	char* p = *(datablock + (inode_list + pwd)->db1); //p = ���� ���͸�(== ���͸��� �߰��� ���͸�) �� ���̳���� �����ͺ�

	int len = 0;

	for (int i = 0; 8 * i < 256; i++) { //�����ͺ��� 0, 8, 16...��° �� Ȯ�� �� �� ���� �˻�

		len = 0; //�̸��� ���� ��
		for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1��° ������ 7���ڰ� �ǰų� '\0'�� ���Ë����� len++, ������ �̸� ��������
			len++;
		}

		if (*(p + (8 * i)) != -1) {
			//if (strlen(name) == len && my_strncmp(p + (8 * i) + 1, name, strlen(name))) { //������ �̸��� ���� ����� �����ź�
			//	DEBUGMOD printf("")
			//	printf("%s is already exist\n", name);
			//	return;
			//}
		}
		else {
			tmp = *(p + (8 * i) + 8);
			*(p + (8 * i)) = dir_alloc(pwd); //�ش� ������� �� ���͸��� ���̳�� �Ҵ�
			sprintf(p + (8 * i) + 1, "%s", name); //�ش� ���͸��� �̸��� ����
			*(p + (8 * i) + 8) = tmp;
			return;
		}
	}

	printf("this directory is full, 1 directory can contain 32 files\n"); //32ĭ�� �̹� ����á�ٸ� �����ź�

	return;
}

//�� �߰��Ϸ�
void mytree(unsigned char pwd, int cnt) {
	for (int i = 2; 8 * i < 256; i++) { //�ش� ���͸��� ���� ���͸� Ž��
		if (*(*(datablock + (inode_list + pwd)->db1) + 8 * i) != -1 && (inode_list + *(*(datablock + (inode_list + pwd)->db1) + 8 * i))->is_dir == 1) {
			for (int k = 0; k < cnt; k++) { //cnt ������ŭ ����
				printf("  ");
			}
			printf("->");
			for (int j = 1; j < 8 && *(*(datablock + (inode_list + pwd)->db1) + 8 * i + j) != '\0'; j++) { //���� ���͸��� �̸� ���
				printf("%c", *(*(datablock + (inode_list + pwd)->db1) + 8 * i + j));
			}
			printf("\n");
			mytree(*(*(datablock + (inode_list + pwd)->db1) + 8 * i), cnt + 1); //�� ���͸��� ���� ���͸��� ���Ͽ� ����Լ�
		}
	}
}

void mymkfs(unsigned char* pwd) { //���Ͻý��� ����
	FILE* myfs = fopen("myfs", "r");
	_Bool keep = (myfs != NULL);
	//fclose(myfs);
	if (keep) { //������ �̹� �����Ѵٸ�
		DEBUGMOD printf("myfs closed\n");

		while (1) {
			printf("myfs already exist, rewrite myfs? (y / n) : ");
			switch (getchar()) {
			case 'y': //���Ͻý��� ���� ����
				for (int i = 0; i < 128; i++) { //��� ���̳�� �Ҵ�����, ����� �����ͺ��� �ڵ����� ������
					if (!is_inode_empty(i)) {
						inode_free(i);
					}
				}

				free(inode_list); //���̳�� ����Ʈ(������) �Ҵ�����
				free(datablock); //�����ͺ� ����Ʈ(����������) �Ҵ�����
				setup(); //���ۺ�, ��Ʈ ��� �缳��
				fs_save(); //����

				*pwd = 0;

				printf("new myfs recreated\n");

				getchar();
				return;
			case 'n': //�ź�
				printf("myfs didn't recreated\n");
				getchar();
				return;
			default: //����
				printf("wrong input, input (y / n)\n");
				getchar();
				break;
			}

		}
	}
	else { //�ּ��� ������ ���� ����
		for (int i = 0; i < 128; i++) {
			if (!is_inode_empty(i)) {
				inode_free(i);
			}
		}

		free(inode_list);
		free(datablock);
		setup();
		fs_save();

		*pwd = 0;

		printf("new myfs created\n");
		return;
	}
}

void mydatablock(unsigned char db) { //���õ� ��ȣ�� �����ͺ� �ϳ� ���, �δ��̷�Ʈ��, ���͸��� �����ͺ��� ����� ��µ��� ����
	if (db < 0 || db > 255) { //�������
		printf("%d is put of range, input 1 ~ 256\n", db + 1);
		return;
	}
	if (is_db_empty(db)) {
		printf("%d is empty datablock\n", db + 1);
		return;
	}
	else {
		printf("printing datablock %d\n\n", db + 1);
		db_print(db); //�����ͺ� ��� �Լ�
		printf("\n");
	}
}

/*--------------------------*/

int relative_route_inode_return(char* str, unsigned char pwd) { //str : �Է¹��� ����� ex) asdf/qwer
	int j = 1;
	int k = 0;
	int t = 0;
	int rewq = 0;
	int row = 0;
	int col = 0;
	int N = 0;
	if (*str == '/') {//�����η� �Է� �� �����޽��� ���
		printf("fault route\n");
		return -2;
	}
	while (1) {//�Է¹��� ���͸� ���� üũ
		if (*(str + t) == '/') {
			N++;
		}
		else if (*(str + t) == '\0') {
			break;
		}
		t++;
	}
	N++;
	N++;

	int dir_inode;
	char** dir_str;
	dir_str = (char**)calloc(N, sizeof(char*));//���͸� �̸����� ���ڿ��� ������ ����
	for (int i = 0; i < N; i++) {
		*(dir_str + i) = (char*)calloc(8, sizeof(char));
	}
	while (1) {// �Է¹��� ����� �и�
		if (*(str + k) == '\0') {
			break;
		}
		else if (*(str + k) == '/') {
			row++;
			col = 0;
		}
		else {
			*(*(dir_str + row) + col) = *(str + k);
			col++;
		}
		k++;
	}

	if (dir_search(pwd, *(dir_str + 0)) == -1)//����, ���͸� ����
		dir_inode = file_search(pwd, *(dir_str + 0));
	else
		dir_inode = dir_search(pwd, *(dir_str + 0));
	rewq = dir_inode;
	for (j = 1; j < N; j++) {//���̳�� Ž��, ����
		if (strcmp(*(dir_str + j), "\0") == 0 || dir_inode < 0) {
			rewq = dir_inode;
			break;
		}
		if ((inode_list + dir_inode)->is_dir == 0 && **(dir_str + j) != '\0') {
			rewq = -1;
			break;
		}
		if (dir_search(dir_inode, *(dir_str + j)) == -1 && **(dir_str + j + 1) == '\0') {
			dir_inode = file_search(dir_inode, *(dir_str + j));
		}
		else {
			dir_inode = dir_search(dir_inode, *(dir_str + j));
		}
	}
	for (int i = 0; i < N; i++) {
		free(*(dir_str + i));
		*(dir_str + i) = NULL;
	}
	free(dir_str);
	dir_str = NULL;
	return rewq;
}

int absolute_route_inode_return(char* str) { //str : �Է¹��� ������ ex) /asdf/qwer //��Ʈ �ϳ��� �Է��ϸ� 0����
	int j = 1;
	int k = 1;
	int t = 0;
	int rewq = 0;
	int row = 0;
	int col = 0;
	int N = 0;
	if (*str != '/') {//��� ��� �Է� �� ���� �޽��� ���
		printf("fault route\n");
		return -2;
	}
	if (strcmp(str, "/") == 0) {
		return 0;
	}
	while (1) {
		if (*(str + t) == '/') {
			N++;
		}
		else if (*(str + t) == '\0') {
			break;
		}
		t++;
	}
	N++;

	int dir_inode;
	char** dir_str;
	dir_str = (char**)calloc(N, sizeof(char*));//�Է¹��� ���͸� ������ ����
	for (int i = 0; i < N; i++) {
		*(dir_str + i) = (char*)calloc(8, sizeof(char));
	}
	while (1) {// �Է¹��� ������ �и�
		if (*(str + k) == '\0') {
			break;
		}
		else if (*(str + k) == '/') {
			row++;
			col = 0;
		}
		else {
			*(*(dir_str + row) + col) = *(str + k);
			col++;
		}
		k++;
	}
	if (dir_search(0, *(dir_str + 0)) == -1)
		dir_inode = file_search(0, *(dir_str + 0));
	else
		dir_inode = dir_search(0, *(dir_str + 0)); //���� ���͸����� Ž��
	for (j = 1; j < N; j++) {
		DEBUGMOD printf("j = %d, N = %d, dir_inode = %d\n",j, N, dir_inode);
		if (strcmp(*(dir_str + j), "\0") == 0 || dir_inode < 0) {
			rewq = dir_inode;
			break;
		}
		if ((inode_list + dir_inode)->is_dir == 0 && **(dir_str + j) != '\0') {
			rewq = -2;
			break;
		}
		if (dir_search(dir_inode, *(dir_str + j)) == -1 && **(dir_str + j + 1) == '\0') {
			dir_inode = file_search(dir_inode, *(dir_str + j));
		}
		else if (dir_search(dir_inode, *(dir_str + j)) == -1) {
			rewq = -2;
		}
		else {
			dir_inode = dir_search(dir_inode, *(dir_str + j));
		}
	}
	for (int i = 0; i < N; i++) {
		free(*(dir_str + i));
		*(dir_str + i) = NULL;
	}
	free(dir_str);
	dir_str = NULL;
	return rewq;
}

void myinode(int inode) { //pwd : Ȯ���ϰ���� ���͸��� ���̳��

	if (inode < 0 || inode > 127) { //���� Ȯ��
		printf("%d is out of range, input 1 ~ 128\n", inode);
	}
	
	if (is_inode_empty(inode)) { //����ó��
		printf("empty inode\n");
		return;
	}
	DEBUGMOD printf("db_mask : %u (0x%x)\n", (inode_list + inode)->db_mask, (inode_list + inode)->db_mask);

	//���̳�� ���� ���� ���
	struct tm tm;
	printf("%s", (inode_list + inode)->is_dir ? "Type : directory\n" : "Type : file\n");//���� ����
	printf("size : %d\n", (inode_list + inode)->file_size);//���� ũ��
	tm = (inode_list + inode)->time;
	printf("%4d/%2d/%2d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);//��¥
	printf("%2d:%2d:%2d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

	if (is_db_empty((inode_list + inode)->db1) || (((inode_list + inode)->db_mask & (unsigned char)0x80) == 0)) {// �����ͺ� ��� ����
		printf("db1 : not alloced\n");
	}
	else {
		printf("db1 : %u\n", (inode_list + inode)->db1 + 1);
	}
	if (is_db_empty((inode_list + inode)->db2) || (((inode_list + inode)->db_mask & (unsigned char)0x40) == 0)) {
		printf("db2 : not alloced\n");
	}
	else {
		printf("db2 : %u\n", (inode_list + inode)->db2 + 1);
	}
	if (is_db_empty((inode_list + inode)->db3) || ((inode_list + inode)->db_mask & (unsigned char)0x20) == 0) {
		printf("db3 : not alloced\n");
	}
	else {
		printf("db3 : %u\n", (inode_list + inode)->db3 + 1);
	}
	if (is_db_empty((inode_list + inode)->db4) || ((inode_list + inode)->db_mask & (unsigned char)0x10) == 0) {
		printf("db4 : not alloced\n");
	}
	else {
		printf("db4 : %u\n", (inode_list + inode)->db4 + 1);
	}
	if (is_db_empty((inode_list + inode)->db5) || ((inode_list + inode)->db_mask & (unsigned char)0x8) == 0) {
		printf("db5 : not alloced\n");
	}
	else {
		printf("db5 : %u\n", (inode_list + inode)->db5 + 1);
	}
	if (is_db_empty((inode_list + inode)->db6) || ((inode_list + inode)->db_mask & (unsigned char)0x4) == 0) {
		printf("db6 : not alloced\n");
	}
	else {
		printf("db6 : %u\n", (inode_list + inode)->db6 + 1);
	}
	if (is_db_empty((inode_list + inode)->db7) || ((inode_list + inode)->db_mask & (unsigned char)0x2) == 0) {
		printf("db7 : not alloced\n");
	}
	else {
		printf("db7 : %u\n", (inode_list + inode)->db7 + 1);
	}
	if (is_db_empty((inode_list + inode)->db8) || ((inode_list + inode)->db_mask & (unsigned char)0x1) == 0) {
		printf("db8 : not alloced\n");
	}
	else {
		printf("db8 : %u\n", (inode_list + inode)->db8 + 1);
	}
	if ((inode_list + inode)->is_idb == 1) {
		printf("single idb : %u\n", (inode_list + inode)->single_idb + 1);
	}
	else {
		printf("single idb : not alloced\n");
	}
	return;
}

char* now_dir(unsigned char pwd) {
	char* route;
	char* inode_save;
	int cnt = 1;
	int inode = pwd;
	int route_cnt = 0;
	while (inode != 0) {//���� ���͸� ���� Ȯ��
		inode = dir_search(inode, "..");
		cnt++;
	}
	inode = pwd;
	route = (char*)calloc(8 * cnt, sizeof(char));
	inode_save = (char*)calloc(cnt, sizeof(char)); //�������� ���͸� ������ ���̳�� ��ȣ ������ ����
	for (int i = 0; i < cnt; i++) {
		*(inode_save + i) = inode;
		inode = dir_search(inode, "..");
	}//inode�������� ����
	//*(inode + cnt - 1) == ��Ʈ
	if (cnt == 1) {//��Ʈ�� �ԷµǾ��� ��
		*(route + 0) = '/';
		free(inode_save);
		return route;
	}
	*(route + route_cnt) = '/';
	route_cnt++;
	for (; cnt > 1; cnt--) {//inode_saveȰ���Ͽ� ���͸����� /�� ���е� ���ڿ��� route�� ����
		for (int i = 0; i * 8 < 256; i++) {
			if (*(*(datablock + (inode_list + *(inode_save + cnt - 1))->db1) + i * 8) == *(inode_save + cnt - 2)) {
				for (int j = 1; j < 8; j++) {
					*(route + route_cnt) = *(*(datablock + (inode_list + *(inode_save + cnt - 1))->db1) + i * 8 + j);
					route_cnt++;
					if (*(*(datablock + (inode_list + *(inode_save + cnt - 1))->db1) + i * 8 + (j + 1)) == '\0') {
						*(route + route_cnt) = '/';
						route_cnt++;
						break;
					}
					else if (j == 7) {
						*(route + route_cnt) = '/';
						route_cnt++;
						break;
					}
				}
			}
		}
	}
	free(inode_save);
	*(route + route_cnt - 1) = '\0';
	return route;
}

/*--------------------------------------------------*/

void print_inode_db(void)
{
	int n = sizeof(int) * 16;
	int m = sizeof(int) * 64;
	int cnt_i = 0;
	int cnt_d = 0;
	unsigned long long int mask = (unsigned long long int)1 << (n - 1);

	/*
	for (int i = 1; i <= n; i++)
	{
		putchar((sad & mask == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 8 == 0)
			putchar(' ');
	}
	*/
	printf("Inode state :\n");
	printf("   Total : 128\n");
	//������� ���̳�� ī��Ʈ
	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.inode1 & mask) != 0)
			cnt_i++;
		mask >>= 1;
	}
	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.inode2 & mask) != 0)
			cnt_i++;
		mask >>= 1;
	}
	printf("   Used : %d\n", cnt_i);
	printf("   Available : %d\n", 128 - cnt_i);
	printf("   Inode Map : \n");
	printf("   ");

	//���̳�� ���� ���

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.inode1 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}

	printf("\n");
	printf("   ");
	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.inode2 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}
	printf("\n");

	mask = (unsigned long long int)1 << (n - 1);

	/*
	for (int i = 1; i <= m; i++)
	{
		putchar((((inode_list)->db1 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 8 == 0)
			putchar(' ');
	}
	printf("\n");
	*/

	//������ �� ��뷮 ī��Ʈ
	int byte_cnt = 0;
	for (int i = 0; i < 127; i++)
	{
		byte_cnt += (inode_list + i)->file_size;
	}

	printf("Data Block state : \n");
	printf("   Total : 256 blocks / 65536 byte\n");

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.datablock1 & mask) != 0)
			cnt_d++;
		mask >>= 1;
	}

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.datablock2 & mask) != 0)
			cnt_d++;
		mask >>= 1;
	}

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.datablock3 & mask) != 0)
			cnt_d++;
		mask >>= 1;
	}

	//������ �� ���� ���

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		if ((super.datablock4 & mask) != 0)
			cnt_d++;
		mask >>= 1;
	}
	printf("   Used : %d blocks / %d byte\n", cnt_d, byte_cnt);
	printf("   Available : %d blocks / %d byte\n", 256 - cnt_d, 65536 - byte_cnt);
	printf("   Datablock Map : \n");
	printf("   ");

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.datablock1 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}
	printf("\n");
	printf("   ");
	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.datablock2 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}
	printf("\n");
	printf("   ");
	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.datablock3 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}
	printf("\n");
	printf("   ");

	mask = (unsigned long long int)1 << (n - 1);

	for (int i = 1; i <= n; i++)
	{
		putchar(((super.datablock4 & mask) == 0) ? '0' : '1');
		mask >>= 1;
		if (i % 4 == 0)
			putchar(' ');
	}
	printf("\n");

	return;
}

//void myshowfile(int start, int end, unsigned char file_inode)
//{
//	char* str = NULL;
//	file_read(file_inode, &str);
//	if (start > end)
//		printf("Wrong input\n");
//	else if(start > (inode_list + file_inode)->file_size || end > (inode_list + file_inode)->file_size)
//		printf("Wrong input\n");
//	else
//	{
//		for (int i = start - 1; i <= end; i++) { //���� ���� ���
//			putchar(*(str + i));
//		}
//		putchar('\n');
//	}
//}

void myshowfile(int start, int end, unsigned char file_inode)
{
	char* str = NULL;
	file_read(file_inode, &str);
	if (start > end)
		printf("Start byte is bigger than end byte\n");
	else if (start > (inode_list + file_inode)->file_size || end > (inode_list + file_inode)->file_size)
		printf("start or end is bigger than file size\n");
	else
	{
		for (int i = start - 1; i < end; i++) { //start������ end������ ���� ���� ���
			putchar(*(str + i));
		}
		putchar('\n');
	}
}

//------------------------------------------------------

int my_strncmp(const char* a, const char* b, int n) { //������ ���ڼ���ŭ ���ڿ� ��
	for (int i = 0; i < n; i++) {
		if (*(a + i) != *(b + i)) {
			return 0;
		}
	}
	return 1;
}