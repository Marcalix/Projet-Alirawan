#pragma once
#include <time.h>

static inline int _tc_is_leap(int y){
    return (y%4==0) && (y%100!=0 || y%400==0);
}

static inline time_t timegm_compat(const struct tm *tm_utc)
{
    if (!tm_utc) return (time_t)-1;

    // Refuse des valeurs manifestement hors bornes (optionnel)
    if (tm_utc->tm_mon < 0 || tm_utc->tm_mon > 11) return (time_t)-1;

    static const int mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    int y = tm_utc->tm_year + 1900;  // années depuis 1900
    int m = tm_utc->tm_mon;          // 0..11
    int d = tm_utc->tm_mday;         // 1..31

    long long days = 0;

    if (y >= 1970)  for (int yy=1970; yy<y; ++yy) days += _tc_is_leap(yy)?366:365;
    else            for (int yy=y;     yy<1970; ++yy) days -= _tc_is_leap(yy)?366:365;

    for (int mm=0; mm<m; ++mm)
        days += mdays[mm] + ((mm==1 && _tc_is_leap(y)) ? 1 : 0);

    days += (d - 1);

    long long secs = days*86400LL
                   + (long long)tm_utc->tm_hour*3600LL
                   + (long long)tm_utc->tm_min *60LL
                   + (long long)tm_utc->tm_sec;

    return (time_t)secs;
}

#define timegm(tm_ptr) timegm_compat((tm_ptr))
