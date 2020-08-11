#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <curl/curl.h>
#include <time.h>
#include <string.h>
#include <wiringPi.h>
#include <pthread.h>
#include <math.h>
#include "peter_ibeacon_Android.h"
#include "peter_ibeacon_scanner.c"


//#include "textfile.h"
//#include "oui.h"

//scanner.c ibeacon-master

#define TRIG 0
#define ECHO 1
#define LED1 2

// #define SUCCESS 0
// #define HOST 1
// #define ERROR 2
// #define FAIL 3

/* peter_httprequest.h */
/*
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

*/
/* ���� ���� ���� �� */
/*
enum {
	SUCCESS = 1,	// ���� ���� ���� (�����)
	FAIL = 2,
	ERROR = 3,		// ���� ���� ����
	HOST = 4		// ������
};
*/
/* peter_httprequest.h */
/* response */
/*typedef struct ResponseData {
	long statCode;			// response state (ex: 200, 400, 404)
	char* responseBody;		// responseBody
	size_t size;			// data size
}ResponseData;*/

/* peter_httprequest.h */
/* ������ ���� �Ķ���� */
/* main���� �Ҵ��� ResponseData �� CURL �ּ� ���� */
/*
typedef struct threadParam {
	ResponseData* responseData;	// responsedata ptr
	CURL* curl;					// curl ptr
	char url[150];			// ���� �ּ�	
}threadParam;
*/
/* peter_httprequest.h */
/* ���� ���� */
/*
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

	char* addr[13];			// ����ü ���� ���� �ּ� ����
							// addr[0] = status;

	int addr_length;		// addr �迭 ũ��
}reservationInfo;
<<<<<<< HEAD
*/
static volatile int signal_received = 0;
=======

>>>>>>> 19f5a6d261b9fb7c1542b678d37fc883f66d7fc1
// http://blazingcode.asuscomm.com/api/check/1
// http://blazingcode.asuscomm.com/api/check-in/1-4
// http://blazingcode.asuscomm.com/api/check-out/1-4

/* peter_httprequest.h */
/*const char* deviceID = "1";					// ����ȣ
const char* path_check = "check/";			// ���� Ȯ�� url
const char* path_checkin = "check-in/";		// ���� url
const char* path_checkout = "check-out/";	// ���� url
const char* server = "http://blazingcode.asuscomm.com/api/"; // ���� �ּ�
char check_parameter[30];	// ��ũ�� üũ�ƿ� �� ����� parameter
*/

double first_distance;
double last_distance;
int isCar;

int gs_cnt = 0, ge_cnt = 0;

double ultraSensor();
void isCorrectObject();

/* peter_httprequest.h */
/*size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp);	// curl_perform �Լ� ���� �� responsebody�� �����ϴ� �ݹ� �Լ�
CURL* initialize(ResponseData* responseData);								// curl �ʱ�ȭ, curl �ɼ� ����
char* setPostData();														// post method ���� ������ ����				
int parsingData(ResponseData* data, reservationInfo* info);					// responsebody �Ľ�
void sendPostRequest(CURL* curl, ResponseData* responseData);				// post method request fuction
void* t_sendPostRequest(void*);												// post method request function (thread)
void sendGetRequest(CURL* curl, ResponseData* responseData);				// get method request fuction
void concat_url(int idx, threadParam* tparam);								// url ����
*/

//������ �ñ׾˶��ڵ鷯
void handler1()
{
	double num;																					
	last_distance = ultraSensor();				//2�� �Ŀ� ��ȯ�� �Ÿ��� last_distance�� ����
	num = fabs(last_distance - first_distance); //(last_distance - first_distance)���밪

	printf("first_distance = %.2f cm, last_distance = %.2f cm\n", first_distance, last_distance);

	//���밪�� �ش� ������ ������ ��ġ�ϴٰ� �ν�, LED ����(�׽��Ϳ�)
	if ((num) <= 4.0) {
		printf("num : %.2f\n------------!!YES!!------------\n", num);
		pinMode(LED1, OUTPUT);
		digitalWrite(LED1, HIGH);
		delay(20);
		isCar = 1;
	}
	//���밪�� �ش� ������ ũ�� ����ġ�ϴٰ� �ν�, LED �ҵ�(�׽��Ϳ�)
	else {
		printf("num : %.2f\n------------!!NO!!------------\n", num);
		digitalWrite(LED1, LOW); delay(20);
		isCar = 0;
	}
}

//�����Ÿ����� �����ð����� ���� -> ��
void isCorrectObject() {
	double distance = ultraSensor();
	if (distance > 20.0 && distance < 40.0)		//20.0cm < ultra < 40.0cm �϶�
	{
		first_distance = distance;				//�Ÿ� ���� ���� ���� ��ȯ�Ǹ� first_distance�� ����
		signal(SIGALRM, handler1);
		alarm(2);								//SIGALRM�̿�, ���� �ð� �Ŀ� �Լ� ȣ��
		printf("test1\n");
		pause();
		printf("test2\n");
		//alarm(2);
	}
}

//���� ����
void isOutCar() {
	double distance = ultraSensor();
	int i;

	if (!(distance < 40.0 && distance > 20.0)) {

		//�����ϰ� 3�� �� ���� Ȯ��
		for (i = 0; i < 3; i++) {
			distance = ultraSensor();
			delay(1000);

			if (distance < 40.0 && distance > 20.0) {
				printf("-----------CAR IN----------\n");
				return;
			}
		}
		isCar = 0;
		printf("---------------------------------------------\n-----CAR OUT-----\n------------------------------------------------\n");
	}
	else {
		printf("-----------CAR IN------------\n");
	}
}

//�����ĸ� �̿��Ͽ� ��ü�� �Ÿ��� ����
double ultraSensor()
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	unsigned long startTime, endTime;					//Echo���� �����ĸ� ��, �����ϴ� �ð�����
	double distance;									//��ȯ�� �Ÿ� ����, ���ϰ�
	unsigned int cnt = 0;
	FILE* fp = NULL;

	sleep(0.01);
	fp = fopen("cntoutLog.txt", "a");
	if (fp == NULL) {
		printf("FILE OPEN ERROR! \n");
		exit(0);
	}

	pinMode(TRIG, OUTPUT);                              //trig, wiringPi GPIO 0�� = BCM GPIO 17��
	pinMode(ECHO, INPUT);								//echo, wiringPi GPIO 1�� = BCM GPIO 18��

	digitalWrite(TRIG, LOW);							//trig�� Low�� ���
	digitalWrite(TRIG, HIGH);							//trig�� High�� ���
	delayMicroseconds(10);								//10us ���� delay
	digitalWrite(TRIG, LOW);							//trig�� Low�� ���

	while (digitalRead(ECHO) == LOW) {
		if (cnt > 100000) {								//���� ����(�����İ� �������� ���� ��� ���)
			//exit(1);
			printf("cnt! start\n");
			gs_cnt++;
			fprintf(fp, "%d# error startCnt %d:%d\n", gs_cnt, tm.tm_hour, tm.tm_min);
			fclose(fp);
			return -1;
		}
		cnt++;
	}
	cnt = 0;
	startTime = micros();

	while (digitalRead(ECHO) == HIGH) {
		if (cnt > 1000000) {							//���� ����(������ �۽��� ����� �ȵǾ��� ��츦 ���)
			printf("cnt! end\n");

			ge_cnt++;

			fprintf(fp, "%d# error endCnt %d:%d\n", ge_cnt, tm.tm_hour, tm.tm_min);
			fclose(fp);
			return -1;
		}
		cnt++;
	}
	endTime = micros();

	distance = (((double)(endTime - startTime) * 17.0) / 1000.0); //(cm)��ȯ �Ÿ�����, data seet ����, �Ÿ� = �ӷ� * �ð�

	delay(50);
	fclose(fp);
	return distance;
}

int main(int argc, char** argv) {
	uint8_t uuidFromServer[IBEACON_UUID_L];
	int isCorrectCar;
	int isCorrectTime;

	/*���� ����*/
	pthread_t t_id;
	curl_global_init(CURL_GLOBAL_ALL);
	int* status;

	wiringPiSetup();                         //wiringPi �������� PIN��ȣ
	// Response �� ������ ����ü ����  
	ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
	memset(responseData, 0, sizeof(ResponseData));

	// curl �ʱ�ȭ
	CURL* curl = initialize(responseData);

	// thread parameter initialize
	threadParam* tParam = (threadParam*)malloc(sizeof(threadParam));
	memset(tParam, 0, sizeof(threadParam));

	// main ������ �����͸� thread parameter �� ����Ű���� ����
	tParam->responseData = responseData;
	tParam->curl = curl;

	// ���� ���� ��û url ����
	concat_url(1, tParam);
	reservationInfo* info = (reservationInfo*)malloc(sizeof(reservationInfo));
	memset(info, 0, sizeof(reservationInfo));

	// initilaze
	info->addr[0] = info->status;
	info->addr[1] = info->id;
	info->addr[2] = info->user_uuid;
	info->addr[3] = info->start_year;
	info->addr[4] = info->start_month;
	info->addr[5] = info->start_day;
	info->addr[6] = info->start_hour;
	info->addr[7] = info->start_min;
	info->addr[8] = info->end_year;
	info->addr[9] = info->end_month;
	info->addr[10] = info->end_day;
	info->addr[11] = info->end_hour;
	info->addr[12] = info->end_min;
	info->addr_length = 13;

	
	while (1) {
		//parking a Car
		//��������
		isCar = 0;

		//ultrasonicSansor
		//��� ���� ���� �ִ��� ���÷� Ȯ��
		while (!isCar)
			isCorrectObject();

		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
			perror("thread create error: ");
		}
		// �����尡 ���� ������ main ���� ����
		pthread_join(t_id, NULL);
		// responsebody parsing
		int status = parsingData(responseData, info);

		//SUCCESS: �����ڰ� ������ �ð� ���� ������ �ð��� ����
		//HOST: �����ڰ� ������ �ð� �ܿ� ����
		//ERROR DEFAULT: �����ڰ� ������ �ð� ���� �������� ���� �ð��� ����
		switch (status) {
			// ���೻�� ���� (�����)
		case SUCCESS:
			char2hex(info->user_uuid, uuidFromServer);
			isCorrectCar = ibeaconScanner(uuidFromServer);		//�����ڰ� ������ �ð� ���� ������ �ð��� ������ ���� UUID�� ���� -> 1 / Ʋ�� -> 0
			break;
			// ������
		case HOST:
			isCorrectCar = 1;
			break;
			// ���� ���� ����
		case ERROR:
			isCorrectCar = 0;
			break;

		default:
			isCorrectCar = 0;
			break;
		}

		//uuid�� ������ ����Ȯ��
		//Ʋ���� �ٽ� �����ĺ��� �˻�
		if (isCorrectCar) {
			printf("correct car...!!\n");

			// check-in url ����		
			concat_url(2, tParam);
			// �������� ���
			if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
				perror("thread create error: ");
			}
			// �����尡 ���� ������ main ���� ����
			pthread_join(t_id, NULL);
		}
		else {
			printf("incorrect car...\n");
			continue;
		}

		//������ �� ��, while Ż�� 
		while (isCar)
			isOutCar();
		
		// check-out url ����
		concat_url(3, tParam);
		// ���� ���� ���
		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
			perror("thread create error: ");
		}
		// �����尡 ���� ������ main ���� ����
		pthread_join(t_id, NULL);

		//server get output time() requested
	}
	
	// ����
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	free(responseData);

}

/* url ���� */
/*
void concat_url(int idx, threadParam* tparam) {
	tparam->url[0] = '\0';

	switch (idx) {
		// http://blazingcode.asuscomm.com/api/check/1
	case 1: // ���� ���� ��û
		strcpy(tparam->url, server);
		strcat(tparam->url, path_check);
		strcat(tparam->url, deviceID);

		break;
	case 2: // ���� ���� ���
		// http://blazingcode.asuscomm.com/api/check-in/1-4
		strcpy(tparam->url, server);
		strcat(tparam->url, path_checkin);
		strcat(tparam->url, check_parameter);

		break;
	case 3:	// ���� ���� ���
		// http://blazingcode.asuscomm.com/api/check-out/1-4
		strcpy(tparam->url, server);
		strcat(tparam->url, path_checkout);
		strcat(tparam->url, check_parameter);

		break;
	default:
		break;
	}
}
*/
/* responsebody �Ľ� */
/*
int parsingData(ResponseData* data, reservationInfo* info) {
	char c;
	char str[100000];
	int idx = 0, info_index = 0;
	int key_flag = 0, value_flag = 0;
	int status;
	printf("- - - RESPONSE DATA PARSING START - - -\n");
	*/
	/* reseponseBody contents..
	{
	"status": "success",
	"content": {
	"user_uuid": "1111111111",
	"start_year": "2020",
	"start_month": "08",
	"start_day": "06",
	"start_hour": "13",
	"start_min": "00",
	"end_year": "2020",
	"end_month": "08",
	"end_day": "06",
	"end_hour": "14",
	"end_min": "00"`
	}
	}
	*/
/*
	while (data->responseBody[idx] != '\0') {
		c = data->responseBody[idx++];
		if (c == '"') {

			// key flag = false , value_flag = false
			if (!key_flag && !value_flag) {
				// key ���� -> true
				key_flag = 1;
			}
			// key flag = true , value_flag = false
			else if (key_flag && !value_flag) {
				// key ���� -> false
				key_flag = 0;
				// printf("key: %s \n", str);
				str[0] = 0;

				if (strcmp(str, "contents")) {
					continue;
				}
			}

			// value_flag = true �϶� " �� �������
			else if (value_flag) {
				continue;
			}
		}
		else {
			if (c == ':') {
				// value ���� -> true
				value_flag = 1;

			}

			else if (c == ',' || (value_flag && c == '}')) {
				// value �� -> false
				value_flag = 0;
				printf("value: %s \n", str);
				// value �� ����
				strcpy(info->addr[info_index++], str);
				str[0] = 0;
			}
			// contents ���� ����
			// value_flag �� 0���� �ٲ� key���� �ٽ� �����ϰ� ��
			else if (c == '{') {
				value_flag = 0;
			}
			// key_flag �̰ų� value_flag �� ��� ���ڿ� ����
			else if (key_flag || value_flag) {
				strncat(str, &c, 1);
			}
		}
	}
	printf("- - - RESPONSE DATA PARSING END - - -\n");

	// ���� ���� ���� (�����)
	if (strcmp(info->status, "success") == 0) {
		check_parameter[0] = '\0';
		strcat(check_parameter, info->id);
		strcat(check_parameter, "-");
		strcat(check_parameter, deviceID);

		return SUCCESS;
	}
	// ������
	else if (strcmp(info->status, "host") == 0) {
		return HOST;
	}
	// ���� ���� ����
	else {
		return ERROR;
	}
}
*/
/* callback func - save body content to memory */
/*
size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	ResponseData* mem = (ResponseData*)userp;

	mem->responseBody = (char*)realloc(mem->responseBody, mem->size + realsize + 1);
	if (mem->responseBody == NULL) {
		printf("not enougn\n");
		return 0;
	}
	memcpy(&(mem->responseBody[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responseBody[mem->size] = 0;

	return size * nmemb;
}
*/
/* curl �ʱ�ȭ, curl �ɼ� ���� */
/*
CURL* initialize(ResponseData* responseData) {
	// �ʱ�ȭ
	CURL* curl = curl_easy_init();
	if (curl != NULL) {
		//curl_easy_setopt( curl , CURLOPT_URL,  server ) ;
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, OPTION_TRUE);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Picker Parker");
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1);
		// ������ ���� �ݹ��Լ� ����
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		// ������ ���� ��ġ ���� (responseData ����ü)
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)responseData);

		return curl;
	}
	else {
		printf("Unable to initialize cURL interface\n");
		exit(EXIT_FAILURE);
	}
}
*/
/* post method ���� ������ ���� */
/*
char* setPostData() {
	const char* postParams[] = {
		"user_id"      , "test" ,
		"password"      , "testtest" ,
		NULL
	};

	char* buf = (char*)malloc(sizeof(char) * 200);
	memset(buf, 0, sizeof(buf));
	const char** postParamsPtr = postParams;

	while (NULL != *postParamsPtr) {
		// curl_escape( {string} , 0 ): replace special characters
		// (such as space, "&", "+", "%") with HTML entities.
		// ( 0 => "use strlen to find string length" )
		// remember to call curl_free() on the strings on the way out
		char* key = curl_escape(postParamsPtr[0], FLAG_DEFAULT);
		char* val = curl_escape(postParamsPtr[1], FLAG_DEFAULT);

		// parameter �� ������ url ����
		sprintf(buf, "%s%s=%s&", buf, key, val);
		printf("POST param: %s\n", buf);
		postParamsPtr += 2;

		// the cURL lib allocated the escaped versions of the
		// param strings; we must free them here
		curl_free(key);
		curl_free(val);
	}
	return buf;
}
*/
/* post method request function (thread) */
/*
void* t_sendPostRequest(void* tParam) {
	CURL* curl = ((threadParam*)tParam)->curl;
	ResponseData* responseData = ((threadParam*)tParam)->responseData;

	//printf("t_sendPostRequest\n");
	//printf("t_sendPostRequest curl address: %p\n", curl);
	//printf("t_sendPostRequest responsedata address: %p\n", responseData);


	//threadParam* tParam = (threadParam*)tParam;
	const char* url = "http://blazingcode.asuscomm.com/api/check/1";
	printf("url: %s\n", ((threadParam*)tParam)->url);
	curl_easy_setopt(curl, CURLOPT_URL, ((threadParam*)tParam)->url);
	
	memset(responseData, 0, sizeof(ResponseData));
	

	char* postData;
	//printf("print post Data : %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	/* CURLOPT_POSTFIELDS...
	POST �� �ݵ�� �� �ɼ� �����ؾ���
	POST ��������� ���� �� �Ķ���Ͱ� ���� -> ���� ���� �ϰ� �Ķ���ͷ� ����
	postData = NULL�� �����ϸ� �ȵ�
	�� �Լ� ������ ������ GET ������� request�ϰڴٿ� ���� ��
	*/
/*
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_TRUE);

	printf("- - - THREAD POST START: response - - -\n");
	// action!
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - THREAD POST FINISH: response - - -\n");

	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
	}
	else {
		// HTTP �����ڵ带 ���´�.    
		if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
			printf("code: %ld\n", responseData->statCode);
			printf("data: %s\n", responseData->responseBody);
		}
	}
}
*/
/* post method request function */
/*
void sendPostRequest(CURL* curl, ResponseData* responseData) {
	// ���� ��� �ּ� ����
	curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/login");
	memset(responseData, 0, sizeof(ResponseData));

	char* postData = setPostData();
	printf("print post Data: %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	//printf("print post Data : %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	/* CURLOPT_POSTFIELDS...
	POST �� �ݵ�� �� �ɼ� �����ؾ���
	POST ��������� ���� �� �Ķ���Ͱ� ���� -> ���� ���� �ϰ� �Ķ���ͷ� ����
	postData = NULL�� �����ϸ� �ȵ�
	�� �Լ� ������ ������ GET ������� request�ϰڴٿ� ���� ��
	*/
/*
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_TRUE);


	printf("- - - POST START: response - - -\n");
	// action!
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - POST FINISH: response - - -\n");

	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
		return;
	}

	// HTTP �����ڵ带 ���´�.    
	if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
		printf("code: %ld\n", responseData->statCode);
		printf("data: %s\n", responseData->responseBody);
	}
}
*/
/* get method request function */
/*void sendGetRequest(CURL* curl, ResponseData* responseData) {
	// ���� ��� �ּ� ����
	curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/parking-lot/40-120-30-130");
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_FALSE);

	// ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
	memset(responseData, 0, sizeof(ResponseData));

	printf("- - - GET START: response - - -\n");
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - GET FINISH: response - - -\n");


	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
		return;
	}

	if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
		printf("code: %ld\n", responseData->statCode);
		printf("body: %s\n", responseData->responseBody);
	}
}
*/
/*void setresponseHeader(){
curl_slist* responseHeaders = NULL;
responseHeaders = curl_slist_append( responseHeaders , "Expect: 100-continue" ) ;
responseHeaders = curl_slist_append( responseHeaders , "User-Agent: Picker Parker" ) ;
curl_easy_setopt( curl , CURLOPT_HTTPHEADER , responseHeaders ) ;
}*/
