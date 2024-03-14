#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"fs.h"

#define DEBUG printf("DEBUG\n")
_Bool debug_mode;
#define DEBUGMOD if(debug_mode == 1)

superblock super = {0, 0, 0, 0, 0, 0}; //슈퍼블럭(0으로 초기화)
inode* inode_list; //아이노드배열의 포인터
char** datablock; //데이터블럭배열의 포인터

void setup() { //초기 파일시스템 생성, 128개의 아이노드 생성, 256개의 데이터블럭의 포인터 생성, 루트디렉터리 생성
	inode_list = (inode*)calloc(128, sizeof(inode)); //아이노드 128개 할당
	datablock = (char**)calloc(256, sizeof(char*)); //데이터블럭 256개 할당

	//루트디렉터리 생성
	dir_alloc(0);
	(inode_list + 0)->is_dir = 1;

	debug_mode = 0;
}

void inode_timeset(unsigned char inode) { //아이노드의 시간 설정
	time_t now = time(NULL);
	(inode_list + inode)->time = *localtime(&now);

	DEBUGMOD printf("inode_timeset : time set finish\n");

	return;
}

int find_empty_inode() { //빈 inode 번호리턴
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

	printf("no empty inode\n"); //없음
	return -1;
}

int find_empty_db() { //빈 db 번호리턴
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

	printf("no empty db\n"); //없음
	return -1;
}

int db_alloc() { //데이터블럭 할당 // ex : (inode_list + n)->db1 = db_alloc();
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크
	int n = find_empty_db(); //빈공간 찾기
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


int idb_alloc(unsigned int size) { //아이노드를 위해 인다이렉트블럭 할당, 인다이렉트 블럭도 데이터블럭중 하나의 자리를 차지함

	int add = db_alloc(); //데이터블럭 할당

	DEBUGMOD
	printf("allocating idb\n");

	int db_num = (size / 256) + (size % 256 != 0); //필요한 db개수

	(*(*(datablock + add) + 0)) = db_num - 128; //인다이렉트 블럭의 첫번째 칸은 인다이렉트 블럭 내부의 데이터블럭 갯수, char는 -까지 포함하므로 128빼서 저장했다가 128 더해서 참조

	for (int i = 0; i < db_num; i++) { //데이터블럭은 unsigned char 가 아니라 char 이므로 0-255를 표현하기 위해 -128을 한 이후 저장, 다시 읽어올때는 +128
		(*(*(datablock + add) + i + 1)) = db_alloc() - 128;
	}

	return add;
}

//디렉터리가 아닌 아이노드 할당
//inode 할당 // ex : (inode_list + n)->single_idb = inode_alloc(~, ~);
int inode_alloc(unsigned int size) { // size : 파일의 크기 (바이트단위)
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크
	int n = find_empty_inode(); //빈공간 찾기

	inode_timeset(n);

	if (n == -1) {
		printf("not alloced\n");
		return -1;
	}

	//아이노드 리스트 수정, 디렉터리여부 저장
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
	//인다렉트블럭 초
	(inode_list + n)->is_idb = 0;

	//파일의 크기 저장
	(inode_list + n)->file_size = size;

	int db_num = (size / 256) + (size % 256 != 0); //필요한 db개수

	if (db_num > 8) { //필요한 데이터블럭이 8개 초과 (indirect 필요)

		//db8개 할당
		(inode_list + n)->db1 = db_alloc();
		(inode_list + n)->db2 = db_alloc();
		(inode_list + n)->db3 = db_alloc();
		(inode_list + n)->db4 = db_alloc();
		(inode_list + n)->db5 = db_alloc();
		(inode_list + n)->db6 = db_alloc();
		(inode_list + n)->db7 = db_alloc();
		(inode_list + n)->db8 = db_alloc();

		(inode_list + n)->db_mask = 0xff;


		//8개를 할당하고 남은 데이터를 인다이렉트블럭에 다시할당(또 모자라면 거기서 또 반복)
		(inode_list + n)->single_idb = idb_alloc(size - 8 * 256);
		(inode_list + n)->is_idb = 1;
	}
	else if (db_num <= 8) { //필요한 데이터블럭이 8개 이하 (indirect 불필요)
		switch (db_num) { //데이터블럭 1-8개 할당
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
//!!! : db_free((inode_liat + n)->db1);,사용후 db_mask도 수정할 것
void db_free(unsigned char db) { //해당 번호의 db를 할당해제하고 슈퍼블럭을 수정함
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크

	free(*(datablock + db)); //데이터블럭 할당 해제

	//슈퍼블럭 수정
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

void idb_free(unsigned char single_idb) { //위 함수의 인다이렉트블럭 버전, 위 함수를 호출하면서 작동함
	int db_num = *(*(datablock + single_idb) + 0);

	for (int i = 0; i < db_num + 128; i++) {
		DEBUGMOD printf("idb_free : db_%d free\n", *(*(datablock + single_idb) + i) + 128);
		db_free(*(*(datablock + single_idb) + i + 1) + 128);
	}

	db_free(single_idb);
}

void inode_free(unsigned char inode) { //해당 번호의 아이노드를 할당해제하기위해 슈퍼블럭을 수정함, 하위 데이터블럭까지 자동으로 할당해제(원래 아니였는데 바뀜)
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크

	//슈퍼블럭 수정
	if (inode >= 0 && inode < 64) {
		super.inode1 &= ~(mask >> inode);
	}
	else if (inode >= 64 && inode < 128) {
		super.inode2 &= ~(mask >> (inode - 64));
	}

	if (!is_db_empty((inode_list + inode)->db1) && ((inode_list + inode)->db_mask & (unsigned char)0x80) != 0) { //해당 db가 비어있지 않다면
		db_free((inode_list + inode)->db1); //해당 db할당해제
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

	if ((inode_list + inode)->is_idb == 1) { //인다이렉트 블럭이 비어있지 않다면
		idb_free((inode_list + inode)->single_idb); //해당 아이노드 할당해제
		(inode_list + inode)->is_idb = 0; //해당 인다이렉트 블럭이 할당없음을 표시

		DEBUGMOD printf("inode_free : single_idb unalloced\n");
	}

}

//디렉터리인 아이노드 할당
//데이터블럭의 0-6이름 + 7아이노드, 8-14이름, 15아이노드 ...
int dir_alloc(unsigned char pwd) { //pwd : 부모디렉터리
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크
	int n = find_empty_inode(); //빈공간 찾기
	if (n == -1) {
		printf("not alloced\n");
		return -1;
	}

	inode_timeset(n);
	(inode_list + n)->file_size = 256;

	//아이노드 리스트 수정, 디렉터리여부 저장
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

	(inode_list + n)->db1 = db_alloc(); //디렉터리에는 하나의 데이터블럭만 할당
	(inode_list + n)->db_mask = 0x80;

	(inode_list + n)->is_idb = 0;;

	//디렉터리 데이터블럭의 작동방식 :
	//0, 8, 16... 번째 값 : 해당 파일의 아이노드번호 저장, -1은 할당된 아이노드가 없다는 뜻
	//1-7, 9-15, 17-23...번째 값 : 0, 8, 16...번째 값의 아이노드에 해당하는 파일의 이름저장, 총 7글자
	//마지막의 널문자를 용량절약으로 제거했으므로 데이터블럭에 대하여 %s는 가급적 사용X
	for (int i = 0; i < 256; i++) {
		*(*(datablock + (inode_list + n)->db1) + i) = '\0'; //해당 데이터블럭 전부를 '\0'로 초기화
		if (i % 8 == 0) {
			*(*(datablock + (inode_list + n)->db1) + i) = -1; //0, 8, 16...번째 값 -1로 초기화
		}
	}

	*(*(datablock + (inode_list + n)->db1) + 0) = n; //상대경로에서 '.'는 현재 디렉터리를 의미, . == 그 디렉터리의 아이노드번호
	*(*(datablock + (inode_list + n)->db1) + 1) = '.';

	*(*(datablock + (inode_list + n)->db1) + 8) = pwd; //상대경로에서 ".."는 부모 디렉터리를 의미, .. == 부모 디렉터리 아이노드번호
	sprintf((*(datablock + (inode_list + n)->db1) + 9), "..");

	/*
	* (0 == '\0' == NULL)
	* 해당 데이터블럭 상태 : 
	* [n]['.'][0][0][0][0][0][0]
	* [pwd]['.']['.'][0][0][0][0][0]
	* [-1][0][0][0][0][0][0][0]
	* [-1][0][0][0][0][0][0][0]
	...
	*/

	return n;
}

int dir_search(unsigned char pwd, char* name) { //pwd : 확인하고싶은 디렉터리가 존재하는 디렉터리의 아이노드, name : 찾고자하는 파일의 이름
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd값을 통해 검색대상이 되는 디렉터리주소 저장

	int len = 0;
	for (int i = 0; 8 * i < 256; i++) {

		if (*(p + (8 * i)) != -1) { //이름은 비교할 파일 확인, 8n번째 값이 -1이라면 파일이 없는것이므로 무시

			len = 0; //이름의 길이 비교
			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1번째 값부터 7글자가 되거나 '\0'이 나올떄까지 len++, 파일의 이름 길이저장
				len++;
			}

			//strncmp(name, (p + (8 * i) + 1), len) == 0
			if (strlen(name) == len && my_strncmp(name, (p + (8 * i) + 1), len)) { //그 파일의 이름의 길이와 name의 길이가 같은지 확인 
				if (!(inode_list + *(p + (8 * i)))->is_dir) { //그 파일이 디렉터리파일인지 확인
					return -1; //해당 파일은 이름이 같지만 디렉터리가 아님
				}
				return *(p + (8 * i)); //이름이 name인 디렉터리파일 발견, 그 파일의 아이노드값 리턴
			}
		}
	}

	return -2; //해당 이름의 파일이 존재하지 않음
}

void dir_arr(unsigned char pwd) { //디렉터리 내부를 사전순으로 재정렬
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd값을 통해 검색대상이 되는 디렉터리주소 저장
	unsigned char* arr;
	unsigned char tmp;
	char** name_arr;
	char* name_tmp;
	int cnt = 0, cnt_2 = 0;

	for (int i = 2; 8 * i < 256; i++) { //파일의 갯수 측정
		if (*(p + (8 * i)) != -1) {
			cnt++;
		}
	}
	DEBUGMOD printf("dir_arr : cnt = %d\n", cnt);

	arr = malloc(sizeof(unsigned char) * cnt);
	name_arr = malloc(sizeof(char*) * cnt);

	//파일목록 읽고 지움
	cnt = 0;
	for (int i = 2; 8 * i < 256; i++) {
		if (*(p + (8 * i)) != -1) { //파일 발견
			*(arr + cnt) = *(p + (8 * i)); //아이노드 복사
			DEBUGMOD printf("dir_arr : inode copied, %d\n", *(p + (8 * i)));
			*(p + (8 * i)) = -1;
			*(name_arr + cnt) = calloc(8, sizeof(char));

			DEBUGMOD printf("dir_arr : name copy start : ");
			for (int j = 1;j < 8 && *(p + (8 * i) + j) != '\0'; j++) { //파일이름 복사
				*(*(name_arr + cnt) + j - 1) = *(p + (8 * i) + j);
				DEBUGMOD printf("%c", *(p + (8 * i) + j));
				*(p + (8 * i) + j) = '\0';
			}
			DEBUGMOD printf("\ndir_arr : name copied\n");

			cnt++;
		}
	}

	//읽어온 파일목록 사전순 정렬
	for (int j = 0; j < cnt - 1; j++) {
		for (int i = 0; i < cnt - 1; i++) {
			if (strcmp(*(name_arr + i), *(name_arr + i + 1)) > 0) { //사전순 비교
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

	//정렬된 내용 다시 디렉터리에 입력
	cnt_2 = 0;
	for (int i = 2; 8 * i < 256 && cnt_2 < cnt; i++) {
		if (*(p + (8 * i)) == -1) { //빈공간 발견
			*(p + (8 * i)) = *(arr + cnt_2); //아이노드 붙여넣기
			DEBUGMOD printf("dir_arr : inode pasted, %d\n", *(p + (8 * i)));

			DEBUGMOD printf("dir_arr : name paste start : ");
			for (int j = 1;j < 8 && *(*(name_arr + cnt_2) + j - 1) != '\0'; j++) { //이름 붙여넣기
				*(p + (8 * i) + j) = *(*(name_arr + cnt_2) + j - 1);
				DEBUGMOD printf("%c", *(p + (8 * i) + j));
			}
			DEBUGMOD printf("\ndir_arr : name pasted\n");

			cnt_2++;
		}
	}

	for (int i = 0; i < cnt - 1; i++) { //할당해제
		free(*(name_arr + i));
	}
	free(arr);
	free(name_arr);
}

int is_inode_empty(int n) { //해당 번호의 아이노드가 빈 공간인지 슈퍼블럭을 통해 확인
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크
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

int is_db_empty(int n) { //해당 번호의 db가 빈 공간인지 슈퍼블럭을 통해 확인
	unsigned long long int mask = (unsigned long long int)1 << 63; //마스크
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

//하나의 데이터블럭에 대하여 256자를 모두 출력하거나 '\0'이 나올때까지 출력
void db_print(unsigned char db) {
	char c;
	for (int i = 0; i < 256 && *(*(datablock + db) + i) != '\0'; i++)
		putchar(*(*(datablock + db) + i));
}

void idb_print(unsigned char single_idb) { //위 함수의 idb버전

	int db_num = *(*(datablock + single_idb) + 0) + 128;

	for (int i = 0; i < db_num; i++) {
		DEBUGMOD printf("idb_print : printing db_%d\n", *(*(datablock + single_idb) + i + 1) + 128);
		db_print(*(*(datablock + single_idb) + i + 1) + 128);
	}

	printf("\n");
}

//해당 매크로는 file_write에만 사용됨, 함수아래에 #undef로 해제되어있음
#define db_set(n) \
for (int i = 0; i < ((strlen(str + (n - 1) * 256 + 1) < 256) ? strlen(str + (n - 1) * 256) + 1 : 256); i++) {\
	*(*(datablock + (inode_list + inode)->db##n) + i) = *(str + (n - 1) * 256 + i);}
//문자열을 파일의 db에 저장, db가 이미 충분히 할당되어있다고 가정하고 실행되므로 db_alloc()으로 적절한 갯수만큼 할당해주는 과정 필요
//file_rewrite()에서 이미 다 되어있음
void file_write(unsigned char inode, char* str) {
	DEBUGMOD
	printf("file_write : string length : %lu\n", strlen(str));

	int db_num = (strlen(str)) / 256 + ((strlen(str)) % 256 != 0); //문자열 저장을 위해 필요한 db의 갯수 계산

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

		//남는 문자열 재귀함수로 전달
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
void idb_write(unsigned char single_idb, char* str) { //인다이렉트 블럭을 읽어서 하위 데이터블럭에 문자열 저장, 주로 file_write에만 사용됨
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

//해당 디렉터리에 내용없는 파일 생성, dir_add()와 과정 유사
void file_add(unsigned char pwd, char* name) { //pwd : 파일이 생성될 디렉터리의 아이노드, name : 생성할 파일의 이름
	if (strlen(name) > 7) { //파일이름 7글자제한
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

	char* p = *(datablock + (inode_list + pwd)->db1); //p = 현재 디렉터리(== 파일이 추가될 디렉터리) 의 아이노드의 데이터블럭

	/*if (file_search(pwd, name) >= 0) {
		printf("%s already exist\n", name);
		return;
	}*/

	for (int i = 0; 8 * i < 256; i++) { //데이터블럭의 0, 8, 16...번째 값 확인 후 빈 공간 검색
		if (*(p + (8 * i)) == -1) { //-1은 빈공간임을 의미
			if((8 * i) + 8 < 256)
				tmp = *(p + (8 * i) + 8);

			*(p + (8 * i)) = inode_alloc(0); //해당 빈공간에 새 파일의 아이노드 할당
			sprintf(p + (8 * i) + 1, "%s", name);

			if ((8 * i) + 8 < 256)
				*(p + (8 * i) + 8) = tmp; 
			return;
		}
	}

	printf("this directory is full, 1 directory can contain 32 files\n"); //32칸이 이미 가득찼다면 생성거부

	return;
}

//해당 디렉터리에서 원하는 이름의 파일 찾음, 있다면 그 파일의 아이노드리턴, 없으면 -2, -1리턴
int file_search(unsigned char pwd, char* name) {

	char* p = *(datablock + (inode_list + pwd)->db1); //pwd값을 통해 검색대상이 되는 디렉터리주소 저장

	int len = 0;
	for (int i = 0; 8 * i < 256; i++) {

		if (*(p + (8 * i)) != -1) { //이름은 비교할 파일 확인, 8n번째 값이 -1이라면 파일이 없는것이므로 무시

			len = 0; //이름의 길이 비교
			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1번째 값부터 7글자가 되거나 '\0'이 나올떄까지 len++, 파일의 이름 길이저장
				len++;
			}
			DEBUGMOD printf("file_search : len = %d\n", len);

			//strncmp(name, (p + (8 * i) + 1), len) == 0
			DEBUGMOD printf("file_search : %d\n", my_strncmp(name, (p + (8 * i) + 1), len));
			if (strlen(name) == len && my_strncmp(name, (p + (8 * i) + 1), len)) { //그 파일의 이름의 길이와 name의 길이가 같은지 확인 
				if ((inode_list + *(p + (8 * i)))->is_dir) { //그 파일이 일반파일인지 확인
					return -1; //해당 파일은 이름이 같지만 일반파일이 아님
				}
				return *(p + (8 * i)); //이름이 name인 일반파일 발견, 그 파일의 아이노드값 리턴
			}
		}
	}

	return -2; //해당 이름의 파일이 존재하지 않음
}

//파일의 내용을 전부 지워버리고 db를 할당해제함, 파일의 내용만 지워지고 파일자체는 유지됨
void file_empty(unsigned char inode) {

	if (!is_db_empty((inode_list + inode)->db1) && ((inode_list + inode)->db_mask & (unsigned char)0x80) != 0){ //해당 db가 비어있지 않다면
		db_free((inode_list + inode)->db1); //해당 db할당해제
		(inode_list + inode)->db_mask -= 0x80; //아이노드의 db_mask 수정
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

	if ((inode_list + inode)->is_idb == 1) { //인다이렉트 블럭이 비어있지 않다면
		idb_free((inode_list + inode)->single_idb); //해당 인다이렉트 블럭에 대해위 과정 동일시행
		(inode_list + inode)->is_idb = 0; //해당 인다이렉트 블럭의 포인터 NULL로 설정
	}

	(inode_list + inode)->file_size = 0;

}

//해당 아이노드의 파일의 내용을 지우고, str을 저장하기위해 필요한 db의 갯수를 계산 후 할당하고 내용을 저장함
void file_rewrite(unsigned char inode, char* str) {

	file_empty(inode); //내용 재작성 이전에 데이터블럭 지우기

	int db_num = ((strlen(str)) / 256) + ((strlen(str)) % 256 != 0); //필요한 db개수

	if (db_num > 8) { //필요한 데이터블럭이 8개 초과 (indirect 필요)

		//db8개 할당
		(inode_list + inode)->db1 = db_alloc();
		(inode_list + inode)->db2 = db_alloc();
		(inode_list + inode)->db3 = db_alloc();
		(inode_list + inode)->db4 = db_alloc();
		(inode_list + inode)->db5 = db_alloc();
		(inode_list + inode)->db6 = db_alloc();
		(inode_list + inode)->db7 = db_alloc();
		(inode_list + inode)->db8 = db_alloc();

		(inode_list + inode)->db_mask = 0xff;

		//8개를 할당하고 남은 데이터를 인다이렉트블럭에 다시할당(또 모자라면 거기서 또 반복)
		(inode_list + inode)->single_idb = idb_alloc((strlen(str)) - 8 * 256);
		(inode_list + inode)->is_idb = 1;
	}
	else if (db_num <= 8) { //필요한 데이터블럭이 8개 이하 (indirect 불필요)
		switch (db_num) { //데이터블럭 1-8개 할당
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

	file_write(inode, str); //새로운 내용 작성
	(inode_list + inode)->file_size = strlen(str);
}

void file_read(unsigned char inode, char** buffer) { //파일의 내용을 읽어서 버퍼에 저장, 버퍼를 할당해제후 재할당
	int db_num = ((inode_list + inode)->file_size / 256) + ((inode_list + inode)->file_size % 256 != 0); //데이터블럭의 갯수 계산

	free(*buffer); //버퍼의 재할당 이전 버퍼를 할당해제함
	*buffer = calloc((inode_list + inode)->file_size, sizeof(char)); //파일의 크기만큼 버퍼 재할당

	if (db_num <= 8) { //데이터블럭이 8개 이하
		switch (db_num) {
		case 8:
			strcat(*buffer, *(datablock + (inode_list + inode)->db8)); //데이터블럭을 읽어서버퍼에 저장함
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
	else { //데이터블럭이 8개 초과
		strcat(*buffer, *(datablock + (inode_list + inode)->db8));
		strcat(*buffer, *(datablock + (inode_list + inode)->db7));
		strcat(*buffer, *(datablock + (inode_list + inode)->db6));
		strcat(*buffer, *(datablock + (inode_list + inode)->db5));
		strcat(*buffer, *(datablock + (inode_list + inode)->db4));
		strcat(*buffer, *(datablock + (inode_list + inode)->db3));
		strcat(*buffer, *(datablock + (inode_list + inode)->db2));
		strcat(*buffer, *(datablock + (inode_list + inode)->db1));

		//남는 문자열 인다이렉트 블럭에 전달
		idb_read((inode_list + inode)->single_idb, buffer);
		DEBUGMOD printf("file_read : idb_read finished\n");
	}
}

void idb_read(unsigned char single_idb, char** buffer) { //인다이렉트 블럭을 읽어옴, 거의 file_read에 종속되어 사용될 것으로 예상
	int db_num = *(*(datablock + single_idb) + 0) + 128;

	for (int i = 0; i < db_num; i++) {
		strcat(*buffer, *(datablock + *(*(datablock + single_idb) + i + 1) + 128));
		DEBUGMOD printf("idb_read : (%d/%d) strcat db_%d", i, db_num, *(*(datablock + single_idb) + i + 1) + 128);
	}
}

//현재 파일시스템의 상태를 myfs에 저장
void fs_save() {
	FILE* myfs = fopen("myfs", "wb");
	if (myfs == NULL) { //파일을 불러오지 못했다면
		printf("failed to open myfs, fs not saved\n");
		return;
	}

	fwrite(&super, sizeof(super), 1, myfs); //슈퍼블럭 저장
	DEBUGMOD
	printf("superblock saved\n");

	fwrite(inode_list, sizeof(inode), 128, myfs); //아이노드 저장

	DEBUGMOD
	printf("inode_list saved\n");

	//인다이렉트 블럭의 구동방식이 바뀌면서 버려짐
	//인다이렉트블럭 저장
	//idb* current_idb;
	//for (int i = 0; i < 128; i++) {
	//	if (!is_inode_empty(i) && (inode_list + i)->single_idb != NULL) { //i번째 아이노드가 사용중이며 idb가 사용중이라면
	//		current_idb = (inode_list + i)->single_idb;
	//		while (current_idb != NULL) { //idb를 계속 따라가며 저장
	//			fwrite(current_idb, sizeof(idb), 1, myfs);
	//			current_idb = current_idb->single_idb;

	//			DEBUGMOD
	//			printf("single indirect block of inode%d saved\n", i);
	//		}
	//	}
	//}

	for (int i = 0; i < 256; i++) {
		if (!is_db_empty(i)) { //i번째 db가 사용중이라면
			fwrite(*(datablock + i), sizeof(char), 256, myfs); //해당 db저장

			DEBUGMOD
			printf("datablock%d saved\n", i);
		}
	}

	fclose(myfs);
	DEBUGMOD printf("fs_save : myfs closed\n");
}

//myfs로부터 파일시스템의 내용 읽어옴
void fs_load() {
	FILE* myfs = fopen("myfs", "rb");
	if (myfs == NULL) { //파일을 불러오지 못했다면
		printf("failed to open myfs, fs not loaded\n");
		return;
	}

	fread(&super, sizeof(super), 1, myfs); //슈퍼블럭 불러옴

	DEBUGMOD
	printf("superblock loaded\n");

	fread(inode_list, sizeof(inode), 128, myfs); //아이노드 리스트 불러옴

	DEBUGMOD
	printf("inode_list loaded\n");

	//인다이렉트 블럭의 구동방식이 바뀌면서 버려짐
	//idb불러오기
	//idb** current_idb;
	//for (int i = 0; i < 128; i++) {
	//	if (!is_inode_empty(i) && (inode_list + i)->single_idb != NULL) { //i번째 아이노드가 사용중이며 idb가 사용중이라면
	//		(inode_list + i)->single_idb = malloc(sizeof(idb)); //해당 idb에 메모리할당
	//		current_idb = &((inode_list + i)->single_idb);
	//		fread(*current_idb, sizeof(idb), 1, myfs); //해당 idb의 내용 읽어오기
	//		current_idb = &((*current_idb)->single_idb); //다음 idb로 연결

	//		DEBUGMOD
	//		printf("single indirect block of inode%d loaded\n", i);

	//		while (*current_idb != NULL) { //이후 하위의 idb에 대하여 NULL이 나올때까지 반복
	//			*current_idb = malloc(sizeof(idb));
	//			fread(*current_idb, sizeof(idb), 1, myfs);
	//			current_idb = &((*current_idb)->single_idb);

	//			DEBUGMOD
	//			printf("single indirect block of inode%d loaded\n", i);
	//		}
	//		
	//	}
	//}

	//데이터블럭 읽어오기
	for (int i = 0; i < 256; i++) {
		if (!is_db_empty(i)) { //i번째 db가 사용중이라면
			*(datablock + i) = (char*)calloc(256, 1); //메모리할당
			fread(*(datablock + i), sizeof(char), 256, myfs); //데이터 읽어오기

			DEBUGMOD
			printf("datablock%d loaded\n", i);
		}
	}

	fclose(myfs);
}

//실제 파일을 읽어서 버퍼에 내용저장
void realfile_read(char* name, char** buffer) { //버퍼가 할당해제된 후 다시 할당됨, file_read랑 같음
	FILE* file = fopen(name, "r"); //파일 열기
	if (file == NULL) { //파일 열기 실패
		printf("failed to open %s, function didn't worked\n", name);
		*buffer = NULL;
		return;
	}

	int len = 0;
	char c; 
	while ((c = getc(file)) != EOF) { //파일 길이 저장
		len++;
	}

	DEBUGMOD
	printf("file length : %d\n", len);

	free(*buffer);
	*buffer = calloc(len, sizeof(char)); //파일의 길이만큼 메모리할당
	rewind(file); //파일의 커서 처음으로


	for (int i = 0; i < len && (c = getc(file)) != EOF; i++) { //버퍼에 파일 내용 저장

		*(*buffer + i) = c;
	}

	fclose(file); //파일 닫음
	return;
}

void realfile_write(char* name, char* str) {
	FILE* file = fopen(name, "w"); //파일 열기
	if (file == NULL) { //파일 열기 실패
		printf("failed to open %s, function didn't worked\n", name);
		return;
	}

	for (int i = 0; *(str + i) != '\0'; i++) { //파일에 내용 입력하기
		putc(*(str + i), file);
	}

	fclose(file); //파일 닫음
}





void myrmdir(unsigned char pwd, unsigned char target_inode) { //디렉터리 제거

	if (target_inode == pwd) { //현재 디렉터리 제거 불가능
		printf("can't remove current directory\n");
		return;
	}

	for (int i = 2; 8 * i < 256; i++) { //타겟 디렉터리의 하위에".", ".."을 제외한 파일이 존재하는지 확인
		if (*(*(datablock + (inode_list + target_inode)->db1) + 8 * i) != -1) {
			printf("that directory is not an empty directory\n");
			return;
		}
	}

	DEBUGMOD
		printf("myrmdir : %d is empty\n", target_inode);

	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //타겟 디렉터리의 상위 디렉터리에서 타겟 디렉터리의 아이노드 탐색
			DEBUGMOD
				printf("myrmdir : found target\n");

			*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1; //상위 디렉터리에서 해당 아이노드공간 -1로 대체
			for (int j = 1; j < 8; j++) { //이름 '\0'로 대체
				*(*(datablock + (inode_list + pwd)->db1) + i * 8 + j) = '\0';
			}
			DEBUGMOD
				printf("myrmdir : target removed\n");

			inode_free(target_inode); //해당 아이노드 할당 해제
		}
	}
}

void myrm(unsigned char pwd, unsigned char target_inode) { //파일 제거

	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //대상 아이노드의 파일 탐색
			DEBUGMOD
				printf("myrm : found target : %d\n", *(*(datablock + (inode_list + pwd)->db1) + i * 8));

			*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1;
			for (int j = 1; j < 8; j++) { //이름 전체 '\0'로 대체
				*(*(datablock + (inode_list + pwd)->db1) + i * 8 + j) = '\0';
			}
			DEBUGMOD
				printf("myrm : target removed from direcoty\n");

			inode_free(target_inode); //파일의 아이노드 할당 해제, 하위 데이터블럭도 할당해제됨 
			DEBUGMOD
				printf("myrm : target removed from filesystem\n");
		}
	}
}

void mymv_dir(unsigned char pwd, unsigned char target_inode, unsigned char dir_inode) { //대상 파일의 위치를 다른 디렉터리로 옮김
	char* name = (char*)malloc(7);
	for (int i = 0; i * 8 < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //대상 아이노드의 파일 탐색
			DEBUGMOD
				printf("mymv_dir : found target from directory, ready to copy\n");

			for (int j = 1; j < 8; j++) { //파일의 이름 복사
				*(name + j - 1) = *(*(datablock + (inode_list + pwd)->db1) + i * 8 + j);

				DEBUGMOD
					printf("%c", *(*(datablock + (inode_list + pwd)->db1) + i * 8 + j));
			}
			DEBUGMOD
				printf("mymv_dir : target name copied %s\n", name);
		}
	}

	for (int i = 0; 8 * i < 256; i++) {
		if (*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i) == -1) {//옮겨질 디렉터리에서 빈공간 탐색
			DEBUGMOD
				printf("mymv_dir : empty place found\n");

			*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i) = target_inode; //대상의 아이노드 붙여넣기
			DEBUGMOD
				printf("mymv_dir : pasted inode %d\n", target_inode);

			for (int j = 1; j < 8 && *(name + j - 1) != '\0'; j++) { //대상 디렉터리에 파일의 이름 붙여넣기
				*(*(datablock + (inode_list + dir_inode)->db1) + 8 * i + j) = *(name + j - 1);
			}
			DEBUGMOD
				printf("mymv_dir : name of %s pasted to direcory\n", name);

			for (int i = 0; i * 8 < 256; i++) {
				if (*(*(datablock + (inode_list + pwd)->db1) + i * 8) == target_inode) { //원래의 디렉터리에서 기존 파일 정보 제거

					DEBUGMOD
						printf("mymv_dir : found target from directory, ready to remove\n");

					*(*(datablock + (inode_list + pwd)->db1) + i * 8) = -1; //디렉터리의 해당 아이노드값 -1로 대체
					for (int j = 1; j < 8; j++) { //이름의 7바이트 전체 '\0'로 대체
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

	printf("there is no empty place in directory\n"); //대상 디렉터리에 빈공간 없음
	free(name);
	return;
}

void mymv_file(unsigned char pwd, unsigned char target_inode, char* str) { //파일의 이름을 변경
	if (file_search(pwd, str) != -2) { //파일이 이미 존재함
		printf("%s is already exist\n", str);
		return;
	}
	if (strlen(str) > 7) { //파일이름 7글자제한
		printf("filename is too long\n");
		return;
	}
	for (int i = 0; *(str + i) != '\0'; i++) { //이름에 불가능한 문자 포함
		switch (*(str + i)) {
		case '/':
			printf("filename contains impossible text (ex : '/')\n");
			return;
		}
	}
	

	DEBUGMOD
		printf("mymv_file : start change name to %s\n", str);

	for (int i = 0; 8 * i < 256; i++) {
		if (*(*(datablock + (inode_list + pwd)->db1) + 8 * i) == target_inode) { //디렉터리에서 이름을 바꾸고자 하는 파일의 아이노드 탐색
			DEBUGMOD
				printf("mymv : found target\n");

			for (int j = 0; (j < ((strlen(str) + 1 < 7) ? strlen(str) + 1 : 7)); j++) { //이름 변경
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

//쉘 추가완료
//해당 디렉터리의 파일들 출력
void myls(unsigned char pwd) { //pwd : 확인하고싶은 디렉터리의 아이노드
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd값을 통해 확인하고싶은 디렉터리주소 저장
	struct tm tm;

	dir_arr(pwd); //출력이전 내부 파일 재정렬

	for (int i = 0; (8 * i) < 256; i++) { //0, 8, 16 ...번째 값 확인 후 비어있지 않은 공간 검색
		if (*(p + (8 * i)) != -1) { //-1은 빈공간임을 의미

			DEBUGMOD
				printf("myls : printing data of inode %d\n", *(p + (8 * i)));

			tm = (inode_list + *(p + (8 * i)))->time;
			printf(" %4d/%2d/%2d ",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //그 파일의 아이노드 표시(나중에 지워질 수 있음)
			printf("%2d:%2d:%2d ", tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("%9s ", (inode_list + *(p + (8 * i)))->is_dir ? "directory" : "file");
			printf("%u ", *(p + (8 * i)) + 1);
			printf("%u bytes ", (inode_list + *(p + (8 * i)))->file_size);

			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //그 파일의 이름 출력
				printf("%c", *(p + (8 * i) + j)); //마지막 널문자를 표시하지 않기때문에 %s는 사용 불가능
			}
			
			printf("\n");
		}
	}
}

void myls_target(unsigned char pwd, unsigned char inode) { //pwd : 확인하고싶은 파일의 아이노드 (myls와 비슷, 단 하나의 일반파일만을 대상으로 실행되기위해 존재)
	char* p = *(datablock + (inode_list + pwd)->db1); //pwd값을 통해 확인하고싶은 디렉터리주소 저장
	struct tm tm;

	for (int i = 0; (8 * i) < 256; i++) { //0, 8, 16 ...번째 값 확인 후 비어있지 않은 공간 검색
		if (*(p + (8 * i)) == inode) { //-1은 빈공간임을 의미

			DEBUGMOD
				printf("myls_target : printing data of inode %d\n", *(p + (8 * i)));

			tm = (inode_list + *(p + (8 * i)))->time;
			printf(" %4d/%2d/%2d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); //그 파일의 아이노드 표시(나중에 지워질 수 있음)
			printf("%2d:%2d:%2d ", tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("%9s ", (inode_list + *(p + (8 * i)))->is_dir ? "directory" : "file");
			printf("%u ", *(p + (8 * i)) + 1);
			printf("%u bytes ", (inode_list + *(p + (8 * i)))->file_size);

			for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //그 파일의 이름 출력
				printf("%c", *(p + (8 * i) + j)); //마지막 널문자를 표시하지 않기때문에 %s는 사용 불가능
			}

			printf("\n");
		}
	}
}

//쉘 추가완료
void mycpto(unsigned char target_inode, char* host_dest) { //가상의 파일시스템 파일을 실제 컴퓨터로 보냄
	char* str = NULL;
	file_read(target_inode, &str); //파일 내용 읽어옴
	realfile_write(host_dest, str); //실제 파일 생성
	free(str);
}

//쉘 추가완료
void mycpfrom(char* host_name, unsigned char dest_inode) { //실제 파일을 가상 파일시스템으로 읽어들임
	char* str = NULL;
	realfile_read(host_name, &str); //파일 읽어옴
	DEBUGMOD
		printf("mycpfrom : reading realfile finished\n");

	if (str == NULL) { //없음
		printf("there is no such file in host pc %s\n", host_name);
		return;
	}

	file_rewrite(dest_inode, str); //내용이 저장될 파일에 내용저장
	DEBUGMOD
		printf("mycpfrom : file_size : %u\n", (inode_list + dest_inode)->file_size);
	DEBUGMOD
		printf("mycpfrom : writing file finished\n");

	free(str);
}

void mycp(unsigned char pwd, unsigned char target_inode, char* name) { //파일 복사
	char* str = NULL;
	file_read(target_inode, &str); //파일내용을 읽어서 str저장
	file_add(pwd, name); //빈 파일 추가
	file_rewrite(file_search(pwd, name), str); //빈 파일에 내용 저장
	free(str);
}

//쉘 추가완료
//name이라는 이름의 디렉터리를 찾고 해당 디렉터리로 현재경로 변경
void mycd(unsigned char* pwd, char* name) { //pwd : 들어가고자 하는 디렉터리의 상위디렉터리의 아이노드, name : 들어가고자 하는 디렉터리의 아이노드

	int n = dir_search(*pwd, name); //pwd에서 name에 해당하는 디렉터리 검색
	switch (n) {
	case -2: //그런 파일 없음
		printf("no such file or directory\n");
		return;
	case -1: //디렉터리가 아님
		printf("%s is not a directory\n", name);
		return;
	default: //해당 디렉터리로 이동
		*pwd = n;
		return;
	}
}

//쉘 추가완료
void mycat(unsigned char inode) { //해당 번호의 아이노드의 데이터블럭을 읽어옴, 디렉터리 아이노드를 대상으로는 정상적으로 작동하지 않음
	if (is_inode_empty(inode)) {
		printf("%d is empty inode\n", inode);
		return;
	}

	//비어있지 않은 데이터블럭 출력
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

	if ((inode_list + inode)->is_idb == 1) { //인다이렉트 블럭 읽어오고 출력
		idb_print((inode_list + inode)->single_idb);
	}

	printf("\n");
}

//쉘 추가완료
void mymkdir(unsigned char pwd, const char* name) {
	char tmp;

	if (strlen(name) > 7) { //파일이름 7글자제한
		printf("filename is too long\n");
		return;
	}


	char* p = *(datablock + (inode_list + pwd)->db1); //p = 현재 디렉터리(== 디렉터리가 추가될 디렉터리) 의 아이노드의 데이터블럭

	int len = 0;

	for (int i = 0; 8 * i < 256; i++) { //데이터블럭의 0, 8, 16...번째 값 확인 후 빈 공간 검색

		len = 0; //이름의 길이 비교
		for (int j = 1; *(p + (8 * i) + j) != '\0' && j < 8; j++) { //8n+1번째 값부터 7글자가 되거나 '\0'이 나올떄까지 len++, 파일의 이름 길이저장
			len++;
		}

		if (*(p + (8 * i)) != -1) {
			//if (strlen(name) == len && my_strncmp(p + (8 * i) + 1, name, strlen(name))) { //동일한 이름의 파일 존재시 생성거부
			//	DEBUGMOD printf("")
			//	printf("%s is already exist\n", name);
			//	return;
			//}
		}
		else {
			tmp = *(p + (8 * i) + 8);
			*(p + (8 * i)) = dir_alloc(pwd); //해당 빈공간에 새 디렉터리의 아이노드 할당
			sprintf(p + (8 * i) + 1, "%s", name); //해당 디렉터리의 이름을 설정
			*(p + (8 * i) + 8) = tmp;
			return;
		}
	}

	printf("this directory is full, 1 directory can contain 32 files\n"); //32칸이 이미 가득찼다면 생성거부

	return;
}

//쉘 추가완료
void mytree(unsigned char pwd, int cnt) {
	for (int i = 2; 8 * i < 256; i++) { //해당 디렉터리의 하위 디렉터리 탐색
		if (*(*(datablock + (inode_list + pwd)->db1) + 8 * i) != -1 && (inode_list + *(*(datablock + (inode_list + pwd)->db1) + 8 * i))->is_dir == 1) {
			for (int k = 0; k < cnt; k++) { //cnt 개수만큼 여백
				printf("  ");
			}
			printf("->");
			for (int j = 1; j < 8 && *(*(datablock + (inode_list + pwd)->db1) + 8 * i + j) != '\0'; j++) { //하위 디렉터리의 이름 출력
				printf("%c", *(*(datablock + (inode_list + pwd)->db1) + 8 * i + j));
			}
			printf("\n");
			mytree(*(*(datablock + (inode_list + pwd)->db1) + 8 * i), cnt + 1); //그 디렉터리의 하위 디렉터리에 대하여 재귀함수
		}
	}
}

void mymkfs(unsigned char* pwd) { //파일시스템 생성
	FILE* myfs = fopen("myfs", "r");
	_Bool keep = (myfs != NULL);
	//fclose(myfs);
	if (keep) { //파일이 이미 존재한다면
		DEBUGMOD printf("myfs closed\n");

		while (1) {
			printf("myfs already exist, rewrite myfs? (y / n) : ");
			switch (getchar()) {
			case 'y': //파일시스템 새로 생성
				for (int i = 0; i < 128; i++) { //모든 아이노드 할당해제, 연결된 데이터블럭도 자동으로 해제됨
					if (!is_inode_empty(i)) {
						inode_free(i);
					}
				}

				free(inode_list); //아이노드 리스트(포인터) 할당해제
				free(datablock); //데이터블럭 리스트(더블포인터) 할당해제
				setup(); //슈퍼블럭, 루트 등등 재설정
				fs_save(); //저장

				*pwd = 0;

				printf("new myfs recreated\n");

				getchar();
				return;
			case 'n': //거부
				printf("myfs didn't recreated\n");
				getchar();
				return;
			default: //예외
				printf("wrong input, input (y / n)\n");
				getchar();
				break;
			}

		}
	}
	else { //주석의 내용은 위와 같음
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

void mydatablock(unsigned char db) { //선택된 번호의 데이터블럭 하나 출력, 인다이렉트블럭, 디렉터리용 데이터블럭은 제대로 출력되지 않음
	if (db < 0 || db > 255) { //범위벗어남
		printf("%d is put of range, input 1 ~ 256\n", db + 1);
		return;
	}
	if (is_db_empty(db)) {
		printf("%d is empty datablock\n", db + 1);
		return;
	}
	else {
		printf("printing datablock %d\n\n", db + 1);
		db_print(db); //데이터블럭 출력 함수
		printf("\n");
	}
}

/*--------------------------*/

int relative_route_inode_return(char* str, unsigned char pwd) { //str : 입력받은 상대경로 ex) asdf/qwer
	int j = 1;
	int k = 0;
	int t = 0;
	int rewq = 0;
	int row = 0;
	int col = 0;
	int N = 0;
	if (*str == '/') {//절대경로로 입력 시 오류메시지 출력
		printf("fault route\n");
		return -2;
	}
	while (1) {//입력받은 디렉터리 갯수 체크
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
	dir_str = (char**)calloc(N, sizeof(char*));//디렉터리 이름들을 문자열로 저장할 공간
	for (int i = 0; i < N; i++) {
		*(dir_str + i) = (char*)calloc(8, sizeof(char));
	}
	while (1) {// 입력받은 상대경로 분리
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

	if (dir_search(pwd, *(dir_str + 0)) == -1)//파일, 디렉터리 구분
		dir_inode = file_search(pwd, *(dir_str + 0));
	else
		dir_inode = dir_search(pwd, *(dir_str + 0));
	rewq = dir_inode;
	for (j = 1; j < N; j++) {//아이노드 탐색, 리턴
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

int absolute_route_inode_return(char* str) { //str : 입력받은 절대경로 ex) /asdf/qwer //루트 하나만 입력하면 0리턴
	int j = 1;
	int k = 1;
	int t = 0;
	int rewq = 0;
	int row = 0;
	int col = 0;
	int N = 0;
	if (*str != '/') {//상대 경로 입력 시 오류 메시지 출력
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
	dir_str = (char**)calloc(N, sizeof(char*));//입력받은 디렉터리 저장할 공간
	for (int i = 0; i < N; i++) {
		*(dir_str + i) = (char*)calloc(8, sizeof(char));
	}
	while (1) {// 입력받은 절대경로 분리
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
		dir_inode = dir_search(0, *(dir_str + 0)); //현재 디렉터리에서 탐색
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

void myinode(int inode) { //pwd : 확인하고싶은 디렉터리의 아이노드

	if (inode < 0 || inode > 127) { //범위 확인
		printf("%d is out of range, input 1 ~ 128\n", inode);
	}
	
	if (is_inode_empty(inode)) { //예외처리
		printf("empty inode\n");
		return;
	}
	DEBUGMOD printf("db_mask : %u (0x%x)\n", (inode_list + inode)->db_mask, (inode_list + inode)->db_mask);

	//아이노드 관련 정보 출력
	struct tm tm;
	printf("%s", (inode_list + inode)->is_dir ? "Type : directory\n" : "Type : file\n");//파일 종류
	printf("size : %d\n", (inode_list + inode)->file_size);//파일 크기
	tm = (inode_list + inode)->time;
	printf("%4d/%2d/%2d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);//날짜
	printf("%2d:%2d:%2d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

	if (is_db_empty((inode_list + inode)->db1) || (((inode_list + inode)->db_mask & (unsigned char)0x80) == 0)) {// 데이터블럭 사용 여부
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
	while (inode != 0) {//상위 디렉터리 갯수 확인
		inode = dir_search(inode, "..");
		cnt++;
	}
	inode = pwd;
	route = (char*)calloc(8 * cnt, sizeof(char));
	inode_save = (char*)calloc(cnt, sizeof(char)); //역순으로 디렉터리 가지의 아이노드 번호 저장할 공간
	for (int i = 0; i < cnt; i++) {
		*(inode_save + i) = inode;
		inode = dir_search(inode, "..");
	}//inode역순으로 저장
	//*(inode + cnt - 1) == 루트
	if (cnt == 1) {//루트만 입력되었을 때
		*(route + 0) = '/';
		free(inode_save);
		return route;
	}
	*(route + route_cnt) = '/';
	route_cnt++;
	for (; cnt > 1; cnt--) {//inode_save활용하여 디렉터리들을 /로 구분된 문자열로 route에 저장
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
	//사용중인 아이노드 카운트
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

	//아이노드 상태 출력

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

	//데이터 블럭 사용량 카운트
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

	//데이터 블럭 상태 출력

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
//		for (int i = start - 1; i <= end; i++) { //파일 내용 출력
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
		for (int i = start - 1; i < end; i++) { //start값부터 end값까지 파일 내용 출력
			putchar(*(str + i));
		}
		putchar('\n');
	}
}

//------------------------------------------------------

int my_strncmp(const char* a, const char* b, int n) { //지정한 글자수만큼 문자열 비교
	for (int i = 0; i < n; i++) {
		if (*(a + i) != *(b + i)) {
			return 0;
		}
	}
	return 1;
}