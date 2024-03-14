#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"fs.h"

#define DEBUG printf("DEBUG\n")
extern _Bool debug_mode;
#define DEBUGMOD if(debug_mode == 1)

extern superblock super;
extern inode* inode_list;
extern char** datablock;



void prompt_read(char* str, int n, char* buffer) { //문자열을 ' ' 공백 단위로 쪼갬, 그중 n번째 단어를 buffer에 저장(buffer는 미리 메모리할당이 되어있어야함) 
    int cnt = 1;

    for (int i = 0; cnt < n + 1 && *(str + i) != '\0'; i++) { //문자열을 바이트단위로 읽어옴
        if (cnt == n) { //원하는 번호의 단어에 도착
            for (int j = 0; *(str + i + j) != ' ' && *(str + i + j) != '\0'; j++) { //버퍼에 문자열 저장
                *(buffer + j) = *(str + i + j);
            }
            return;
        }
        if (*(str + i) == ' ') { //공백 확인시 cnt증가
            cnt++;
        }
    }

    return;
}

#define COMMAND(str) else if(strcmp(op1, #str) == 0) //명령어 구분 편이용 매크로

int main(void) {
    unsigned char pwd = 0;//현재 디렉터리 아이노드

    char* command;
    char *op1, *op2, *op3, *op4;//띄어쓰기로 구분하여 명령어 라인 인자 획득
    char* tmp;
    //실행 시 기본값 설정 및 파일 시스템 재생성 여부 확인
    setup();
    mymkfs(&pwd);
    fs_load();

    //getchar();//개행문자 삭제

    while (1) {

        tmp = now_dir(pwd);//현재 디렉터리 절대경로
        printf("[%s]$ ", tmp);//프롬프트
        free(tmp);

        command = (char*)calloc(100, sizeof(char));
        op1 = (char*)calloc(100, sizeof(char));
        op2 = (char*)calloc(100, sizeof(char));
        op3 = (char*)calloc(100, sizeof(char));
        op4 = (char*)calloc(100, sizeof(char));
        scanf("%[^\n]s", command);
        getchar();

        prompt_read(command, 1, op1);
        prompt_read(command, 2, op2);
        prompt_read(command, 3, op3);
        prompt_read(command, 4, op4);

        if (*op1 != '\0') {
            if (strcmp(op1, "exit") == 0) { //종료
                printf("end program\n");
                fs_save();
                return 0;
            }
            COMMAND(debugmod) { //디버깅용
                debug_mode = 1;
                printf("debugmod on\n");
            }
            COMMAND(mymkfs) { //파일시스템 생성
                mymkfs(&pwd);
            }
            COMMAND(mymkdir) { //경로지정 불가능, 디렉터리 생성
                if (*op2 != '\0')
                    if (dir_search(pwd, op2) >= 0) {
                        printf("%s is already exist\n", op2);
                    }
                    else
                        mymkdir(pwd, op2);
                else
                    printf("input argument\n");
            }
            COMMAND(myrmdir) { //경로지정 가능 (디렉터리를 대상으로 실행되므로 예외적으로 상위디렉터리를 추적 가능하기 때문)
                if (*op2 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if (pwd == route(op2, pwd)) //현재디렉터리 대상 실행
                        printf("can't remove current directory\n");
                    else if ((inode_list + route(op2, pwd))->is_dir == 1) {
                        myrmdir(dir_search(route(op2, pwd), ".."), route(op2, pwd));
                    }
                    else
                        printf("%s is not a directory\n", op2);
                }
            }
            COMMAND(mycd) { //경로지정 가능
                if (*op2 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + route(op2, pwd))->is_dir == 1)
                        pwd = route(op2, pwd);
                    else
                        printf("%s is not a directory\n", op2);
                }
                else
                    pwd = 0;
            }
            COMMAND(myls) { //디렉터리에 대해서 경로지정 가능, 파일에 대해서 불가능
                if (*op2 == '\0')
                    myls(pwd);
                else {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory %s\n", op2);
                    else if ((inode_list + route(op2, pwd))->is_dir == 1)
                        myls(route(op2, pwd));
                    else if((inode_list + route(op2, pwd))->is_dir == 0 && file_search(pwd, op2) < 0)
                        printf("can't use path when targeting normal file\n");
                    else if (file_search(pwd, op2) >= 0)
                        myls_target(pwd, file_search(pwd, op2));
                    else
                        printf("wrong input\n");
                }
            }
            COMMAND(mytouch) { //새파일 생성에 대해서 경로지정 불가능, 기존 파일 시간 업데이트에 대해서 경로지정 가능
                if (*op2 != '\0') {
                    if (route(op2, pwd) >= 0) {
                        inode_timeset(route(op2, pwd));
                    }
                    else {
                        file_add(pwd, op2);
                    }
                }
            }
            COMMAND(myrm) { //경로지정 불가능
                if (*op2 != '\0') {
                    if (file_search(pwd, op2) == -1) {
                        printf("%s is not a normal file\n", op2);
                    }
                    else if (file_search(pwd, op2) == -2) {
                        printf("no such file or directory\n");
                    }
                    else {
                        myrm(pwd, file_search(pwd, op2));
                    }
                }
                else {
                    printf("input argument\n");
                }
            }
            COMMAND(mycpto) { //경로지정 가능
                if (*op2 != '\0' && *op3 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + route(op2, pwd))->is_dir == 0) {
                        mycpto(route(op2, pwd), op3);
                    }
                    else
                        printf("%s is not a normal file\n", op2);
                }
            }
            COMMAND(mycpfrom) { //경로지정 불가능
                if (*op2 != '\0' && *op3 != '\0') {
                    if (file_search(pwd, op3) == -2) {
                        file_add(pwd, op3);
                        mycpfrom(op2, file_search(pwd, op3));
                    }
                    else if ((inode_list + route(op3, pwd))->is_dir == 0) {
                        mycpfrom(op2, route(op3, pwd));
                    }
                    else
                        printf("%s is not a normal file\n", op3);
                }
            }
            COMMAND(mycp) { //복사할 대상에 대하여 경로지정가능, 복사될 위치에 대하여 경로지정 불가능
                if (*op2 != '\0' && *op3 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if (file_search(pwd, op3) == -2)
                        mycp(pwd, route(op2, pwd), op3);
                    else
                        printf("%s is already exist\n", op3);
                }
            }
            COMMAND(mycat) { //경로지정 가능
                if (*op2 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + route(op2, pwd))->is_dir == 0) {
                        mycat(route(op2, pwd));
                    }
                    else
                        printf("%s is not a normal file\n", op2);
                }
            }
            COMMAND(myshowfile) {
                if (*op4 != '\0') {
                    if (route(op4, pwd) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + route(op4, pwd))->is_dir == 0) {
                        myshowfile(atoi(op2), atoi(op3), route(op4, pwd));
                    }
                    else {
                        printf("%s is directory\n", op4);
                    }

                }
            }

            COMMAND(mymv) { //옮기거나 개명할 파일에 대해서 경로지정 불가능
                if (*op2 != '\0' && *op3 != '\0') {
                    if (file_search(pwd, op2) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + file_search(pwd, op2))->is_dir == 0) {
                        if (route(op3, pwd) == -2) {
                            mymv_file(pwd, file_search(pwd, op2), op3);
                        }
                        else if ((inode_list + route(op3, pwd))->is_dir == 1) {
                            DEBUGMOD printf("%d %d\n", file_search(pwd, op2), route(op3, pwd));
                            mymv_dir(pwd, file_search(pwd, op2), route(op3, pwd));
                        }
                        else {
                            printf("%s is not a directory\n", op3);
                        }
                    }
                    else
                        printf("%s is not a normal file\n", op2);
                }
            }
            COMMAND(mytree) { //경로지정 가능
                if (*op2 != '\0') {
                    if (route(op2, pwd) == -2)
                        printf("no such file or directory\n");
                    else if ((inode_list + route(op2, pwd))->is_dir == 1) {
                        mytree(route(op2, pwd), 0);
                    }
                    else
                        printf("%s is not a directory file\n", op2);
                }
                else
                    mytree(pwd, 0);
            }
            COMMAND(myinode) {
                if (*op2 != '\0') {
                    if (atoi(op2) >= 1 && atoi(op2) <= 128) {
                        myinode(atoi(op2) - 1);
                    }
                    else {
                        printf("wrong inode number (1 ~ 128)\n");
                    }
                }
                else {
                    printf("wrong input\n");
                }
            }
            COMMAND(mydatablock) {
                if (*op2 != '\0') {
                    if (atoi(op2) >= 1 && atoi(op2) <= 256) {
                        mydatablock(atoi(op2) - 1);
                    }
                    else {
                        printf("wrong datablock number (1 ~ 256)\n");
                    }
                }
                else {
                    printf("input argument\n");
                }
            }
            COMMAND(mystate) {
                print_inode_db();
            }
            COMMAND(command) {
                printf("Input linux command : ");
                scanf("%[^\n]s", command);
                getchar();
                system(command);
            }
            COMMAND(mypwd) {
                tmp = now_dir(pwd);
                printf("%s\n", tmp);
                free(tmp);
            }
            else {
                printf("unknown command %s, wrong input\n", op1);
            }
        }

        free(command);
        command = NULL;
    }
    return 0;
}