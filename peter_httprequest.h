#ifndef __PETER_HTTPREQUEST_H__
#define __PETER_HTTPREQUEST_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>

enum {
	ERROR_ARGS = 1,
	ERROR_CURL_INIT = 2
};

enum {
	OPTION_FALSE = 0,
	OPTION_TRUE = 1
};

enum {
	FLAG_DEFAULT = 0
};

/* ���� ���� ���� �� */
enum {
	SUCCESS = 1,	// ���� ���� ���� (�����)
	FAIL = 2,
	ERROR = 3,		// ���� ���� ����
	HOST = 4		// ������
};


/* response */
typedef struct ResponseData {
	long statCode;			// response state (ex: 200, 400, 404)
	char* responseBody;		// responseBody
	size_t size;			// data size
}ResponseData;

/* ������ ���� �Ķ���� */
/* main���� �Ҵ��� ResponseData �� CURL �ּ� ���� */
typedef struct threadParam {
	ResponseData* responseData;	// responsedata ptr
	CURL* curl;					// curl ptr
	char url[150];			// ���� �ּ�
}threadParam;

/* ���� ���� */
typedef struct reservationInfo {
	char status[10];		// ����
	char id[3];				// ���� ��ȣ
	char user_uuid[50];		// ����� uuid
							// ���� ���� ��¥
	char start_year[5];		// ��
	char start_month[3];	// ��
	char start_day[3];		// ��
	char start_hour[3];		// ��
	char start_min[3];		// ��
							// ���� �� ��¥
	char end_year[5];		// ��
	char end_month[3];		// ��
	char end_day[3];		// ��
	char end_hour[3];		// ��
	char end_min[3];		// ��

	char* addr[14];			// ����ü ���� ���� �ּ� ����
							// addr[0] = status;

	int addr_length;		// addr �迭 ũ��
}reservationInfo;


size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp);	// curl_perform �Լ� ���� �� responsebody�� �����ϴ� �ݹ� �Լ�
CURL* initialize(ResponseData* responseData);								// curl �ʱ�ȭ, curl �ɼ� ����
char* setPostData();														// post method ���� ������ ����				
int parsingData(ResponseData* data, reservationInfo* info);					// responsebody �Ľ�
void sendPostRequest(CURL* curl, ResponseData* responseData);				// post method request fuction
void* t_sendPostRequest(void*);												// post method request function (thread)
void sendGetRequest(CURL* curl, ResponseData* responseData);				// get method request fuction
void concat_url(int idx, threadParam* tparam);								// url ����

#endif // !__PETER_HTTPREQUEST_H__

