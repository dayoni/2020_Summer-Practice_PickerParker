#ifndef __PETER_ULTRASENSOR_H__
#define __PETER_ULTRASENSOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define RED 2
#define GREEN 3
#define BLUE 0
#define TRIG 5
#define ECHO 6
#define BUZZER 26

static double first_distance;
static double last_distance;
/*static*/ int isCar;
static int test = 4;
static int gs_cnt = 0, ge_cnt = 0;

double ultraSensor();
void isCorrectObject();
void isOutCar();
void redOn();
void greenOn();
void blueOn();
void buzzerOn();
void buzzerOff();

#endif // !__PETER_ULTRASENSOR_H__
