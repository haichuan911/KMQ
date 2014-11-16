#include <stdio.h>
#include <kmq/errno.h>
#include "io.h"
#include "sectionreader.h"
#include "log.h"


KMQ_DECLARATION_START

int SectionReadWriter::ReadSection(Conn *conn, char *rbuffer) {
    int ret;
    if (!rbuffer || !conn) {
	KMQLOG_ERROR("Invalid conn or rbuffer");
	return -1;
    }

    while (rindex < rlen) {
	if ((ret = conn->Read(rbuffer + rindex, rlen - rindex)) < 0) {
	    KMQLOG_ERROR("Read section: %u of %u", rindex, rlen);
	    return -1;
	}
	rindex += ret;
    }

    ResetReader();
    return 0;
}

int SectionReadWriter::WriteSection(Conn *conn, char *wbuffer) {
    int ret;
    if (!conn || !wbuffer) {
	KMQLOG_ERROR("Invalid conn or wbuffer");
	return -1;
    }

    while (windex < wlen) {
	if ((ret = conn->Write(wbuffer + windex, wlen - windex)) < 0) {
	    KMQLOG_ERROR("Write section: %u of %u", windex, wlen);
	    return -1;
	}
	windex += ret;
    }

    ResetWriter();
    return 0;
}



}
