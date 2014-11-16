#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <kmq/errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "module_stats.h"

KMQ_DECLARATION_START

int generic_trigger_item_parse(const string &str, const string &item, int &tr, int &v) {
    char *pos = NULL, *newstr = NULL;
    const char *str_p = str.c_str();
    const char *item_p = item.c_str();

    if (!(pos = (char *)strstr(str_p, item_p)) || pos + item.size() + 4 > str_p + str.size())
	return -1;
    pos += item.size() + 1;
    switch (pos[0]) {
    case 's': tr = SL_S;
	break;
    case 'm': tr = SL_M;
	break;
    case 'h': tr = SL_H;
	break;
    case 'd': tr = SL_D;
	break;
    default:
	return -1;
    }
    pos += 2;
    newstr = strdup(pos);
    pos = newstr;
    while (pos < newstr + strlen(newstr)) {
	if (pos[0] == ';') {
	    pos[0] = '\0';
	    break;
	}
	pos++;
    }
    if ((v = atoi(newstr)) <= 0)
	v = 1;
    free(newstr);
    return 0;
}

int generic_set_trigger_threshold(module_stat *stat, int key, int tr, int val) {
    switch (tr) {
    case SL_S: stat->set_s_threshold(key, val);
	break; 
    case SL_M: stat->set_m_threshold(key, val);
	break;
    case SL_H: stat->set_h_threshold(key, val);
	break;
    case SL_D: stat->set_d_threshold(key, val);
	break;
    }
    return 0;
}


}
