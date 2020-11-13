#include "peter_ultraSensor.h"

//������ �ñ׾˶��ڵ鷯
void handler1() {
	double num;
	last_distance = ultraSensor();				//2�� �Ŀ� ��ȯ�� �Ÿ��� last_distance�� ����
	num = fabs(last_distance - first_distance); //(last_distance - first_distance)���밪

	printf("first_distance = %.2f cm, last_distance = %.2f cm\n", first_distance, last_distance);

	//���밪�� �ش� ������ ������ ��ġ�ϴٰ� �ν�
	if ((num) <= 4.0) {
		printf("num : %.2f\n------------!!YES!!------------\n", num);
		isCar = 1;
	}
	//���밪�� �ش� ������ ũ�� ����ġ�ϴٰ� �ν�
	else {
		printf("num : %.2f\n------------!!NO!!------------\n", num);
		isCar = 0;
	}
}

//�����Ÿ����� �����ð����� ���� -> ��
void isCorrectObject() {
	double distance = ultraSensor();
	if (distance > 20.0 && distance < 40.0)	{	//20.0cm < ultra < 40.0cm �϶�
		first_distance = distance;				//�Ÿ� ���� ���� ���� ��ȯ�Ǹ� first_distance�� ����
		signal(SIGALRM, handler1);
		alarm(2);								//SIGALRM�̿�, ���� �ð� �Ŀ� �Լ� ȣ��
		pause();
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
				//printf("-----------CAR IN----------\n");
				return;
			}
		}
		isCar = 0;
		printf("---------------------------------------------\n-----CAR OUT-----\n------------------------------------------------\n");
	}
	else {
		//printf("-----------CAR IN------------\n");
	}
}

//�����ĸ� �̿��Ͽ� ��ü�� �Ÿ��� ����
double ultraSensor() {
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

void redOn() {
	digitalWrite(RED, HIGH);
	digitalWrite(GREEN, LOW);
	digitalWrite(BLUE, LOW);
}

void greenOn() {
	digitalWrite(RED, LOW);
	digitalWrite(GREEN, HIGH);
	digitalWrite(BLUE, LOW);
}

void blueOn() {
	digitalWrite(RED, LOW);
	digitalWrite(GREEN, LOW);
	digitalWrite(BLUE, HIGH);
}

void buzzerOn() {
	digitalWrite(BUZZER, HIGH);
}

void buzzerOff() {
	digitalWrite(BUZZER, LOW);
}
