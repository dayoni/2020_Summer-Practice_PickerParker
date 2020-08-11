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
#include "ibeaconAndroid.h"


//#include "textfile.h"
//#include "oui.h"

//scanner.c ibeacon-master

#define HEX_LENGTH  2

#define FLAGS_AD_TYPE   0x01
#define FLAGS_LIMITED_MODE_BIT  0x01
#define FLAGS_GENERAL_MODE_BIT  0x02

#define EIR_FLAGS           0x01
#define EIR_UUID16_SOME     0x02
#define EIR_UUID16_ALL      0x03
#define EIR_UUID32_SOME     0x04
#define EIR_UUID32_ALL      0x05
#define EIR_UUID128_SOME    0x06
#define EIR_UUID128_ALL     0x07
#define EIR_NAME_SHORT      0x08
#define EIR_NAME_COMPLETE   0x09
#define EIR_TX_POWER        0x0A
#define EIR_DEVICE_ID       0x10

/*
�ٿ��ٿ��ٿ�
#define TRIG 0
#define ECHO 1
#define LED1 2
*/

// #define SUCCESS 0
// #define HOST 1
// #define ERROR 2
// #define FAIL 3


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

	char* addr[13];			// ����ü ���� ���� �ּ� ����
							// addr[0] = status;

	int addr_length;		// addr �迭 ũ��
}reservationInfo;

static volatile int signal_received = 0;
// http://blazingcode.asuscomm.com/api/check/1
// http://blazingcode.asuscomm.com/api/check-in/1-4
// http://blazingcode.asuscomm.com/api/check-out/1-4

const char* deviceID = "1";					// ����ȣ
const char* path_check = "check/";			// ���� Ȯ�� url
const char* path_checkin = "check-in/";		// ���� url
const char* path_checkout = "check-out/";	// ���� url
const char* server = "http://blazingcode.asuscomm.com/api/"; // ���� �ּ�

char check_parameter[30];	// ��ũ�� üũ�ƿ� �� ����� parameter

int g_isTimeout = 0;
/*
�ٿ��ٿ��ٿ�
double first_distance;
double last_distance;
int isCar;
*/

int gs_cnt = 0, ge_cnt = 0;

static int print_advertising_devices(int dd, uint8_t filter_type, uint8_t* uuidFromServer);
static void sigint_handler(int sig);
static int check_report_filter(uint8_t procedure, le_advertising_info* info);
static int read_flags(uint8_t* flags, const uint8_t* data, size_t size);
static void eir_parse_name(uint8_t* eir, size_t eir_len, char* buf, size_t buf_len);

/*
�ٿ��ٿ��ٿ�
double ultraSensor();
void isCorrectObject();
*/

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp);	// curl_perform �Լ� ���� �� responsebody�� �����ϴ� �ݹ� �Լ�
CURL* initialize(ResponseData* responseData);								// curl �ʱ�ȭ, curl �ɼ� ����
char* setPostData();														// post method ���� ������ ����				
int parsingData(ResponseData* data, reservationInfo* info);					// responsebody �Ľ�
void sendPostRequest(CURL* curl, ResponseData* responseData);				// post method request fuction
void* t_sendPostRequest(void*);												// post method request function (thread)
void sendGetRequest(CURL* curl, ResponseData* responseData);				// get method request fuction
void concat_url(int idx, threadParam* tparam);								// url ����


// ����̽� �˻� signal
static void sig_alrm_handler(int signo)
{
	signal_received = signo;
	g_isTimeout = 1; // global
}

/*
�ٿ��ٿ��ٿ�
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
*/

static void eir_parse_name(uint8_t* eir, size_t eir_len, char* buf, size_t buf_len)
{
	size_t offset;

	offset = 0;

	while (offset < eir_len)
	{
		uint8_t field_len = eir[0];
		size_t name_len;

		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		switch (eir[1])
		{
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > buf_len)
				goto failed;

			memcpy(buf, &eir[2], name_len);
			return;
		}

		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
	snprintf(buf, buf_len, "(unknow)");
}

//major ��ȣ = 0x0206 / minor ��ȣ = 0x0406 �� ibeacon�� �˻� 
static int eir_parse_ibeacon_info(uint8_t* nearData, struct ibeacon_info* info)
{
	if (nearData[0] == 0x1A && nearData[1] == 0xff &&
		nearData[ANDROID_IBEACON_MAJOR_S] == 0x02 && nearData[ANDROID_IBEACON_MAJOR_S + 1] == 0x06
		&& nearData[ANDROID_IBEACON_MINOR_S] == 0x04 && nearData[ANDROID_IBEACON_MINOR_S + 1] == 0x06) {

		memcpy(&info->uuid, &nearData[ANDROID_IBEACON_UUID_S], IBEACON_UUID_L);
		memcpy(&info->major, &nearData[ANDROID_IBEACON_MAJOR_S], IBEACON_MAJOR_L);
		memcpy(&info->minor, &nearData[ANDROID_IBEACON_MINOR_S], IBEACON_MINOR_L);
		return 0;
	}
	return 1;
}

static int read_flags(uint8_t* flags, const uint8_t* data, size_t size)
{
	size_t offset;

	if (!flags || !data)
		return -EINVAL;

	offset = 0;
	while (offset < size)
	{
		uint8_t len = data[offset];
		uint8_t type;

		if (len == 0)
			break;

		if (len + offset > size)
			break;

		type = data[offset + 1];

		if (type == FLAGS_AD_TYPE)
		{
			*flags = data[offset + 2];
			return 0;
		}

		offset += 1 + len;
	}

	return -ENOENT;

}

static int check_report_filter(uint8_t procedure, le_advertising_info* info)
{
	//printf("report filter..\n");
	uint8_t flags;


	if (procedure == 0)
		return 1;

	printf("read flags...\n");
	if (read_flags(&flags, info->data, info->length))
		return 0;

	switch (procedure)
	{
	case 1:
		if (flags & FLAGS_LIMITED_MODE_BIT)
			return 1;
		break;

	case 'g':
		if (flags & (FLAGS_LIMITED_MODE_BIT | FLAGS_GENERAL_MODE_BIT))
			return 1;
		break;

	default:
		fprintf(stderr, "Unknow discovery procedure\n");
		break;
	}

	return 0;
}


static void sigint_handler(int sig)
{
	signal_received = sig;
}

//�ֺ� ����̽� �˻� �� ���� ������ ��
static int print_advertising_devices(int dd, uint8_t filter_type, uint8_t* uuidFromServer)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter nf, of;
	struct sigaction sa;
	struct ibeacon_info iInfo;
	socklen_t olen;
	int timeoutt;
	int isRightCar = 0;
	int len;
	int i;

	fd_set set;
	struct timeval timeout;
	int rv;


	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0)
	{
		printf("Could not get socket option\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0)
	{
		printf("Could not set socket option\n");
		return -1;
	}

	memset(&sa, 0x00, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	timeoutt = 10; // 10sec timeout
	if (signal(SIGALRM, sig_alrm_handler) == SIG_ERR) {
		printf("err\n");
		return 0;
	}

	g_isTimeout = 0; // globel
	alarm(timeoutt);


	//10�ʵ��� ���˻� �� while�� Ż��
	while (!g_isTimeout)
	{
		evt_le_meta_event* meta;
		le_advertising_info* info;
		char addr[18];

		FD_ZERO(&set);
		FD_SET(dd, &set); /* add our file descriptor to the set */

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		//5�ʵ��� read�� ������ ������ ���������� ���� read
		rv = select(dd + 1, &set, NULL, NULL, &timeout);

		if (signal_received == SIGALRM)
			printf("alarm!!\n");

		else if (rv == -1)
			perror("select"); /* an error accured */
		else if (rv == 0)
			printf("timeout!!\n"); /* a timeout occured */
		else
			len = read(dd, buf, sizeof(buf)); /* there was data to read */

		while (len < 0)
		{
			printf("while...\n");

			if (errno == EINTR && signal_received == SIGINT)
			{
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}

		//10�ʰ� ������ �� �ൿ
		if (signal_received == SIGALRM) {
			isRightCar = 0;
			signal_received = 0;
			g_isTimeout = 0;
			return 0;
		}

		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void*)ptr;

		if (meta->subevent != 0x02)
			goto done;

		/* ignoring multiple reports */
		info = (le_advertising_info*)(meta->data + 1);

		//�˻��� �������� �� ���������� �ֺ� uuid�� ��
		if (check_report_filter(filter_type, info))
		{
			char name[30];

			memset(name, 0x00, sizeof(name));

			if (!eir_parse_ibeacon_info(info->data, &iInfo)) {

				ba2str(&info->bdaddr, addr);
				eir_parse_name(info->data, info->length, name, sizeof(name) - 1);
				printf("%s %s\n", addr, name);

				for (i = 0; i < 16; i++)
					printf("%02X ", uuidFromServer[i]);
				printf("\n");

				printf("uuid=");
				for (i = 0; i < IBEACON_UUID_L; i++)
					printf("%02X ", iInfo.uuid[i]);
				printf("\n");
				printf("major=%02X %02X\n", iInfo.major[0], iInfo.major[1]);
				printf("minor=%02X %02X\n", iInfo.minor[0], iInfo.minor[1]);

				for (i = 0; i < IBEACON_UUID_L; i++) {
					if (uuidFromServer[i] == iInfo.uuid[i])
						isRightCar++;
				}

				if (isRightCar == IBEACON_UUID_L) {
					alarm(0);
					return 1;
				}
			}
		}
		//printf("\n");
		isRightCar = 0;
	}
	alarm(0);

done:
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}


//ibeaconscanner
// ���������� �ֺ� uuid�� ���� ����� ��ȯ
// uuidFromServer -> �����κ��� ������ ���� ����(uuid)
//return 1/0
int ibeaconScanner(uint8_t* uuidFromServer) {
	int dev_id, dd;

	int err;
	int isCorrectCar;

	uint8_t own_type = 0x00;
	uint8_t scan_type = 0x01; // 0x00 passive sacan 0x01 random scan
	uint8_t filter_type = 0;
	uint8_t filter_dup = 1;
	uint16_t interval = htobs(0x0010); // 0x12 none filtering duplicates
	uint16_t window = htobs(0x0010);        // same upper

	dev_id = hci_get_route(NULL);

	if (dev_id < 0)
	{
		perror("Could not open device");
		exit(1);
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0)
	{
		perror("Could not open device");
		exit(1);
	}

	interval = htobs(0x0012);
	window = htobs(0x0012);

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window, own_type, 0x00, 1000);

	if (err < 0)
	{
		perror("set scan parameters failed");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);

	if (err < 0)
	{
		perror("Enable scan failed");
		exit(1);
	}
	printf("LE Scan ...\n");

	isCorrectCar = print_advertising_devices(dd, filter_type, uuidFromServer);

	if (isCorrectCar < 0)
	{
		perror("Could not reveive adverising events");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 1000);
	if (err < 0)
	{
		perror("Disable scan failed");
		exit(1);
	}

	hci_close_dev(dd);

	return isCorrectCar;
}

//�ӽ� uuid ��ȯ�Լ�
char* getUuidFromServer() {

	//temp UUID
	return "2f234454cf6d4a0fadf2f4911ba9ffa6";  //hex
}


//not use
char* getReservedTimeFromServer() {
	// ##test case
	//"1400-1600"
	//"0230-0530"
	//"1100-1830"
	//"1730-0130"
	//"0000-0100"

	//1730/0130/20(y)/08(m)/04(d)
	return "1730-0130";
}

//�ӽ� ���¹�ȯ �Լ�
int getStatusFromServer() {
	/*
	* SUCCESS: success store UUID
	* HOST: host External time
	* ERROR(FAIL): error
	*      different device number
	* FAIL: fail
	*      incorrect time
	*      incorrect uuid
	*/
	return SUCCESS;
}


//not use
int compareTime(char* reservation) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	int startTime;
	int endTime;
	int curTime;

	sscanf(reservation, "%4d-%4d", &startTime, &endTime);
	curTime = tm.tm_hour * 100 + tm.tm_min;
	printf("reservedTime: %04d-%04d \n", startTime, endTime);

	curTime = 1830; ////////////////////////////
	printf("curTime: %d \n", curTime);

	if (startTime > endTime) {
		if (curTime < endTime || curTime >= startTime)
			return 1;
	}
	else {
		if (curTime >= startTime && curTime < endTime)
			return 1;
	}
	return 0;
}

//char -> hex 
//32�ڸ� ���ڿ� -> 16�ڸ� hex
void char2hex(char* uuid, uint8_t* num) {

	int i, j;

	//uuid == "host"

	//str parsing 2 -> 12 34 56 78...    
	for (i = 0; i < IBEACON_UUID_L; i++) {
		for (j = 0; j < HEX_LENGTH; j++) {
			if (uuid[i * HEX_LENGTH + j] >= 'A' && uuid[i * HEX_LENGTH + j] <= 'F')
				num[i] = num[i] * 16 + uuid[i * HEX_LENGTH + j] - 'A' + 10;
			else if (uuid[i * HEX_LENGTH + j] >= 'a' && uuid[i * HEX_LENGTH + j] <= 'f')
				num[i] = num[i] * 16 + uuid[i * HEX_LENGTH + j] - 'a' + 10;
			else if (uuid[i * HEX_LENGTH + j] >= '0' && uuid[i * HEX_LENGTH + j] <= '9')
				num[i] = num[i] * 16 + uuid[i * 2 + j] - '0';
		}
	}
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
		int status = parsingData2(responseData, info);

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

/* responsebody �Ľ� */
int parsingData(ResponseData* data, reservationInfo* info) {
	char c;
	char str[100000];
	int idx = 0, info_index = 0;
	int key_flag = 0, value_flag = 0;
	int status;
	printf("- - - RESPONSE DATA PARSING START - - -\n");
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

/* callback func - save body content to memory */
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

/* curl �ʱ�ȭ, curl �ɼ� ���� */
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

/* post method ���� ������ ���� */
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

/* post method request function (thread) */
void* t_sendPostRequest(void* tParam) {
	CURL* curl = ((threadParam*)tParam)->curl;
	ResponseData* responseData = ((threadParam*)tParam)->responseData;

	//printf("t_sendPostRequest\n");
	//printf("t_sendPostRequest curl address: %p\n", curl);
	//printf("t_sendPostRequest responsedata address: %p\n", responseData);


	//threadParam* tParam = (threadParam*)tParam;
	const char* url = "http://blazingcode.asuscomm.com/api/check/1";
	printf("url: %s\n", ((threadParam*)tParam)->url);
	curl_easy_setopt(curl, CURLOPT_URL, /*"http://blazingcode.asuscomm.com/api/check/1"*/ ((threadParam*)tParam)->url);
	memset(responseData, 0, sizeof(ResponseData));


	char* postData/*=  setPostData()*/;
	//printf("print post Data : %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	/* CURLOPT_POSTFIELDS...
	POST �� �ݵ�� �� �ɼ� �����ؾ���
	POST ��������� ���� �� �Ķ���Ͱ� ���� -> ���� ���� �ϰ� �Ķ���ͷ� ����
	postData = NULL�� �����ϸ� �ȵ�
	�� �Լ� ������ ������ GET ������� request�ϰڴٿ� ���� ��
	*/
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

/* post method request function */
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
/* get method request function */
void sendGetRequest(CURL* curl, ResponseData* responseData) {
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
/*void setresponseHeader(){
curl_slist* responseHeaders = NULL;
responseHeaders = curl_slist_append( responseHeaders , "Expect: 100-continue" ) ;
responseHeaders = curl_slist_append( responseHeaders , "User-Agent: Picker Parker" ) ;
curl_easy_setopt( curl , CURLOPT_HTTPHEADER , responseHeaders ) ;
}*/

