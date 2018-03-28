
/* hftime.h
 *
 * Copyright (C) 2013-2020 ShangHai High-flying Electronics Technology Co.,Ltd.
 *
 * This file is part of HSF.
 *
 */


#ifndef HFTIME_H_
#define HFTIME_H_

#if 1

struct _timezone 
{
	char none;
};



int HSF_API settimeofday(const struct timeval * tv, const struct _timezone *tz);

int HSF_API gettimeofday(struct timeval *__p, void *__tz);
#endif



#endif /* HFTIME_H_ */



